void log_error_if_nonzero(String s, int v){
	if( v != 0)
		ESP_LOGI(TAG, s);
}

//refrence
//https://github.com/espressif/esp-protocols/blob/master/components/esp_websocket_client/examples/target/main/websocket_example.c
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
	switch (event_id) {
		case WEBSOCKET_EVENT_BEGIN:
			ESP_LOGI(TAG, "WEBSOCKET_EVENT_BEGIN");
			break;
		case WEBSOCKET_EVENT_CONNECTED:
			ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
			break;
		case WEBSOCKET_EVENT_DISCONNECTED:
      		ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
			break;
		case WEBSOCKET_EVENT_ERROR:
			ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
			log_error_if_nonzero("HTTP status code",  data->error_handle.esp_ws_handshake_status_code);
			if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
				log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
				log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
				log_error_if_nonzero("captured as transport's socket errno",  data->error_handle.esp_transport_sock_errno);
			}
			break;
		case WEBSOCKET_EVENT_FINISH:
			ESP_LOGI(TAG, "WEBSOCKET_EVENT_FINISH");
			break;
		case WEBSOCKET_EVENT_DATA:
			ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
			int len = data->payload_len;
			const char * buff = (const char *)&data->data_ptr[data->payload_offset];
			Serial.println(data->op_code);   //Print return code
			//check the response for pending commands
			if ( len > 0 ){
				//numCmds can be 0-9 (one ascii digit)
				//numCmds(1)|cmdName(16)|value(32)|...
				uint8_t numCmds = buff[0] - '0';
				if( numCmds > 0 ){ // a command was recieved
					activelyCommanded = NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND;
					//do all recieved commands
					uint16_t idx = 0;
					for( uint8_t i = 0; i < numCmds; ++i ){
						doCommand( &(buff[idx]), &(buff[idx+CMD_LEN]) );
						idx += CMD_LEN + 32;
					}
				}
			}
			break;
	}
}