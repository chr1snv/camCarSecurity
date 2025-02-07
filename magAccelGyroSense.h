#include <Wire.h>

//#include <QMC5883LCompass.h>
#include <HMC5883L_Simple.h>

//QMC5883LCompass compass;
HMC5883L_Simple compass;

void mag_init(){
  // Initialize the Compass.
  //compass.init();
  compass.SetDeclination(11, 25, 'E'); //https://www.magnetic-declination.com
  compass.SetSamplingMode(COMPASS_SINGLE);
  //compass.SetScale(COMPASS_SCALE_088);
}

int MAG_ALARM_DELTA_THRESHOLD = 30;
//HMC5883L_Simple::MagnetometerSample alarmInit_magSample;
float magHeading;
int magX, magY, magZ;
void mag_sense(){
  // Read compass values
  //Serial.print("compass read ");
  
  magHeading = compass.GetHeadingDegrees(); //calls ReadAxies and fills in compass.magSample
  //Serial.println( magHeading );
  magX = compass.magSample.X;
  magY = compass.magSample.Y;
  magZ = compass.magSample.Z;
  //compass.read();
  //int x = compass.getX();
  //int y = compass.getY();
  //int z = compass.getZ();
}

bool magAlarmTriggered = false;
int magAX, magAY, magAZ;
void mag_alarm_init(){
	magAX = magX;
	magAY = magY;
	magAZ = magZ;
}

int magAlarmDiff;
void mag_checkAlarmTriggered(){
	int mXD = magX-magAX;//compass.magSample.X - alarmInit_magSample.X;
	int mYD = magY-magAY;//compass.magSample.Y - alarmInit_magSample.Y;
	int mZD = magZ-magAZ;//compass.magSample.Z - alarmInit_magSample.Z;

	magAlarmDiff = sqrt( mXD*mXD + mYD*mYD + mZD*mZD );

	if( magAlarmDiff > MAG_ALARM_DELTA_THRESHOLD ){
		magAlarmTriggered = true;
		UpdateAlarmOutput( true );
	}else{
		magAlarmTriggered = false;
	}

}

//accelerometer- gyroscope GY-521
const int MPU=0x68; 
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
void accelGyro_init(){
	//Wire.begin();
	Wire.beginTransmission(MPU);
	Wire.write(0x6B);  
	Wire.write(0);    
	Wire.endTransmission(true);
	//Serial.begin(9600);
}
void accelGyro_sense(){
	Wire.beginTransmission(MPU);
	Wire.write(0x3B);  
	Wire.endTransmission(false);
	Wire.requestFrom(MPU,12,true);  
	AcX=Wire.read()<<8|Wire.read();    
	AcY=Wire.read()<<8|Wire.read();  
	AcZ=Wire.read()<<8|Wire.read();  
	GyX=Wire.read()<<8|Wire.read();  
	GyY=Wire.read()<<8|Wire.read();  
	GyZ=Wire.read()<<8|Wire.read();  

	Serial.print("Accelerometer: ");
	Serial.print("X = "); Serial.print(AcX);
	Serial.print(" | Y = "); Serial.print(AcY);
	Serial.print(" | Z = ");  Serial.println(AcZ); 

	Serial.print("Gyroscope: ");
	Serial.print("X  = "); Serial.print(GyX);
	Serial.print(" | Y = "); Serial.print(GyY);
	Serial.print(" | Z = "); Serial.println(GyZ);
	Serial.println(" ");
	delay(333);
}

void ori_init(){
	Wire.begin(14, 15, 100000);
	mag_init();
	//accelGyro_init();
}

void ori_sense(){
	//accelGyro_sense();
	mag_sense();
	/*
	Serial.print("magAxies ");
	Serial.print(compass.magSample.X);
	Serial.print(" ");
	Serial.print(compass.magSample.Y);
	Serial.print(" ");
	Serial.println(compass.magSample.Z);
	*/
}