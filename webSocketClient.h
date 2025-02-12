#ifndef Web_Socket_Client
#define Web_Socket_Client

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "Arduino.h"
#include "esp_websocket_client.h"

#define WS_LOG_TAG "ws"
//const char tag[] = "ws";

void log_error_if_nonzero(String s, int v);

//refrence
//https://github.com/espressif/esp-protocols/blob/master/components/esp_websocket_client/examples/target/main/websocket_example.c
extern uint16_t payloadLen;
extern const char * payload;


static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
  ESP_LOGI( tag, "websocket_event_handler\n" );
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



void printPayload(uint16_t start, uint16_t end, const char * payload );

uint8_t doCommandsInRecievedData( uint16_t payloadLen, const char * payload );

#endif