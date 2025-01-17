
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<html>
	<head>
		<title>ESP32-Setings</title>
		<meta name="viewport" content="width=device-width, initial-scale=1">
	</head>
	<body>
		<h1>ESP32-CAM Car Alarm Settings</h1>
		<table>
		<tr>
			<td ><button id="armAlarmButton" class="button" onclick="sendCmd('ArmAlarm');">Arm Alarm</button></td>
			<td ><button id="disarmAlarmButton" class="button" onclick="sendCmd('DisarmAlarm');">Disarm Alarm</button></td>
		</tr>
		</table>
		<br/>
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
				<td><input id="svrUrlVal" type="text"></input></td>
				<td><button onclick="sendSvrUrl()">Set Cloud Server Url and cert (wss://<ip or url>:port)</button></td>
			</tr>
      <tr>
        <td><textarea id="svrCert" cols="80" rows="10"></textarea></td>
      </tr>
    </table>
    <table>
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
			xhr.open("POST", "/action", true);
			xhr.send(x.padEnd(16)+val);
			if(x == "ArmAlarm"){
				document.getElementById("armAlarmButton").style.backgroundColor = nonClickableButtonBgColor;
				document.getElementById("disarmAlarmButton").style.backgroundColor = clickableButtonBgColor;
			}else if( x == "DisarmAlarm"){
				document.getElementById("armAlarmButton").style.backgroundColor = clickableButtonBgColor;
				document.getElementById("disarmAlarmButton").style.backgroundColor = nonClickableButtonBgColor;
			}
		}

		function sendTxPower(){ sendCmd("txPower", document.getElementById("txPowerNum").value ); }
		function sendSvrUrl(){ 
      sendCmd("svrUrl", document.getElementById("svrUrlVal").value );
      sendCmd("svrCert", document.getElementById("svrCert").value );
    }
		function sendWifi_SSID_Pass(){ sendCmd("ssidPass", document.getElementById("ssid").value.padEnd(32) + document.getElementById("pass").value.padEnd(32) ); }

		function strFromStrRange(combinedStr, strt, lenDel){
		let strPart = "";
		for( let i = strt; i < strt+lenDel; ++i)
			strPart += combinedStr[i];
		return strPart;
		}
	</script>
	</body>
</html>
)rawliteral";
