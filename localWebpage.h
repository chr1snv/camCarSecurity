
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
