

function strFromStrRange(combinedStr, strt, lenDel){
	let strPart = "";
	for( let i = strt; i < strt+lenDel; ++i){
		let char = combinedStr[i];
		if( char != '\u0000' )
		strPart += char;
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
function sendWebsocketServerMessage(signalingMessage, nonRateLimitedMessage=false){

	if( socketInstance == null ){
		queuedToSendWebsocketMessage = signalingMessage;
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
			let pktNum = atoir_n(td.decode( response.slice(0,3) ), 3 ); datIdx += 3;
			//document.getElementById("pktNum").innerText = pktNum;
			let pktFromDeviceId = atoir_n( td.decode( response.slice(3,7) ), 4 ); datIdx += 4;
			//document.getElementById('devId').innerText = pktFromDeviceId;
			let numData = Number.parseInt(td.decode( response.slice(datIdx,datIdx+1) )); datIdx += 1;
			let respFrom = td.decode( response.slice(datIdx,datIdx+1) ); datIdx+=1;
			
			for(let dNum=0; dNum < numData; ++dNum ){

				let datType = td.decode( response.slice(datIdx,datIdx+11) ); datIdx += 11;
				let datLen = atoir_n( td.decode( response.slice(datIdx,datIdx+6) ), 6 ); datIdx += 6;
				let dat = response.slice(datIdx,datIdx+datLen); datIdx += datLen;
				let datStr = td.decode( dat );
				handleWebSockDataType(datType, dat);

			}
			
		}

	}else{
		if( socketInstance.readyState == socketInstance.OPEN ){
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

let pktIdx = 0;
function sendCmds( datas ) {

	let pktIdxStr = pktIdx.toString().padStart(3);
	let devIdStr = "0".padStart(4);
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
		else if( datLen == undefined ){
			datLen = dat.length;
		}

		let datTypeStr = datType.padEnd(11);
		let datLenStr = (datLen.toString()).padStart(6);
		
		sendStr += datTypeStr+datLenStr+dat;
	}

	sendWebsocketServerMessage(sendStr);
	pktIdx += 1;
	if( pktIdx > 255 )
		pktIdx = 0
}

function sendCmd(datType, dat, datLen=undefined){
	sendCmds([[datType, dat, datLen]]);
}



