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

from autobahn.wamp import WampServerProtocol, WampClientProtocol, \
    WampServerFactory, WampClientFactory
from autobahn.websocket import listenWS, connectWS
from bluetoothManager import BluetoothManager
from twisted.internet import reactor
from twisted.python import log
from twisted.web.server import Site
from twisted.web.static import File
import sys

class PubSubServer(WampServerProtocol):
    def onSessionOpen(self):
        self.registerForPubSub("http://leddimmer.unserHaus.name/event")

class PubSubClient(WampClientProtocol):
    def onSessionOpen(self):
        self.subscribe("http://leddimmer.unserHaus.name/event", self.onSimpleEvent)
    
    def onSimpleEvent(self, topicUri, event):
        for i in event:
            print i, event[i]

def onBluetoothMessage(message):
    print message

def sendBluetoothMessage(message):
    print message

if __name__ == '__main__':
    bm = BluetoothManager("07:12:05:16:67:00", 1)
    bm.registerListener(onBluetoothMessage)
    bm.start()
    
    log.startLogging(sys.stdout)
    
    factory = WampServerFactory("ws://localhost:9000", debugWamp=False)
    factory.protocol = PubSubServer
    factory.setProtocolOptions(allowHixie76=True)
    listenWS(factory)
    
    factory = WampClientFactory("ws://localhost:9000", debugWamp=False)
    factory.protocol = PubSubClient
    connectWS(factory)
    
    webdir = File(".")
    web = Site(webdir)
    reactor.listenTCP(8080, web)
    
    reactor.run()
