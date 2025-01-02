#web cloud server for security cameras and robots
#the devices establish connection to the internet and to this server
#and then poll this server for commands (requests for images, movement commands, etc)
#the client connects to this server and 
#requests an interface page, images, and uses ajax to request the status of devices, commands, etc

#when a device is actively requested by a client, it increases its polling rate to 30/60 times per second
#vs when idle or timed out it polls / lets the server know it is operational every 1 or 10 seconds to reduce server load

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

class MyHandler(http.server.SimpleHTTPRequestHandler):
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
				print('get settings')
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
			output.write("<h2>Device Time: " + now.strftime("%Y-%m-%d %H:%M:%S") + "</h2>")
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
		else: #status or settings fixed length string
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




#https://stackoverflow.com/questions/50120102/python-http-server-keep-connection-alive
def start_backend_server(server_address,requestHandler):
	backend_server = http.server.ThreadingHTTPServer(server_address, requestHandler)
	context = get_ssl_context("cert.pem", "key.pem")
	backend_server.socket = context.wrap_socket(backend_server.socket, server_side=True)
	f = lambda : backend_server.serve_forever()
	backend_thread = threading.Thread(target=f)
	backend_thread.daemon=True
	backend_thread.start()
	return backend_thread


backend_thread = start_backend_server(server_address, MyHandler)

time.sleep(9e9)
#httpd = http.server.ThreadingHTTPServer(server_address, MyHandler)

#context = get_ssl_context("cert.pem", "key.pem")
#httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

#httpd.serve_forever()
