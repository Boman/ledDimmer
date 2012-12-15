#!/usr/bin/env python
# -*- coding: utf-8 -*-

from serial import *
from time import sleep
import binascii

def sendMessageAndWait(ser, message):
    for c in message:
        ser.write(chr(c))
        #print '>', chr(c)
    while True:
        sleep(0.001)
        data = ser.read(1)
        if len(data) > 0:
            #print '<', data
            if ord(data) == 1:
                return

def programDevice(ser, hexFile):
    f = open(hexFile, 'r')
    content = ''.join(f.readlines())
    f.close()
    totalLength = len(content)
    while len(content) > 0:
        tmp = content[:8]
        content = content[8:]
        bitlist = [7, len(tmp) + 2]
        for c in tmp:
            bitlist.append(ord(c))
        sendMessageAndWait(ser, bitlist)
        print "%.2f percent" % (100.0 * len(content) / totalLength)

def sendTest(ser):
    content = "testfhdoufavudihbdsaihbdsfdsvads"
    while len(content) > 0:
        tmp = content[:8]
        content = content[8:]
        bitlist = [7, len(tmp) + 2]
        for c in tmp:
            bitlist.append(ord(c))
        sendMessageAndWait(ser, bitlist)

if __name__ == '__main__':
    port = "/dev/rfcomm0"
    ser = Serial(port=port,
        baudrate=9600,
        bytesize=EIGHTBITS,
        parity=PARITY_NONE,
        stopbits=STOPBITS_ONE,
        timeout=0.1,
        xonxoff=1,
        rtscts=0,
        interCharTimeout=None)
    
    hexFile = "../../ledDimmerFirmware/Release/ledDimmerFirmware.hex"
    
    #start bootloader
    #sendMessageAndWait(ser, [6, 1])
    
    sendMessageAndWait(ser, [2, 1, 0])
    
    sleep(0.5)
        
    #programDevice(ser, hexFile)
    #sendTest(ser)
    
    while True:
        sleep(0.001)
        
        data = ser.read(1)
        if len(data) > 0:
            print '<', data
    
    ser.close()



## Objekt zum Zugriff auf serielle Schnittstelle anlegen
#s = serial.Serial(dev, baudrate=9600, parity='N', stopbits=1, timeout=0.01)
## moeglicherweise noch im Puffer vorhandene Daten loeschen
#s.flushInput()
## in diese Datei werden die empfangenen Werte geschrieben
##f = open("motor_kennlinie.txt","w")
#
## Endlosschleife, liest Daten von der Schnittstelle und schreibt diese
## sofort in die angegebene Datei sowie auf den Bildschirm.
## Mit Strg-C abbrechen!
#while 1:
#    print "reading"
#    tmp = s.read(100)
#    #tmp = string.replace(tmp,"\0","") # Bestätigungszeichen vom Bootloader entfernen
#    #tmp = string.replace(tmp,"@","") # Bestätigungszeichen vom Bootloader entfernen
#    #f.write(str(tmp))
#    #f.flush()
#    print(tmp)
#    s.write("a")
#    tmp = s.read(100)
#    print(tmp)
#    sleep(1)
    
    

#def readLastLine(ser):
#    last_data = ''
#    while True:
#        data = ser.read(100)
#        if data != '':
#            last_data = data
#        else:
#            return last_data
#
#if __name__ == '__main__':
#    ser = Serial(
#        port=dev,
#        baudrate=9600,
#        bytesize=EIGHTBITS,
#        parity=PARITY_NONE,
#        stopbits=STOPBITS_ONE,
#        timeout=0.1,
#        xonxoff=1,
#        rtscts=0,
#        interCharTimeout=None
#    )
#    
#    while True:
#        readLastLine(ser)
