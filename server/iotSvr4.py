#web cloud server for security cameras and robots
#the devices establish connection to the internet and to this server
#and then poll this server for commands (requests for images, movement commands, etc)
#the client connects to this server and 
#requests an interface page, images, and uses ajax to request the status of devices, commands, etc

#when a device is actively requested by a client, it increases its polling rate to 30/60 times per second
#vs when idle or timed out it polls / lets the server know it is operational every 1 or 10 seconds to reduce server load

import asyncio
from websockets.server import serve
import pathlib


#from Crypto import Random
#from Crypto.Cipher import AES
#import base64


from random import random

import http.server
import threading
import ssl
import sys
import time
from datetime import datetime, timezone
import io
import re
import os
import numpy as np
#import cv2
#from requests_toolbelt.multipart import decoder

from netifaces import interfaces, ifaddresses, AF_INET

import base64

#import json

certfile = "cert.pem"
keyfile = "key.pem"

def get_ssl_context(certfile, keyfile):
	context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
	context.load_cert_chain(certfile, keyfile)
	context.set_ciphers("@SECLEVEL=1:ALL")
	return context

strSendError = ""

async def sendPkt(wSocket, pktNum, fromDevId, datInfoArr ):
	global strSendError
	if( pktNum >= 256 ):
		pktNum = 0
	sendHdr = lPadStr( 3, str(pktNum) ) + lPadStr(4, str(fromDevId) )
	sendBytes = b''
	for dInf in datInfoArr: #datInfoArr datType, datLen, dat
		#print(dInf)
		datType = dInf[0]
		datLen = dInf[1]
		dat = dInf[2]
		sendBytes += rPadStr(11, datType) + lPadStr(6, str(datLen)) + dat
	try:
		await wSocket.send( sendHdr + str(len(datInfoArr)).encode('utf-8') + 's'.encode('utf-8') + sendBytes )
		pktNum += 1
	except Exception as e:
		strSendError = str(e)
		#None#print("sendPkt error: %s" % str(e) )
	return pktNum

def curMillis():
	return int(datetime.now(tz=timezone.utc).timestamp() * 1000)

#last status
class Device:

	def __init__(self):
	
		self.cmds = [] #pending commands to send to the device

		self.devId = -1

		self.controlingCliId = -1

		self.description = 'Device Name'

		self.wSock = None

		self.sendPktIdx = 0

		self.lastImage = b''
		self.lastImageLength = 0
		self.lastImageTime = int(datetime.now(tz=timezone.utc).timestamp() * 1000)

		self.postSettings = b''
		self.lastSettingsTime = int(datetime.now(tz=timezone.utc).timestamp() * 1000)
		
		self.postStatus = b''
		self.lastStatusTime = int(datetime.now(tz=timezone.utc).timestamp() * 1000)

		self.alarmArmed        = ""
		self.magAX             = ""
		self.magAY             = ""
		self.magAZ             = ""

		self.numServos         = 0
		self.servoAngles       = []

		self.staRssi           = ""
		self.lastTemperature   = ""
		self.magX              = ""
		self.magY              = ""
		self.magZ              = ""

		self.magHeading        = ""
		self.magAlarmDiff      = ""
		self.magAlarmTriggered = ""
		self.alarmOutput       = ""

	async def send( self, fromDevId, datInfoArr ):
		if self.wSock != None:
			#print("send to device " )
			#print(datInfoArr)
			prevPktIdx = self.sendPktIdx
			print( self.sendPktIdx )
			self.sendPktIdx = await sendPkt(self.wSock, self.sendPktIdx, fromDevId, datInfoArr )
			print( self.sendPktIdx ) 
			if self.sendPktIdx != prevPktIdx:
				return True
		return False

	def fillValues(self, statStr):
		self.postStatus = statStr
		self.lastStatusTime = int(datetime.now(tz=timezone.utc).timestamp() * 1000)
		cmdValArr = []
		numSrvos = int( statStr[66:68] )
		#print("stat numSrvos %i" % numSrvos)
		idx = 68
		for i in range(numSrvos):
			cmdValArr.append( [ "angAxis"+str(i), statStr[idx:idx+3] ] )
			idx += 3
			#print( "a%i %i" % (i, int(cmdValArr[-1][1])) )
		numCmdsCleared = clearCompletedCommands(self.cmds, cmdValArr)
		print( "cmdsCleared %i" % numCmdsCleared )

	def fillSettings(self, datStr, datStrLen ):
		self.lastSettings = datStr
		self.lastSettingsLen = datStrLen
		self.lastSettingsTime = int(datetime.now(tz=timezone.utc).timestamp() * 1000)
		self.description = datStr[37:69]

	def fillImage(self, datStr, datStrLen):
		#print( "last image len %i datStrLen %i " % (len(datStr), datStrLen )  )
		self.lastImage = datStr
		self.lastImageLength = datStrLen
		self.lastImageTime = int(datetime.now(tz=timezone.utc).timestamp() * 1000)

