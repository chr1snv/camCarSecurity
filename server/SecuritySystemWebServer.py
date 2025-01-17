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

import http.server
import threading
import ssl
import sys
import time
import datetime
import io
import re
import os
import numpy as np
import cv2
from requests_toolbelt.multipart import decoder

from netifaces import interfaces, ifaddresses, AF_INET

#import json

def get_ssl_context(certfile, keyfile):
	context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
	context.load_cert_chain(certfile, keyfile)
	context.set_ciphers("@SECLEVEL=1:ALL")
	return context


#last status
class Device:

	def __init__(self):

		self.lastImage = b''
		self.lastImageLength = 0
		self.lastImageTime = datetime.datetime.now()
		
		self.postSettings = ""
		self.lastSettingsTime = datetime.datetime.now()
		
		self.postStatus = ""
		self.lastStatusTime = datetime.datetime.now()
		
		self.alarmArmed        = ""
		self.magAX             = ""
		self.magAY             = ""
		self.magAZ             = ""
		
		self.servo1Angle       = ""
		self.servo2Angle       = ""
		
		self.staRssi           = ""
		self.lastTemperature   = ""
		self.magX              = ""
		self.magY              = ""
		self.magZ              = ""
		
		self.magHeading        = ""
		self.magAlarmDiff      = ""
		self.magAlarmTriggered = ""
		self.alarmOutput       = ""

	def fillValues(self, postStr):
		self.postStatus = postStr
		self.lastStatusTime = datetime.datetime.now()
		
		self.alarmArmed        = postStr[-80:-75]
		self.magAX             = postStr[-75:-70]
		self.magAY             = postStr[-70:-65]
		self.magAZ             = postStr[-65:-60]
		
		self.servo1Angle       = postStr[-60:-55]
		self.servo2Angle       = postStr[-55:-50]
		
		self.staRssi           = postStr[-50:-45]
		self.lastTemperature   = postStr[-45:-40]
		self.magX              = postStr[-40:-35]
		self.magY              = postStr[-35:-30]
		self.magZ              = postStr[-30:-25]
		
		self.magHeading        = postStr[-25:-15]
		self.magAlarmDiff      = postStr[-15:-10]
		self.magAlarmTriggered = postStr[-10:-5]
		self.alarmOutput       = postStr[-5:]
		
		#print( "alarmArmed " + self.alarmArmed )
		#print( "magAX " + self.magAX )
		#print( "magAY " + self.magAY )
		#print( "magAZ " + self.magAZ )
		
		#print( "servo1Angle " + self.servo1Angle )
		#print( "servo2Angle " + self.servo2Angle )
		
		#print( "staRssi " + self.staRssi )
		#print( "lastTemperature " + self.lastTemperature )
		#print( "magX " + self.magX )
		#print( "magY " + self.magY )
		#print( "magZ " + self.magZ )
		
		#print( "magHeading " + self.magHeading )
		#print( "magAlarmDiff " + self.magAlarmDiff )
		#print( "magAlarmTriggered " + self.magAlarmTriggered )
		#print( "alarmOutput " + self.alarmOutput )

device = Device()

#pending commands
cmds = []

def setKeepAlive(rqh):
	rqh.send_header("Connection", "keep-alive")
	rqh.send_header("keep-alive", "timeout=5, max=30")
	
#status		: 0,
#settings	: 1,
#image 		: 2
async def websocketHandler(websocket):
	async for message in websocket:
		#print("rcvd msg: %s:%i" % (message, len(message)))
		if message.startswith('1'):
			cmd = message[1:17]
			val = message[17:]
			print( 'action: ' + cmd + ':' + val + "|")
			cmds.append( [cmd, val] )
		elif message == 'status':
			#print( device.postStatus )
			await websocket.send( '0' + device.postStatus + str(device.lastStatusTime) )
		elif message == 'settings':
			await websocket.send( '1' + device.postSettings + str(device.lastSettingsTime) )
		elif message == 'image':
			#print(device.lastImage)
			await websocket.send( device.lastImage )


