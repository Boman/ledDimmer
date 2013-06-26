###############################################################################
##
##  Copyright 2011,2012 Tavendo GmbH
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.
##
###############################################################################

import sys

from bluetoothManager import BluetoothManager

from twisted.python import log
from twisted.internet import reactor

from autobahn.websocket import connectWS
from autobahn.wamp import WampClientFactory, \
                          WampClientProtocol


class PubSubClient1(WampClientProtocol):
    def onOpen(self):
        self.bm = BluetoothManager("07:12:05:16:67:00", 1)
        self.bm.registerListener(self.onBluetoothMessage)
        self.bm.start()
    
    def onSessionOpen(self):
        self.subscribe("http://leddimmer.unserHaus.name/event", self.onSimpleEvent)
    
    def onSimpleEvent(self, topicUri, event):
        self.sendBluetoothMessage(event)
    
    def onBluetoothMessage(self, message):
        print message
        self.publish("http://leddimmer.unserHaus.name/event", message)
     
    def sendBluetoothMessage(self, message):
        self.bm.sendMessage(message)        

if __name__ == '__main__':    
    log.startLogging(sys.stdout)
    debug = len(sys.argv) > 1 and sys.argv[1] == 'debug'
    debug = True

    factory = WampClientFactory("ws://localhost:9000", debugWamp=debug)
    factory.protocol = PubSubClient1
    factory.setProtocolOptions(tcpNoDelay=True)

    connectWS(factory)

    reactor.run()