class Client:
	def __init__(self):
		self.cliId = -1 #the id of the users browser session
		self.devId = -1 #the selected device to control
		self.sendPktIdx = 0
		self.wSock = None
		self.login = None
		self.addr = None
		self.loginAttempts = 0
		self.nextAllowedLoginAttemptTime = curMillis()

	def getAndIncrementNextLoginAttemptTime(self, time):
		ret = self.nextAllowedLoginAttemptTime
		if time > self.nextAllowedLoginAttemptTime:
			self.nextAllowedLoginAttemptTime = self.nextAllowedLoginAttemptTime + pow(10,self.loginAttempts)
			self.loginAttempts += 1
		return ret

	def resetLoginTimeout(self):
		self.loginAttempts = 0
		self.nextAllowedLoginAttemptTime = curMillis() + 1000

	async def send( self, fromDevId, datInfoArr, sendWithoutAuth=False): #, auth ):
		okToSend = False
		if self.wSock != None:
			if sendWithoutAuth:
				okToSend = True
			elif self.login != None: #should it be checked that self.login[LOGIN_AUTHKEY_IDX] == auth: ?
				if self.login[LOGIN_REMAINING_RESPONSES_IDX] > 0:
					self.login[LOGIN_REMAINING_RESPONSES_IDX] -= 1
					okToSend = True
				else:
					await logoutClient(self)
					self.login = None #not sure if necessary because this client instance would then be garbage collected
			else:
				print("no login for cli not okToSend")
		else:
			print( "cant send to cliId %i without wSock" % self.cliId )

		if okToSend:
			if not sendWithoutAuth:
				remPkts = str(self.login[LOGIN_REMAINING_RESPONSES_IDX]).encode('utf-8')
				datInfoArr.append( ('remPkts', len(remPkts), remPkts) )
			self.sendPktIdx = await sendPkt(self.wSock, self.sendPktIdx, fromDevId, datInfoArr )
			if self.login != None:
				self.login[LOGIN_REMAINING_RESPONSES_IDX] -= 1

svrDevId = 1
devices = {}
clients = {}
lastAllocatedClientId = -1

def cleanupNotRecentlyConnectedDevicesAndClients():
	tenSecAgo = int(datetime.now(tz=timezone.utc).timestamp() * 1000) - (10 * 1000)
	for dev in list(devices.keys()):
		if devices[dev].lastStatusTime  < tenSecAgo:
			print( "cleaning up device" )
			del devices[dev]
	for cli in list(clients.keys()):
		if clients[cli].lastStatusTime < tenSecAgo:
			print( "cleaning up client" )
			del clients[cli]

def GetOrAllocateDevice( devId ):
	if not ( devId in devices.keys() ):
		dev = Device()
		dev.devId = devId
		devices[devId] = dev
	return devices[devId]

def GetOrAllocateClient( cliId ):
	if not ( cliId in clients.keys() ):
		cli = Client()
		cli.cliId = cliId
		clients[cliId] = cli
	return clients[cliId]

validClientLogins = {}
#username(0), password(1), loginAttempts(2), loggedinAuthKey(3), remaining authorized responses(4), client instance
activeClientLogins = {}
activeGetKeys = {}

NUM_PKTS_A_LOGIN_AUTHORIZES = 10000

LOGIN_USERNAME_IDX				= 0
LOGIN_PASSWORD_IDX				= 1
LOGIN_ATTEMPTS_IDX				= 2
LOGIN_AUTHKEY_IDX				= 3
LOGIN_REMAINING_RESPONSES_IDX	= 4

