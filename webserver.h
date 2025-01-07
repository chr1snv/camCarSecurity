
#include "esp_http_server.h"
//#include "esp_http_client.h"
#include "HTTPClient.h"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<html>
	<head>
		<title>ESP32-Car Status</title>
		<meta name="viewport" content="width=device-width, initial-scale=1">
		<style>
		body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px;}
		table { margin-left: auto; margin-right: auto; align:center; }
		th { border: 1px solid; }
		td { padding: 8 px; align:center; }
		.button {
			background-color: #2f4468;
			border: none;
			color: white;
			padding: 10px 20px;
			text-align: center;
			text-decoration: none;
			display: inline-block;
			font-size: 18px;
			margin: 6px 3px;
			cursor: pointer;
			/*-webkit-touch-callout: none;
			-webkit-user-select: none;
			-khtml-user-select: none;
			-moz-user-select: none;
			-ms-user-select: none;
			user-select: none;*/
			-webkit-tap-highlight-color: rgba(0,0,0,0);
		}
		img {  width: auto ;
			max-width: 100% ;
			height: auto ; 
		}
		</style>
	</head>
	<body>
		<h1>ESP32-CAM Car Alarm</h1>
		<img src="" id="photo" >
		<table>
		<tr>
				<td></td>
				<td align="center"><button class="button" onclick="sendCmd('up');">Up</button></td>
				<td></td>
		</tr>
		<tr>
				<td align="center"><button class="button" onclick="sendCmd('left');">Left</button></td>
				<td align="center"><button class="button" style="background-color:dimgray; border-radius:10px;" onclick="sendCmd('center');">Center</button></td>
				<td align="center"><button class="button" onclick="sendCmd('right');">Right</button></td>
		</tr>
		<tr>
			<td></td>
			<td align="center"><button class="button" onclick="sendCmd('down');">Down</button></td>
			<td></td>
		</tr>                   
		<tr>
			<td></td>
			<td align="center"><button class="button" style="background-color: darkorange;" onclick="sendCmd('Light');">Light</button></td>
		</tr>
		</table>
		<table>
		<tr>
			<td ><button id="armAlarmButton" class="button" onclick="sendCmd('ArmAlarm');">Arm Alarm</button></td>
			<td ><button id="disarmAlarmButton" class="button" onclick="sendCmd('DisarmAlarm');">Disarm Alarm</button></td>
		</tr>
		</table>
		<br/>
		<table style="border:1px sold;">
			<tr id=status>System Status</tr>
			<tr>
				<th>alarm armed</th><th>magArmX</th><th>magArmY</th><th>magArmZ</th>
			</tr>
			<tr>
				<td id="alarmArmed"></td>
				<td id="magArmX"></td>
				<td id="magArmY"></td>
				<td id="magArmZ"></td>
			</tr>
			<tr>
				<th style="border: none;"></th><th>servo1</th><th>servo2</th>
			</tr>
				<td></td>
				<td id="servo1"></td>
				<td id="servo2"></td>
			<tr>
			</tr>
			<tr>
				<th>rssi</th><th>temp</th><th>magX</th><th>magY</th><th>magZ</th>
			</tr>
			<tr>
				<td id="rssi"></td>
				<td id="temp"></td>
				<td id="magX"></td>
				<td id="magY"></td>
				<td id="magZ"></td>
			</tr>
			<tr>
				<th>Mag Heading</th>
				<th>Mag Alarm Diff</th>
				<th>Mag Alrm Trgr</th>
				<th>Alarm Output</th>
				<th>Stat Ping</th>
			</tr>
			<tr>
				<td id="magHeading"></td>
				<td id="magAlarmDiff"></td>
				<td id="magAlarmTrgr"></td>
				<td id="alarmOutput"></td>
				<td id="statusPing"></td>
			</tr>
		</table>
		Settings
		<table>
			<tr>
				<th>Mag Threshold</th>
				<th>Cam Quality</th>
				<th>Cam Resolution</th>
				<th>Tx Power</th>
			</tr>
			<tr>
				<td id="magThreshold"></td>
				<td id="camQuality"></td>
				<td id="camResolution"></td>
				<td id="txPower"></td>
			</tr>
		</table>
		<table>
			<tr><th>Server Url</th><td id="svrUrl"></td></tr>
			<tr><th>Net0</th><td id="net0"></td><th>Pass0</th><td id="pass0"></td></tr>
			<tr><th>Net1</th><td id="net1"></td><th>Pass1</th><td id="pass1"></td></tr>
			<tr><th>Net2</th><td id="net2"></td><th>Pass2</th><td id="pass2"></td></tr>
			<tr><th>Net3</th><td id="net3"></td><th>Pass3</th><td id="pass3"></td></tr>
			<tr><th>Net4</th><td id="net4"></td><th>Pass4</th><td id="pass4"></td></tr>
			<tr><th>Net5</th><td id="net5"></td><th>Pass5</th><td id="pass5"></td></tr>
			<tr><th>Net6</th><td id="net6"></td><th>Pass6</th><td id="pass6"></td></tr>
			<tr><th>Net7</th><td id="net7"></td><th>Pass7</th><td id="pass7"></td></tr>
			<tr><th>Net8</th><td id="net8"></td><th>Pass8</th><td id="pass8"></td></tr>
			<tr><th>Net9</th><td id="net9"></td><th>Pass9</th><td id="pass9"></td></tr>
		</table>
		<table>
			<tr>
				<td><input id="magThresholdNum" type="number" min="0" max="500" value="20"></input></td>
				<td><button onclick="sendMagThreshold();">Set Mag Alarm Threshold</button></td>
				<td><input id="camQualityNum" type="number" min="0" max="63" value="30"></input></td>
				<td><button onclick="sendCamQuality();">Set Cam Quality (0-63)</button></td>
				<td><input id="txPowerNum" type="number" min="8" max="80" value="50" title="[8, 84] -> 2dBm - 20dBm"></input></td>
				<td><button onclick="sendTxPower();">Set Tx Power (8-80)</button></td>
			</tr>
			<tr>
				<td><button onclick="sendCmd('camFlipVertical', 1);">Vert Flip</button></td>
				<td><button onclick="sendCmd('camFlipVertical', 0);">Vert No Flip</button></td>
				<td><button onclick="sendCmd('camFlipHoriz', 1);">Horiz Flip</button></td>
				<td><button onclick="sendCmd('camFlipHoriz', 0);">Horiz No Flip</button></td>
				<td><button onclick="camRot90(1);">90deg Rotate</button></td>
				<td><button onclick="camRot90(0);">No Rotate</button></td>
			</tr>
			<tr>
				<td><button onclick="sendCmd('camResolution', 'QVGA');">QVGA</button></td>
				<td><button onclick="sendCmd('camResolution', 'CIF');">CIF</button></td>
				<td><button onclick="sendCmd('camResolution', 'VGA');">VGA</button></td>
				<td><button onclick="sendCmd('camResolution', 'SVGA');">SVGA</button></td>
			</tr>
			<tr>
				<td><input id="svrUrlVal" type="text"></input></td>
				<td><button onclick="sendSvrUrl()">Set Cloud Server Url (http(s)://<ip/url>:port)</button></td>
			</tr>
			<tr>
				<td><input id="ssid" type="text"></input></td>
				<td><input id="pass" type="text"></input></td>
				<td><button onclick="sendWifi_SSID_Pass()">Add SSID and Password</button></td>
			</tr>
		</table>
	<script>
		const clickableButtonBgColor = "#45B147"; //rgb(69, 177, 71)";
		const nonClickableButtonBgColor = "#8fa9d7";
		function sendCmd(x, val) {
			var xhr = new XMLHttpRequest();
			xhr.open("GET", "/action?go=" + x + "&val=" + val, true);
			xhr.send();
			if(x == "ArmAlarm"){
				document.getElementById("armAlarmButton").style.backgroundColor = nonClickableButtonBgColor;
				document.getElementById("disarmAlarmButton").style.backgroundColor = clickableButtonBgColor;
			}else if( x == "DisarmAlarm"){
				document.getElementById("armAlarmButton").style.backgroundColor = clickableButtonBgColor;
				document.getElementById("disarmAlarmButton").style.backgroundColor = nonClickableButtonBgColor;
			}
		}

		function sendMagThreshold(){ sendCmd("magAlarmThreshold", document.getElementById("magThresholdNum").value ); }
		function sendCamQuality(){ sendCmd("camQuality", document.getElementById("camQualityNum").value ); }
		function sendTxPower(){ sendCmd("txPower", document.getElementById("txPowerNum").value ); }
		function sendSvrUrl(){ sendCmd("svrUrl", document.getElementById("svrUrlVal").value ); }
		function sendWifi_SSID_Pass(){ sendCmd("ssidPass", document.getElementById("ssid").value.padEnd(32) + document.getElementById("pass").value.padEnd(32) ); }

		function camRot90(rot){
			let photoElm = document.getElementById("photo");
			if( rot ){
				let widthMinHeight = (photoElm.width - photoElm.height)/2;
				photoElm.style = "transform: rotate(-90deg); margin-bottom: " + widthMinHeight + "px; margin-top: " + widthMinHeight + "px;";
			}else{
				photoElm.style = "";
			}
		}

		window.onload = document.getElementById("photo").src = window.location.href.slice(0, -1) + ":81/stream";

		function strFromStrRange(combinedStr, strt, lenDel){
		let strPart = "";
		for( let i = strt; i < strt+lenDel; ++i)
			strPart += combinedStr[i];
		return strPart;
		}

		//variables to wait for response
		var inProgressRequest = false;
		var numLoopsInProgress = 0;
		const MaxProgressLoops = 10;

		var numStatusRequests = 0;
		const numStatusRequestsBeforeSettingsRequest = 100;
		function MainLoop(){
		if(!inProgressRequest){
			const timeRequestedStatus = new Date();
			var xhr = new XMLHttpRequest();
			xhr.timeout = 300;
			xhr.responseType = "text";
			xhr.onreadystatechange = function(){
				if(xhr.readyState == 4){
					const now = new Date();
					const diff = now - timeRequestedStatus;
					document.getElementById("statusPing").innerText = diff;
					inProgressRequest = false;

					++numStatusRequests;

					let a = xhr.responseText;
					let rL = xhr.responseText.length;
					if( rL < 80 ) //avoid overwriting with NaN/undefined on bad response
						return;

					document.getElementById("alarmArmed").innerText		= parseInt(strFromStrRange(a, rL-80, 3));
					document.getElementById("light").innerText			= parseInt(strFromStrRange(a, rL-87, 2));
					document.getElementById("magArmX").innerText		= parseInt(strFromStrRange(a, rL-75, 5));
					document.getElementById("magArmY").innerText		= parseInt(strFromStrRange(a, rL-70, 5));
					document.getElementById("magArmZ").innerText		= parseInt(strFromStrRange(a, rL-65, 5));

					document.getElementById("servo1").innerText			= parseInt(strFromStrRange(a, rL-60, 5));
					document.getElementById("servo2").innerText			= parseInt(strFromStrRange(a, rL-55, 5));

					document.getElementById("rssi").innerText			= parseInt(strFromStrRange(a, rL-50, 5));
					document.getElementById("temp").innerText			= parseInt(strFromStrRange(a, rL-45, 5));
					document.getElementById("magX").innerText			= parseInt(strFromStrRange(a, rL-40, 5));
					document.getElementById("magY").innerText			= parseInt(strFromStrRange(a, rL-35, 5));
					document.getElementById("magZ").innerText			= parseInt(strFromStrRange(a, rL-30, 5));

					document.getElementById("magHeading").innerText		= parseFloat(strFromStrRange(a, rL-25, 5));
					document.getElementById("magAlarmDiff").innerText	= parseInt(strFromStrRange(a, rL-15, 5));
					document.getElementById("magAlarmTrgr").innerText	= parseInt(strFromStrRange(a, rL-10, 5));
					document.getElementById("alarmOutput").innerText	= parseInt(strFromStrRange(a, rL-5, 5));
				}
			}
			xhr.ontimeout = function(e){
				inProgressRequest = false;
			}
			xhr.onabort = xhr.ontimeout;
			xhr.onerror = xhr.ontimeout;
			if( numStatusRequests >= numStatusRequestsBeforeSettingsRequest ){
				numStatusRequests = 0;
				xhr.onreadystatechange = function(){
				if(xhr.readyState == 4){
					if(xhr.status == 200 || xhr.status == 0){
						let a = xhr.responseText;
						let rL = a.length;
						if( rL < 10)
						return; //avoid printing undefined

						document.getElementById("magThreshold").innerText	= strFromStrRange(a, 0, 5);
						document.getElementById("camQuality").innerText		= strFromStrRange(a, 5, 5);
						document.getElementById("camResolution").innerText	= strFromStrRange(a, 10, 5);
						document.getElementById("txPower").innerText		= strFromStrRange(a, 15, 5);

						document.getElementById("svrUrl").innerText			= strFromStrRange(a, 20, 32);

						let MAX_STORED_NETWORKS = 10;
						let NETWORK_NAME_LEN = 32;
						for( let i = 0; i < MAX_STORED_NETWORKS; ++i ){
							document.getElementById("net"+i).innerText       = strFromStrRange(a, 52+(NETWORK_NAME_LEN*2)*i, 32);
							document.getElementById("pass"+i).innerText       = strFromStrRange(a, 52+(NETWORK_NAME_LEN*2)*i+32, 32);
						}
					}
				}
				}

				xhr.open("GET", "/settings", true);
			}else{
				xhr.open("GET", "/status", true);
			}
			xhr.send();
			inProgressRequest = true;
		}else{
			if( numLoopsInProgress > MaxProgressLoops ){ //done waiting for response
				numLoopsInProgress = 0;
				inProgressRequest = false;
			}
			numLoopsInProgress += 1;
		}
		}

		setInterval(MainLoop, 100); //100ms (10x a second)
		camRot90(true);
	</script>
	</body>
</html>
)rawliteral";

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
