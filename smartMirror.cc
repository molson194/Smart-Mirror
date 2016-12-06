/** Smart Mirror Hardware Code
 * Handles three buttons (polling in three separate threads)
 * Handles 8 LEDs (timer in a separate thread)
 * Handles two photoresistor reads (polling in a separate thread)
 *
 * @author Saeed Alrahma, Jeremy Schreck, Matt Olson
 * Dec 1, 2016
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <string.h>


using namespace std;

/* GPIO Pins */
#define CLK 4 // Phyiscal 7
#define BUTTON_LEFT 10 // Phyiscal 19
#define BUTTON_RIGHT 11 // Phyiscal 23
#define BUTTON_CENTER 9 // Phyiscal 21

#define LED1 21 // Phyiscal 40
#define LED2 20 // Phyiscal 38
// #define LED3 26 // Phyiscal 37
// #define LED4 16 // Phyiscal 36
// #define LED5 19 // Phyiscal 35
// #define LED6 6 // Phyiscal 31
// #define LED7 5 // Phyiscal 29
// #define LED8 0 // Phyiscal 27

#define RED 18 // Phyiscal 12
#define GREEN 13 // Phyiscal 33
#define BLUE 12 // Phyiscal 32

#define PHRES_A3 2 // Phyiscal 3
#define PHRES_A4 3 // Phyiscal 5
#define PHRES_A5 14 // Phyiscal 8
#define PHRES_A6 15 // Phyiscal 10
#define PHRES_A7 17 // Phyiscal 11
#define PHRES_B3 27 // Phyiscal 13
#define PHRES_B4 22 // Phyiscal 15
#define PHRES_B5 23 // Phyiscal 16
#define PHRES_B6 24 // Phyiscal 18
#define PHRES_B7 25 // Phyiscal 22

/* Constants */
#define LED_SWITCH_TIME 15000 // in milliseconds
#define LED_BLINK_TIME 1000 // in milliseconds
#define LED_DELAY_PERIOD 500 // in milliseconds
#define PHRES_READ_PERIOD 100 // in milliseconds
#define BUTTON_POLLING_TIME 5 // in milliseconds
#define BUTTON_WAIT 25 // in milliseconds
#define LINK_PAGE_NUMBER_INDEX 46
#define LINK_USER_ID_INDEX 58
const char USER1_PASSWORD[] = "LCRLCR";
const char USER2_PASSWORD[] = "RCLRCL";
const char USER3_PASSWORD[] = "LLCCRR";
const char GUEST_PASSWORD[] = "CCCCCC";


/* Global Variables */
// volatile int countLeft = 0;
// volatile int countRight = 0;
// volatile int countCenter = 0;
int currentApp; // current web app page (Welcome '0', Weather 1', Youtube '2', or Calendar '3')
char ledThreadRunning; // boolean for led thread
short phAvg; // photoresistor running average
// char runningAvgN = 8; // number of values in running average
char runningAvgShift = 3; // 3 to discard 3 least significant bits (unused bits)
char terminateCode; // boolean to track when user terminates code
string firefoxCall = "sudo -u $SUDO_USER firefox ";
string baseURL = "localhost:8000/";
string welcomePage = "welcome.html";
string mainPage = "mirror.html";
string apps = ["welcome", "calendar", "weather", "youtube"];
//string pages[] = ["page0.html", "page1.html", "page2.html", "page3.html"];

string goLeft = "sh left.sh";
string goRight = "sh right.sh";

char password[6]; // array when password entered
char pwInd; // password current index (track password input)
char currentUser; // current user ID ('0' -> guest, '1' -> user1, etc)
unsigned long timePasswordInputReceived; // time stamp when password input received
unsigned long timeButtonCenterPressed; // time stamp when center button was pressed
char blockButtonPress; // boolean to indicate when to block button presses


/* Functions */
int rpi_init(); // initialize GPIOs, threads, interrupts, and variables
void INThandler(int sig); // handles "CTRL-c" exit
PI_THREAD(ledThread); // Thread for LED timing adn control
int ledDelay(int led, int delayDuration); // executes timer delay between LEDs turning on
void turnOffLEDs(); // turns off all LEDs
void turnOnLEDs(); // turns on all LEDs
void turnOffPWM(); // turns off all PWM outputs
void goToWelcomePage();
void goToMainPage(char user);
void goToMainPage(int appIndex, char user);
void openURL(string url);