def GetKeyIsActive(getKey):
	if getKey in activeGetKeys:
		del activeGetKeys[getKey]
		return True
	else:
		return False


async def logoutClient(client):
	login = client.login
	login[LOGIN_REMAINING_RESPONSES_IDX] = 0
	client.sendPktIdx = await sendPkt(client.wSock, client.sendPktIdx, svrDevId, [('logout', 0, b'')] )
	del activeClientLogins[login[LOGIN_AUTHKEY_IDX]]



def GetCommandListBytes(cmds):
	#output = io.BytesIO()
	numCmdsToSend = min(9, len(cmds))
	#print( "numCmds in getList %i" % numCmdsToSend )
	#output.write( str(numCmdsToSend).encode('utf-8') ) #number of commands
	#output.write( b's' ) #commands are from server
	retArr = []
	for cmdIdx in range(0,numCmdsToSend):
		cmd = cmds[cmdIdx]
		cmdPart = (cmd[0])[:11]
		#output.write( cmdPart + bytes(11-len(cmdPart)) ) #command
		#output.write( bytes(4-1) + b'0') #device id
		#lenDat = str(len(cmd[1])).encode('utf-8')
		#output.write( bytes(6-len(lenDat))+lenDat)
		dat = (cmd[1])
		#output.write( dat )
		retArr.append( (cmdPart, len(dat), dat) )
	#in python "assigning something to elements of that list, will change the original list ( reason for [:] (selection of all elements of the list))"
	#cmds[:] = cmds[numCmdsToSend:] #should add wait for device to confirm recept of commands, though clearing it here now for simplicity
	#outBytes = output.getvalue()#.encode('utf-8')
	#print( 'sending Cmds %s' % outBytes )
	return retArr#outBytes

def putCmdList(cmds, cmdDatArr):
	#add a command to list of not yet executed commands
	#allow only 1 (most recent instance of command) 
	#i.e. if angle commanded, only keep most recent one
	print("putting cmds")
	notPutNewCmdIdxs = []
	for newCmdIdx in range(len(cmdDatArr)):
		newCmd = cmdDatArr[newCmdIdx]
		cmdPut = False
		for i in range(len(cmds)):
			queuedCmd = cmds[i]
			if queuedCmd[0] == newCmd[0]:
				cmds[i] = newCmd
				cmdPut = True
		if not cmdPut:
			notPutNewCmdIdxs.append( newCmdIdx )
	for i in range(len(notPutNewCmdIdxs)):
		newCmd = cmdDatArr[notPutNewCmdIdxs[i]]
		print("appendingCmd %s" % newCmd[0])
		if len(cmds) < 1:
			cmds[:] = [ newCmd ]
		else:
			cmds.append( newCmd )
	print(len(cmds))

def clearCompletedCommands(cmds, cmdValArr):
	#check most recent recieved statuses from device (in corresponding command format)
	#to find if commands have been applied or if they need to be sent again
	numCmdsCleared = 0
	
	#clear non servo position commands (could check command types later)
	numCmds = len(cmds)
	i = 0
	while i < numCmds:
		cmd = cmds[i]
		if cmd[0][:7] != b'angAxis':
			print("clearing %s" % str(cmd[0][:7]) )
			del cmds[i]
			numCmds -= 1
			numCmdsCleared += 1
		i += 1
	
	#clear servo commanded positions if met
	for newValIdx in range(len(cmdValArr)):
		newVal = cmdValArr[newValIdx]
		for cmdIdx in range(len(cmds)):
			cmd = cmds[cmdIdx]
			newCmdType = bytes(newVal[0], 'utf8')
			#print( "cmp %s %s %i %i" % (newCmdType, cmd[0], int(newVal[1]), int(cmd[1])) )
			if newCmdType == cmd[0] and int(newVal[1]) == int(cmd[1]):
				#print( "clear match %s %s" % ( str(cmd), str(newVal) ) )
				del cmds[cmdIdx]
				numCmdsCleared += 1
				break
	
	#return a count of how many cleared (to know if things changed)
	return numCmdsCleared

def setKeepAlive(rqh):
	rqh.send_header("Connection", "keep-alive")
	rqh.send_header("keep-alive", "timeout=5, max=30")


