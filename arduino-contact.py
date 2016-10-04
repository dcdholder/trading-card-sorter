import serial
import time

portName = "/dev/ttyACM3"
port = serial.Serial(portName,9600,timeout=1)
port.flushInput()

gotLine = False

while gotLine==False:
	line = port.readline()[:-2] #apparently the Arduino uses Windows-style newlines
	
	if line=="Arduino ready":
		port.write("PC ready")
		print "Arduino connected"
		gotLine = True
		
while True:
	line = port.readline()[:-2]
	
	if line!="":
		print line