// Main Process
int main(void) {	
	piHiPri(50); // avg priority
	
	if (rpi_init() != 0)
		return 1;

	// Start web app
	system("fuser -k tcp/8000"); // kill process runnning localhost:8000
	system("python -m SimpleHTTPServer 8000 &");
	currentUser = '0';
	currentApp = 0;
	goToWelcomePage();

	// Loop until shutdown
	while(!terminateCode) {
		// Password entry step
		if (currentApp == 0) {
			if (pwInd == 6)
			{
				if(strncmp(password, USER1_PASSWORD, 6) == 0) {
					currentUser = '1';
					printf("Logged in as %c!\n", currentUser);
					currentApp = 1;
				} else if(strncmp(password, USER2_PASSWORD, 6) == 0) {
					currentUser = '2';
					printf("Logged in as %c!\n", currentUser);
					currentApp = 1;
				} else if(strncmp(password, USER3_PASSWORD, 6) == 0) {
					currentUser = '3';
					printf("Logged in as %c!\n", currentUser);
					currentApp = 1;
				} else if(strncmp(password, GUEST_PASSWORD, 6) == 0) {
					currentUser = '0';
					printf("Logged in as %c!\n", currentUser);
					currentApp = 2;
				} else {
					// TODO: move to page (incorrect password)
					currentApp = 0;
				}
				goToMainPage(currentApp, currentUser);
				pwInd = 0; // reset password entry
			} else if(pwInd > 0) {
				if((millis() - timePasswordInputReceived)>3000) {
					// timeout in 5 seconds
					goToWelcomePage();
					pwInd = 0;

				}
			}
			
		}
		
		// Youtube/Video page
		if((currentApp==3) && !ledThreadRunning) {
			// Start LED thread/timer!
			if(piThreadCreate(ledThread) != 0){
				fprintf(stderr, "Unable to start LED thread: $s\n", strerror(errno));
				//return 1;
			} else {
				ledThreadRunning = 1; // block attempts to start another led thread
				printf("Timer started!\n");
			}
		}

		// TEMPORARY
		// printf("Web Page %c\n", currentApp);
		// printf("left: %d\n", countLeft);
		// printf("right: %d\n", countRight);
		// printf("center: %d\n", countCenter);
		printf("phAvg: %d\n", phAvg);
		printf("Password entered so far: %s\n", password);
		// countLeft = 0;
		// countRight = 0;
		// countCenter = 0;
		delay(2000);
	}

	return 0;
}

void goToWelcomePage() {
	currentApp = 0;
	currentUser = '0';
	string url = baseURL;
  	url += welcomePage;
  	url += " &";

  	openURL(url);
}

void goToMainPage(char user) {
  goToMainPage(1, user);
 
}

void goToMainPage(int appIndex, char user) {
  currentApp = appIndex;
  currentUser = user;
  string url = baseURL;
  url += mainPage;
  url += "?";
  url += "app=" + apps[currentApp];
  url += "user=" 
  url += user;
  url += " &";

  openURL(url);
 
}

void openURL(string url) {
  //string command = "open -a safari " + url;
  string command = firefoxCall + url;
  system(command.c_str());
}

/*
void goToURL(int page, char user) {
  currentUser = user;
  currentApp = page;
  string url = baseURL;
  url += pages[page];
  url += "?";
  url += "user=" + user;
  url += " &";

  //string command = "open -a safari " + url;
  string command = firefoxCall + url;
  system(command.c_str());
}
*/

/** Exit handler
 *	turns off all outputs
 *	sets exit boolean
 */
void INThandler(int sig) {
	turnOffLEDs(); // turn off all LEDs (stop output)
	turnOffPWM();
	// TODO: turn off clock output
	terminateCode = 1; // exit infinite loop	
	// exit(0);
}

/** Executes timer delay for LED switching
 *  checks regularly if app page changed
 *  return 1 if app page changed, otherwise 0 if timer completed
 */