#data is sent over the websocket in the format
	#|numData(1) | dataTypeStr (12) | deviceId(4) | dataLen(6) | data
#commands are recieved in the format
	# | num commands(1)    ||| cmd name(12) | cmd length(4) | cmd value(cmd length) |||
	#||| - |||| repeats num commands times up to CMD_BUFF_MAX_LEN

#everything except the login.html and commonFunctions.js require a vaild authorization to access
#i.e. num commands 2 with the first command being auth and a non timed out key
#auth keys are used to identify connections, if a hashed auth key is copied,
#the copied one will be one behind and disallowed access

#https://stackoverflow.com/questions/30990129/encrypt-in-python-decrypt-in-javascript


def getRandomASCIIByteArrWithLength( leng ):
	buf = bytearray()
	numRange   = b'9'[0] - b'0'[0]
	upperRange = b'Z'[0] - b'A'[0]
	lowerRange = b'z'[0] - b'a'[0]
	ovrAllRange = numRange + upperRange + lowerRange
	for i in range( 0, leng ):
		c = round( random() * ovrAllRange )
		if c <= numRange:
			buf.append( c + b'0'[0] )
		elif ( c <= numRange + upperRange ):
			buf.append( (c - numRange) + b'A'[0] )
		else:
			buf.append( (c - (numRange + upperRange) ) + b'a'[0] )
	return buf #b.decode('utf-8')



#ascii to int reverse iteration for n characters
#input is end of number (1's place)
#counts up in significance (x10), decrementing string index from start index
def atoir_n( c, n ):
	accum = 0
	mult = 1
	#print( "atoir_n d " )
	for i in range(n) :
		d = c[n-1-i]
		if( d >= ord('0') and d <= ord('9') ):
			accum += (d - b'0'[0])*mult
		else:
			break
		mult *= 10
		#print( " %c acum %i d " % ( d, accum ) )
	#print(" accum %i " % (accum) )
	return accum

#print( "atoir_n( \" 12\", 3 ) %i\n" % atoir_n( " 12", 3 ) )

def lPadStr(n, chars):
	bStr = str(chars).encode('utf-8') #left pad, another option may be str.rjust(10, '0')
	return bytes(n-len(bStr)) + bStr
	
def rPadStr(n, chars):
	if type(chars) == type(b''):
		bStr = chars
	else:
		bStr = str(chars).encode('utf-8') #left pad, another option may be str.rjust(10, '0')
	#print("bStr %s len %i" % (bStr, len(bStr)) )
	return bStr + bytes(n-len(bStr))

