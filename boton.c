#include <stdio.h>
#include <wiringPi.h>

#define BUTTON_PIN 17
#define WPM_20_POINT 60 //ms
#define WPM_20_LINE 180
#define UMBRAL 120
#define CHAR_TIME 180
#define WORD_TIME 420
int main(void){
	if(wiringPiSetupGpio() == -1){
		printf("Error al iniciar wiringPi\n");
		return 1;
	}
	//Intentar leer el botón
	pinMode(BUTTON_PIN, INPUT);
	pullUpDnControl(BUTTON_PIN, PUD_OFF);

	int pulsando = 0;

	while(1){
		int estado = digitalRead(BUTTON_PIN);

		if(estado == LOW && !pulsando){
			pulsando = 1;
			unsigned int startTime = millis();

			while(digitalRead(BUTTON_PIN) == LOW)delay(1);
			unsigned int duration = millis() - startTime;

			pulsando = 0;
			if (duration < UMBRAL) {
                		printf(". ,%d\n", duration);
            		} else {
                		printf("- ,%d\n", duration);
            		}
            		fflush(stdout);
		}

	}
	return 0;
}
