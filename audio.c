#include "audio.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

// CONFIGURACION SINTETIZADOR
#define AMPLITUD_MAX 16000      // Volumen (0 - 32767)
#define FRECUENCIA_TONO 700     // Frecuencia pitido en Hz
#define TIEMPO_RAMPA_MS 5       // Tiempo ataque en ms

void *hilo_audio_alsa(void *arg)
{
    int err;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int rate = 44100;
    int dir = 0;
    
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

    //buffer grande para evitar microcortes en ALSA
    snd_pcm_uframes_t buffer_size = 8192; 
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

    //variables optimizadas
    double fase = 0.0;
    double incremento_fase = 2.0 * M_PI * FRECUENCIA_TONO / rate;

    //calculo rampa
    int muestras_rampa = (rate * TIEMPO_RAMPA_MS) / 1000;
    if (muestras_rampa == 0) muestras_rampa = 1; //control seguridad por si se pone 0
    
    int incremento_amplitud = AMPLITUD_MAX / muestras_rampa;
    int amplitud_actual = 0;

    memset(buffer, 0, frames * 2 * sizeof(short));

    while (continuar_ejecucion_hilo)
    {
        for (int i = 0; i < (int)frames; i++)
        {
            //1 control amplitud
            if (emitir_tono && sonido_activado) {
                amplitud_actual += incremento_amplitud;
                if (amplitud_actual > AMPLITUD_MAX) amplitud_actual = AMPLITUD_MAX;
            } else {
                amplitud_actual -= incremento_amplitud;
                if (amplitud_actual < 0) amplitud_actual = 0;
            }

            //2 generación onda
            short muestra = 0;
            if (amplitud_actual > 0) {
                //coma flotantes para el seno
                muestra = (short)(amplitud_actual * sin(fase));
            }

            //mantenemos fase corriendo para evitar saltos
            fase += incremento_fase;
            if (fase >= 2.0 * M_PI) {
                fase -= 2.0 * M_PI;
            }

            buffer[2 * i]     = muestra; //canal izq
            buffer[2 * i + 1] = muestra; //canal der
        }

        err = snd_pcm_writei(handle, buffer, frames);

        if (err == -EPIPE)
        {
            snd_pcm_prepare(handle);
            amplitud_actual = 0; //reset para evitar chasquidos al reiniciar
        }
        else if (err < 0)
        {
            snd_pcm_recover(handle, err, 0);
        }
    }

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    return NULL;
}