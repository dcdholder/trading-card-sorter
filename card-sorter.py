import pygame.camera
import pygame.image
import wand.image
import pytesseract
import json
import PIL
import os
import sys

CARD_NAMES_JSON_FILENAME = "Resources/AllCards.json"

FOCUS = 250

TEXT_BOX_X_PERCENT = 0.8900
TEXT_BOX_Y_PERCENT = 0.0476

TEXT_BOX_WIDTH_PERCENT  = 0.0568
TEXT_BOX_HEIGHT_PERCENT = 0.9048

PERCENT_MATCH_THRESHOLD = 0.5

def takePhoto( filename ):
	#these commands force a consistent
	os.system("v4l2-ctl -d 0 -c focus_auto=0 >> dump.txt") #TODO: figure out why this is complaining
	os.system("v4l2-ctl -d 0 -c focus_absolute=" + str(FOCUS) + " >> dump.txt")

	pygame.camera.init()
	cam = pygame.camera.Camera(pygame.camera.list_cameras()[0],(1920,1080))
	cam.start()
	cardImage = cam.get_image()
	pygame.image.save(cardImage, filename)
	pygame.camera.quit()
	
	os.system("v4l2-ctl -d 0 -c focus_auto=1")
	os.system("v4l2-ctl -d 0 -c focus_absolute=0")

def cleanImage( inputFilename, outputFilename ):
	deskewCmd = "convert +repage -background white -fuzz 80% -deskew 40% -trim " + inputFilename + " " + outputFilename
	os.system(deskewCmd)

#TODO: use jpgs instead so that wand doesn't complain (pngs have a weird resolution scheme)
#TODO: rotate and crop according to initial orientation (look at image dimensions)
def cropToTextbox( inputFilename, outputFilename ):
	img = wand.image.Image(filename=inputFilename)

	cropX = int(img.width  * TEXT_BOX_X_PERCENT)
	cropY = int(img.height * TEXT_BOX_Y_PERCENT)
	cropWidth  = int(img.width  * TEXT_BOX_WIDTH_PERCENT)
	cropHeight = int(img.height * TEXT_BOX_HEIGHT_PERCENT)
	
	#TODO: figure out how to replace the system call with a 'wand' call
	cropCmd = "convert +repage -crop " + str(cropWidth) + "x" + str(cropHeight) + "+" + str(cropX) + "+" + str(cropY) + " " + inputFilename + " " + outputFilename
	os.system(cropCmd)
	
	rotateCmd = "convert " + outputFilename + " -rotate -90 " + outputFilename
	os.system(rotateCmd)

def ocrImage( inputFilename ):
	return pytesseract.image_to_string(PIL.Image.open(inputFilename))

#TODO: heavily document this - it's complicated!
#TODO: work on improving the run time
def matchOcr( ocrText, cardNames ):
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
					
	if int(len(bestMatchingName) * PERCENT_MATCH_THRESHOLD) < largestMatchingCount:
		return bestMatchingName
	else:
		raise ValueError("No card names matched OCR output to within " + str(PERCENT_MATCH_THRESHOLD))

def getCardNamesFromJson():
	nameDataFile = open(CARD_NAMES_JSON_FILENAME)
	nameData = json.load(nameDataFile)
	cardNames = nameData.keys()
	cardNames = set(cardNames)
	
	return cardNames

def printCardUnderWebcam():
	takePhoto("sample-card.png")
	cleanImage("sample-card.png","cleaned-sample-card.png")
	cropToTextbox("cleaned-sample-card.png","cropped-cleaned-sample-card.png")
	ocrOut = ocrImage("cropped-cleaned-sample-card.png")
	cardNames = getCardNamesFromJson()

	print("OCR text: " + ocrOut)
	try:
		cardName = matchOcr(ocrOut,cardNames)
		print("Your card is: '" + cardName + "'")
	except ValueError as e:
		print(str(e))
	except:
		print("Unexpected error")
		raise
	
printCardUnderWebcam()

