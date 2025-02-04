#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

const char tag[] = "ws";

void log_error_if_nonzero(String s, int v){
	if( v != 0)
		ESP_LOGI( tag, s );
}

//refrence
//https://github.com/espressif/esp-protocols/blob/master/components/esp_websocket_client/examples/target/main/websocket_example.c
uint16_t payloadLen = 0;
const char * payload;
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
	switch (event_id) {
		case WEBSOCKET_EVENT_BEGIN:
			ESP_LOGI( tag, "WEBSOCKET_EVENT_BEGIN\n" );
			break;
		case WEBSOCKET_EVENT_CONNECTED:
			ESP_LOGI( tag, "WEBSOCKET_EVENT_CONNECTED\n" );
			break;
		case WEBSOCKET_EVENT_DISCONNECTED:
			ESP_LOGI( tag, "WEBSOCKET_EVENT_DISCONNECTED\n" );
			break;
		case WEBSOCKET_EVENT_ERROR:
			ESP_LOGI( tag, "WEBSOCKET_EVENT_ERROR\n" );
			log_error_if_nonzero("HTTP status code\n",  data->error_handle.esp_ws_handshake_status_code);
			if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
				log_error_if_nonzero("reported from esp-tls\n", data->error_handle.esp_tls_last_esp_err);
				log_error_if_nonzero("reported from tls stack\n", data->error_handle.esp_tls_stack_err);
				log_error_if_nonzero("captured as transport's socket errno\n",  data->error_handle.esp_transport_sock_errno);
			}
			break;
		case WEBSOCKET_EVENT_FINISH:
			ESP_LOGI( tag, "WEBSOCKET_EVENT_FINISH\n" );
			break;
		case WEBSOCKET_EVENT_DATA:
			ESP_LOGI( tag, "WEBSOCKET_EVENT_DATA len " );
			payloadLen = data->payload_len;
			ESP_LOGI( tag, payloadLen );
			payload = (const char *)&data->data_ptr[data->payload_offset];
			ESP_LOGI( tag, " op_code " );
			ESP_LOGI( tag, data->op_code );   //Print return code
			break;
	}
}

void printPayload(uint16_t start, uint16_t end){
	uint16_t maxPrintIdx = min( end, (uint16_t)(payloadLen-1) );
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

void doCommandsInRecievedData(){
	//check the response for pending commands
	if ( payloadLen > WEB_SEND_HDR_LEN-1 ){
    Serial.print( "pLen "); Serial.println( payloadLen );
		//numData can be 0-9 (one ascii digit)
		//|numData(1) | 'd'(1) |dataTypeStr (11) | deviceId(4) | dataLen(6) | data
		printPayload( 0, WEB_SEND_HDR_LEN );
		
		uint8_t numData = payload[0] - '0';
		char from = payload[1];
		Serial.print(" numData ");
		Serial.println( numData );
		
		if( numData > 0 && numData < 10 ){ // a number of commands was recieved
			activelyCommanded = NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND;
			//do all recieved commands
			uint16_t idx = 2;
			for( uint8_t i = 0; i < numData; ++i ){
				Serial.print("i "); Serial.println(i);
				printPayload( idx, (uint16_t)(idx+WEB_SEND_HDR_LEN) );

				uint16_t datTypeIdx = idx;
				uint16_t datDevNumIdx = datTypeIdx+DAT_TYPE_LEN;
				uint16_t datDatLenIdx = datDevNumIdx+DAT_DEV_ID_LEN;
				uint16_t datDatIdx = datDatLenIdx+DAT_DAT_LEN;
				uint16_t datLen = atoir_n(&(payload[datDatLenIdx+DAT_DAT_LEN-1]), 6);
				Serial.print( " datDatLenIdx " ); Serial.print( datDatLenIdx );
				Serial.print( " datDatIdx " ); Serial.print( datDatIdx );
				Serial.print( " datLen "); Serial.println( datLen );
				doCommand( &(payload[datTypeIdx]), datLen, &(payload[datDatIdx]) );
				idx += datDatIdx + datLen;
				Serial.print( " idx " ); Serial.println( idx );
			}
		}
		payloadLen = 0;
		Serial.println( "end doCommandsInRecievedData" );
	}
	
}