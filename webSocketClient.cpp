#include "Arduino.h"
//#include <stdint.h>
//#include "HardwareSerial.h"
#include "webSocketClient.h"
//#include <String>
#include "GlobalDefinesAndFunctions.h"

void log_error_if_nonzero(String s, int v){
	if( v != 0)
		ESP_LOGI( tag, s );
}


//refrence
//https://github.com/espressif/esp-protocols/blob/master/components/esp_websocket_client/examples/target/main/websocket_example.c
uint16_t payloadLen = 0;
const char * payload;


void printPayload( uint16_t start, uint16_t end, const char * payload ){
	uint16_t maxPrintIdx = end;
	Serial.print( "payload[");Serial.print(start);Serial.print("-");
	Serial.print(maxPrintIdx);Serial.print("]");
	for(uint8_t k = start; k < maxPrintIdx; ++k){
		if( payload[k] == 0 )
			Serial.print("~");
		else
			Serial.print( payload[k] );
	}
	Serial.print("|\n");
}

uint8_t lastReadPktIdx = 0;
uint8_t doCommandsInRecievedData( uint16_t payloadLen, const char * payload ){
	uint8_t retVal; 

	//check the response for pending commands
	if ( payloadLen > WEB_SEND_HDR_LEN-1 ){
		Serial.print( "pLen "); Serial.println( payloadLen );
		//numData can be 0-9 (one ascii digit)
		//pktIdx(3) deviceId(4) | numData(1) | 'd'(1) || dataTypeStr (11) | dataLen(6) | data ||
		//printPayload( 0, min( (uint16_t)(WEB_SEND_HDR_LEN), payloadLen ), payload );
		
		uint16_t idx = 0;
		uint8_t pktIdx = atoir_n( &payload[idx+2], 3 ); idx +=3;
		Serial.print(" pktIdx ");
		Serial.println( pktIdx );
		if(pktIdx == lastReadPktIdx){
			lastReadPktIdx = pktIdx;
			return 0;
		}else{
			lastReadPktIdx = pktIdx;
		}
		uint16_t fromDevId = atoir_n( &payload[idx+4-1], 4 ); idx += 4;
		uint8_t numData = payload[idx] - '0'; idx += 1;
		char from = payload[idx]; idx += 1;
		Serial.print(" numData ");
		Serial.println( numData );
		
		if( numData > 0 && numData < 10 ){ // a number of commands was recieved
			activelyCommanded = NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND;
			//do all recieved commands
			for( uint8_t i = 0; i < numData; ++i ){
				Serial.print("i "); Serial.println(i);
				printPayload( idx, min( (uint16_t)(idx+WEB_SEND_HDR_LEN), payloadLen ), payload );

				uint16_t datTypeIdx = idx; idx+= DAT_TYPE_LEN;
				uint16_t datDatLenIdx = idx; idx += DAT_DAT_LEN;
				uint16_t datLen = atoir_n(&(payload[datDatLenIdx+DAT_DAT_LEN-1]), DAT_DAT_LEN);
				uint16_t datDatIdx = idx;
				Serial.print( " datDatLenIdx " ); Serial.print( datDatLenIdx );
				Serial.print( " datDatIdx " ); Serial.print( datDatIdx );
				Serial.print( " datLen "); Serial.println( datLen );
				retVal = doCommand( &(payload[datTypeIdx]), datLen, &(payload[datDatIdx]) );
				idx += datLen;
				Serial.print( " idx " ); Serial.println( idx );
			}
		}
		payloadLen = 0;
		Serial.println( "end doCommandsInRecievedData" );
	}

	return retVal;
	
}