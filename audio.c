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
    unsigned int rate = 44100; // Calidad CD
    int dir = 0;
    snd_pcm_uframes_t frames = 512; // Tamaño del bloque de audio
    
    // 1. Abrir dispositivo de sonido ALSA por defecto
    if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("ERROR Audio: No se pudo abrir ALSA: %s\n", snd_strerror(err));
        return NULL;
    }
    
    // 2. Configurar los parámetros de hardware
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE); // 16 bits
    snd_pcm_hw_params_set_channels(handle, params, 2); // Estéreo (I2S suele requerirlo)
    snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
    
    if ((err = snd_pcm_hw_params(handle, params)) < 0) {
        printf("ERROR Audio: Parámetros inválidos: %s\n", snd_strerror(err));
        return NULL;
    }
    
    // 3. Generar las ondas digitales
    int freq_tono = 700; // Frecuencia del pitido Morse (700Hz es el estándar clásico)
    short *buffer_tono = malloc(frames * 2 * sizeof(short)); // *2 por ser estéreo
    short *buffer_silencio = calloc(frames * 2, sizeof(short)); // Relleno de ceros
    
    for (int i = 0; i < frames; i++) {
        // Amplitud de 16000 (suficiente volumen sin distorsionar)
        short muestra = 16000 * sin(2 * M_PI * freq_tono * i / rate); 
        buffer_tono[2*i] = muestra;     // Canal Izquierdo
        buffer_tono[2*i + 1] = muestra; // Canal Derecho
    }
    
    // 4. Bucle principal de envío continuo a I2S
    // asumo que 'continuar_ejecucion_hilo' es tu variable para apagar el programa
    while (continuar_ejecucion_hilo) { 
        // Si el botón está pulsado usamos la onda, si no, silencio puro
        short *buffer_actual = emitir_tono ? buffer_tono : buffer_silencio;
        
        err = snd_pcm_writei(handle, buffer_actual, frames);
        
        if (err == -EPIPE) {
            // "Underrun": el audio se ha quedado atascado, lo reiniciamos rápido
            snd_pcm_prepare(handle);
        } else if (err < 0) {
            printf("ERROR Audio: Falla de escritura ALSA: %s\n", snd_strerror(err));
        }
    }
    
    // 5. Limpieza al cerrar el programa
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer_tono);
    free(buffer_silencio);
    return NULL;
}