#https://www.optimizationcore.com/coding/websocket-python-parsing-binary-frames-from-a-tcp-socket/
async def websocketHandler(websocket):
	global strSendError
	async for msg in websocket:#for _ in range(3):
		#msg = await websocket.read_message()#frame(4096)
		#print(msg[:50])
		#async for msg in websocket:
		rcvTime = curMillis()
		#print(dir(websocket))
		#msgOpcode = 
		msgLen = len(msg)
		if msgLen < 1:
			return
		if( type(msg) != type(b'') ):
			msg = msg.encode('utf-8')
		mIdx = 0
		pktIdx = atoir_n(msg[0:3],3)
		mIdx += 3
		devOrCliId  = atoir_n(msg[mIdx : mIdx+4  ], 4)
		mIdx += 4
		numCmd = msg[mIdx] - b'0'[0] #ascii character difference to convert digit to int
		mIdx += 1
		cmdIdx = 0
		fromDorC = msg[mIdx]
		#print( "pktIdx %i devOrCliId %s numCmd %i fromType %c" % (pktIdx, devOrCliId, numCmd, fromDorC) )
		mIdx += 1
		
		deviceWithNewCmds = None
		
		pktLoginUname = ''
		pktAuth = ''
		while cmdIdx < numCmd:
			#print(msg[mIdx : mIdx+11+20])
			datType = msg[mIdx:mIdx+11]
			mIdx += 11
			datLen = atoir_n(msg[mIdx : mIdx+6], 6)
			mIdx += 6
			datStr = msg[mIdx:mIdx+datLen]
			if fromDorC == ord('d'): #data from device
				device = GetOrAllocateDevice(devOrCliId)
				client = None
				if device.controlingCliId >= 0 and device.controlingCliId in clients.keys():
					client = clients[device.controlingCliId]
				cliIdNum = -2
				if client:
					cliIdNum = client.cliId
				print( "from %s devId: %i datType: %s datLen: %i controllingCliId %i client %i" % (chr(fromDorC), devOrCliId, datType, datLen, device.controlingCliId, cliIdNum) )
				device.wSock = websocket #for sending data to device
				if datType.startswith(b"Stat"):
					device.fillValues( datStr ) #read the status data in from device
					#respond with queued commands
					cmdDatArr = GetCommandListBytes(device.cmds)
					print("recvd Stat sending commands %s" % ( cmdDatArr ) )
					if not await device.send( svrDevId, cmdDatArr ):
						putCmdList( deviceWithNewCmds.cmds, cmdDatArr )
					lastStatTimeStr = str(device.lastStatusTime).encode('utf-8')
					if client:
						await client.send( device.devId, [('Stat', len(device.postStatus), device.postStatus), ('Time', len(lastStatTimeStr), lastStatTimeStr)] )
					else:
						print("no cli to forward stat to")
				if datType.startswith(b"Set"):
					device.fillSettings( datStr, datLen )
					lastSetTimeStr = str(device.lastSettingsTime).encode('utf-8')
					if client:
						await client.send( device.devId, [('Set', device.lastSettingsLen, device.lastSettings), ('Time', len(lastSetTimeStr), lastSetTimeStr)] )
					else:
						print("no cli to forward set to")
				if datType.startswith(b"Img"):
					device.fillImage( datStr, datLen )
					lastSetTimeStr = str(device.lastImageTime).encode('utf-8')
					#print('sending image to browser')
					if client:
						await client.send( device.devId, [('Img', device.lastImageLength, device.lastImage), ('Time', len(lastSetTimeStr), lastSetTimeStr)] )
					else:
						print("no cli to forward image to")
				if datType.startswith(b"cmdResults"):
					if client:
						await client.send( device.devId, [ ('cmdResults', datLen, datStr) ] )
			else: #request or command from client (browser http page)
				client = GetOrAllocateClient( devOrCliId )
				client.wSock = websocket #for sending data to browser client
				client.addr = websocket.remote_address
				if datType.startswith(b'auth'):
					pktAuth = datStr
					if not pktAuth in activeClientLogins:
						return
				elif datType.startswith(b'loginUname'):
						pendingLoginUname = datStr
						print( 'loginUname %s' % pendingLoginUname )
				elif datType.startswith(b'loginPass'):
					loginPass = datStr
					print( 'loginPass: pendingLoginUname %s loginPass %s' % (pendingLoginUname, loginPass) )
					try:
						nextAllowedAttemptTime = client.getAndIncrementNextLoginAttemptTime(rcvTime)
						if nextAllowedAttemptTime  > rcvTime:
							raise Exception("rate limit wait %i secs" % ((nextAllowedAttemptTime - rcvTime )/1000) )
						if not pendingLoginUname in validClientLogins.keys():
							raise Exception("username not found")
						storedLogin = validClientLogins[pendingLoginUname]
						print('UserName found')
						if storedLogin[0] == loginPass:
							print('loginPassMatches')
							client.resetLoginTimeout()
							authKey = getRandomASCIIByteArrWithLength(16).decode('utf-8').encode('utf-8')
							LOGIN_USERNAME_IDX = 0
							LOGIN_PASSWORD_IDX = 1
							LOGIN_ATTEMPTS_IDX = 2
							LOGIN_AUTHKEY_IDX  = 3
							LOGIN_REMAINING_RESPONSES_IDX = 4
							storedLogin[LOGIN_ATTEMPTS_IDX] = 0
							storedLogin[LOGIN_AUTHKEY_IDX] = authKey
							storedLogin[LOGIN_REMAINING_RESPONSES_IDX] = NUM_PKTS_A_LOGIN_AUTHORIZES
							#validClientLogins[pendingLoginUname] = storedLogin
							client.login = storedLogin
							print("setting authKey %s as active" % authKey)
							activeClientLogins[authKey] = storedLogin
							await client.send( svrDevId, [('auth', len(authKey), authKey)] )
							print('sent auth to client and set login for cliId %i' % client.cliId)
						else:
							print("password doesn't match")
							await client.send( svrDevId, [('authErr', 0, b'')], True )
					except Exception as e:
						print(' %s' % e)
						await client.send( svrDevId, [('authErr', 0, b'')], True )
				elif pktAuth != '': #a valid pktAuth has been recieved for the data packet
					#the following requests are allowed
					if client.login == None:
						client.login = activeClientLogins[pktAuth]
					if datType.startswith(b'getKey'): #generate a get key make it active and return it
						getKey = getRandomASCIIByteArrWithLength(16).decode('utf-8').encode('utf-8');
						activeGetKeys[getKey] = client
						await client.send(svrDevId, [('getKey', len(getKey), getKey)])
					if datType.startswith(b'logout'):
						key = datStr
						if key == client.login[LOGIN_AUTHKEY_IDX]: #only allow user to logout themselves
							await logoutClient(client)
					device = None
					if client.devId >= 0 and client.devId in devices.keys():
						device = devices[client.devId]
						if datType.startswith(b'status'):
							#print( 'sending status to browser' + str(len(device.postStatus)) )
							timeStr = str(device.lastStatusTime).encode('utf-8')
							await client.send( device.devId, [('Stat', len(device.postStatus), device.postStatus), ('Time', len(timeStr), timeStr)] )
						elif datType.startswith(b'settings'):
							#setLen = str(len(device.postSettings)).encode('utf-8')
							print( 'req settings from dev' )
							await device.send( svrDevId, [('getSettings', 0, b'')] )
							print( 'sending last settings to browser' )
							lastSetTimeStr = str(device.lastSettingsTime).encode('utf-8')
							await client.send( device.devId, [('Set', len(device.postSettings), device.postSettings), ('Time', len(lastSetTimeStr), lastSetTimeStr)] )
						elif datType.startswith(b'image'):
							#print( 'sending img to browser' )
							await client.send( device.devId, [('Img', len(device.lastImage), device.lastImage)]  )
						else:
							cmd = datType.strip()
							val = datStr
							print( 'action: ' + str(cmd) + ':' + str(val) + "|" )
							putCmdList( device.cmds, [ [cmd, val] ] )
							deviceWithNewCmds = device
					else:
						print( "dev %i not in devices" % client.devId )
			mIdx += datLen
			cmdIdx += 1
			#print( " mIdx %i" % mIdx )
		if deviceWithNewCmds: #send commands immediately to device instead of waiting for device to poll for them
			cmdDatArr = GetCommandListBytes(deviceWithNewCmds.cmds)
			print("sending immediately to %s commands %s" % ( str(deviceWithNewCmds.devId), str(cmdDatArr) ) )
			if not await deviceWithNewCmds.send( svrDevId, cmdDatArr ): #put back the unsent commands
				putCmdList(deviceWithNewCmds.cmds, cmdDatArr)
		if len(strSendError) > 0:
			print(strSendError)
			strSendError = ""



