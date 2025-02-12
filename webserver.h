
#include "esp_http_server.h"
//#include "esp_http_client.h"
#include "HTTPClient.h"

httpd_handle_t camera_httpd = NULL;


#include "WebComDefines.h"

uint16_t serverUrlLen = 0;
uint16_t serverCertLen = 0;
char serverUrl[MAX_SVR_URL_LEN] = {'\0'};
char serverCert[SVR_CERT_MAX_LEN] = {'\0'};

uint16_t devId;

#include "localWebpage.h"

static esp_err_t index_handler(httpd_req_t *req){
	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

#define STATUS_RESPONSE_LENGTH 80
void fillStatusString(){
  //+1's to snprintf lengths because of \0 null terminator always appended

	//fill header
	lastCsiInfoStr[0] = '1';
	lastCsiInfoStr[1] = 'd';
	snprintf( &(lastCsiInfoStr[2]), 11+1, "Stat" );
	snprintf( &(lastCsiInfoStr[13]), 4+1, "% 4d", devId );
	snprintf( &(lastCsiInfoStr[17]), 6+1, "% 6d", STATUS_RESPONSE_LENGTH );

	//fill data
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+0 ]), 3+1,  "% 3d", alarmArmed);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+3 ]), 2+1,  "% 2d", (int)lightLedValue);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+5 ]), 5+1,  "% 5i", magAX);//alarmInit_magSample.X);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+10]), 5+1,  "% 5i", magAY);//alarmInit_magSample.Y);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+15]), 5+1,  "% 5i", magAZ);//alarmInit_magSample.Z);

	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+20]), 5+1,  "% 5d",servo1Angle);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+25]), 5+1,  "% 5d",servo2Angle);

	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+30]), 5+1,  "% 5i", staRssi);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+35]), 5+1,  "% 5d", lastTemperature);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+40]), 5+1,  "% 5i", magX);//compass.magSample.X);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+45]), 5+1,  "% 5i", magY);//compass.magSample.Y);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+50]), 5+1,  "% 5i", magZ);//compass.magSample.Z);

	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+55]), 10+1, "% 10f", magHeading);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+65]), 5+1,  "% 5i", magAlarmDiff);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+70]), 5+1,  "% 5i", magAlarmTriggered);
	snprintf( &(lastCsiInfoStr[WEB_SEND_HDR_LEN+75]), 5+1,  "% 5i", alarmOutput);
	//memset() //snprintf will set a \0 at end so don't need to memset
}

void ObfsucatePass(char *str, uint8_t len){
  for(uint16_t i = 0; i < len; ++i){
    if(str[i] != '\0')
      str[i] = '*';
  }
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
	Serial.println("fSS storedVals");
	preferences.begin("storedVals", true);
		String svrUrlStr = preferences.getString("svrUrl");
		uint16_t svrUrlStrLen = min( (uint16_t)NETWORK_NAME_LEN, (uint16_t)svrUrlStr.length() );
		memcpy( &(lastCsiInfoStr[20]), svrUrlStr.c_str(), svrUrlStrLen );
		memset( &(lastCsiInfoStr[20]) + svrUrlStrLen,
		' ', NETWORK_NAME_LEN - svrUrlStrLen );
		//Serial.println("svrUrl");
		uint8_t storedNetworksSettingsStrStart = 52;
		for( uint8_t i = 0; i < MAX_STORED_NETWORKS; ++i ){
			char networkInfo[NETWORK_NAME_LEN];
		snprintf(storedPrefKey, 8,"net%i", i );
		memset( networkInfo, '\0', NETWORK_NAME_LEN );
			preferences.getBytes(storedPrefKey, &networkInfo[0], NETWORK_NAME_LEN);
			memcpy( &(lastCsiInfoStr[storedNetworksSettingsStrStart+(NETWORK_NAME_LEN*2*i)]),
				networkInfo, NETWORK_NAME_LEN );
			//Serial.print("net");Serial.println(i);
			snprintf(storedPrefKey, 8,"pass%i", i );
		memset( networkInfo, '\0', NETWORK_NAME_LEN );
			preferences.getBytes(storedPrefKey, &networkInfo[0], NETWORK_NAME_LEN);
			ObfsucatePass(networkInfo, NETWORK_NAME_LEN);
			memcpy( &(lastCsiInfoStr[storedNetworksSettingsStrStart+(NETWORK_NAME_LEN*((2*i)+1))]),
				networkInfo, NETWORK_NAME_LEN );
			//Serial.print("pass");Serial.println(i);
		}
	preferences.end();
	memset( &(lastCsiInfoStr[storedNetworksSettingsStrStart+(NETWORK_NAME_LEN*(2*MAX_STORED_NETWORKS))]),
			'\0', 1 ); //prevent segfault reboot from missing \0
}

