
#include "esp_http_server.h"
//#include "esp_http_client.h"
#include "HTTPClient.h"

httpd_handle_t camera_httpd = NULL;

#define CMD_LEN 16

#include "localWebpage.h"

static esp_err_t index_handler(httpd_req_t *req){
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

#define STATUS_RESPONSE_LENGTH 80
void fillStatusString(){
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+0 ]), 3,  "%i\n", alarmArmed);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+3 ]), 2,  "%i\n", (int)lightLedValue);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+5 ]), 5,  "%i\n", magAX);//alarmInit_magSample.X);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+10]), 5,  "%i\n", magAY);//alarmInit_magSample.Y);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+15]), 5,  "%i\n", magAZ);//alarmInit_magSample.Z);

	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+20]), 5,  "%i\n",servo1Angle);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+25]), 5,  "%i\n",servo2Angle);

	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+30]), 5,  "%i\n", staRssi);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+35]), 5,  "%i\n", lastTemperature);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+40]), 5,  "%i\n", magX);//compass.magSample.X);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+45]), 5,  "%i\n", magY);//compass.magSample.Y);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+50]), 5,  "%i\n", magZ);//compass.magSample.Z);

	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+55]), 10, "%f\n", magHeading);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+65]), 5,  "%i\n", magAlarmDiff);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+70]), 5,  "%i\n", magAlarmTriggered);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH+75]), 5,  "%i\n", alarmOutput);
}

#define SETTINGS_RESPONSE_LENGTH 52+((NETWORK_NAME_LEN*2)*MAX_STORED_NETWORKS)
void fillSettingsString(){
	sensor_t * s = esp_camera_sensor_get();
	int8_t txPower;
	esp_wifi_get_max_tx_power(&txPower);

	snprintf( &(lastCsiInfoStr[0]),  5, "%i\n", MAG_ALARM_DELTA_THRESHOLD);
	snprintf( &(lastCsiInfoStr[5]),  5, "%i\n", s->status.quality);
	snprintf( &(lastCsiInfoStr[10]), 5, "%i\n", s->status.framesize);
	snprintf( &(lastCsiInfoStr[15]), 5, "%i\n", txPower);

	preferences.begin("storedVals", true);
		snprintf( &(lastCsiInfoStr[20]), 32, "%s\n", preferences.getString("svrUrl").c_str() );
		uint8_t storedNetworksSettingsStrStart = 52;
		for( uint8_t i = 0; i < MAX_STORED_NETWORKS; ++i ){
		snprintf(storedPrefKey, 8, "net%i", i );
		snprintf( &(lastCsiInfoStr[storedNetworksSettingsStrStart+(NETWORK_NAME_LEN*2)*i]),
				NETWORK_NAME_LEN, "%s\n", preferences.getString(storedPrefKey).c_str() );
		snprintf(storedPrefKey, 8, "pass%i", i );
		snprintf( &(lastCsiInfoStr[storedNetworksSettingsStrStart+(NETWORK_NAME_LEN*2)*i]),
				NETWORK_NAME_LEN, "%s\n", preferences.getString(storedPrefKey).c_str() );
		}
	preferences.end();
}

static esp_err_t settings_handler(httpd_req_t *req){
	fillSettingsString();

	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, lastCsiInfoStr, SETTINGS_RESPONSE_LENGTH);
}