class HTTPAsyncHandler(http.server.SimpleHTTPRequestHandler):
	def __init__(self, request, client_address, server):
		#enable http 1.1 to avoid tls and tcp setup time per request by 
		self.protocol_version = 'HTTP/1.1' #keeping connections open until calling self.finish()
		try:
			super().__init__(request, client_address, server)
		except Exception as e:
			None

	def replyWithStartFile(self, filePath):
		filePathStr = os.getcwd() + os.path.sep + filePath
		self.send_response(200)
		print( filePath )
		if filePath.endswith('.jpg'):
			f = open(filePathStr, 'rb')
			print("sending jpg")
			self.send_header('Content-type','image/jpeg')
			self.end_headers()
			#print("reading jpg")
			jpgFile = f.read()
			#print("closing jpg file")
			f.close()
			self.wfile.write(jpgFile)
			f.close()
			return
		
		f = open(filePathStr)
		if filePath.endswith('.js'):
			print("sending js")
			self.send_header('Content-type','application/javascript')
		else:
			print("sending text")
			self.send_header('Content-type','text/html')
		self.end_headers()
		self.wfile.write(f.read().encode('utf-8'))
		f.close()

	def replyWithEndFile(self, filePath):
		f = open(os.getcwd() + os.path.sep + filePath)
		self.wfile.write(f.read().encode('utf-8'))
		f.close()
		self.finish() #https://stackoverflow.com/questions/6594418/simplehttprequesthandler-close-connection-before-returning-from-do-post-method

	def do_GET(self):
		global cmds, lastAllocatedClientId

		try:
			#print("get path " + self.path )
			parts = re.split(r"[/?&=]", self.path)
			print('parts %s ' % str(parts) )
			getKeyValid = False
			if len(parts) > 2:
				getKeyValid = GetKeyIsActive( parts[2].encode('utf-8') )

			if getKeyValid: #then allowed to request the following
				"""
				if parts[1] == 'action':
					self.send_response(200)
					self.send_header('Content-type','text/html')
					self.end_headers()
					cmd = parts[3]
					print( 'action: ' + str(cmd))
					val = parts[5]
					cmds.append( [cmd, val] )
					self.finish()
					return
				elif parts[1] == 'status':
					#print('return last device status info')
					self.send_response(200)
					self.send_header('Content-type','text/html')
					statusStr = device.postStatus.encode('utf-8')
					self.send_header('Content-Length', len(statusStr))
					self.end_headers()
					self.wfile.write(statusStr)
					self.finish()
					return
				elif parts[1] == 'settings':
					#print('get settings')
					self.send_response(200)
					self.send_header('Content-type','text/html')
					settingsStr = device.postSettings.encode('utf-8')
					self.send_header('Content-Length', len(settingsStr))
					self.end_headers()
					self.finish()
					return
				elif parts[1] == 'image':
					self.send_response(200)
					self.send_header('Content-type','image/jpeg')
					self.send_header('Content-Length', device.lastImageLength)
					self.end_headers()
					self.wfile.write(device.lastImage)
					self.finish()
					return
				"""
				if parts[1] == "camControl.html": #device control page
					#self.path has /index.htm
					devId = int(parts[4])
					cliId = int(parts[6])
					client = GetOrAllocateClient( cliId )
					client.devId = devId
					dev = GetOrAllocateDevice( devId )
					dev.controlingCliId = cliId
					self.replyWithStartFile( "camControlBegin.html" )
					print("writing cliId %i" % (cliId))
					self.wfile.write(("<div id=\"cliId\" style=\"display:none;\">" + str(cliId) + "</div>").encode('utf-8'))
					print("writing devId %i" % (devId))
					self.wfile.write(("<h2 id=\"devId\">" + str(devId) + "</h2>").encode('utf-8'))
					print("finishing writing camControl.html")
					self.replyWithEndFile( "camControlEnd.html" )
					print( " camControl devId : %i cliId : %i  cli.devId : %i" % (clients[cliId].devId, clients[cliId].cliId, clients[cliId].devId) )
					return

				elif parts[1] == "devSelection.html": #device control page
					# else the index / device selection / overview page
					self.send_response(200)
					self.send_header('Content-type','text/html')
					self.end_headers()

					now = datetime.now()

					output = io.StringIO()
					output.write("<html><head>")
					output.write("<style type=\"text/css\">")
					output.write("h1 {color:blue;}")
					output.write("h2 {color:orange;}")
					output.write("</style>")
					output.write("<script src='commonFunctions.js'></script>")
					output.write("<h2>Page Generated Time: " + now.strftime("%Y-%m-%d %H:%M:%S") + "</h2>")
					output.write("<table><tr>")
					output.write("<td><button onclick=\"logout()\">Log out</button></td>")
					output.write("<td><h4 style=\"margin-bottom:0px;\">Packets until auto-logout</h4></td><td><p id=\"remainingPackets\">?</p></td>")
					output.write("</tr></table>")
					output.write("<h1>Avaliable devices</h1>")
					lastAllocatedClientId += 1
					cliIdStr = str( lastAllocatedClientId )
					output.write("<div id=\"cliId\" style=\"display:none;\">" + cliIdStr + "</div>")
					for devId, dev in devices.items():
						devIdStr = str(dev.devId)
						output.write('<button onclick="gotoUrlPlusAuthKeyAndArgs(\'camControl.html\', [[\'devId\', ' + devIdStr + '],[\'cliId\', ' + cliIdStr + ']])">' + devIdStr + " : " + str(dev.description) + '</button>')
					output.write("</body>")
					output.write("</html>")

					self.wfile.write(output.getvalue().encode('utf-8'))
					self.finish()

					return

			elif parts[1].endswith("commonFunctions.js") or ( parts[1].endswith(".js") and getKeyValid ):
				self.replyWithStartFile(self.path)
				self.finish()
				
			elif parts[1].endswith(".jpg"):
				self.replyWithStartFile(parts[1])
				self.finish()

			else: # the login page
				self.replyWithStartFile( "login.html" )
				self.finish()

		except Exception as e:#@IOError:
			print(e)
			#self.send_error(404,'File Not Found: %s' % self.path)