void fillWebsvrStr(){
  //fill header
	lastCsiInfoStr[0] = '1';
	lastCsiInfoStr[1] = 'd';
	snprintf( &(lastCsiInfoStr[2]), 11+1, "Stat" );
	snprintf( &(lastCsiInfoStr[13]), 4+1, "% 4d", devId );
	snprintf( &(lastCsiInfoStr[17]), 6+1, "% 6d", STATUS_RESPONSE_LENGTH );
}

static esp_err_t settings_handler(httpd_req_t *req){
	fillSettingsString();

	httpd_resp_set_type(req, "text/html");
	return httpd_resp_send(req, lastCsiInfoStr, SETTINGS_RESPONSE_LENGTH);
}

uint8_t jpgBufType;
camera_fb_t * fb;
size_t _jpg_buf_len = 0;
uint8_t * _jpg_buf = NULL;
void freeJpegBuf( ){
	if( jpgBufType == 2 )
		free( _jpg_buf );
	else if( jpgBufType == 1 )
		esp_camera_fb_return( fb );
	jpgBufType = 0;
}


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
				jpgBufType = 0;
				return 0;
			}else{
				jpgBufType = 2;
				return 2;
			}
		} else {
			_jpg_buf_len[0] = fb->len;
			_jpg_buf[0] = fb->buf;
			jpgBufType = 1;
			return 1;
		}
	}
	jpgBufType = 1;
	return 1;
	}
}

void fillJpegSendHdr( size_t _jpg_buf_len ){
	//fill header
	lastCsiInfoStr[0] = '1';
	lastCsiInfoStr[1] = 'd';
	snprintf( &(lastCsiInfoStr[2]), 11+1, "Img" );
	snprintf( &(lastCsiInfoStr[13]), 4+1, "% 4d", devId );
	snprintf( &(lastCsiInfoStr[17]), 6+1, "% 6d", _jpg_buf_len );
}

//ascii to int reverse iteration for n characters
//input is end of number (1's place)
//counts up in significance (x10), decrementing string index from start index
uint16_t atoir_n( const char * c, uint8_t n ){
	uint16_t accum = 0;
	uint16_t mult = 1;
	//Serial.print( "atoir_n d ");
	for( uint8_t i = 0; i < n; ++i ){
		char d = c[-i];
		if( d >= '0' && d <= '9' )
			accum += (d - '0')*mult;
		else
			break;
		mult *= 10;
		//Serial.print( d ); Serial.print(" acum ");Serial.print(accum);Serial.print(" d ");
	}
	//Serial.print(" accum ");Serial.println(accum);
	return accum;
}

