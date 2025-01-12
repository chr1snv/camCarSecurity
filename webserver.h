
#include "esp_http_server.h"
//#include "esp_http_client.h"
#include "HTTPClient.h"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

#include "localWebpage.h"

static esp_err_t index_handler(httpd_req_t *req){
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

void fillStatusString(){
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-80]), 3,  "%i\n", alarmArmed);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-87]), 2,  "%i\n", lightLedValue);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-75]), 5,  "%i\n", magAX);//alarmInit_magSample.X);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-70]), 5,  "%i\n", magAY);//alarmInit_magSample.Y);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-65]), 5,  "%i\n", magAZ);//alarmInit_magSample.Z);

	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-60]), 5,  "%i\n",servo1Angle);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-55]), 5,  "%i\n",servo2Angle);

	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-50]), 5,  "%i\n", staRssi);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-45]), 4,  "%i\n", lastTemperature);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-40]), 5,  "%i\n", magX);//compass.magSample.X);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-35]), 5,  "%i\n", magY);//compass.magSample.Y);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-30]), 5,  "%i\n", magZ);//compass.magSample.Z);

	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-25]), 10, "%f\n", magHeading);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-15]), 5,  "%i\n", magAlarmDiff);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-10]), 5,  "%i\n", magAlarmTriggered);
	snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-5]),  5,  "%i\n", alarmOutput);
}

static esp_err_t status_handler(httpd_req_t *req){
	//prints the last 
	//wifi_csi info
	//temperature
	//magnetic field sample
	//acceleration reading

	fillStatusString();

	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, lastCsiInfoStr, CSI_INF_STR_LEN);
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