svrIp = '127.0.0.1'
def getIp():
	currentIp = '127.0.0.1'
	for ifaceName in interfaces():
		addresses = [i['addr'] for i in ifaddresses(ifaceName).setdefault(AF_INET, [{'addr':'No IP addr'}] )]
		if addresses[0] != '127.0.0.1' and addresses[0] != 'No IP addr':
			currentIp = addresses[0]
		#print ('%s: %s' % (ifaceName, ', '.join(addresses)))
	return currentIp

############
#concurrent / threaded http server for serving the html page
############

#https://stackoverflow.com/questions/50120102/python-http-server-keep-connection-alive
def start_http_server_in_new_thread(server_address,requestHandler):
	backend_server = http.server.ThreadingHTTPServer(server_address, requestHandler)
	context = get_ssl_context(certfile, keyfile)
	backend_server.socket = context.wrap_socket(backend_server.socket, server_side=True)
	f = lambda : backend_server.serve_forever()
	backend_thread = threading.Thread(target=f)
	backend_thread.daemon=True
	backend_thread.start()
	return backend_thread

backend_thread = None
webSocketSvrThread = None
stop = 0

########
###asyncio websocket server (concurrent event loops)
########
async def startWebsocketServer():
	global stop
	print("start websocket server init")
	port = 9999
	ssl_context = get_ssl_context(certfile, keyfile)

	stop = asyncio.Future()

	print( "serving websocket at %s port %i" % (svrIp, port) )
	#https://stackoverflow.com/questions/67810506/websockets-exceptions-connectionclosederror-code-1011-unexpected-error-no
	async with serve(websocketHandler, svrIp, port, ping_interval=None, ssl=ssl_context) as ws:
		print('ws before await stop')
		await stop
		ws.close()
		print('ws close called')