camera_fb_t * fb;
uint8_t getJpeg(uint8_t ** _jpg_buf, size_t * _jpg_buf_len){
  //after done with result need to free allocated memory
  //if returns 1 call "esp_camera_fb_return( fb )" 
  //if returns 2 call  "free( _jpg_buf )"
  fb = esp_camera_fb_get();
  esp_err_t res = ESP_OK;
  if (!fb) {
    Serial.println("Cam fb get failed");
    return 0;
  } else {
    if(fb->width > 200){ //400
      if(fb->format != PIXFORMAT_JPEG){
        bool jpeg_converted = frame2jpg(fb, 80, _jpg_buf, _jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;
        if(!jpeg_converted){
          Serial.println("JPEG compres failed");
          return 0;
        }else{
          return 2;
        }
      } else {
        _jpg_buf_len[0] = fb->len;
        _jpg_buf[0] = fb->buf;
        return 1;
      }
    }
    return 1;
  }
}

bool doCommand( const char * cmd, const char * value ){

	bool sucessfulyHandledCmd = true;

	//for camera setting changes
	sensor_t * s = esp_camera_sensor_get();

	if(!strncmp(cmd, "camFlipVertical", 15)) { //flip the camera vertically
		s->set_vflip(s, atoi(value));          // 0 = disable , 1 = enable
	}
	else if(!strncmp(cmd, "camFlipHoriz", 12)) { //flip the camera horizontally
		s->set_hmirror(s, atoi(value));          // 0 = disable , 1 = enable
	}
	else if(!strncmp(cmd, "camResolution", 13)){
		if( !strncmp(value, "QVGA", 4) )
			s->set_framesize(s, FRAMESIZE_QVGA );
		else if( !strncmp(value, "CIF", 3) )
			s->set_framesize(s, FRAMESIZE_CIF );
		else if( !strncmp(value, "VGA", 3) )
			s->set_framesize(s, FRAMESIZE_VGA );
		else if( !strncmp(value, "SVGA", 4) )
			s->set_framesize(s, FRAMESIZE_SVGA );
	}
	else if(!strncmp(cmd, "camQuality", 10)){
		s->set_quality(s, atoi(value));
	}

	//servo position control
	else if(!strncmp(cmd, "up", 2)) {
		if(servo1Angle <= 170) {
		servo1Angle += 10;
		servo1.write(servo1Angle);
		}
		Serial.println(servo1Angle);
		Serial.println("Up");
	}
	else if(!strncmp(cmd, "left", 4)) {
		if(servo2Angle <= 170) {
		servo2Angle += 10;
		servo2.write(servo2Angle);
		}
		Serial.println(servo2Angle);
		Serial.println("Left");
	}
	else if(!strncmp(cmd, "right", 5)) {
		if(servo2Angle >= 10) {
		servo2Angle -= 10;
		servo2.write(servo2Angle);
		}
		Serial.println(servo2Angle);
		Serial.println("Right");
	}
	else if(!strncmp(cmd, "down", 4)) {
		if(servo1Angle >= 10) {
		servo1Angle -= 10;
		servo1.write(servo1Angle);
		}
		Serial.println(servo1Angle);
		Serial.println("Down");
	}
	else if(!strncmp(cmd, "center", 6)) {
		servo1Angle = 90;
		servo2Angle = 90;
		servo1.write(servo1Angle);
		servo2.write(servo2Angle);
		
		Serial.print(servo1Angle);
		Serial.println(servo2Angle);
		Serial.println("center");
	}
	else if(!strncmp(cmd, "Light", 5)){
		lightLedValue = !lightLedValue;
		digitalWrite(LightLEDPin, lightLedValue);
		Serial.print("Light");
		Serial.println(lightLedValue);
	}
	else if(!strncmp(cmd, "ArmAlarm", 8)){
		ArmAlarm(true);
		Serial.println("ArmAlarm");
	}
	else if(!strncmp(cmd, "DisarmAlarm", 11)){
		ArmAlarm(false);
		Serial.println("DisarmAlarm");
	}
	else if(!strncmp(cmd, "magAlarmThreshold", 17)){
		MAG_ALARM_DELTA_THRESHOLD = atoi(value);
	}
	else if(!strncmp(cmd, "txPower", 7)){
		MAG_ALARM_DELTA_THRESHOLD = atoi(value);
	}
	else if(!strncmp(cmd, "svrUrl", 6)){
		preferences.begin("storedVals", false);
		preferences.putString("svrUrl", String(value) );
		Serial.print(" storing serverUrl "); Serial.println( String(value) );
		preferences.end();
	}
	else if(!strncmp(cmd, "svrCert", 7)){
		preferences.begin("storedVals", false);
		preferences.putString("svrCert", value );
		Serial.println(" stored serverCert ");
		preferences.end();
	}
	else if(!strncmp(cmd, "ssid", 4)){
		uint8_t netNum = cmd[4] -'0';
		preferences.begin("storedVals", false);
		sprintf( storedPrefKey, "net%i", netNum );
		char cStrToStore[32]; strlcpy( cStrToStore, value, NETWORK_NAME_LEN );
		preferences.putString( storedPrefKey, String(cStrToStore) );
		Serial.print( "storing at " ); Serial.println( storedPrefKey ); Serial.print( " " ); Serial.println( String(cStrToStore) );
		preferences.end();
	}
	else if(!strncmp(cmd, "pass", 4)){
		uint8_t netNum = cmd[4] -'0';
		preferences.begin("storedVals", false);
		sprintf( storedPrefKey, "pass%i", netNum );
		char cStrToStore[32]; strlcpy( cStrToStore, &(value[32]), NETWORK_NAME_LEN );
		preferences.putString( storedPrefKey, String(cStrToStore) );
		Serial.print( "storing at " ); Serial.println( storedPrefKey ); Serial.print( " " ); Serial.println( String(cStrToStore) );
		preferences.end();
	}
	else {
		sucessfulyHandledCmd = -1;
	}

	return sucessfulyHandledCmd;
}

//https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html
static esp_err_t cmd_post_handler( httpd_req_t * req ){
	char content[2048];
	size_t recv_size = min( req->content_len, sizeof(content));

	int ret = httpd_req_recv(req, content, recv_size);
	if( ret <= 0 ){
		if( ret == HTTPD_SOCK_ERR_TIMEOUT ){
			httpd_resp_send_408(req);
		}
		return ESP_FAIL;
	}

	if( !doCommand( content, &content[CMD_LEN] ) )
		return httpd_resp_send_500(req); //did not understand or was able to handle the requested cmd

	const char resp[] = "cmd recieved";
	httpd_resp_send( req, resp, HTTPD_RESP_USE_STRLEN );
	return ESP_OK;
}


static esp_err_t cmd_handler( httpd_req_t *req ){
	char*  buf;
	size_t buf_len;
	char cmd[32]	= {0,};
	char value[2048] 	= {0,};

	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = (char*)malloc(buf_len);
		if(!buf){
			httpd_resp_send_500(req);
			return ESP_FAIL;
		}
		
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) { //get the part of url after ?
			if (httpd_query_key_value(buf, "go", cmd, sizeof(cmd)) == ESP_OK) { //get go= value
				httpd_query_key_value(buf, "val", value, sizeof(value));
			} else {
				free(buf);
				httpd_resp_send_404(req);
				return ESP_FAIL;
			}
		} else {
			free(buf);
			httpd_resp_send_404(req);
			return ESP_FAIL;
		}
		free(buf);
	} else {
		httpd_resp_send_404(req);
		return ESP_FAIL;
	}

	if( !doCommand( cmd, value ) )
		return httpd_resp_send_500(req); //did not understand or was able to handle the requested cmd

	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	return httpd_resp_send(req, NULL, 0);
}

