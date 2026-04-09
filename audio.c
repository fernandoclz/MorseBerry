#include "audio.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

void *hilo_audio_alsa(void *arg) {
    int err;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int rate = 44100;
    int dir = 0;
    snd_pcm_uframes_t frames = 256;

    if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
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

    if ((err = snd_pcm_hw_params(handle, params)) < 0) {
        printf("ERROR Audio: Parámetros inválidos: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return NULL;
    }

    int freq_tono = (morse_frecuency > 0) ? morse_frecuency : 700;
    short *buffer = malloc(frames * 2 * sizeof(short));
    if (!buffer) {
        snd_pcm_close(handle);
        return NULL;
    }

    // *** CLAVE: fase acumulada entre iteraciones del bucle ***
    double fase = 0.0;
    double incremento_fase = 2.0 * M_PI * freq_tono / rate;

    while (continuar_ejecucion_hilo) {
    if (emitir_tono) {
        // Si el stream estaba pausado, lo reanudamos
        snd_pcm_state_t estado = snd_pcm_state(handle);
        if (estado == SND_PCM_STATE_PAUSED) {
            snd_pcm_pause(handle, 0); // 0 = reanudar
        }

        for (int i = 0; i < (int)frames; i++) {
            short muestra = (short)(16000 * sin(fase));
            buffer[2*i]     = muestra;
            buffer[2*i + 1] = muestra;
            fase += incremento_fase;
            if (fase >= 2.0 * M_PI) fase -= 2.0 * M_PI;
        }

        err = snd_pcm_writei(handle, buffer, frames);
        if (err == -EPIPE) {
            snd_pcm_prepare(handle);
        } else if (err < 0) {
            printf("ERROR Audio: Falla escritura: %s\n", snd_strerror(err));
        }

    } else {
        // Sin tono: pausamos el stream y vaciamos el buffer para latencia cero
        snd_pcm_state_t estado = snd_pcm_state(handle);
        if (estado == SND_PCM_STATE_RUNNING) {
            snd_pcm_drop(handle);   // Descarta inmediatamente lo encolado
            snd_pcm_prepare(handle); // Deja el stream listo para reanudar
        }
        // Esperamos activamente sin saturar la CPU
        usleep(5000); // 5ms de polling cuando hay silencio
    }
}

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    return NULL;
}