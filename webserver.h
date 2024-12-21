

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
        </tr>
        <tr>
          <td id="magHeading"></td>
          <td id="magAlarmDiff"></td>
          <td id="magAlarmTrgr"></td>
          <td id="alarmOutput"></td>
        </tr>
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
          <td><button onclick="sendCmd('camFlipVertical', 1);">Cam Vert Flip On</button></td>
          <td><button onclick="sendCmd('camFlipVertical', 0);">Cam Vert Flip Off</button></td>
        </tr>
        <tr>
          <td><button onclick="sendCmd('camResolution', 'QVGA');">QVGA</button></td>
          <td><button onclick="sendCmd('camResolution', 'CIF');">CIF</button></td>
          <td><button onclick="sendCmd('camResolution', 'VGA');">VGA</button></td>
          <td><button onclick="sendCmd('camResolution', 'SVGA');">SVGA</button></td>
        </tr>
      </table>
   <script>
    const clickableButtonBgColor = "#45B147;"; //rgb(69, 177, 71)";
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

    function sendMagThreshold(){
      sendCmd("magAlarmThreshold", document.getElementById("magThresholdNum").value );
    }
    function sendCamQuality(){
      sendCmd("camQuality", document.getElementById("camQualityNum").value );
    }
    function sendTxPower(){
      sendCmd("txPower", document.getElementById("txPowerNum").value );
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
    function MainLoop(){
      if(!inProgressRequest){
        var xhr = new XMLHttpRequest();
        xhr.timeout = 300;
        xhr.responseType = "text";
        xhr.onreadystatechange = function(){
          if(xhr.readyState == 4){
            if(xhr.status == 200 || xhr.status == 0){
              //document.getElementById("status").innerText = xhr.responseText;
              inProgressRequest = false;
              let a = xhr.responseText;
              let rL = 1024;

              if ( xhr.responseText.length < rL )
                return;

              let alarmArmed = strFromStrRange(a, rL-80, 5);
              let magArmX    = strFromStrRange(a, rL-75, 5);
              let magArmY    = strFromStrRange(a, rL-70, 5);
              let magArmZ    = strFromStrRange(a, rL-65, 5);
            
              let servo1     = strFromStrRange(a, rL-60, 5);
              let servo2     = strFromStrRange(a, rL-55, 5);

              let rssi = strFromStrRange(a, rL-50, 5);
              let temp = strFromStrRange(a, rL-45, 5);
              let magX = strFromStrRange(a, rL-40, 5);
              let magY = strFromStrRange(a, rL-35, 5);
              let magZ = strFromStrRange(a, rL-30, 5);

              let magHeading   = strFromStrRange(a, rL-25, 5);
              let magAlarmDiff = strFromStrRange(a, rL-20, 5);
              let magAlarmTrgr = strFromStrRange(a, rL-15, 5);
              let alarmOutput  = strFromStrRange(a, rL-10, 5);

              document.getElementById("alarmArmed").innerText = alarmArmed;
              document.getElementById("magArmX").innerText    = magArmX;
              document.getElementById("magArmY").innerText    = magArmY;
              document.getElementById("magArmZ").innerText    = magArmZ;

              document.getElementById("servo1").innerText = servo1;
              document.getElementById("servo2").innerText = servo2;

              document.getElementById("rssi").innerText = rssi;
              document.getElementById("temp").innerText = temp;
              document.getElementById("magX").innerText = magX;
              document.getElementById("magY").innerText = magY;
              document.getElementById("magZ").innerText = magZ;

              document.getElementById("magHeading").innerText = magHeading;
              document.getElementById("magAlarmDiff").innerText = magAlarmDiff;
              document.getElementById("magAlarmTrgr").innerText = magAlarmTrgr;
              document.getElementById("alarmOutput").innerText  = alarmOutput;
            }
          }
        }
        xhr.ontimeout = function(e){
          inProgressRequest = false;
        }
        xhr.onabort = xhr.ontimeout;
        xhr.onerror = xhr.ontimeout;
        xhr.open("GET", "/status", true);
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
  </script>
  </body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t status_handler(httpd_req_t *req){
  //prints the last 
  //wifi_csi info
  //temperature
  //magnetic field sample
  //acceleration reading

  temp_sense();
  sense_wifi_rssi();

  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-80]), 5, "%i\n",alarmArmed);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-75]), 5, "%i\n",magAX);//alarmInit_magSample.X);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-70]), 5, "%i\n",magAY);//alarmInit_magSample.Y);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-65]), 5, "%i\n",magAZ);//alarmInit_magSample.Z);

  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-60]), 5, "%i\n",servo1Pos);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-55]), 5, "%i\n",servo2Pos);

  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-50]), 5, "%i\n",staRssi);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-45]), 4, "%i\n",lastTemperature);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-40]), 5, "%i\n",magX);//compass.magSample.X);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-35]), 5, "%i\n",magY);//compass.magSample.Y);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-30]), 5, "%i\n",magZ);//compass.magSample.Z);

  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-25]), 5, "%f\n",magHeading);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-20]), 5, "%i\n",magAlarmDiff);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-15]), 5, "%i\n",magAlarmTriggered);
  snprintf( &(lastCsiInfoStr[CSI_INF_STR_LEN-10]), 5, "%i\n",alarmOutput);

  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, lastCsiInfoStr, CSI_INF_STR_LEN);
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

