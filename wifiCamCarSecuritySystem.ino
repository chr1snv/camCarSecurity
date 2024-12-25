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


#define PART_BOUNDARY "123456789000000000000987654321"

#define SERVO_1      12//14
#define SERVO_2      13//15

#define ALARM_OUTPUT_PIN 2 //pull down strapping pin

#define LightLEDPin  4
#define redLEDPin    33
bool lightLedValue = false;

//#define SERVO_STEP   5 //pull up strapping pin

bool alarmArmed = false;
bool alarmOutput = false;
void UpdateAlarmOutput(bool on){
  alarmOutput = on;
  digitalWrite(ALARM_OUTPUT_PIN, LOW);
}

#include "esp_http_server.h"
#include <ESP32Servo.h>
#include "temperatureSensor.h"
#include "signalStrengthAndMotionDetection.h"
#include "camera.h"
#include "magAccelGyroSense.h"

Servo servo1;
Servo servo2;

int servo1Angle = 90;
int servo2Angle = 90;


void ArmAlarm(bool enable){
  if(enable){
    mag_alarm_init();
  }
  UpdateAlarmOutput( false );
  alarmArmed = enable;
}

#include "wifiConnection.h"
#include "webserver.h"

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  //init servos
  servo1.setPeriodHertz(50);    // standard 50 hz servo
  servo2.setPeriodHertz(50);    // standard 50 hz servo
  servo1.attach( SERVO_1 ); //defaults to 500, 2500 microseconds //, 1000, 2000);
  servo2.attach( SERVO_2 ); //, 1000, 2000);
  servo1.write(servo1Angle);
  servo2.write(servo2Angle);

  //init lights
  pinMode(LightLEDPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);
  digitalWrite(redLEDPin, LOW);

  //initalize sensor communications
  ori_init();
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
  digitalWrite(redLEDPin, HIGH);

  ArmAlarm(false); //set the siren output low

  //esp_wifi_init();
  wifi_scanNetworks();
  connectWiFi();
  
  // Start streaming web server
  startCameraServer();

  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.softAPIP());//localIP());
}

#define PRINT_PER_LINE 10
uint8_t conPrints = PRINT_PER_LINE;
void loop() {
  if(WiFi.status() != WL_CONNECTED){ //print a dot while not connected to an ap for uplink
    Serial.print(".");
  }
  uint8_t sNum = WiFi.softAPgetStationNum(); //print the number of connected clients (other esp32's)
  Serial.print(sNum);

  if(--conPrints < 1){
    conPrints = PRINT_PER_LINE;
    Serial.println("");
  }

  //read sensors
  ori_sense();
  //check if alarm conditions met
  if( alarmArmed ){
    mag_checkAlarmTriggered();
  }

  delay(100);
}