int ledDelay(int led, int delayDuration) {
	int i=0;
	unsigned long t1 = millis();
	// delay and check app page regularly
	for(;i<delayDuration; i+=LED_DELAY_PERIOD) {
		delay(LED_DELAY_PERIOD); // partial delay
		if(currentApp!='2') {
			turnOffLEDs(); // turn off all LEDs
			ledThreadRunning = 0; // led thread will exit
			return 1; // app page changed
		}
	}
	unsigned long t2 = millis();
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
	// if currentApp changes, turn off LEDs and close thread 
	if(ledDelay(1, LED_SWITCH_TIME)) return;
	// TODO: do we want to wait longer?
	// How long does the Youtube video take to load/start?
	digitalWrite(LED1, HIGH);
	if(ledDelay(2, LED_SWITCH_TIME)) return;
	digitalWrite(LED2, HIGH);
	// if(ledDelay(3, LED_SWITCH_TIME)) return;
	// digitalWrite(LED3, HIGH);
	// if(ledDelay(4, LED_SWITCH_TIME)) return;
	// digitalWrite(LED4, HIGH);
	// if(ledDelay(5, LED_SWITCH_TIME)) return;
	// digitalWrite(LED5, HIGH);
	// if(ledDelay(6, LED_SWITCH_TIME)) return;
	// digitalWrite(LED6, HIGH);
	// if(ledDelay(7, LED_SWITCH_TIME)) return;
	// digitalWrite(LED7, HIGH);
	// if(ledDelay(8, LED_SWITCH_TIME)) return;
	// digitalWrite(LED8, HIGH);

	// Blink for LED_SWITCH_TIME to indicate timer done
	int i = 0;
	for (; i<LED_SWITCH_TIME; i+=LED_BLINK_TIME+LED_BLINK_TIME) {
		if(ledDelay(10, LED_BLINK_TIME)) return;
		turnOnLEDs(); // turn on all LEDs
		if(ledDelay(10, LED_BLINK_TIME)) return;
		turnOffLEDs(); // turn off all LEDs
	}

	// turn all LEDs off
	turnOffLEDs(); // writes to first 8 gpio in wiring Pi
	
	// delay until web page changed
	while (currentApp == '2') {
		ledDelay(11, LED_BLINK_TIME);
	}
	
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
		char phresB = (digitalRead(PHRES_B3)<<3)+
			(digitalRead(PHRES_B4)<<4)+(digitalRead(PHRES_B5)<<5)+
			(digitalRead(PHRES_B6)<<6)+(digitalRead(PHRES_B7)<<7);
		char readingAvg = (phresA>>1)+(phresB>>1); // add and divide by 2

		// recalculate running average
		phAvg -= (phAvg>>runningAvgShift); // subtract average reading
		//phAvg += (phresA>>runningAvgShift); // add new reading
		phAvg += (readingAvg>>runningAvgShift); // add new reading

		//printf("Photoresistor A: %d\n", phresA);
		//printf("Photoresistor B: %d\n", phresB);
		//printf("Photoresistor Avg: %d\n", readingAvg);
		
		// LED color
		if (ledThreadRunning==1) {
			// TODO find a good adjustment formula
			pwmWrite(BLUE, 1023-(phAvg*2));
			pwmWrite(RED, 600-(phAvg*2));
			pwmWrite(GREEN, 400-(phAvg*2));
		} else { // keep LOW output while LEDs off
			turnOffPWM();
		}
		
		// wait until next period
		delay(PHRES_READ_PERIOD);
	}
}

// Button Left Thread
PI_THREAD(btnLeftThread) {
	piHiPri(90); // high priority
	while(1) {
		if (blockButtonPress == 0) {
			while(digitalRead(BUTTON_LEFT)==1) {
				blockButtonPress = 1; // block all concurrent button presses
				// button pressed (block bouncing)
				delay(BUTTON_WAIT); // min button press duration (WAIT + POLLING)
				if (digitalRead(BUTTON_LEFT)==1) { // ignore if pressed less than 30 ms
					while (digitalRead(BUTTON_LEFT)==1) {
						// wait for button release
						delay(BUTTON_WAIT); // depends on our specs
					}
					// button released
					// countLeft++;
						system(goLeft.c_str());
						currentApp = currentApp - 1;
      					if (currentApp == 0) currentApp = 3;
					}
					// password input
					else if ((currentApp == '0') && (pwInd < 6)) {
						password[pwInd] = 'L';
						pwInd++;
						timePasswordInputReceived = millis(); // start/reset timer
					}
				}
			}
			blockButtonPress = 0; // unbock all button presses	
		}
		delay(BUTTON_POLLING_TIME);
	}
}

