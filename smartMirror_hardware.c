/** Smart Mirror Hardware Code
 * Handles three button (polling in three separate threads)
 * Handles 6 LED (timer in one thread)
 * Handles two photoresistor reads (polling in one thread)
 *
 * @author Saeed Alrahma, Jeremy Schreck, Matt Olson
 * Nov 8, 2016
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <math.h>

/* GPIO Pins */
#define CLK 4
#define BUTTON_LEFT 5
#define BUTTON_RIGHT 6
#define BUTTON_CENTER 13
#define LED1 17
#define LED2 18
#define LED3 27
#define LED4 22
#define LED5 23
#define LED6 24
#define PHRES_A3 7
#define PHRES_A4 1
#define PHRES_A5 16
#define PHRES_A6 20
#define PHRES_A7 21
// #define PHRES_B3
// #define PHRES_B4
// #define PHRES_B5
// #define PHRES_B6
// #define PHRES_B7

/* Constants */
#define LED_SWITCH_TIME 20000 // in milliseconds
#define LED_BLINK_TIME 1000 // in milliseconds
#define LED_DELAY_PERIOD 500 // in milliseconds
#define PHRES_READ_PERIOD 100 // in milliseconds
#define DASH 400 // in milliseconds

/* Global Variables */
volatile int countLeft = 0;
volatile int countRight = 0;
volatile int countCenter = 0;
// unsigned long btnLeftTime = 0;
// unsigned long btnRightTime = 0;
// unsigned long btnCenterTime = 0;
char webPage; // current web app page (1, 2, or 3)
char ledThreadRunning; // boolean for led thread
char phAvg; // photoresistor running average
char runningAvgN = 32; // number of values in running average
char runningAvgShift = 5;
char terminateCode; // boolean to track when user terminates code
char firefoxCall[] = "sudo -u $SUDO_USER firefox /home/pi/Desktop/Code/page1.html";

/* Functions */
int rpi_init(); // initialize GPIOs, threads, interrupts, and variables
void INThandler(int sig); // handles "CTRL-c" exit
int ledDelay(int led, int delayDuration); // executes timer delay between LEDs turning on
PI_THREAD(ledThread);


// Main Process
int main(void) {	
	piHiPri(50); // avg priority
	
	if (rpi_init() != 0)
		return 1;
		
	system(firefoxCall);
	
	while(!terminateCode) {
		if((webPage==2) && !ledThreadRunning) {
			// We are on youtube page. Start LED timer!
			if(piThreadCreate(ledThread) != 0){
				fprintf(stderr, "Unable to start LED thread: $s\n", strerror(errno));
				//return 1;
			} else {
				ledThreadRunning = 1; // block attempts to start led thread
				printf("Timer started!\n");
			}
		}

		// TEMPORARY
		printf("Web Page %d\n", webPage);
		printf("left: %d\n", countLeft);
		printf("right: %d\n", countRight);
		printf("center: %d\n", countCenter);
		printf("phAvg: %d\n", phAvg);
		countLeft = 0;
		countRight = 0;
		countCenter = 0;
		delay(2000);
	}

	return 0;
}

/** Exit handler
 *	turns off LEDs
 *	sets exit boolean
 */
void INThandler(int sig) {
	digitalWriteByte(0); // turn off all LEDs
	terminateCode = 1; // exit infinite loop	
	// exit(0);
}

/** Executes timer delay for LED switching
 *  checks regularly if app page changed
 *  return 1 if app page changed, otherwise 0 if timer completed
 */
int ledDelay(int led, int delayDuration) {
	int i=0;
	unsigned int t1 = millis();
	// delay and check app page regularly
	for(;i<delayDuration; i+=LED_DELAY_PERIOD) {
		delay(LED_DELAY_PERIOD); // partial delay
		if(webPage!=2) {
			digitalWriteByte(0); // turn off all LEDs
			ledThreadRunning = 0; // led thread will exit
			return 1; // app page changed
		}
	}
	unsigned int t2 = millis();
	if ((t2-t1-delayDuration)>1000) {
		printf("LED%d: Delay time is %d\n", led, t2-t1);
	}
	return 0;
}

