from bluetooth import *
from datetime import datetime, timedelta
from time import sleep
import re
import thread

class BluetoothManager:
    def __init__ (self, host, port):
        self.connection = BluetoothSocket(RFCOMM)
        self.host = host
        self.port = port
        self.receiveBuffer = ""
        self.listener = []
        self.waitForState = 0
        self.waitForMessage = {}
        self.registerListener(self.waitForListener)
    
    def connect(self):
        self.connection = BluetoothSocket(RFCOMM)
        self.connection.connect((self.host, self.port))
    
    def registerListener(self, listener):
        self.listener.append(listener)
    
    def unregisterListener(self, listener):
        self.listener.remove(listener)
    
    def waitForListener(self, message):
        print message, self.waitForMessage, message == self.waitForMessage
        if self.waitForState == 1 and message == self.waitForMessage:
            self.waitForState = 2
        
    def waitFor(self, message, timeout):
        start = datetime.now()
        self.waitForState = 0
        self.waitForMessage = message
        self.waitForState = 1
        while datetime.now() - start < timedelta(seconds=timeout) and self.waitForState == 1:
            pass
        print "wait over:", (self.waitForState == 2)
        return self.waitForState == 2
    
    def sendMessage(self, message):
        if message["event"] == "lightSet":
            sending = "ls%01x%01x%02x" % (message["adressMask"], message["lightType"], message["value"])
            print "sending:", sending
            try:
                self.connection.send(sending)
            except IOError:
                pass
        if message["event"] == "bootLoad":
            sending = "bs%02x" % (message["adressMask"])
            print "sending:", sending
            try:
                self.connection.send(sending)
            except IOError:
                pass
            if self.waitFor({"event" : "bootStart", "adressMask" : message["adressMask"]}, 10):
                content = message["hex"]
                totalLength = len(content)
                while len(content) > 0:
                    tmp = content[:20]
                    content = content[20:]
                    hex1 = hex(len(tmp) / 16)[-1:]
                    hex2 = hex(len(tmp) % 16)[-1:]
                    sending = "bh%s%s%s" % (hex1, hex2, tmp)
                    print "sending:", sending
                    try:
                        self.connection.send(sending)
                    except IOError:
                        pass
                    if not self.waitFor({'event' : 'bootAcknowledge'}, 10):
                        return
                    for listener in self.listener:
                        listener({'event' : 'bootProgress', 'progress' :100 - 100 * len(content) / totalLength})
                    print "loaded %.2f percent" % (100 - 100.0 * len(content) / totalLength)

    def extractMessage(self):
        matchObj = re.match(r'ls([0-9a-f])([0-9a-f])([0-9a-f]{2})(.*)', self.receiveBuffer, re.I)
        if matchObj:
            matches = matchObj.groups()
            if len(matches) > 0:
                self.receiveBuffer = matches[-1]
                return {'event' : 'lightSet', 'adressMask' : int(matches[0], 16), 'lightType' : int(matches[1], 16), 'value' : int(matches[2], 16)}
        matchObj = re.match(r'ba(.*)', self.receiveBuffer, re.I)
        if matchObj:
            matches = matchObj.groups()
            if len(matches) > 0:
                self.receiveBuffer = matches[-1]
                return {'event' : 'bootAcknowledge'}
        matchObj = re.match(r'bs([0-9a-f]{2})(.*)', self.receiveBuffer, re.I)
        if matchObj:
            matches = matchObj.groups()
            if len(matches) > 0:
                self.receiveBuffer = matches[-1]
                return {"event" : "bootStart", "adressMask" : int(matches[0], 16)}
        return {}
    
    def run(self):
        while True:
            try:
                print "try connect"
                self.connect()
                try:
                    while True:
                        data = self.connection.recv(1024)
                        if len(data) > 0:
                            print "received:", data
                            self.receiveBuffer = "%s%s" % (self.receiveBuffer, data)
                            if len(self.listener) > 0:
                                message = self.extractMessage()
                                while len(message) > 0:
                                    for listener in self.listener:
                                        listener(message)
                                    message = self.extractMessage()
                except IOError:
                    pass
                sleep(1)
            except IOError:
                pass
        
    def start(self):
        try:
            thread.start_new_thread(self.run, ())
        except:
            print "Error: unable to start thread"

if __name__ == '__main__':
    bm = BluetoothManager("07:12:05:16:67:00", 1)
    bm.start()
    while True:
        bm.sendMessage("ls3400")
        sleep(1)
        bm.sendMessage("ls34ff")
        sleep(1)
    
