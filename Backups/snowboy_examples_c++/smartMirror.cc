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
#include <string>
#include <cassert>
#include <csignal>
#include <iostream>
#include <pa_ringbuffer.h>
#include <pa_util.h>
#include <portaudio.h>
#include <vector>

#include "include/snowboy-detect.h"

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
std::string firefoxCall = "sudo -u $SUDO_USER firefox ";
string baseURL = "localhost:8000/";
string welcomePage = "welcome.html";
string mainPage = "mirror.html";
string apps[] = {"welcome", "calendar", "weather", "youtube"};
//string pages[] = ["page0.html", "page1.html", "page2.html", "page3.html"];

string goLeft = "sh left.sh";
string goRight = "sh right.sh";
string goCalendar = "sh c.sh";
string goWeather = "sh w.sh";
string goYoutube = "sh y.sh";
string goMiroslav = "sh m.sh";

char password[6]; // array when password entered
int pwInd; // password current index (track password input)
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


///////Voice

int PortAudioCallback(const void* input,
                      void* output,
                      unsigned long frame_count,
                      const PaStreamCallbackTimeInfo* time_info,
                      PaStreamCallbackFlags status_flags,
                      void* user_data);

class PortAudioWrapper {
 public:
  // Constructor.
  PortAudioWrapper(int sample_rate, int num_channels, int bits_per_sample) {
    num_lost_samples_ = 0;
    min_read_samples_ = sample_rate * 0.1;
    Init(sample_rate, num_channels, bits_per_sample);
  }

  // Reads data from ring buffer.
  template<typename T>
  void Read(std::vector<T>* data) {
    assert(data != NULL);

    // Checks ring buffer overflow.
    if (num_lost_samples_ > 0) {
      std::cerr << "Lost " << num_lost_samples_ << " samples due to ring"
          << " buffer overflow." << std::endl;
      num_lost_samples_ = 0;
    }

    ring_buffer_size_t num_available_samples = 0;
    while (true) {
      num_available_samples =
          PaUtil_GetRingBufferReadAvailable(&pa_ringbuffer_);
      if (num_available_samples >= min_read_samples_) {
        break;
      }
      Pa_Sleep(5);
    }

    // Reads data.
    num_available_samples = PaUtil_GetRingBufferReadAvailable(&pa_ringbuffer_);
    data->resize(num_available_samples);
    ring_buffer_size_t num_read_samples = PaUtil_ReadRingBuffer(
        &pa_ringbuffer_, data->data(), num_available_samples);
    if (num_read_samples != num_available_samples) {
      std::cerr << num_available_samples << " samples were available,  but "
          << "only " << num_read_samples << " samples were read." << std::endl;
    }
  }

  int Callback(const void* input, void* output,
               unsigned long frame_count,
               const PaStreamCallbackTimeInfo* time_info,
               PaStreamCallbackFlags status_flags) {
    // Input audio.
    ring_buffer_size_t num_written_samples =
        PaUtil_WriteRingBuffer(&pa_ringbuffer_, input, frame_count);
    num_lost_samples_ += frame_count - num_written_samples;
    return paContinue;
  }

  ~PortAudioWrapper() {
    Pa_StopStream(pa_stream_);
    Pa_CloseStream(pa_stream_);
    Pa_Terminate();
    PaUtil_FreeMemory(ringbuffer_);
  }