static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 200){ //400
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
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

bool doCommand( char * variable, char * value ){

	bool sucessfulyHandledCmd = true;

	//for camera setting changes
	sensor_t * s = esp_camera_sensor_get();

	if(!strncmp(variable, "camFlipVertical", 15)) { //flip the camera vertically
		s->set_vflip(s, atoi(value));          // 0 = disable , 1 = enable
	}
	else if(!strncmp(variable, "camFlipHoriz", 12)) { //flip the camera horizontally
		s->set_hmirror(s, atoi(value));          // 0 = disable , 1 = enable
	}
	else if(!strncmp(variable, "camResolution", 13)){
		if( !strncmp(value, "QVGA", 4) )
			s->set_framesize(s, FRAMESIZE_QVGA );
		else if( !strncmp(value, "CIF", 3) )
			s->set_framesize(s, FRAMESIZE_CIF );
		else if( !strncmp(value, "VGA", 3) )
			s->set_framesize(s, FRAMESIZE_VGA );
		else if( !strncmp(value, "SVGA", 4) )
			s->set_framesize(s, FRAMESIZE_SVGA );
	}
	else if(!strncmp(variable, "camQuality", 10)){
		s->set_quality(s, atoi(value));
	}

	//servo position control
	else if(!strncmp(variable, "up", 2)) {
		if(servo1Angle <= 170) {
		servo1Angle += 10;
		servo1.write(servo1Angle);
		}
		Serial.println(servo1Angle);
		Serial.println("Up");
	}
	else if(!strncmp(variable, "left", 4)) {
		if(servo2Angle <= 170) {
		servo2Angle += 10;
		servo2.write(servo2Angle);
		}
		Serial.println(servo2Angle);
		Serial.println("Left");
	}
	else if(!strncmp(variable, "right", 5)) {
		if(servo2Angle >= 10) {
		servo2Angle -= 10;
		servo2.write(servo2Angle);
		}
		Serial.println(servo2Angle);
		Serial.println("Right");
	}
	else if(!strncmp(variable, "down", 4)) {
		if(servo1Angle >= 10) {
		servo1Angle -= 10;
		servo1.write(servo1Angle);
		}
		Serial.println(servo1Angle);
		Serial.println("Down");
	}
	else if(!strncmp(variable, "center", 6)) {
		servo1Angle = 90;
		servo2Angle = 90;
		servo1.write(servo1Angle);
		servo2.write(servo2Angle);
		
		Serial.print(servo1Angle);
		Serial.println(servo2Angle);
		Serial.println("center");
	}
	else if(!strncmp(variable, "Light", 5)){
		lightLedValue = !lightLedValue;
		digitalWrite(LightLEDPin, lightLedValue);
		Serial.print("Light");
		Serial.println(lightLedValue);
	}
	else if(!strncmp(variable, "ArmAlarm", 8)){
		ArmAlarm(true);
		Serial.println("ArmAlarm");
	}
	else if(!strncmp(variable, "DisarmAlarm", 11)){
		ArmAlarm(false);
		Serial.println("DisarmAlarm");
	}
	else if(!strncmp(variable, "magAlarmThreshold", 17)){
		MAG_ALARM_DELTA_THRESHOLD = atoi(value);
	}
	else if(!strncmp(variable, "txPower", 7)){
		MAG_ALARM_DELTA_THRESHOLD = atoi(value);
	}
	else if(!strncmp(variable, "svrUrl", 6)){
		preferences.begin("storedVals", false);
		preferences.putString("svrUrl", String(value) );
		Serial.print(" storing serverUrl "); Serial.println( String(value) );
		preferences.end();
	}
	else if(!strncmp(variable, "clearSvr", 8)){
		preferences.begin("storedVals", false);
		preferences.putString("svrUrl", "" );
		Serial.println(" clearing serverUrl ");
		preferences.end();
	}
	else if(!strncmp(variable, "ssidPass", 8)){
		uint8_t netNum = variable[8] = -'0';
		preferences.begin("storedVals", false);
		
		char cStrToStore[32];
		sprintf( storedPrefKey, "net%i", netNum );
		strlcpy( cStrToStore, value, NETWORK_NAME_LEN );
		preferences.putString( storedPrefKey, String(cStrToStore) );
		Serial.print( "storing at " ); Serial.println( storedPrefKey ); Serial.print( " " ); Serial.println( String(cStrToStore) );
		sprintf( storedPrefKey, "pass%i", netNum );
		strlcpy( cStrToStore, &(value[32]), NETWORK_NAME_LEN );
		preferences.putString( storedPrefKey, String(cStrToStore) );
		Serial.print( "storing at " ); Serial.println( storedPrefKey ); Serial.print( " " ); Serial.println( String(cStrToStore) );
		
		preferences.end();
	}
	else if(!strncmp(variable, "clearNet", 8)){
		uint8_t netNum = variable[8] = -'0';
		preferences.begin("storedVals", false); //false -> read write
		
		//clear the selected network
		sprintf( storedPrefKey, "net%i", netNum );
		preferences.putString( storedPrefKey, "" );
		sprintf( storedPrefKey, "pass%i", netNum );
		preferences.putString( storedPrefKey, "" );
		
		preferences.end();
	}
	else {
		sucessfulyHandledCmd = -1;
	}

	return sucessfulyHandledCmd;
}

static esp_err_t cmd_handler( httpd_req_t *req ){
	char*  buf;
	size_t buf_len;
	char variable[32]	= {0,};
	char value[64] 	= {0,};

	buf_len = httpd_req_get_url_query_len(req) + 1;
	if (buf_len > 1) {
		buf = (char*)malloc(buf_len);
		if(!buf){
			httpd_resp_send_500(req);
			return ESP_FAIL;
		}
		
		if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) { //get the part of url after ?
			if (httpd_query_key_value(buf, "go", variable, sizeof(variable)) == ESP_OK) { //get go= value
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

	if( !doCommand( variable, value ) )
		return httpd_resp_send_500(req); //did not understand or was able to handle the requested cmd

	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	return httpd_resp_send(req, NULL, 0);
}

#include "esp-protocols-master/components/esp_websocket_client/include/esp_websocket_client.h"
#include "webSocketClient.h"

//one loop is 1/10th of a second ( from delay(100); in wifiCamCarSecuritySystem.ino )
#define INACTIVE_COMMAND_LOOP_INTERVAL 10
#define NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND 100
int activelyCommanded = 0;
HTTPClient cloudHttp;
String serverUrl;
typedef enum{
	DEV_STATUS   ,
	DEV_SETTINGS ,
	IMAGE    
} PostType;
void PostAndFetchDataFromCloudServer(PostType postType){

	if( postType == DEV_STATUS )
		--activelyCommanded;

	if( activelyCommanded <= 0 ){ //if it's been NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND
		if( activelyCommanded < -INACTIVE_COMMAND_LOOP_INTERVAL )
			activelyCommanded = 0; //only send after interval num loops
		else
			return; //skip sending for INACTIVE_COMMAND_LOOP_INTERVAL loops
	}

	if(false /*!esp_websocket_client_is_connected(webSockClient)*/){
		const esp_websocket_client_config_t ws_cfg = {
			.uri = "wss://echo.websocket.org",
			.port = 4567,
			//.cert_pem = (const char *)websocket_org_pem_start,
		};
		esp_websocket_client_handle_t webSockClient = esp_websocket_client_init(&ws_cfg);
		esp_websocket_register_events(webSockClient, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)webSockClient);


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

	if( !cloudHttp.connected() ){
		preferences.begin("storedVals", true);
		serverUrl = preferences.getString("svrUrl");
		preferences.end();
		Serial.print( "Con to " );
		Serial.println( serverUrl );
		cloudHttp.setReuse(true);
		cloudHttp.begin(serverUrl);
	}

	int httpResponseCode;
	if( postType == DEV_STATUS ){
		fillStatusString();
		cloudHttp.addHeader("Content-Type", "text/status", true, true);
		char lenStr[8];
		snprintf( lenStr, 8, "%i", CSI_INF_STR_LEN );
		cloudHttp.addHeader("Content-Length", lenStr, false, true);
		httpResponseCode = cloudHttp.POST( (uint8_t *)&(lastCsiInfoStr[0]),  CSI_INF_STR_LEN );
	}else if( postType == DEV_SETTINGS ){
		fillSettingsString();
		cloudHttp.addHeader("Content-Type", "text/settings", true, true);
		char lenStr[8];
		snprintf( lenStr, 8, "%i", SETTINGS_RESPONSE_LENGTH );
		cloudHttp.addHeader("Content-Length", lenStr, false, true);
		httpResponseCode = cloudHttp.POST( (uint8_t *)&(lastCsiInfoStr[0]),  SETTINGS_RESPONSE_LENGTH );
	}else if( postType == IMAGE ){
		cloudHttp.addHeader("Content-Type", "image/jpeg", true, true);
		size_t _jpg_buf_len = 0;
		uint8_t * _jpg_buf = NULL;
		uint8_t jpgBufType = getJpeg( &_jpg_buf, &_jpg_buf_len);
		if( jpgBufType != 0 ){
			char lenStr[8];
			snprintf( lenStr, 8, "%i", _jpg_buf_len );
			cloudHttp.addHeader("Content-Length", lenStr, false, true);
			Serial.print("send img "); Serial.println( _jpg_buf_len );
			httpResponseCode = cloudHttp.POST(_jpg_buf, _jpg_buf_len);
			if( jpgBufType == 2 )
				free(_jpg_buf);
			else
				esp_camera_fb_return(fb);
		}
	}
  
	if( httpResponseCode > 0  && postType == DEV_STATUS ){
		String response = cloudHttp.getString();  //Get the response to the request
		Serial.println(httpResponseCode);   //Print return code
		Serial.println(response);           //Print request answer
		//check the response for pending commands
		if ( response.length() > 0 ){
			uint8_t numCmds = response[0] - '0';
			if( numCmds > 0 ){ // a command was recieved
				activelyCommanded = NUMLOOPS_TO_STAY_ACTIVE_AFTER_COMMAND;
				//do all recieved commands
				char cmd[32];
				char val[32];
				for( uint8_t i = 0; i < numCmds; ++i ){
					snprintf( cmd, 32, "%s", &(response[1+i*64]) );
					snprintf( val, 32, "%s", &(response[1+i*64+32]) );
					doCommand( cmd, val );
				}
			}
		}
	}

	Serial.print("POST resp: ");
	Serial.println(httpResponseCode);

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

	httpd_uri_t status_uri = {
		.uri       = "/status",
		.method    = HTTP_GET,
		.handler   = status_handler,
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
		.method    = HTTP_GET,
		.handler   = cmd_handler,
		.user_ctx  = NULL
	};
	httpd_uri_t stream_uri = {
		.uri       = "/stream",
		.method    = HTTP_GET,
		.handler   = stream_handler,
		.user_ctx  = NULL
	};
	if (httpd_start(&camera_httpd, &config) == ESP_OK) {
		httpd_register_uri_handler(camera_httpd, &index_uri);
		httpd_register_uri_handler(camera_httpd, &cmd_uri);
		httpd_register_uri_handler(camera_httpd, &status_uri);
		httpd_register_uri_handler(camera_httpd, &settings_uri);
	}
	config.server_port += 1;
	config.ctrl_port += 1;
	if (httpd_start(&stream_httpd, &config) == ESP_OK) {
		httpd_register_uri_handler(stream_httpd, &stream_uri);
	}
}
