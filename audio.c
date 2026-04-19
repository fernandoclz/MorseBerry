#include "audio.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

void *hilo_audio_alsa(void *arg)
{
    int err;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int rate = 44100;
    int dir = 0;
    
    // 1. AUMENTAMOS LOS FRAMES: 1024 da ~23ms de margen, ideal para evitar chisporroteos
    snd_pcm_uframes_t frames = 1024; 

    if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        printf("ERROR Audio: No se pudo abrir ALSA: %s\n", snd_strerror(err));
        return NULL;
    }

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(handle, params, 2);
    snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    snd_pcm_uframes_t buffer_size = frames * 4; // Aumentamos el ring buffer total
    snd_pcm_hw_params_set_buffer_size_near(handle, params, &buffer_size);

    if ((err = snd_pcm_hw_params(handle, params)) < 0)
    {
        printf("ERROR Audio: Parámetros inválidos: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return NULL;
    }

    short *buffer = malloc(frames * 2 * sizeof(short));
    if (!buffer)
    {
        snd_pcm_close(handle);
        return NULL;
    }

    double fase = 0.0;
    int freq_tono = 700;
    double incremento_fase = 2.0 * M_PI * freq_tono / rate;

    // 2. VARIABLES DE SUAVIZADO (Envolvente)
    double amplitud_actual = 0.0;
    double amplitud_objetivo = 0.0;
    // factor_suavizado: Determina lo rápido que sube/baja el volumen. 
    // 0.02 a 44100Hz significa una transición de ~2 milisegundos (perfecto para Morse)
    double factor_suavizado = 0.02; 

    memset(buffer, 0, frames * 2 * sizeof(short));

    while (continuar_ejecucion_hilo)
    {
        // Actualizamos la frecuencia por si cambia dinámicamente
        incremento_fase = 2.0 * M_PI * 700 / rate;
        
        // Decidimos la amplitud objetivo según el estado
        amplitud_objetivo = (emitir_tono && sonido_activado) ? 16000.0 : 0.0;

        for (int i = 0; i < (int)frames; i++)
        {
            // Acercamos progresivamente la amplitud actual a la objetivo
            amplitud_actual += (amplitud_objetivo - amplitud_actual) * factor_suavizado;

            short muestra = 0;
            
            // Solo calculamos el seno si hay volumen audible (optimización)
            if (amplitud_actual > 1.0) {
                muestra = (short)(amplitud_actual * sin(fase));
                fase += incremento_fase;
                if (fase >= 2.0 * M_PI) {
                    fase -= 2.0 * M_PI;
                }
            } else {
                muestra = 0;
                // Dejamos que la fase se mantenga contigua
                fase += incremento_fase;
                if (fase >= 2.0 * M_PI) fase -= 2.0 * M_PI;
            }

            buffer[2 * i]     = muestra; // Canal Izquierdo
            buffer[2 * i + 1] = muestra; // Canal Derecho
        }

        err = snd_pcm_writei(handle, buffer, frames);

        if (err == -EPIPE)
        {
            // Under-run: Intentamos recuperarnos silenciosamente
            snd_pcm_prepare(handle);
        }
        else if (err < 0)
        {
            printf("ERROR Audio: %s\n", snd_strerror(err));
        }
    }

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    return NULL;
}