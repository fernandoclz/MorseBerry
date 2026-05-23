#include "gpio_input.h"
#include "config.h"
#include "utils.h"
#include <gpiod.h>

void *funcion_hilo_gpio(void *arg)
{
    struct gpiod_line_request *request = (struct gpiod_line_request *)arg;
    struct gpiod_edge_event_buffer *buffer = gpiod_edge_event_buffer_new(1);
    struct gpiod_edge_event *event;

    long long tiempo_pulsado = 0;
    long long tiempo_sin_pulsar = obtener_tiempo_actual();
    int pulsado = 0;
    emitir_tono = 0;
    int espacio_corto_emitido = 0;
    int ret;

    while (continuar_ejecucion_hilo)
    {
        // espera hasta 100ms viendo si se produce evento se subida o bajada
        ret = gpiod_line_request_wait_edge_events(request, 100000000);

        if (ret > 0)
        {
            if (gpiod_line_request_read_edge_events(request, buffer, 1) > 0)
            {
                // sacamos el tipo del evento
                event = gpiod_edge_event_buffer_get_event(buffer, 0);
                int tipo = gpiod_edge_event_get_event_type(event);
                unsigned int linea = gpiod_edge_event_get_line_offset(event);


                // se actua segun el evento sea de bajada o subida
                if (tipo == GPIOD_EDGE_EVENT_FALLING_EDGE)
                { // detecta flanco de bajada -> cuando se pulsa
                    if (linea == manip_der_gpio || linea == manip_izq_gpio) //manipulador
                    {
                        pthread_mutex_lock(&mutex_morse);

                        if (linea == manip_izq_gpio) {
                            simbolo_detectado = SIMBOLO_PUNTO;
                            emitir_tono = 1;
                            usleep(tiempo_punto * 1000); //emitir tono durante el tiempo del punto
                            emitir_tono = 0;
                        }
                        else if (linea == manip_der_gpio){
                            simbolo_detectado = SIMBOLO_RAYA;
                            emitir_tono = 1;
                            usleep(tiempo_raya * 1000); //emitir tono durante el tiempo de la raya
                            emitir_tono = 0;
                        }

                        pthread_mutex_unlock(&mutex_morse);

                        tiempo_sin_pulsar = obtener_tiempo_actual();
                        espacio_corto_emitido = 0;
                        pulsado = 0;
                        emitir_tono = 0;

                        continue; //salta 1 iteracion del bucle
                    }

                    tiempo_pulsado = obtener_tiempo_actual();
                    pulsado = 1;
                    emitir_tono = 1;
                    espacio_corto_emitido = 0;
                }
                else if (tipo == GPIOD_EDGE_EVENT_RISING_EDGE)
                {
                    long long tiempo_ahora = obtener_tiempo_actual();
                    long long duracion_pulsacion = tiempo_ahora - tiempo_pulsado;
                    tiempo_sin_pulsar = obtener_tiempo_actual();

                    if (pulsado == 1)
                    {
                        pulsado = 0;
                        emitir_tono = 0;

                        pthread_mutex_lock(&mutex_morse);
                        if (tiempo_punto - desviacion <= duracion_pulsacion &&
                            duracion_pulsacion <= tiempo_punto + desviacion)
                        {
                            simbolo_detectado = SIMBOLO_PUNTO;
                        }
                        else if (tiempo_raya - desviacion <= duracion_pulsacion &&
                                 duracion_pulsacion <= tiempo_raya + desviacion)
                        {
                            simbolo_detectado = SIMBOLO_RAYA;
                        }
                        else
                        {
                            simbolo_detectado = SIMBOLO_DESCONOCIDO;
                        }
                        pthread_mutex_unlock(&mutex_morse);
                    }
                }
            }
        }
        else if (ret == 0)
        {
            long long tiempo_ahora = obtener_tiempo_actual();

            if (pulsado == 1)
            {
                //detecta si pulsado muy largo
                if (tiempo_ahora - tiempo_pulsado >= tiempo_mantener)
                {
                    pthread_mutex_lock(&mutex_morse);
                    simbolo_detectado = SIMBOLO_MANTENER_PULSADO;
                    pthread_mutex_unlock(&mutex_morse);

                    pulsado = 0;
                    emitir_tono = 0;
                    tiempo_sin_pulsar = tiempo_ahora;
                }
            }
            else
            {
                long long tiempo_inactivo = tiempo_ahora - tiempo_sin_pulsar;

                //espacio largo
                if (tiempo_inactivo > tiempo_espacio && tiempo_sin_pulsar != 0)
                {
                    pthread_mutex_lock(&mutex_morse);
                    simbolo_detectado = SIMBOLO_ESPACIO_LARGO;
                    tiempo_sin_pulsar = 0;
                    pthread_mutex_unlock(&mutex_morse);
                }
                //espacio corto
                else if (tiempo_inactivo >= (tiempo_raya - desviacion) &&
                         tiempo_sin_pulsar != 0 &&
                         espacio_corto_emitido == 0)
                {

                    pthread_mutex_lock(&mutex_morse);
                    simbolo_detectado = SIMBOLO_ESPACIO_CORTO;
                    pthread_mutex_unlock(&mutex_morse);

                    espacio_corto_emitido = 1;
                }
            }
        }
    }

    gpiod_edge_event_buffer_free(buffer);
    return NULL;
}
