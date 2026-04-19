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
    snd_pcm_uframes_t frames = 256;

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

    snd_pcm_uframes_t buffer_size = frames * 2;
    snd_pcm_hw_params_set_buffer_size_near(handle, params, &buffer_size);

    if ((err = snd_pcm_hw_params(handle, params)) < 0)
    {
        printf("ERROR Audio: Parámetros inválidos: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return NULL;
    }

    int freq_tono = (morse_frecuency > 0) ? morse_frecuency : 700;
    short *buffer = malloc(frames * 2 * sizeof(short));
    if (!buffer)
    {
        snd_pcm_close(handle);
        return NULL;
    }

    // *** CLAVE: fase acumulada entre iteraciones del bucle ***
    double fase = 0.0;
    double incremento_fase = 2.0 * M_PI * freq_tono / rate;

    memset(buffer, 0, frames * 2 * sizeof(short));

    while (continuar_ejecucion_hilo)
    {
        if (emitir_tono && sonido_activado)
        {
            int freq_actual = (morse_frecuency > 0) ? morse_frecuency : 700;
            double incremento_fase = 2.0 * M_PI * freq_actual / rate;

            for (int i = 0; i < (int)frames; i++)
            {
                short muestra = (short)(16000 * sin(fase));
                buffer[2 * i] = muestra;
                buffer[2 * i + 1] = muestra;
                fase += incremento_fase;
                if (fase >= 2.0 * M_PI)
                    fase -= 2.0 * M_PI;
            }
        }
        else
        {
            memset(buffer, 0, frames * 2 * sizeof(short));
            // fase = 0;
        }

        err = snd_pcm_writei(handle, buffer, frames);

        if (err == -EPIPE)
        {
            // Under-run: ALSA se quedó sin datos
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