 private:
  // Initialization.
  bool Init(int sample_rate, int num_channels, int bits_per_sample) {
    // Allocates ring buffer memory.
    int ringbuffer_size = 16384;
    ringbuffer_ = static_cast<char*>(
        PaUtil_AllocateMemory(bits_per_sample / 8 * ringbuffer_size));
    if (ringbuffer_ == NULL) {
      std::cerr << "Fail to allocate memory for ring buffer." << std::endl;
      return false;
    }

    // Initializes PortAudio ring buffer.
    ring_buffer_size_t rb_init_ans =
        PaUtil_InitializeRingBuffer(&pa_ringbuffer_, bits_per_sample / 8,
                                    ringbuffer_size, ringbuffer_);
    if (rb_init_ans == -1) {
      std::cerr << "Ring buffer size is not power of 2." << std::endl;
      return false;
    }

    // Initializes PortAudio.
    PaError pa_init_ans = Pa_Initialize();
    if (pa_init_ans != paNoError) {
      std::cerr << "Fail to initialize PortAudio, error message is \""
          << Pa_GetErrorText(pa_init_ans) << "\"" << std::endl;
      return false;
    }

    PaError pa_open_ans;
    if (bits_per_sample == 8) {
      pa_open_ans = Pa_OpenDefaultStream(
          &pa_stream_, num_channels, 0, paUInt8, sample_rate,
          paFramesPerBufferUnspecified, PortAudioCallback, this);
    } else if (bits_per_sample == 16) {
      pa_open_ans = Pa_OpenDefaultStream(
          &pa_stream_, num_channels, 0, paInt16, sample_rate,
          paFramesPerBufferUnspecified, PortAudioCallback, this);
    } else if (bits_per_sample == 32) {
      pa_open_ans = Pa_OpenDefaultStream(
          &pa_stream_, num_channels, 0, paInt32, sample_rate,
          paFramesPerBufferUnspecified, PortAudioCallback, this);
    } else {
      std::cerr << "Unsupported BitsPerSample: " << bits_per_sample
          << std::endl;
      return false;
    }
    if (pa_open_ans != paNoError) {
      std::cerr << "Fail to open PortAudio stream, error message is \""
          << Pa_GetErrorText(pa_open_ans) << "\"" << std::endl;
      return false;
    }

    PaError pa_stream_start_ans = Pa_StartStream(pa_stream_);
    if (pa_stream_start_ans != paNoError) {
      std::cerr << "Fail to start PortAudio stream, error message is \""
          << Pa_GetErrorText(pa_stream_start_ans) << "\"" << std::endl;
      return false;
    }
    return true;
  }

 private:
  // Pointer to the ring buffer memory.
  char* ringbuffer_;

  // Ring buffer wrapper used in PortAudio.
  PaUtilRingBuffer pa_ringbuffer_;

  // Pointer to PortAudio stream.
  PaStream* pa_stream_;

  // Number of lost samples at each Read() due to ring buffer overflow.
  int num_lost_samples_;

  // Wait for this number of samples in each Read() call.
  int min_read_samples_;
};

int PortAudioCallback(const void* input,
                      void* output,
                      unsigned long frame_count,
                      const PaStreamCallbackTimeInfo* time_info,
                      PaStreamCallbackFlags status_flags,
                      void* user_data) {
  PortAudioWrapper* pa_wrapper = reinterpret_cast<PortAudioWrapper*>(user_data);
  pa_wrapper->Callback(input, output, frame_count, time_info, status_flags);
  return paContinue;
}

void SignalHandler(int signal){
  std::cerr << "Caught signal " << signal << ", terminating..." << std::endl;
  exit(0);
}

void hotwordDetected(int hotword) {
	//Todo make sure we're signed in
  if (hotword == 1) {
    // page 1
    printf("1");
    //goToMainPage(1, currentUser);
    system(goCalendar.c_str());
  } else if (hotword == 2) {
    // page 2
	printf("2");
	//goToMainPage(2, currentUser);
    system(goWeather.c_str());


  } else if (hotword == 3) {
    // page 3
	printf("3");
    //goToMainPage(3, currentUser);
    system(goYoutube.c_str());

  } else if (hotword == 4) {
    // miroslav
     system(goMiroslav.c_str());


  } else if (hotword == 5) {
    // vuk

  } else if (hotword == 6) {
    // safkat

  } else if (hotword == 7) {
    // turn off
  } else if (hotword == 8) {
    // turn on
  } else if (hotword == 9) {
    // home
    //goToURL(0, '0');
  }

}


////// End Voice

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
				fprintf(stderr, "Unable to start LED thread: %s\n", strerror(errno));
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
  url += "&";
  url += "user="; 
  url += user;
  url += " &";

  openURL(url);
 
}