bool isAUrl(String str){
	int strLen = str.length();
	if( strLen < 1 )
		return false;
	for( uint8_t i = 0; i < strLen; ++i )
		if( str[i] != ' ' && str[i] != '\0')
			return true;
}

//one loop is 1/10th of a second ( from delay(100); in wifiCamCarSecuritySystem.ino )
#define INACTIVE_COMMAND_LOOP_INTERVAL 10
#define NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND 100
int activelyCommanded = 0;
//not reccomended, though to bypass ESP_ERR_MBEDTLS_SSL_SETUP_FAILED
//https://github.com/espressif/esp-idf/issues/13109
//https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig-reference.html#config-esp-tls-insecure
///.arduino/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.3-083aad99-v2/esp32/sdkconfig
// CONFIG_ESP_TLS_INSECURE=y
// CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY=y
// CONFIG_WS_OVER_TLS_SKIP_COMMON_NAME_CHECK=y
#include "esp_websocket_client.h"
esp_websocket_client_handle_t webSockClient = NULL;
#include "webSocketClient.h"
typedef enum{
	DEV_STATUS   ,
	DEV_SETTINGS ,
	IMAGE    
} PostType;
uint16_t mainLoopsSinceWebSockStartedConnecting = 0;
void PostAndFetchDataFromCloudServer(PostType postType){

	if( postType == DEV_STATUS )
		--activelyCommanded;

	if( activelyCommanded <= 0 ){ //if it's been NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND
		if( activelyCommanded < -INACTIVE_COMMAND_LOOP_INTERVAL )
			activelyCommanded = 0; //only send after interval num loops
		else
			return; //skip sending for INACTIVE_COMMAND_LOOP_INTERVAL loops
	}

	if( webSockClient == NULL || !esp_websocket_client_is_connected(webSockClient) && mainLoopsSinceWebSockStartedConnecting > 500 ){
		mainLoopsSinceWebSockStartedConnecting = 0;
		preferences.begin("storedVals", true);
		String serverUrl = preferences.getString("svrUrl");
		String serverCert = preferences.getString("svrCert");
		preferences.end();

		if( isAUrl(serverUrl) ){

			//https://docs.espressif.com/projects/esp-protocols/esp_websocket_client/docs/latest/index.html
			Serial.print( "WebSock to " ); Serial.print( serverUrl ); Serial.println( "|" );
			const esp_websocket_client_config_t ws_cfg = {
				.uri = serverUrl.c_str(),//"wss://echo.websocket.org",
				//.port = 4567,
				.cert_pem = (const char *)serverCert.c_str(),
				//.subprotocol = 
				.transport = WEBSOCKET_TRANSPORT_OVER_SSL,
				.skip_cert_common_name_check = true,
				
			};
			esp_websocket_client_handle_t webSockClient = esp_websocket_client_init(&ws_cfg);
			Serial.println( "webSock cli inited");
			esp_err_t wEvts = esp_websocket_register_events(webSockClient, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)webSockClient);
			Serial.print( "webSock reg " ); Serial.println( wEvts );
			wEvts = esp_websocket_client_start(webSockClient);
			Serial.print( "webSock start " ); Serial.println( wEvts );

			//esp_websocket_client_send_text(client, data, len, portMAX_DELAY);
			/*
			ESP_LOGI(TAG, "Sending fragmented binary message");
			char binary_data[5];
			memset(binary_data, 0, sizeof(binary_data));
			esp_websocket_client_send_bin_partial(client, binary_data, sizeof(binary_data), portMAX_DELAY);
			memset(binary_data, 1, sizeof(binary_data));
			esp_websocket_client_send_cont_msg(client, binary_data, sizeof(binary_data), portMAX_DELAY);
			esp_websocket_client_send_fin(client, portMAX_DELAY);
			*/

		}
	}

	if( esp_websocket_client_is_connected(webSockClient) ){
		int httpResponseCode;
		if( postType == DEV_STATUS ){
			fillStatusString();
			httpResponseCode = esp_websocket_client_send_text(webSockClient, (const char *)&(lastCsiInfoStr[CSI_INF_STR_LEN-STATUS_RESPONSE_LENGTH]), STATUS_RESPONSE_LENGTH, portMAX_DELAY);
		}else if( postType == DEV_SETTINGS ){
			fillSettingsString();
			httpResponseCode = esp_websocket_client_send_text(webSockClient, (const char *)&(lastCsiInfoStr[0]), SETTINGS_RESPONSE_LENGTH, portMAX_DELAY);
		}else if( postType == IMAGE ){
			size_t _jpg_buf_len = 0;
			uint8_t * _jpg_buf = NULL;
			uint8_t jpgBufType = getJpeg( &_jpg_buf, &_jpg_buf_len);
			if( jpgBufType != 0 ){
				Serial.print("send img "); Serial.println( _jpg_buf_len );
				httpResponseCode = esp_websocket_client_send_bin(webSockClient, (const char *)_jpg_buf, _jpg_buf_len, portMAX_DELAY);
				if( jpgBufType == 2 )
					free(_jpg_buf);
				else
					esp_camera_fb_return(fb);
			}
		}

		Serial.print("POST resp: ");
		Serial.println(httpResponseCode);
	}

	//cloudHttp.end();
}

void startCameraServer(){
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = 80;
	httpd_uri_t index_uri = {
		.uri       = "/",
		.method    = HTTP_GET,
		.handler   = index_handler,
		.user_ctx  = NULL
	};

	httpd_uri_t settings_uri = {
		.uri       = "/settings",
		.method    = HTTP_GET,
		.handler   = settings_handler,
		.user_ctx  = NULL
	};

	httpd_uri_t cmd_uri = {
		.uri       = "/action",
		.method    = HTTP_POST,
		.handler   = cmd_post_handler,
		.user_ctx  = NULL
	};
	if (httpd_start(&camera_httpd, &config) == ESP_OK) {
		httpd_register_uri_handler(camera_httpd, &index_uri);
		httpd_register_uri_handler(camera_httpd, &cmd_uri);
		httpd_register_uri_handler(camera_httpd, &settings_uri);
	}
}