// Button Right Thread
PI_THREAD(btnRightThread) {
	piHiPri(90); // high priority
	while(1) {
		if (blockButtonPress == 0) {
			while(digitalRead(BUTTON_RIGHT)==1) {
				blockButtonPress = 1; // block all concurrent button presses
				// button pressed (block bouncing)
				delay(BUTTON_WAIT); // min button press duration (WAIT + POLLING)
				if (digitalRead(BUTTON_RIGHT)==1) { // ignore if pressed less than 30 ms
					while (digitalRead(BUTTON_RIGHT)==1) {
						// wait for button release
						delay(BUTTON_WAIT); // depends on our specs
					}
					// button released
					// countRight++;
					system(goRight.cstr());
					currentApp = currentApp + 1;
					if(currentApp == 4) currentApp = 1;
			
					// password input
					else if ((currentApp == 0) && (pwInd < 6)) {
						password[pwInd] = 'R';
						pwInd++;
						timePasswordInputReceived = millis(); // start/reset timer
					}
				}
			}
			blockButtonPress = 0; // unbock all button presses	
		}
		delay(BUTTON_POLLING_TIME);
	}
}

// Button Center Thread
PI_THREAD(btnCenterThread) {
	piHiPri(90); // high priority
	while(1) {
		if (blockButtonPress == 0) {
			while(digitalRead(BUTTON_CENTER)==1) {
				blockButtonPress = 1; // block all concurrent button presses
				timeButtonCenterPressed = millis(); // capture time button pressed
				// button pressed (block bouncing)
				delay(BUTTON_WAIT); /// min button press duration (WAIT + POLLING)
				if (digitalRead(BUTTON_CENTER)==1) { // ignore if pressed less than 30 ms
					while (digitalRead(BUTTON_CENTER)==1) {
						// wait for button release
						delay(BUTTON_WAIT); // depends on our specs
						if((millis()-timeButtonCenterPressed)>2000) {
							break;
						}
					}
					// button released
					if((millis()-timeButtonCenterPressed)>2000) {
						// Sign out
						goToWelcomePage();
						while(digitalRead(BUTTON_CENTER)==1) {
							// wait until user releases button
						}
					}
					// countCenter++;
					// TO-DO: handle turn on/off tv
					else if(currentApp=='2'){
						// TO-DO: handle start/stop video
					}
					// password input
					else if ((currentApp == 0) && (pwInd < 6)) {
						password[pwInd] = 'C';
						pwInd++;
						timePasswordInputReceived = millis(); // start/reset timer
					}
				}
			}
			blockButtonPress = 0; // unbock all button presses	
		}
		delay(BUTTON_POLLING_TIME);
	}
}

/** Initialize i/o, threads, and variables
 *
 */
int rpi_init(){
	signal(SIGINT, INThandler); // handle exit

	// Initialize variables
	//firefoxCall = "sudo -u $SUDO_USER firefox localhost:8000/page0.html &"; // initialize to welcome page
	currentApp = 0;
	currentUser = '0'; // default user is guest
	ledThreadRunning = 0;
	phAvg = 64; // initialize to mean (range [0, 127])
	terminateCode = 0;
	pwInd = 0;
	blockButtonPress = 0; // unblock button presses
	timePasswordInputReceived = 0;

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
	// pinMode(LED3, OUTPUT);
	// pinMode(LED4, OUTPUT);
	// pinMode(LED5, OUTPUT);
	// pinMode(LED6, OUTPUT);
	// pinMode(LED7, OUTPUT);
	// pinMode(LED8, OUTPUT);
	pinMode(RED, PWM_OUTPUT);
	pinMode(GREEN, PWM_OUTPUT);
	pinMode(BLUE, PWM_OUTPUT);
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

/** Turns off all LEDs
 * 
 */
void turnOffLEDs() {
	digitalWrite(LED1, LOW);
	digitalWrite(LED2, LOW);
	/*digitalWrite(LED3, LOW);
	digitalWrite(LED4, LOW);
	digitalWrite(LED5, LOW);
	digitalWrite(LED6, LOW);
	digitalWrite(LED7, LOW);
	digitalWrite(LED8, LOW);*/
}

/** Turns on all LEDs
 * 
 */
void turnOnLEDs() {
	digitalWrite(LED1, HIGH);
	digitalWrite(LED2, HIGH);
	/*digitalWrite(LED3, HIGH);
	digitalWrite(LED4, HIGH);
	digitalWrite(LED5, HIGH);
	digitalWrite(LED6, HIGH);
	digitalWrite(LED7, HIGH);
	digitalWrite(LED8, HIGH);*/
}

/** Turns off all PWMs
 * 
 */
void turnOffPWM() {
	pwmWrite(RED, 0);
	pwmWrite(GREEN, 0);
	pwmWrite(BLUE, 0);
}
