import pygame.camera
import pygame.image
import wand.image
import pytesseract
import json
import PIL
import os
import sys
import serial
import serial.tools.list_ports
import time

class CardReader:
	CARD_NAMES_JSON_FILENAME = "Resources/AllCards.json"

	MAX_NUM_TRIES = 3

	TEXT_BOX_X_PERCENT = 0.8900
	TEXT_BOX_Y_PERCENT = 0.0476

	TEXT_BOX_WIDTH_PERCENT  = 0.0568
	TEXT_BOX_HEIGHT_PERCENT = 0.9048

	PERCENT_MATCH_THRESHOLD = 0.6

	RIGHT_CARDS = ["Mountain","Plains","Forest"]
	LEFT_CARDS  = ["Swamp","Island"]

	ARDUINO_VID = 2341

	def __init__(self):
		self.cam = self.initCamera()
		self.arduinoPort = self.getArduinoPort()
		self.arduinoHandshake()
		
		self.CARD_NAMES = self.getCardNamesFromJson()

	def initCamera(self):
		self.quitCamera()
		pygame.camera.init()
		cam = pygame.camera.Camera(pygame.camera.list_cameras()[0],(1920,1080))
		return cam

	def quitCamera( self ):
		pygame.camera.quit()

	def getArduinoPort(self):
		ports = list(serial.tools.list_ports.comports())

		portFound = False
		for portDesignation, description, address in ports:
			if str(self.ARDUINO_VID) in address:
				portName = portDesignation
				port = serial.Serial(portName,9600,timeout=1)
				port.flushInput()
				portFound = True
				break

		if portFound==False:
			raise ValueError("Could not find an Arduino on any port")
			
		return port

	def arduinoHandshake(self):
		print("Attempting to handshake Arduino")

		gotLine = False
		while gotLine==False:
			line = self.arduinoPort.readline()[:-2] #apparently the Arduino uses Windows-style newlines
	
			if line=="Arduino ready":
				self.arduinoPort.write("PC ready")
				print "Arduino connected"
				gotLine = True

	def takePhoto( self, filename ):
		self.cam.start()
		cardImage = self.cam.get_image()
		self.cam.stop()
		pygame.image.save(cardImage, filename)

	def cleanImage( self, inputFilename, outputFilename ):
		initialCropFilename = "sample-card-initial-crop.png"

		#crop to just part of the area with the platform, then deskew to collect the card image
		newHeight = int(1080*0.8);
		newWidth  = int(1920*(0.67-0.05));

		heightOffset = int(1080*0.1);
		widthOffset  = int(1080*0.05);

		cropCmd   = "convert +repage -crop " + str(newWidth) + "x" + str(newHeight) + "+" + str(widthOffset) + "+" + str(heightOffset) + " " + inputFilename + " " + initialCropFilename
		os.system(cropCmd)
	
		deskewCmd = "convert +repage -background white -fuzz 80% -deskew 40% -trim " + initialCropFilename + " " + outputFilename
		os.system(deskewCmd)

	#TODO: use jpgs instead so that wand doesn't complain (pngs have a weird resolution scheme)
	#TODO: rotate and crop according to initial orientation (look at image dimensions)
	def cropToTextbox( self, inputFilename, outputFilename ):
		img = wand.image.Image(filename=inputFilename)

		cropX = int(img.width  * self.TEXT_BOX_X_PERCENT)
		cropY = int(img.height * self.TEXT_BOX_Y_PERCENT)
		cropWidth  = int(img.width  * self.TEXT_BOX_WIDTH_PERCENT)
		cropHeight = int(img.height * self.TEXT_BOX_HEIGHT_PERCENT)
	
		#TODO: figure out how to replace the system call with a 'wand' call
		cropCmd = "convert +repage -crop " + str(cropWidth) + "x" + str(cropHeight) + "+" + str(cropX) + "+" + str(cropY) + " " + inputFilename + " " + outputFilename
		os.system(cropCmd)
	
		rotateCmd = "convert " + outputFilename + " -rotate -90 " + outputFilename
		os.system(rotateCmd)

	def ocrImage( self, inputFilename ):
		return pytesseract.image_to_string(PIL.Image.open(inputFilename))

	#TODO: heavily document this - it's complicated!
	#TODO: work on improving the run time
	def matchOcr( self, ocrText, cardNames ):
		largestMatchingCount = 0
		bestMatchingName = ""
	
		for ocrLine in ocrText.splitlines():
			if ocrLine in cardNames: #the elusive perfect match
				bestMatchingName     = ocrLine
				largestMatchingCount = ocrLine
				break
	
			for cardName in cardNames:
				for startingIndex in range(-len(cardName)+1,len(ocrLine)):
					if startingIndex + len(cardName) <= len(ocrLine):
						endingIndex = startingIndex + len(cardName) - 1
					else:
						endingIndex = len(ocrLine) - 1
				
					if startingIndex > 0:
						ocrSubstring = ocrLine[startingIndex:endingIndex+1]
					else:
						ocrSubstring = "@" * -startingIndex + ocrLine[0:endingIndex+1] #assume few to no cards use the symbol '@' to avoid matching prepended symbols
					ocrSubstring = ocrSubstring.replace(" ","@") #matching spaces causes headaches with cards with short names
				
					matchingCount = 0
					for index in range(0,endingIndex-startingIndex+1): #the +1 is important!
						if ocrSubstring[index]==cardName[index]:
							matchingCount+=1	
						
					if matchingCount > largestMatchingCount:
						largestMatchingCount = matchingCount
						bestMatchingName     = cardName
					elif (matchingCount == largestMatchingCount) and (len(cardName) < len(bestMatchingName)): #don't want to match e.g. Island as Turri Island
						bestMatchingName = cardName
					
		if int(len(bestMatchingName) * self.PERCENT_MATCH_THRESHOLD) < largestMatchingCount:
			return bestMatchingName
		else:
			raise ValueError("No card names matched OCR output")

	def getCardNamesFromJson(self):
		nameDataFile = open(self.CARD_NAMES_JSON_FILENAME)
		nameData = json.load(nameDataFile)
		cardNames = nameData.keys()
		cardNames = set(cardNames)
	
		return cardNames

	def printCardsUnderWebcam(self):
		while True:
			self.waitForPhotoRequest()
			cardName = self.printCardUnderWebcam()
			self.sendToPile(self.decideOnDirection(cardName))
			#self.quitCamera

	def printCardUnderWebcam(self,tryNumber=1):
		self.takePhoto("sample-card.png")
		self.cleanImage("sample-card.png","cleaned-sample-card.png")
		self.cropToTextbox("cleaned-sample-card.png","cropped-cleaned-sample-card.png")
		ocrOut = self.ocrImage("cropped-cleaned-sample-card.png")

		print("OCR attempt #" + str(tryNumber) + ":")
		print("OCR text: " + ocrOut)
		try:
			cardName = self.matchOcr(ocrOut,self.CARD_NAMES)
			print("Your card is: '" + cardName + "'")
			print("It will be placed in the " + self.decideOnDirection(cardName) + " pile")
			print("")
		except ValueError as e:
			if tryNumber < self.MAX_NUM_TRIES:
				cardName = self.printCardUnderWebcam(tryNumber+1)
			else:
				print("Could not read card after " + str(tryNumber) + " attempts")
				print("")
				return "error"
		except:
			print("Unexpected error")
			raise
			
		return cardName
	
	def decideOnDirection(self,cardName):
		if cardName in self.RIGHT_CARDS:
			return "right"
		elif cardName in self.LEFT_CARDS:
			return "left"
		else:
			return "error"
			
	def waitForPhotoRequest(self):
		print("Waiting for next card")

		gotLine = False
		while gotLine==False:
			line = self.arduinoPort.readline()[:-2] #apparently the Arduino uses Windows-style newlines
	
			if line=="Say cheese":
				print "Next card ready"
				gotLine = True
	
	def sendToPile(self, pile):
		time.sleep(0.1)
		self.arduinoPort.write(pile)
		
		
cardReader = CardReader()
cardReader.printCardsUnderWebcam()
