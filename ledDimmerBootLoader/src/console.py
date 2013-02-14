import serial
import sys

def receive(ser):
    data = ser.read(1) 
    if len(data) > 0:
        sys.stdout.write(data)
         
def send(ser, messageField):
    for c in messageField:
        ser.write(chr(c))

ser = serial.Serial('/dev/rfcomm0', 9600, timeout=1)

while 1:
    receive(ser)
    #input()

