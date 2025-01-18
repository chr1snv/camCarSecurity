
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
		</table>
    <table id="storedSsidsTable">
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
				<td><button onclick="sendWifi_SSID_Pass(0)">Add SSID and Password</button></td>
			</tr>
		</table>
	<script>
		const clickableButtonBgColor = "#45B147"; //rgb(69, 177, 71)";
		const nonClickableButtonBgColor = "#8fa9d7";
		function sendCmd(x, val, valLen=0) {
      if( val == undefined ){
				val = '';
				valLen = 0;
			}else if( valLen == 0 ){
        valLen = val.length;
      }
			var xhr = new XMLHttpRequest();
			xhr.open("POST", "/action", true);
			xhr.send((x).padEnd(12)+(valLen.toString()).padStart(4)+val);
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
		function sendWifi_SSID_Pass(n){ 
      sendCmd("net"+n, document.getElementById("net"+n).value.padEnd(32) );
			sendCmd("pass"+n, document.getElementById("pass"+n).value.padEnd(32) );
    }
    
    const MAX_STORED_NETWORKS = 10;
    function fillStoredSSIDsTable(){
			let ssidsTable = document.getElementById("storedSsidsTable");

			for(let i = 0; i < MAX_STORED_NETWORKS; ++i){
				let ssidTr = "\
				<tr>\
				<th>Net"+i+"</th>\
				<td><input id='net"+i+"' type='text'></input></td>\
				<th>Pass"+i+"</th>\
				<td><input id='pass"+i+"' type='text'></input></td>\
				<td><button onClick=\"sendWifi_SSID_Pass("+i+")\">Set</button></td>\
				<td><button onClick=\"clearWifi_SSID_Pass("+i+")\">Clear</button></td>\
				</tr>\
				";
				ssidsTable.innerHTML += ssidTr;
			}
		}

		function strFromStrRange(combinedStr, strt, lenDel){
		let strPart = "";
		for( let i = strt; i < strt+lenDel; ++i)
			strPart += combinedStr[i];
		return strPart;
		}

    fillStoredSSIDsTable();
	</script>
	</body>
</html>
)rawliteral";