uint8_t doCommand( const char * cmd, uint16_t valLen, const char * value ){

	uint8_t sucessfulyHandledCmd = 0;

	//for camera setting changes
	sensor_t * s = esp_camera_sensor_get();

	if(!strncmp(cmd, "camFlipV", 8)) { //flip the camera vertically
		s->set_vflip(s, atoi(value));          // 0 = disable , 1 = enable
		sucessfulyHandledCmd = 1;
	}
	else if(!strncmp(cmd, "camFlipH", 8)) { //flip the camera horizontally
		s->set_hmirror(s, atoi(value));          // 0 = disable , 1 = enable
		sucessfulyHandledCmd = 2;
	}
	else if(!strncmp(cmd, "camRes", 6)){
		if( !strncmp(value, "QVGA", 4) )
			s->set_framesize(s, FRAMESIZE_QVGA );
		else if( !strncmp(value, "CIF", 3) )
			s->set_framesize(s, FRAMESIZE_CIF );
		else if( !strncmp(value, "VGA", 3) )
			s->set_framesize(s, FRAMESIZE_VGA );
		else if( !strncmp(value, "SVGA", 4) )
			s->set_framesize(s, FRAMESIZE_SVGA );
		sucessfulyHandledCmd = 3;
	}
	else if(!strncmp(cmd, "camQuality", 10)){
		s->set_quality(s, atoi(value));
		sucessfulyHandledCmd = 4;
	}

	//servo position control
	else if(!strncmp(cmd, "up", 2)) {
		if(servo1Angle <= 170) {
			servo1Angle += 10;
			servo1.write(servo1Angle);
		}
		Serial.println(servo1Angle);
		Serial.println("Up");
		sucessfulyHandledCmd = 5;
	}
	else if(!strncmp(cmd, "left", 4)) {
		if(servo2Angle <= 170) {
			servo2Angle += 10;
			servo2.write(servo2Angle);
		}
		Serial.println(servo2Angle);
		Serial.println("Left");
		sucessfulyHandledCmd = 6;
	}
	else if(!strncmp(cmd, "right", 5)) {
		if(servo2Angle >= 10) {
			servo2Angle -= 10;
			servo2.write(servo2Angle);
		}
		Serial.println(servo2Angle);
		Serial.println("Right");
		sucessfulyHandledCmd = 7;
	}
	else if(!strncmp(cmd, "down", 4)) {
		if(servo1Angle >= 10) {
			servo1Angle -= 10;
			servo1.write(servo1Angle);
		}
		Serial.println(servo1Angle);
		Serial.println("Down");
		sucessfulyHandledCmd = 8;
	}
	else if(!strncmp(cmd, "center", 6)) {
		servo1Angle = 90;
		servo2Angle = 90;
		servo1.write(servo1Angle);
		servo2.write(servo2Angle);
		
		//Serial.print(servo1Angle);
		//Serial.println(servo2Angle);
		//Serial.println("center");
		sucessfulyHandledCmd = 9;
	}
	else if(!strncmp(cmd, "Light", 5)){
		lightLedValue = !lightLedValue;
		digitalWrite(LightLEDPin, lightLedValue);
		Serial.print("Light");
		Serial.println(lightLedValue);
		sucessfulyHandledCmd = 10;
	}
	else if(!strncmp(cmd, "ArmAlarm", 8)){
		ArmAlarm(true);
		Serial.println("ArmAlarm");
		sucessfulyHandledCmd = 11;
	}
	else if(!strncmp(cmd, "DisarmAlarm", 11)){
		ArmAlarm(false);
		Serial.println("DisarmAlarm");
		sucessfulyHandledCmd = 12;
	}
	else if(!strncmp(cmd, "magAlrThrsh", 11)){
		MAG_ALARM_DELTA_THRESHOLD = atoir_n(&value[valLen-1], valLen);
		sucessfulyHandledCmd = 13;
	}
	else if(!strncmp(cmd, "txPower", 7)){
		MAG_ALARM_DELTA_THRESHOLD = atoir_n(&value[valLen-1], valLen);
		sucessfulyHandledCmd = 14;
	}
	else if(!strncmp(cmd, "svrUrl", 6)){
		preferences.begin("storedVals", false);
      uint8_t urlNum = cmd[6] - '0';
			uint16_t storLen = min( valLen, (uint16_t)MAX_SVR_URL_LEN );
			preferences.putBytes("svrUrl"+urlNum, value, storLen );
			Serial.print(" storing svrUrl");Serial.print(urlNum);Serial.print(" len ");Serial.println(storLen);
		preferences.end();
		sucessfulyHandledCmd = 15;
	}
	else if(!strncmp(cmd, "svrCertLen", 10)){

		preferences.begin("storedVals", false);
			uint16_t certLen = atoir_n(&value[valLen-1], valLen );
			preferences.putInt( "svrCertLen", certLen );
			Serial.print("storing svrCert len "); Serial.println(certLen);
		preferences.end();
		sucessfulyHandledCmd = 16;
	}
	else if(!strncmp(cmd, "svrCert", 7)){
		uint8_t certNum = cmd[7] - '0';
		preferences.begin("storedVals", false);
			uint16_t storLen = min( valLen, (uint16_t)SVR_CERT_PART_LENGTH);
			preferences.putBytes("svrCert"+certNum, value, storLen );
			Serial.print("storing svrCert "); Serial.print(certNum); Serial.print(" len ");Serial.println(storLen);
		preferences.end();
		sucessfulyHandledCmd = 17;
	}
	else if(!strncmp(cmd, "net", 3)){
		uint8_t netNum = cmd[3] -'0';
		preferences.begin("storedVals", false);
		uint16_t len = min( valLen, (uint16_t)NETWORK_NAME_LEN );
			snprintf( storedPrefKey, 8, "net%i", netNum );
			preferences.putBytes( storedPrefKey, value, len );
			Serial.print( "storing at |" ); Serial.print( storedPrefKey ); 
			Serial.print( "| |" ); Serial.print( value );
			Serial.print("| ");Serial.println(len);
		preferences.end();
		sucessfulyHandledCmd = 18;
	}
	else if(!strncmp(cmd, "pass", 4)){
		uint8_t netNum = cmd[4] -'0';
		preferences.begin("storedVals", false);
		uint16_t len = min( valLen, (uint16_t)NETWORK_NAME_LEN );
			snprintf( storedPrefKey, 8, "pass%i", netNum );
			preferences.putBytes( storedPrefKey, value, len );
			Serial.print( "storing at |" ); Serial.print( storedPrefKey ); 
			Serial.print( "| |" ); Serial.print( value );
			Serial.print("| ");Serial.println(len);
		preferences.end();
		sucessfulyHandledCmd = 19;
	}

	return sucessfulyHandledCmd;
}