void switchToApp(int appIndex) {
		
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
	if(ledDelay(1, LED_SWITCH_TIME)) return NULL;
	// TODO: do we want to wait longer?
	// How long does the Youtube video take to load/start?
	digitalWrite(LED1, HIGH);
	if(ledDelay(2, LED_SWITCH_TIME)) return NULL;
	digitalWrite(LED2, HIGH);
	// if(ledDelay(3, LED_SWITCH_TIME)) return NULL;
	// digitalWrite(LED3, HIGH);
	// if(ledDelay(4, LED_SWITCH_TIME)) return NULL;
	// digitalWrite(LED4, HIGH);
	// if(ledDelay(5, LED_SWITCH_TIME)) return NULL;
	// digitalWrite(LED5, HIGH);
	// if(ledDelay(6, LED_SWITCH_TIME)) return NULL;
	// digitalWrite(LED6, HIGH);
	// if(ledDelay(7, LED_SWITCH_TIME)) return NULL;
	// digitalWrite(LED7, HIGH);
	// if(ledDelay(8, LED_SWITCH_TIME)) return NULL;
	// digitalWrite(LED8, HIGH);

	// Blink for LED_SWITCH_TIME to indicate timer done
	int i = 0;
	for (; i<LED_SWITCH_TIME; i+=LED_BLINK_TIME+LED_BLINK_TIME) {
		if(ledDelay(10, LED_BLINK_TIME)) return NULL;
		turnOnLEDs(); // turn on all LEDs
		if(ledDelay(10, LED_BLINK_TIME)) return NULL;
		turnOffLEDs(); // turn off all LEDs
	}

	// turn all LEDs off
	turnOffLEDs(); // writes to first 8 gpio in wiring Pi
	
	// delay until web page changed
	while (currentApp == '2') {
		ledDelay(11, LED_BLINK_TIME);
	}
	
	ledThreadRunning = 0; // unblock led thread
	return NULL;
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
					if(currentApp>0){
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
					if(currentApp > 0) {
						system(goRight.c_str());
						currentApp = currentApp + 1;
						if(currentApp == 4) currentApp = 1;
					}
			
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

PI_THREAD(voiceThread) {
	std::string usage =
      "Example that shows how to use Snowboy in C++. Parameters are\n"
      "hard-coded in the parameter section. Please check the source code for\n"
      "more details. Audio is captured by PortAudio.\n"
      "\n"
      "To run the example:\n"
      "  ./demo\n";

  // Configures signal handling.
   struct sigaction sig_int_handler;
   sig_int_handler.sa_handler = SignalHandler;
   sigemptyset(&sig_int_handler.sa_mask);
   sig_int_handler.sa_flags = 0;
   sigaction(SIGINT, &sig_int_handler, NULL);

  // Parameter section.
  // If you have multiple hotword models (e.g., 2), you should set
  // <model_filename> and <sensitivity_str> as follows:
  //   model_filename = "resources/snowboy.umdl,resources/alexa.pmdl";
  //   sensitivity_str = "0.4,0.4";
  std::string resource_filename = "resources/common.res";
  std::string model_filename = "resources/calendar.pmdl,resources/weather.pmdl,resources/youtube.pmdl,resources/miroslav.pmdl";
  std::string sensitivity_str = "0.5,0.5,0.5,0.5";
  float audio_gain = 1;

  // Initializes Snowboy detector.
  snowboy::SnowboyDetect detector(resource_filename, model_filename);
  detector.SetSensitivity(sensitivity_str);
  detector.SetAudioGain(audio_gain);

  // Initializes PortAudio. You may use other tools to capture the audio.
  PortAudioWrapper pa_wrapper(detector.SampleRate(),
                              detector.NumChannels(), detector.BitsPerSample());

  // Runs the detection.
  // Note: I hard-coded <int16_t> as data type because detector.BitsPerSample()
  //       returns 16.
  std::cout << "Listening... Press Ctrl+C to exit" << std::endl;
  std::vector<int16_t> data;
  while (true) {
    pa_wrapper.Read(&data);
    if (data.size() != 0) {
      int result = detector.RunDetection(data.data(), data.size());
      if (result > 0) {
        std::cout << "Hotword " << result << " detected!" << std::endl;
        hotwordDetected(result);
      }
    }
  }

  return NULL;
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
		fprintf(stderr, "Unable to setup wiringPi: %s\n", strerror(errno));
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
		fprintf(stderr, "Unable to start Photoresistor thread: %s\n", strerror(errno));
		//return 1;
	}

	if(piThreadCreate(btnLeftThread) != 0){
		fprintf(stderr, "Unable to start Button Left thread: %s\n", strerror(errno));
		return 1;
	}

	if(piThreadCreate(btnRightThread) != 0){
		fprintf(stderr, "Unable to start Button Right thread: %s\n", strerror(errno));
		return 1;
	}

	if(piThreadCreate(btnCenterThread) != 0){
		fprintf(stderr, "Unable to start Button Center thread: %s\n", strerror(errno));
		return 1;
	}

	if(piThreadCreate(voiceThread) != 0){
		fprintf(stderr, "Unable to start Voice thread: %s\n", strerror(errno));
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
