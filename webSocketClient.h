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
			break;
		case WEBSOCKET_EVENT_ERROR:
			ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
			//log_error_if_nonzero("HTTP status code",  data->error_handle.esp_ws_handshake_status_code);
			if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
				//log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
				//log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
				//log_error_if_nonzero("captured as transport's socket errno",  data->error_handle.esp_transport_sock_errno);
			}
			break;
		case WEBSOCKET_EVENT_FINISH:
			ESP_LOGI(TAG, "WEBSOCKET_EVENT_FINISH");
			break;
	}
}