/** LED Thread
 *  Runs concurrently on a separate thread
 *	Turns on one LED every 20 secs (total 6 LEDs = 2 mins)
 *	Blinks for another 20 secs before turning all LEDs off
 *	thread exists whenever app page changes
 */
PI_THREAD(ledThread) {
	piHiPri(50); // average priority
	// Turn on one LED every 20 seconds
	// if webPage changes, turn off LEDs and close thread 
	if(ledDelay(1, LED_SWITCH_TIME)) return;
	// TO-DO: do we want to wait longer?
	// How long does the Youtube video take to load/start?
	digitalWrite(LED1, HIGH);
	if(ledDelay(2, LED_SWITCH_TIME)) return;
	digitalWrite(LED2, HIGH);
	if(ledDelay(3, LED_SWITCH_TIME)) return;
	digitalWrite(LED3, HIGH);
	if(ledDelay(4, LED_SWITCH_TIME)) return;
	digitalWrite(LED4, HIGH);
	if(ledDelay(5, LED_SWITCH_TIME)) return;
	digitalWrite(LED5, HIGH);
	if(ledDelay(6, LED_SWITCH_TIME)) return;
	digitalWrite(LED6, HIGH);

	// Blink for 20 seconds
	char leds = 0xff;
	int i = 0;
	for (; i<LED_SWITCH_TIME; i+=LED_BLINK_TIME) {
		if(ledDelay(10, LED_BLINK_TIME)) return;
		leds = !leds;
		digitalWriteByte(leds); // control all 6 LEDs
		// TO-DO: watch out I'm changing value of 2 more GPIO
	}

	// turn all LEDs off
	digitalWriteByte(0); // writes to first 8 gpio in wiring Pi
	ledThreadRunning = 0; // unblock led thread
}

/** Photoresistor Thread
 *  Reads photoresistor value once every period (100 ms)
 *	Updates running average
 */
PI_THREAD(phresThread) {
	while(1) {
		// get reading from input
		char phresA = (digitalRead(PHRES_A3)<<3)+
			(digitalRead(PHRES_A4)<<4)+(digitalRead(PHRES_A5)<<5)+
			(digitalRead(PHRES_A6)<<6)+(digitalRead(PHRES_A7)<<7);
		// char phresB = (digitalRead(PHRES_B3)<<3)+
		// 	(digitalRead(PHRES_B4)<<4)+(digitalRead(PHRES_B5)<<5)+
		// 	(digitalRead(PHRES_B6)<<6)+(digitalRead(PHRES_B7)<<7);
		// unsigned int readingAvg = (phresA+phresB)<<1; // add and divide by 2

		// recalculate running average
		phAvg -= (phAvg<<runningAvgShift); // subtract average reading
		phAvg += (phresA<<runningAvgShift); // add new reading
		// phAvg += (readingAvg<<runningAvgShift); // add new reading

		// TO-DO: if brightness changed --> change screen brightness
		//printf("Photoresistor A: %d\n", phresA);
		
		// wait until next period
		delay(PHRES_READ_PERIOD);
	}
}

// Button Left Thread
PI_THREAD(btnLeftThread) {
	piHiPri(90); // high priority
	while(1) {
		while(digitalRead(BUTTON_LEFT)==1) {
			// button pressed (block next button presses)
			unsigned long timePressed = millis();
			delay(30); // min button press duration
			while (digitalRead(BUTTON_LEFT)==1) {
				// wait for button release
				delay(30); // depends on our specs
				// TO-DO: ignore false presses
				// TO-DO: ignore other presses while handling press
			}
			// button released
			countLeft++;
			if(webPage>'1'){
				// Move app page left
				webPage--;
				firefoxCall[53] = webPage;
				system(firefoxCall);
			}
			if(millis()-timePressed >= DASH) {
				// button press was a DASH
			} else {
				// button press was a DOT
			}
			// wait (unblock) for next button press
		}
		delay(5); // polling time
	}
}