def startWebsocketServer_in_new_thread():
	f = lambda : asyncio.run(startWebsocketServer())#, loop)
	wsThread = threading.Thread(target=f)
	wsThread.daemon=True
	wsThread.start()
	return wsThread


svrIp = ''

####concurrent / thread for checking if interfaces / ip addresses have changed
def loopCheckIpHasChanged():
	global svrIp, backend_thread
	webSocketSvrThread = 0
	while(1):
		currentIp = getIp()
		#print(currentIp)
		if currentIp != svrIp:
			print('ip has changed, need to rebind servers to new interface addresses')
			svrIp = currentIp

			if backend_thread != None:
				backend_thread.stop()

			server_address = (svrIp, 5000)
			print( "starting httpAsyncServer at " + server_address[0] + " port " + str(server_address[1]) )
			backend_thread = start_http_server_in_new_thread(server_address, HTTPAsyncHandler)

			if stop != 0: #https://stackoverflow.com/questions/60113143/how-to-properly-use-asyncio-run-coroutine-threadsafe-function
				stop.get_loop().call_soon_threadsafe(stop.set_result, 1)
				#webSocketSvrThread.stop()#stop.set_result(1);
			loop = asyncio.new_event_loop()
			asyncio.set_event_loop(loop)
			loop = asyncio.get_event_loop()
			print('before run coroutine startWebsocketServer')

			webSocketSvrThread = startWebsocketServer_in_new_thread()

		time.sleep(1)

#run the ip change checking loop (main program loop)
f = lambda : loopCheckIpHasChanged()
ipCheck_thread = threading.Thread(target=f)
#ipCheck_thread.daemon=True
ipCheck_thread.start()
