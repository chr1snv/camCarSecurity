//the idea of this is to have some 
//cameras
//ultrasonic or microwave radar distance sensor to detect intrusion under the car
//acclerometer / tilt sensor (to detect jacking up, or moving of the car)

//and connect to known wifi networks 
//(house, work, mobile hotspot, etc)
//to allow monitoring / upload data to a (cloud up/down link)
//internet server

//if in alarm mode and tilt or distance sensors are triggered or
//if the connection breaks to the internet server when in alarm mode
//then ALARM

//ALARM (turned on/off by sensors or remotely from cloud link)
//flashes lights / sounds siren, notifies webserver and sends a 
//push notification via web app connection or email/text
//(also maybe starts car engine)

#include "esp_camera.h"

#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"             // disable brownout problems
#include "soc/rtc_cntl_reg.h"    // disable brownout problems

#include <Preferences.h> //for non volitile memory storage
Preferences preferences;

#include "GlobalDefinesAndFunctions.h"



#include <ESP32Servo.h>
const uint8_t ServoOutputPins[] = { 
    12/*14*/,
    13/*15*/,
    15,
    14,
    2,
    4
  };
uint8_t numServos = 0;
#define MAX_NUM_SERVOS 6
Servo servos[MAX_NUM_SERVOS];
int servoAngles[MAX_NUM_SERVOS];
int defaultServoAngles[MAX_NUM_SERVOS] = {90,90,90, 90,90,90};

bool hasLight;
bool hasMagSensor;

//init servos //to be used after number of servos changes
void initServos(){
  for(uint8_t sNum = 0; sNum < MAX_NUM_SERVOS; ++sNum){
    servos[sNum].detach();
  }
  Serial.print( "setting up " ); Serial.print( numServos ); Serial.println( " num servos" );
  for(uint8_t sNum = 0; sNum < numServos; ++sNum){
	  servos[sNum].setPeriodHertz(50);    // standard 50 hz servo
	  servos[sNum].attach( ServoOutputPins[sNum] ); //defaults to 500, 2500 microseconds //, 1000, 2000);
    servoAngles[sNum] = defaultServoAngles[sNum];
	  servos[sNum].write(servoAngles[sNum]); //start at default angle
  }
}

#define ALARM_OUTPUT_PIN 2 //pull down strapping pin

#define LightLEDPin  4
#define redLEDPin    33
bool lightLedValue = false;

//#define SERVO_STEP   5 //pull up strapping pin

bool alarmArmed = false;
bool alarmOutput = false;



#include "morseCode.h"

#include "temperatureSensor.h"
#include "signalStrengthAndMotionDetection.h"
#include "camera.h"


void UpdateAlarmOutput(bool on){
	alarmOutput = on;
	digitalWrite(ALARM_OUTPUT_PIN, LOW);
}

#include "magAccelGyroSense.h"

void ArmAlarm(bool enable){
	if(enable){
		mag_alarm_init();
	}
	UpdateAlarmOutput( false );
	alarmArmed = enable;
}


int activelyCommanded = 0;
uint16_t connectionAttempts = 0;
bool settingsRequested = false;

uint8_t mainLoopDelayMillis = 100;

#include "wifiConnection.h"
#include "webserver.h"

void setup() {
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  //read config
  preferences.begin("storedVals", true);
    numServos = preferences.getUChar( "numServos" );
    hasLight = preferences.getBool( "hasLight" );
    hasMagSensor = preferences.getBool( "hasMagSensor" );
  preferences.end();

  //setup servos
	initServos();

	//init lights
  if( hasLight ){
	  pinMode(LightLEDPin, OUTPUT);
	  digitalWrite(redLEDPin, LOW);
  }

	//initalize sensor communications
  if( hasMagSensor ){
	  ori_init();
    ArmAlarm(false); //set the siren output low
  }
	temp_init();
	camera_init();

	//init serial ouput
	Serial.begin(115200);
	Serial.setDebugOutput(false);

	//print system info
	Serial.print("Total heap: ");
	Serial.println(ESP.getHeapSize());
	Serial.print("Free heap: ");
	Serial.println(ESP.getFreeHeap());
	Serial.print("Total PSRAM: ");  //psram is spi connected ram
	Serial.println( ESP.getPsramSize());
	Serial.print("Free PSRAM: ");
	Serial.println(ESP.getFreePsram());

	//turn on status led
  pinMode(redLEDPin, OUTPUT);
	digitalWrite(redLEDPin, HIGH);

	//esp_wifi_init();
	connectWiFi(wifi_scanNetworks());

	if( APssid[0] != 0){ //if local ap mode (couldn't find network)
		// Start local web server (should only start if can't connect to wifi or cloud server)
		startCameraServer();
		Serial.print("http://");
		Serial.print( WiFi.softAPIP() );
		Serial.print( " on ");
		Serial.println( APssid );
	}else{ //if joined wifi network as client station
		Serial.print( "local IP " );
		Serial.print( WiFi.localIP() );
		Serial.print( " on " );
		Serial.println( foundNetwork );
	}
}

#define MAIN_LOOPS_BTWN_STAT_PRINTS 100
uint8_t mainLoopsUntilPrintStatus = MAIN_LOOPS_BTWN_STAT_PRINTS;
void loop() {
	
	if(--mainLoopsUntilPrintStatus < 1){
		mainLoopsUntilPrintStatus = MAIN_LOOPS_BTWN_STAT_PRINTS;
		uint8_t sNum = WiFi.softAPgetStationNum(); //print the number of connected clients (other esp32's)
		Serial.print("nCli ");
    Serial.print(sNum);
		Serial.print(" mLoopsSinceWSConec ");
    Serial.println(mainLoopsSinceWebSockStartedConnecting);
    char loopsSinceConnectStr[16];
    uint8_t lscsLen = snprintf( loopsSinceConnectStr, 16, " swc %i  ", mainLoopsSinceWebSockStartedConnecting );
    queueStringForMorseLedOutput(loopsSinceConnectStr, lscsLen );
	}

	//read sensors
	temp_sense();
	sense_wifi_rssi();
  if( hasMagSensor )
	  ori_sense();

	//check if alarm conditions met
	if( alarmArmed ){
		mag_checkAlarmTriggered();
	}

	if( WiFi.status() == WL_CONNECTED || APssid[0] == 0 ){ //connected to a wifi network

		bool sendData = true;
		if( activelyCommanded <= 0 ){ //if it's been NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND
			// less than 0 means inactive, when inactive if waited more than Inactive Interval
			//attempt output/communication transmission
			if( activelyCommanded < -INACTIVE_COMMAND_LOOP_INTERVAL ){ 
				activelyCommanded = 0; //only send after interval num loops
			}else{
				sendData = false; //skip sending for INACTIVE_COMMAND_LOOP_INTERVAL loops
			}
		}
		if(sendData){
      if( activelyCommanded > 0 ) //only send when commanded to save bandwidth
			  PostAndFetchDataFromCloudServer(IMAGE);  //send image
			PostAndFetchDataFromCloudServer(DEV_STATUS); //send status
		}
    --activelyCommanded;

		doCommandsInRecievedData(payloadLen, payload);

		if( webSockClient != NULL && !esp_websocket_client_is_connected(webSockClient) )
			mainLoopsSinceWebSockStartedConnecting += 1;
	}else{
		Serial.print("."); //print a dot while not connected to an ap for uplink
	}
	
  morseOutputLedUpdate( redLEDPin, true );

	delay(mainLoopDelayMillis);
}