//read the stored server cert back from preferences
void readSvrCert(uint8_t num, uint16_t & length, char * svrCert ){
	//Serial.print("readSvrCert len ");
	preferences.begin("storedVals", true);
	length = preferences.getInt( "svrCertLen"+num );
	//Serial.println( length );
	uint16_t partIdx;
	uint16_t partLen;
	for( uint8_t i = 0; i < NUM_SVR_CERT_PARTS; ++i ){
		partIdx = i*SVR_CERT_PART_LENGTH;
		uint16_t remLen = length - partIdx;
		partLen = min( (uint16_t)SVR_CERT_PART_LENGTH, remLen);
		//Serial.print( "partIdx " );Serial.print(i);Serial.print(" ");Serial.print( partIdx ); 
		//Serial.print( " remLen " ); Serial.print(remLen); Serial.print( " partLen " );Serial.println( partLen );
		if( partLen <=0 )
			break;
		preferences.getBytes("svrCert"+i, &svrCert[i*SVR_CERT_PART_LENGTH], partLen );
	}
	svrCert[length] = '\0';
	preferences.end();
}

#include "webSocketClient.h"

//https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html
static esp_err_t cmd_post_handler( httpd_req_t * req ){
	char content[CMD_BUFF_MAX_LEN];
	size_t recv_size = min( (int)(req->content_len), CMD_BUFF_MAX_LEN );
	Serial.print("recv_size ");Serial.println( recv_size );

	int ret = httpd_req_recv(req, &content[0], recv_size);
	if( ret <= 0 ){
		if( ret == HTTPD_SOCK_ERR_TIMEOUT ){
			httpd_resp_send_408(req);
		}
		return ESP_FAIL;
	}

	Serial.print("recvd from lclWebPg ");
	Serial.println( content );

	uint8_t doCmdRes = doCommandsInRecievedData( recv_size, content );
	if( doCmdRes < 1 )
		return httpd_resp_send_500(req); //did not understand or was not able to handle the requested cmd
	Serial.print("doCmdRes " ); Serial.print(doCmdRes);
	const char resp[] = "cmd recieved";
	httpd_resp_send( req, resp, HTTPD_RESP_USE_STRLEN );
	return ESP_OK;
}


bool isAUrl(String str){
	int strLen = str.length();
	if( strLen < 1 )
		return false;
	for( uint8_t i = 0; i < strLen; ++i )
		if( str[i] != ' ' && str[i] != '\0')
			return true;
}


void startCameraServer(){
	if( camera_httpd != NULL )
		return;

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
	Serial.println("Local webpage ready! ");
	}
}


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
uint8_t svrIdx = 0;

