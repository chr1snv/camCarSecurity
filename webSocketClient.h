String logStr;

void log_error_if_nonzero(String s, int v){
	if( v != 0)
		logStr += s;
}

//refrence
//https://github.com/espressif/esp-protocols/blob/master/components/esp_websocket_client/examples/target/main/websocket_example.c
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
	switch (event_id) {
		case WEBSOCKET_EVENT_BEGIN:
			logStr += "WEBSOCKET_EVENT_BEGIN\n";
			break;
		case WEBSOCKET_EVENT_CONNECTED:
			logStr += "WEBSOCKET_EVENT_CONNECTED\n";
			break;
		case WEBSOCKET_EVENT_DISCONNECTED:
      		logStr += "WEBSOCKET_EVENT_DISCONNECTED\n";
			break;
		case WEBSOCKET_EVENT_ERROR:
			logStr += "WEBSOCKET_EVENT_ERROR\n";
			log_error_if_nonzero("HTTP status code\n",  data->error_handle.esp_ws_handshake_status_code);
			if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
				log_error_if_nonzero("reported from esp-tls\n", data->error_handle.esp_tls_last_esp_err);
				log_error_if_nonzero("reported from tls stack\n", data->error_handle.esp_tls_stack_err);
				log_error_if_nonzero("captured as transport's socket errno\n",  data->error_handle.esp_transport_sock_errno);
			}
			break;
		case WEBSOCKET_EVENT_FINISH:
			logStr += "WEBSOCKET_EVENT_FINISH\n";
			break;
		case WEBSOCKET_EVENT_DATA:
			logStr += "WEBSOCKET_EVENT_DATA op_code ";
			int len = data->payload_len;
			const char * buff = (const char *)&data->data_ptr[data->payload_offset];
			logStr += data->op_code;   //Print return code
			//check the response for pending commands
			if ( len > 0 ){
				//numCmds can be 0-9 (one ascii digit)
				//numCmds(1)|cmdName(16)|value(32)|...
				uint8_t numCmds = buff[0] - '0';
        logStr += " numCmds ";
        logStr += numCmds;
				if( numCmds > 0 ){ // a command was recieved
					activelyCommanded = NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND;
					//do all recieved commands
					uint16_t idx = 1;
					for( uint8_t i = 0; i < numCmds; ++i ){
						uint16_t valLen = atoir_n(&(buff[idx+CMD_LEN-1]), 4);
						doCommand( &(buff[idx]), valLen, &(buff[idx+CMD_LEN]) );
						idx += CMD_LEN + valLen;
					}
				}
			}
      logStr += "\n";
			break;
	}
}