class HTTPAsyncHandler(http.server.SimpleHTTPRequestHandler):
	def __init__(self, request, client_address, server):
		#enable http 1.1 to avoid tls and tcp setup time per request by 
		self.protocol_version = 'HTTP/1.1' #keeping connections open until calling self.finish()
		try:
			super().__init__(request, client_address, server)
		except Exception as e:
			None

	def do_GET(self):
		global cmds
		
		try:
			#print("get path " + self.path )
			parts = re.split(r"[/?&=]", self.path)
			if parts[1] == 'action':
				self.send_response(200)
				self.send_header('Content-type','text/html')
				self.end_headers()
				cmd = parts[3]
				print( 'action: ' + cmd)
				val = parts[5]
				cmds.append( [cmd, val] )
				return
			if parts[1] == 'status':
				#print('return last device status info')
				self.send_response(200)
				self.send_header('Content-type','text/html')
				statusStr = device.postStatus.encode('utf-8')
				self.send_header('Content-Length', len(statusStr))
				self.end_headers()
				self.wfile.write(statusStr)
				return
			if parts[1] == 'settings':
				#print('get settings')
				self.send_response(200)
				self.send_header('Content-type','text/html')
				settingsStr = device.postSettings.encode('utf-8')
				self.send_header('Content-Length', len(settingsStr))
				self.end_headers()
				return
			if parts[1] == 'image':
				self.send_response(200)
				self.send_header('Content-type','image/jpeg')
				self.send_header('Content-Length', device.lastImageLength)
				self.end_headers()
				self.wfile.write(device.lastImage)
				return
			
			if self.path.endswith(".html"): #device control page
				#self.path has /index.htm
				f = open(os.getcwd() + os.path.sep + self.path)
				self.send_response(200)
				self.send_header('Content-type','text/html')
				self.end_headers()
				self.wfile.write("<table>".encode('utf-8'))
				self.wfile.write("<td>".encode('utf-8'))
				
				self.wfile.write("</td>".encode('utf-8'))
				self.wfile.write("</table>".encode('utf-8'))
				self.wfile.write(f.read().encode('utf-8'))
				f.close()
				self.finish() #https://stackoverflow.com/questions/6594418/simplehttprequesthandler-close-connection-before-returning-from-do-post-method
				return

			# else the index / device selection page
			self.send_response(200)
			self.send_header('Content-type','text/html')
			self.end_headers()

			now = datetime.datetime.now()

			output = io.StringIO()

			output.write("<html><head>")
			output.write("<style type=\"text/css\">")
			output.write("h1 {color:blue;}")
			output.write("h2 {color:red;}")
			output.write("</style>")
			output.write("<h2>System Time: " + now.strftime("%Y-%m-%d %H:%M:%S") + "</h2>")
			output.write("<h1>Avaliable devices</h2>")
			output.write('<a href="camControl.html">Esp32 Pan tilt camera</a>')
			output.write("</body>")
			output.write("</html>")

			self.wfile.write(output.getvalue().encode('utf-8'))
			self.finish()

			return

		except Exception as e:#@IOError:
			print(e)
			#self.send_error(404,'File Not Found: %s' % self.path)

	def do_POST(self): #accept data from device
		global device, cmds
		content_length = int(self.headers["Content-Length"])
		post_data = self.rfile.read(content_length)
		#print( self.headers["Content-Type"] + " " + str(content_length) )
		if self.headers["Content-Type"] == "image/jpeg":
			#print( post_data )
			#Read image using cv2
			#image_numpy = np.frombuffer(post_data, np.int8)
			#img = cv2.imdecode(image_numpy, cv2.IMREAD_COLOR )
			#cv2.imshow("esp32img", img)
			device.lastImage = post_data
			device.lastImageLength = content_length
			device.lastImageTime = datetime.datetime.now()
			#demonstation that the images are being recieved to the server uncorrupted
			#with open(str(device.lastImageTime)[-5:]+".jpeg", "wb") as file:
			#	file.write(post_data)
			#	file.close()
			self.send_response(200)
			self.send_header('Content-type', 'text/html')
			self.send_header('Content-Length', 0)
			self.end_headers()
			#self.finish()
		elif self.headers["Content-Type"] == "text/settings":
			postStr = post_data.decode("utf-8")
			device.lastSettings =  postStr
			device.lastSettingsTime = datetime.datetime.now()
		elif self.headers["Content-Type"] == "text/status": #status or settings fixed length string
			postStr = post_data.decode("utf-8")
			#print("post_data " + postStr )

			device.fillValues( postStr ) #read the status data in from device

			#respond with queued commands
			self.send_response(200)
			self.send_header('Content-type', 'text/html')
			
			output = io.StringIO()
			output.write( str(len(cmds)) )
			for cmd in cmds:
				output.write( '{:<32}'.format((cmd[0])[:32]) )
				output.write( '{:<32}'.format((cmd[1])[:32]) )
			outBytes = output.getvalue().encode('utf-8')
			self.send_header('Content-Length', len(outBytes))
			self.end_headers()
			self.wfile.write( outBytes )
			#print( output.getvalue() )
			cmds = [] #should add wait for device to confirm recept of commands, though doing this now for simplicity
			#self.finish()
		
	def log_message(self, format, *args):
		return


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
	context = get_ssl_context("cert.pem", "key.pem")
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
	ssl_context = get_ssl_context("cert.pem", "key.pem")
	
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