void ReadNextSvrInfo(){
	preferences.begin("storedVals", true);
	serverUrlLen = 0;
	do{
		Serial.print("reading svrUrl");Serial.println(svrIdx);
		serverUrlLen = preferences.getBytesLength("svrUrl"+svrIdx);
		memset( serverUrl, '\0', MAX_SVR_URL_LEN );
		preferences.getBytes("svrUrl"+svrIdx, serverUrl, serverUrlLen);
		readSvrCert( svrIdx, serverCertLen, serverCert );
		svrIdx ++;
	}while(serverUrlLen < 4 && svrIdx < MAX_NUM_SVRS);
	preferences.end();
}
void PostAndFetchDataFromCloudServer(PostType postType){
  
		if(mainLoopsSinceWebSockStartedConnecting > 100  || svrIdx >= MAX_NUM_SVRS){
			if(webSockClient != NULL ){
				Serial.println("freeing websocket client");
				esp_websocket_client_stop(webSockClient);
    		esp_websocket_client_destroy(webSockClient);
				
				Serial.print("finished freeing webSockClient "); Serial.println((int)webSockClient);
				webSockClient = NULL;
			}
			if(svrIdx >= MAX_NUM_SVRS){
				svrIdx = 0;
				if( camera_httpd == NULL ){
					startCameraServer();
					Serial.println("svrIdx reset");
				}
			}
			if( camera_httpd != NULL ){
				Serial.print("server at http://");
				Serial.print( WiFi.localIP() );
				Serial.print( " on " );
				Serial.println( foundNetwork );
			}
		}

	if( webSockClient == NULL ){//|| !esp_websocket_client_is_connected(webSockClient) && mainLoopsSinceWebSockStartedConnecting > 10000 ){
		//attempt connection to server
		Serial.println("attempt websocket connection");
    mainLoopsSinceWebSockStartedConnecting = 0;
    
		ReadNextSvrInfo();
		
		if( isAUrl(serverUrl) ){

			if( webSockClient != NULL ){
				esp_websocket_client_destroy(webSockClient);
				Serial.println("esp_websocket_client_destroy");
			}

			//https://docs.espressif.com/projects/esp-protocols/esp_websocket_client/docs/latest/index.html
			Serial.print( "WebSock to " ); Serial.print( serverUrl ); Serial.print( "|" ); Serial.println( serverUrlLen );
			Serial.print( "Cert Len ");
			//Serial.print( serverCert );
			Serial.print("|"); Serial.println( serverCertLen );
			const esp_websocket_client_config_t ws_cfg = {
				.uri = serverUrl,//"wss://echo.websocket.org",
				//.port = 4567,
				.cert_pem = (const char *)serverCert,
				.cert_len = serverCertLen + 1,
				//.subprotocol = 
				.transport = WEBSOCKET_TRANSPORT_OVER_SSL,
				.skip_cert_common_name_check = true,
				
			};
			webSockClient = esp_websocket_client_init(&ws_cfg);
			Serial.println( "webSock cli inited");
			  esp_err_t wEvts = esp_websocket_register_events(webSockClient, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)webSockClient);
			  Serial.print( "webSock reg " ); Serial.println( wEvts );
			  wEvts = esp_websocket_client_start(webSockClient);
			Serial.print("free heap "); Serial.print(esp_get_free_heap_size());
			  Serial.print( "webSock start " ); Serial.println( wEvts );

		}
	}

	if( esp_websocket_client_is_connected(webSockClient) ){
    //send a message to server 
		Serial.print("send message to server ");
		int httpResponseCode;
		if( postType == DEV_STATUS ){
			fillStatusString();
			httpResponseCode = esp_websocket_client_send_bin(webSockClient, (const char *)&(lastCsiInfoStr[0]), WEB_SEND_HDR_LEN+STATUS_RESPONSE_LENGTH, portMAX_DELAY);
		}else if( postType == DEV_SETTINGS ){
			fillSettingsString();
			httpResponseCode = esp_websocket_client_send_bin(webSockClient, (const char *)&(lastCsiInfoStr[0]), SETTINGS_RESPONSE_LENGTH, portMAX_DELAY);
		}else if( postType == IMAGE ){
			size_t _jpg_buf_len = 0;
			uint8_t * _jpg_buf = NULL;
			if( getJpeg( &_jpg_buf, &_jpg_buf_len) != 0 ){
				Serial.print("send img "); Serial.println( _jpg_buf_len );
			fillJpegSendHdr( _jpg_buf_len );
			httpResponseCode = esp_websocket_client_send_bin_partial(webSockClient, (const char *)&(lastCsiInfoStr[0]), WEB_SEND_HDR_LEN, portMAX_DELAY);
			Serial.print("jpgHdr Resp ");Serial.println(httpResponseCode);
					httpResponseCode = esp_websocket_client_send_cont_msg(webSockClient, (const char *)_jpg_buf, _jpg_buf_len, portMAX_DELAY);
			esp_websocket_client_send_fin(webSockClient, portMAX_DELAY);
					freeJpegBuf();
			}
		}

		Serial.print("POST resp: ");
		Serial.println(httpResponseCode);
	}

	//cloudHttp.end();
}


