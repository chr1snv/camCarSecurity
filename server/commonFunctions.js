

function strFromByteRange(combinedStr, strt, lenDel){
	let strPart = "";
	for( let i = strt; i < strt+lenDel; ++i){
		let char = combinedStr[i];
		if( char != 0 )//'\u0000' )
		strPart += String.fromCharCode(char);
	}
	return strPart;
}


let td = new TextDecoder();
function sFBa(b){
	return td.decode(b);
}

//ascii to int reverse iteration for n characters
//input is end of number (1's place)
//counts up in significance (x10), decrementing string index from start index
function atoir_n( c, n ){
	let accum = 0;
	let mult = 1;
	let cLIdx = c.length-1;
	//Serial.print( "atoir_n d ");
	for( let i = 0; i < n; ++i ){
		let d = c[cLIdx-i];
		if( d >= '0' && d <= '9' )
			accum += (d - '0')*mult;
		else
			break;
		mult *= 10;
	}
	return accum;
}

var webSocketOnMessage = null;


let webSocketSvrUrlParts = document.URL.split(":");
const webSocketSvrUrl = webSocketSvrUrlParts[0] + ":" + webSocketSvrUrlParts[1] + ":" + "9999"
let socketInstance = null;
let queuedToSendWebsocketMessage = null;
let wsCallBack = null;
let socketErrorTimeoutHandle = null;
function sendWebsocketServerMessage(signalingMessage, nonRateLimitedMessage=false, callBack=undefined){

	if( socketInstance == null ){
		queuedToSendWebsocketMessage = signalingMessage;
		wsCallBack = callBack;
		socketInstance = new WebSocket(webSocketSvrUrl);
		socketInstance.binaryType = 'arraybuffer';
		
		socketInstance.onopen = () => {
			console.log( "socketInstance connection opened");
			socketInstance.send( queuedToSendWebsocketMessage );
		}

		
		socketInstance.onerror = (event) => {
			console.log("socketInstance.onerror " + event.data);
			let nAtElm = document.getElementById("networkAuthText");
			nAtElm.innerHTML = "websocket, certificate may need to be accepted - or server error";
			let nALElm = document.getElementById("networkAuthLink");
			let urlParts = webSocketSvrUrl.split("://");
			let url = "https://"+urlParts[1];
			nALElm.href = url;
			nALElm.innerText = url;
			clearTimeout( socketErrorTimeoutHandle );
			socketErrorTimeoutHandle = setTimeout( 
				function(){ 
					document.getElementById("networkAuthText").innerText = '';
					document.getElementById("networkAuthLink").innerText = '';
				},
				5000 );
		}

		socketInstance.onclose = (event) => {
			console.log("socketInstance.onclose code: " + event.code);
		}

		socketInstance.onmessage = (event) => {
			let response = event.data;
			if( (response.byteLength != undefined && response.byteLength <= 0) || 
				(response.size != undefined && response.size <= 0 ) )
				return;
			//console.log("socketInstance.onmessage " + response);
			
			//parse the num data, from, and data fields from the server
			//|pktNum(3)|devId(4) | numData(1) | 'd','s','c','l'(1) ||||dataTypeStr (11) | deviceFromId(4) | dataLen(6) | data |||
			//||| - |||| repeats num commands times up to CMD_BUFF_MAX_LEN
			let datIdx = 0;
			let pktNum = atoir_n(sFBa( response.slice(0,3) ), 3 ); datIdx += 3;
			//document.getElementById("pktNum").innerText = pktNum;
			let pktFromDeviceId = atoir_n( sFBa( response.slice(3,7) ), 4 ); datIdx += 4;
			//document.getElementById('devId').innerText = pktFromDeviceId;
			let numData = Number.parseInt(sFBa( response.slice(datIdx,datIdx+1) )); datIdx += 1;
			let respFrom = sFBa( response.slice(datIdx,datIdx+1) ); datIdx+=1;
			
			for(let dNum=0; dNum < numData; ++dNum ){

				let datType = sFBa( response.slice(datIdx,datIdx+11) ); datIdx += 11;
				let datLen = atoir_n( sFBa( response.slice(datIdx,datIdx+6) ), 6 ); datIdx += 6;
				let dat = response.slice(datIdx,datIdx+datLen); datIdx += datLen;
				let datStr = sFBa( dat );
				if(datType.startsWith('getKey') )
					finishUrlGoto(sFBa(dat), datLen);
				else if( datType.startsWith("remPkts") ){
					let remPktsElm = document.getElementById("remainingPackets");
					if( remPktsElm )
						remPktsElm.innerText = sFBa(dat);
				}else if( datType.startsWith("logout") ){
					logout();
				}else //per page functionality
					handleWebSockDataType(datType, dat, datLen);

			}
			
			if(wsCallBack != null){
				wsCallBack();
				//wsCallBack = undefined;
			}
		}

	}else{
		if( socketInstance.readyState == socketInstance.OPEN ){
			wsCallBack = callBack;
			socketInstance.send(signalingMessage);
		}else{
			console.log( "socketInstance not readyToSend readyState " + socketInstance.readyState );
			if( socketInstance.readyState > 1 ){
				socketInstance.close();
				socketInstance = null;
			}
		}
	}

}

let thisCliId = -1;

let pktIdx = 0;
function sendCmds( datas, callback ) {

	let pktIdxStr = pktIdx.toString().padStart(3);
	let devIdStr = (thisCliId).toString().padStart(4);
	let numDatAndDevType = datas.length + "c";

	let sendStr = pktIdxStr + devIdStr + numDatAndDevType;

	for( let i = 0; i < datas.length; ++i){
		let data = datas[i];

		let datType	= data[0];
		let dat		= data[1];
		let datLen	= data[2];
		if( dat == undefined ){
			dat = ''
			datLen = 0
		}
		if( datLen == undefined ){
			if( dat.length == undefined )
				dat = String( dat );
			datLen = dat.length;
		}

		let datTypeStr = datType.padEnd(11);
		let datLenStr = (datLen.toString()).padStart(6);
		
		sendStr += datTypeStr+datLenStr+dat;
	}

	sendWebsocketServerMessage(sendStr, callback);
	pktIdx += 1;
	if( pktIdx > 255 )
		pktIdx = 0
}

function sendCmd(datType, dat, datLen=undefined, callBack=undefined){
	key = localStorage.getItem('authKey');
	sendCmds([['auth', key, key.length],[datType, dat, datLen]], callBack);
}

pendingUrlRequest = [];
function gotoUrlPlusAuthKeyAndArgs(url, additionalArgs=undefined){
	pendingUrlRequest = [url, additionalArgs];
	sendCmd('getKey');
};
function finishUrlGoto(key){
	let url = pendingUrlRequest[0];
	let getStr = url + "?" + key;
	let additionalArgs = pendingUrlRequest[1];
	if( additionalArgs != undefined ){
		for( let i = 0; i < additionalArgs.length; ++i ){
			getStr += "&" + additionalArgs[i][0] + "=" + additionalArgs[i][1];
		}
	}
	document.location.href = getStr;
	pendingUrlRequest = [];
}

function logout(){
	sendCmd('logout', localStorage.getItem('authKey') );
	localStorage.setItem('authKey', '');
	document.location.href = '/';
}



