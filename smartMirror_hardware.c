/** Smart Mirror Hardware Code
 * Handles three button interrupts
 * Handles 6 LED timer interrupts
 * Handles two photoresistor reading/interrupts/polling?
 *
 * @author Saeed Alrahma
 * Oct 27, 2016
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define BUTTON_LEFT 5
#define BUTTON_RIGHT 6
#define BUTTON_CENTER 13
#define LED1 17
#define LED2 18
#define LED3 27
#define LED4 22
#define LED5 23
#define LED6 24

volatile int count = 0;

// LED Thread
PI_THREAD(ledThread) {
	while(1) {
		digitalWrite(LED1, HIGH);
		unsigned int t1 = millis();
		delay(1000);
		unsigned int t2 = millis();
		printf("Delay time HIGH is %d\n", t2-t1);
		digitalWrite(LED1, LOW);
		t1 = millis();
		delay(1000);
		t2 = millis();
		printf("Delay time LOW is %d\n", t2-t1);
	}
}

// Button Left Interrupt Callback
void btn_left_isr(void) {
	count++;
	system("/Users/SaeedAlrahma/Downloads/test.sh");
}

// Button Right Interrupt Callback
void btn_right_isr(void) {
	count++;
}

// Button Center Interrupt Callback
void btn_center_isr(void) {
	count++;
}

// Main Process
int main(void) {
	// setup wiringPi with BCM_GPIO pin numbering
	if(wiringPiSetupGpio() < 0) {
		fprintf(stderr, "Unable to setup wiringPi: $s\n", strerror(errno));
		return 1;
	}

	// Setup pin modes
	pinMode(BUTTON_LEFT, INPUT);
	pinMode(BUTTON_RIGHT, INPUT);
	pinMode(BUTTON_CENTER, INPUT);
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(LED3, OUTPUT);
	pinMode(LED4, OUTPUT);
	pinMode(LED5, OUTPUT);
	pinMode(LED6, OUTPUT);

	// Setup pull-down resistance
	pullUpDnControl(BUTTON_LEFT, PUD_DOWN);
	pullUpDnControl(BUTTON_RIGHT, PUD_DOWN);
	pullUpDnControl(BUTTON_CENTER, PUD_DOWN);

	// set BUTTON_LEFT pin to generate interrupt on low-to-high transition
	if(wiringPiISR(BUTTON_LEFT, INT_EDGE_RISING, &btn_left_isr) < 0) {
		fprintf(stderr, "Unable to setup Button Left ISR: $s\n", strerror(errno));
		return 1;
	}
	// set BUTTON_RIGHT pin to generate interrupt on low-to-high transition
	if(wiringPiISR(BUTTON_RIGHT, INT_EDGE_RISING, &btn_right_isr) < 0) {
		fprintf(stderr, "Unable to setup Button Right ISR: $s\n", strerror(errno));
		return 1;
	}
	// set BUTTON_CENTER pin to generate interrupt on low-to-high transition
	if(wiringPiISR(BUTTON_CENTER, INT_EDGE_RISING, &btn_center_isr) < 0) {
		fprintf(stderr, "Unable to setup Button Center ISR: $s\n", strerror(errno));
		return 1;
	}

	if(piThreadCreate(ledThread) != 0){
		fprintf(stderr, "Unable to start LED thread: $s\n", strerror(errno));
		return 1;
	}

	while(1) {
		printf("%d\n", count);
		count = 0;
		delay(1000);
	}

	return 0;
}