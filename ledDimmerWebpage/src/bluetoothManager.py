from bluetooth import *
from datetime import datetime
import thread
from time import sleep
import re

class BluetoothManager:
    def __init__ (self, host, port):
        self.connection = BluetoothSocket(RFCOMM)
        self.host = host
        self.port = port
        self.receiveBuffer = ""
        self.listener = []
    
    def connect(self):
        self.connection = BluetoothSocket(RFCOMM)
        self.connection.connect((self.host, self.port))
    
    def registerListener(self, listener):
        self.listener.append(listener)
        
    def waitFor(self, message, timeout):
        start = datetime.now()
        self.receivedMessage = ""
        while datetime.now() - start < timeout and self.receivedMessage != message:
            pass
        return self.receivedMessage == message
    
    def sendMessage(self, message):
        if message["event"] == "lightSet":
            sending = "ls%01x%01x%02x" % (message["adressMask"], message["lightType"], message["value"])
            print sending
            try:
                self.connection.send(sending)
            except IOError:
                pass
        if message["event"] == "bootLoad":
            sending = "bs%01x" % (message["adressMask"])
            print sending
            try:
                self.connection.send(sending)
            except IOError:
                pass
            if self.waitFor({"event" : "bootStart", "adressMask" : message["adressMask"]}, 60000):
                content = message["hex"]
                totalLength = len(content)
                while len(content) > 0:
                    tmp = content[:20]
                    content = content[20:]
                    hex1 = hex(len(tmp) / 16)[-1:]
                    hex2 = hex(len(tmp) % 16)[-1:]
                    sending = "bh%s%s%s" % (hex1, hex2, tmp)
                    print sending
                    try:
                        self.connection.send(sending)
                    except IOError:
                        pass
                    if not self.waitFor("ba", 60000):
                        return
                    for listener in self.listener:
                        listener({'event' : 'bootProgress', 'progress' :100 - 100 * len(content) / totalLength})
                    print "loaded %.2f percent" % (100 - 100.0 * len(content) / totalLength)

    def extractMessage(self):
        matchObj = re.match(r'(ls[0-9a-f]{4})(.*)', self.receiveBuffer, re.I)
        if matchObj:
            matches = matchObj.groups()
            if len(matches) > 1:
                self.receiveBuffer = matches[1]
                matchObj = re.match(r'((ls)([0-9a-f])([0-9a-f])([0-9a-f]{2}))', matches[0], re.I)
                if matchObj:
                    matches2 = matchObj.groups()
                    if matches2[1] == "ls":
                        return {'event' : 'lightSet', 'adressMask' : int(matches2[2], 16), 'lightType' : int(matches2[3], 16), 'value' : int(matches2[4], 16)}
        return ""
    
    def run(self):
        while True:
            try:
                print "try connect"
                self.connect()
                try:
                    while True:
                        data = self.connection.recv(1024)
                        if len(data) > 0:
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
    
