#include <stdio.h>
#include <wiringPi.h>

#define BUTTON_PIN 6
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
	pullUpDnControl(BUTTON_PIN, PUD_DOWN);

	unsigned int lastReleaseTime = millis();
	int pulsando = 0;
	int letraPend = 0;
	int palabraPend = 0;

	while(1){
		int estado = digitalRead(BUTTON_PIN);

		if(estado == HIGH && !pulsando){
			pulsando = 1;
			unsigned int starTime = millis();

			while(digitalRead(BUTTON_PIN) == HIGH) delay(1);
			unsigned int duration = millis() - starTime;

			lastReleaseTime = millis();
			pulsando = 0;
			if (duration < UMBRAL) {
                		printf(".");
            		} else {
                		printf("-");
            		}
            		fflush(stdout);
			letraPend = 1;
			palabraPend = 1;
		}

		unsigned int silencio = millis() - lastReleaseTime;

		if(letraPend && silencio >= CHAR_TIME){
			printf(" ");
			fflush(stdout);
			letraPend = 0;
		}
	}
	return 0;
}