static esp_err_t jpg_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, "image/jpeg");
  if(res != ESP_OK){
    return res;
  }

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

  return httpd_resp_send(req, (const char *)_jpg_buf, _jpg_buf_len);
}

static esp_err_t cmd_handler(httpd_req_t *req){
  char*  buf;
  size_t buf_len;
  char variable[32] = {0,};
  char value[32] = {0,};
  
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if(!buf){
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "go", variable, sizeof(variable)) == ESP_OK) {
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

  //camera setting changes
  sensor_t * s = esp_camera_sensor_get();

  if(!strcmp(variable, "camFlipVertical")) {
    //flip the camera vertically
    if( httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK ){
      s->set_vflip(s, atoi(value));          // 0 = disable , 1 = enable
      // mirror effect
      //s->set_hmirror(s, 1);          // 0 = disable , 1 = enable
    }
  }
  if(!strcmp(variable, "camResolution")){
    if( httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK){
      if( !strcmp(value, "QVGA") )
        s->set_framesize(s, FRAMESIZE_QVGA );
      else if( !strcmp(value, "CIF") )
        s->set_framesize(s, FRAMESIZE_CIF );
      else if( !strcmp(value, "VGA") )
        s->set_framesize(s, FRAMESIZE_VGA );
      else if( !strcmp(value, "SVGA") )
        s->set_framesize(s, FRAMESIZE_SVGA );
    }
  }

  if(!strcmp(variable, "camQuality")){
    if( httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK)
      s->set_quality(s, atoi(value));
  }

  //config.frame_size = FRAMESIZE_QVGA;
  //config.jpeg_quality = 63;

  int res = 0;
  
  //servo position control
  if(!strcmp(variable, "up")) {
    if(servo1Pos <= 170) {
      servo1Pos += 10;
      servo1.write(servo1Pos);
    }
    Serial.println(servo1Pos);
    Serial.println("Up");
  }
  else if(!strcmp(variable, "left")) {
    if(servo2Pos <= 170) {
      servo2Pos += 10;
      servo2.write(servo2Pos);
    }
    Serial.println(servo2Pos);
    Serial.println("Left");
  }
  else if(!strcmp(variable, "right")) {
    if(servo2Pos >= 10) {
      servo2Pos -= 10;
      servo2.write(servo2Pos);
    }
    Serial.println(servo2Pos);
    Serial.println("Right");
  }
  else if(!strcmp(variable, "down")) {
    if(servo1Pos >= 10) {
      servo1Pos -= 10;
      servo1.write(servo1Pos);
    }
    Serial.println(servo1Pos);
    Serial.println("Down");
  }
  else if(!strcmp(variable, "center")) {
    servo1Pos = 0;
    servo2Pos = 0;
    servo1.write(servo1Pos);
    servo2.write(servo2Pos);
    
    Serial.print(servo1Pos);
    Serial.println(servo2Pos);
    Serial.println("center");
  }
  else if(!strcmp(variable, "Light")){
    lightLedValue = !lightLedValue;
    digitalWrite(LightLEDPin, lightLedValue);
    Serial.print("Light");
    Serial.println(lightLedValue);
  }
  else if(!strcmp(variable, "ArmAlarm")){
    ArmAlarm(true);
    Serial.println("ArmAlarm");
  }
  else if(!strcmp(variable, "DisarmAlarm")){
    ArmAlarm(false);
    Serial.println("DisarmAlarm");
  }

  else if(!strcmp(variable, "magAlarmThreshold")){
    if( httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK)
      MAG_ALARM_DELTA_THRESHOLD = atoi(value);
  }

  else if(!strcmp(variable, "txPower")){
    if( httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK)
      MAG_ALARM_DELTA_THRESHOLD = atoi(value);
  }

  else {
    res = -1;
  }

  if(res){
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
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
  }
  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}