// Button Right Thread
PI_THREAD(btnRightThread) {
	piHiPri(90); // high priority
	while(1) {
		while(digitalRead(BUTTON_RIGHT)==1) {
			// button pressed (block next button presses)
			unsigned long timePressed = millis();
			delay(30); // min button press duration
			while (digitalRead(BUTTON_RIGHT)==1) {
				// wait for button release
				delay(30); // depends on our specs
				// TO-DO: ignore false presses
				// TO-DO: ignore other presses while handling press
			}
			// button released
			countRight++;
			if(webPage<'3'){
				webPage++;
				firefoxCall[53] = webPage;
				system(firefoxCall);
				// Move app page right
			}
			if(millis()-timePressed >= DASH) {
				// button press was a DASH
			} else {
				// button press was a DOT
			}
			// wait (unblock) for next button press
		}
		delay(5); // polling time
	}
}

// Button Center Thread
PI_THREAD(btnCenterThread) {
	piHiPri(90); // high priority
	while(1) {
		while(digitalRead(BUTTON_CENTER)==1) {
			// button pressed (block next button presses)
			unsigned long timePressed = millis();
			delay(30); // min button press duration
			while (digitalRead(BUTTON_CENTER)==1) {
				// wait for button release
				delay(30); // depends on our specs
				// TO-DO: ignore false presses
				// TO-DO: ignore other presses while handling press
			}
			// button released
			countCenter++;
			if(webPage=='2'){
				// start/stop video
				// TO-DO: handle start/stop video
				// TO-DO: handle turn on/off tv
			}
			if(millis()-timePressed >= DASH) {
				// button press was a DASH
			} else {
				// button press was a DOT
			}
			// wait (unblock) for next button press
		}
		delay(5); // polling time
	}
}



/** Initialize i/o, interrupts, and variables
 *
 */
int rpi_init(){
	signal(SIGINT, INThandler); // handle exit

	// Initialize variables
	webPage = '1';
	ledThreadRunning = 0;
	phAvg = 64;
	terminateCode = 0;

	// setup wiringPi with BCM_GPIO pin numbering
	if(wiringPiSetupGpio() < 0) {
		fprintf(stderr, "Unable to setup wiringPi: $s\n", strerror(errno));
		return 1;
	}

	// Setup pin modes
	pinMode(CLK,GPIO_CLOCK);
	pinMode(BUTTON_LEFT, INPUT);
	pinMode(BUTTON_RIGHT, INPUT);
	pinMode(BUTTON_CENTER, INPUT);
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(LED3, OUTPUT);
	pinMode(LED4, OUTPUT);
	pinMode(LED5, OUTPUT);
	pinMode(LED6, OUTPUT);
	pinMode(PHRES_A3, INPUT);
	pinMode(PHRES_A4, INPUT);
	pinMode(PHRES_A5, INPUT);
	pinMode(PHRES_A6, INPUT);
	pinMode(PHRES_A7, INPUT);
	// pinMode(PHRES_B3, INPUT);
	// pinMode(PHRES_B4, INPUT);
	// pinMode(PHRES_B5, INPUT);
	// pinMode(PHRES_B6, INPUT);
	// pinMode(PHRES_B7, INPUT);


	// Setup pull-down resistance
	pullUpDnControl(BUTTON_LEFT, PUD_DOWN);
	pullUpDnControl(BUTTON_RIGHT, PUD_DOWN);
	pullUpDnControl(BUTTON_CENTER, PUD_DOWN);


	if(piThreadCreate(phresThread) != 0){
		fprintf(stderr, "Unable to start Photoresistor thread: $s\n", strerror(errno));
		//return 1;
	}

	if(piThreadCreate(btnLeftThread) != 0){
		fprintf(stderr, "Unable to start Button Left thread: $s\n", strerror(errno));
		return 1;
	}

	if(piThreadCreate(btnRightThread) != 0){
		fprintf(stderr, "Unable to start Button Right thread: $s\n", strerror(errno));
		return 1;
	}

	if(piThreadCreate(btnCenterThread) != 0){
		fprintf(stderr, "Unable to start Button Center thread: $s\n", strerror(errno));
		return 1;
	}

	return 0;
}
