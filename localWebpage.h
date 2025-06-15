
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
			<td><button id="armAlarmButton" class="button" onclick="ArmDisarm('ArmAlarm');">Arm Alarm</button></td>
			<td><button id="disarmAlarmButton" class="button" onclick="ArmDisarm('DisarmAlarm');">Disarm Alarm</button></td>
		</tr>
		</table>
		<br/>
		<table>
			<tr><th>Known Wifi Networks</th></tr>
		</table>
		<table id="storedSsidsTable">
		</table>
		<div id="storedSvrUrlTable">
		</div>
	<script>
		const clickableButtonBgColor = "#45B147"; //rgb(69, 177, 71)";
		const nonClickableButtonBgColor = "#8fa9d7";
    let pktIdx = 0;
		function sendCmd(cmd, val, valLen=0, sendCmpCb) {
			if( val == undefined ){
				val = '';
				valLen = 0;
			}else if( valLen == 0 ){
				valLen = val.length;
			}

			let pktIdxStr = pktIdx.toString().padStart(3);
			let devIdStr = "0".padStart(4);
			let numDatAndDevType = "1c";

			let sendStr = pktIdxStr + devIdStr + numDatAndDevType;

			let datType	= cmd;
			let dat		= val;
			let datLen	= valLen;
			if( dat == undefined ){
				dat = ''
				datLen = 0
			}
			else if( datLen == undefined ){
				datLen = dat.length;
			}

			let datTypeStr = datType.padEnd(11);
			let datLenStr = (datLen.toString()).padStart(6);
		
			sendStr += datTypeStr+datLenStr+dat;

			let xhr = new XMLHttpRequest();
			xhr.onreadystatechange = function(){
				if(xhr.readyState == 4){
					//if(xhr.status == 200 || xhr.status == 0){
						if( sendCmpCb != undefined )
							sendCmpCb(); //callback
					//}
				}
			}
			xhr.open("POST", "/action", true);
			xhr.send(sendStr);

			pktIdx += 1;
			if( pktIdx > 255 )
				pktIdx = 0
		}

		function ArmDisarm(cmd){
			if(cmd == "ArmAlarm"){
				document.getElementById("armAlarmButton").style.backgroundColor = nonClickableButtonBgColor;
				document.getElementById("disarmAlarmButton").style.backgroundColor = clickableButtonBgColor;
			}else if( cmd == "DisarmAlarm"){
				document.getElementById("armAlarmButton").style.backgroundColor = clickableButtonBgColor;
				document.getElementById("disarmAlarmButton").style.backgroundColor = nonClickableButtonBgColor;
			}

			sendCmd(cmd);
		}

		function requestSettings(){
			let xhr = new XMLHttpRequest();
			xhr.onreadystatechange = function(){
				if(xhr.readyState == 4){
					//if(xhr.status == 200 || xhr.status == 0){
						fillStoredSSIDsTable(xhr.response); //callback
            genSvrEntryTable(xhr.response);
					//}
				}
			}
			xhr.open("GET", "/settings", true);
			xhr.send();
		}

		function sendTxPower(){ sendCmd("txPower", document.getElementById("txPowerNum").value ); }
		
		function sendSvrUrl(num){
				let svrUrlStr = document.getElementById("svrUrlVal"+num).value;
				sendCmd("svrUrl"+num, svrUrlStr, svrUrlStr.length );
		}

		function sendSvrCert(num, cbItr=0){

			//send certificate in parts

			let certStr = document.getElementById("svrCert"+num).value;
			let certStrLen = certStr.length;

			if( cbItr == 0 ){
				let certStrLenStr = ""+certStrLen;
				sendCmd("svrCertLen"+num, certStrLenStr, certStrLenStr.length, function(){ sendSvrCert(num, 1) } );
				return;
			}
		
			let NUM_SVR_CERT_PARTS = 8;
			let SVR_CERT_PART_LENGTH = 256;
			let partIdx = 0;
			let partLen = SVR_CERT_PART_LENGTH;

			let i = cbItr - 1;
			partIdx = i*SVR_CERT_PART_LENGTH;
			partLen = Math.min( SVR_CERT_PART_LENGTH, certStrLen - partIdx);
			if( partLen <= 0 )
				return;
			let certPart = certStr.slice(partIdx,partIdx+partLen);
			sendCmd("svrCert"+num+"_"+i, certPart, partLen, function(){ sendSvrCert(num, i+1+1) } );

		}

    const SETTINGS_SVR_URL0_IDX = 37;
		function genSvrEntryTable(settingsStr){
			let svrUrlTable = document.getElementById("storedSvrUrlTable");
			for(let i = 0; i < MAX_STORED_NETWORKS; ++i){
				let svrUrlTbl = 
					'<table>\
							<tr>\
								<th>Server Url'+i+'</th>\
								<td><input id="svrUrlVal'+i+'" type="text" value="'+ 
					          strFromStrRange(settingsStr, SETTINGS_SVR_URL0_IDX+NETWORK_NAME_LENGTH*i, NETWORK_NAME_LENGTH )
					          + '"></input></td>\
								<td><button onclick="sendSvrUrl('+i+')">Set Cloud Server Url (wss://<ip or url>:port)</button></td>\
							</tr>\
						</table>\
						<table>\
							<tr>\
								<td><textarea id="svrCert'+i+'" cols="80" rows="10"></textarea></td>\
							</tr>\
							<tr>\
								<td><button onclick="sendSvrCert('+i+')">Set Cloud Server Cert</button></td>\
							</tr>\
						</table>';
				svrUrlTable.innerHTML += svrUrlTbl;
			}
		}

		function sendWifi_SSID_Pass(n){ 
			let nameStr = document.getElementById("net"+n).value;
			sendCmd("net"+n, nameStr.padEnd(32), nameStr.length );
			let passStr = document.getElementById("pass"+n).value;
			sendCmd("pass"+n, passStr.padEnd(32), passStr.length );
		}

		const MAX_STORED_NETWORKS = 10;
		const SETTINGS_NET0_IDX = 357;
		const NETWORK_NAME_LENGTH = 32;
		function fillStoredSSIDsTable(settingsStr){
			let ssidsTable = document.getElementById("storedSsidsTable");

			for(let i = 0; i < MAX_STORED_NETWORKS; ++i){
				let ssidTr = "\
				<tr>\
				<th>Net"+i+"</th>\
				<td><input id='net"+i+"' type='text' value='"+ 
					strFromStrRange(settingsStr, SETTINGS_NET0_IDX+NETWORK_NAME_LENGTH*2*i, NETWORK_NAME_LENGTH )
					+ "'></input></td>\
				<th>Pass"+i+"</th>\
				<td><input id='pass"+i+"' type='text' value='" +
					strFromStrRange(settingsStr, SETTINGS_NET0_IDX+NETWORK_NAME_LENGTH*(2*i+1), NETWORK_NAME_LENGTH )
					+ "'></input></td>\
				<td><button onClick=\"sendWifi_SSID_Pass("+i+")\">Set</button></td>\
				</tr>\
				";
				ssidsTable.innerHTML += ssidTr;
			}
		}

		function strFromStrRange(combinedStr, strt, maxLen){
			let strPart = "";
			for( let i = strt; i < strt+maxLen; ++i){
				if( combinedStr[i] == '\0' )
					break;
				strPart += combinedStr[i];
			}
			return strPart;
		}

		//genSvrEntryTable();
		requestSettings();
	</script>
	</body>
</html>
)rawliteral";
