from Tkconstants import INSERT, END, TOP, LEFT, RIGHT
from Tkinter import Tk, Text, Button, Entry, Menu
from time import sleep
import serial
import thread
import tkFileDialog

def putText(text, tag):
        textArea.mark_set(INSERT, END)
        textArea.mark_set("sentinel", INSERT)
        textArea.mark_gravity("sentinel" , direction=LEFT)
        for c in text:
            if ord(c) > 31 and ord(c) < 255:
                textArea.insert(INSERT, c)
            else:
                textArea.insert(INSERT, "[%d]" % ord(c))
        textArea.tag_add(tag, "sentinel", INSERT)
        textArea.insert(INSERT, "\n")
        textArea.yview(END)

def sendAction():
    message = messageField.get()
    messageField.delete(0, END)
    sendMessage(message)
         
def sendMessage(message):
    putText(message, "send")
    ser.write(message)

lastReceived = ""

def receive():
    global lastReceived
    while True:
        data = ser.read(64)
        if len(data) > 0:
            lastReceived = "%s%s" % (lastReceived, data)
            putText(data, "recv")
        sleep(0.1)
        
def bootloader(deviceAdress):
    global lastReceived
    #hexFileName = tkFileDialog.askopenfilename()
    hexFileName = "/home/falko/git/ledDimmer/ledDimmerFirmware/Release/ledDimmerFirmware.hex"
    sendMessage("bs0%d" % deviceAdress)
    f = open(hexFileName, 'r')
    content = ''.join(f.readlines())
    f.close()
    #while not lastReceived.endswith("bootloader"):
    #    sleep(0.1)
    totalLength = len(content)
    lastReceived = ""
    while len(content) > 0:
        tmp = content[:20]
        content = content[20:]
        hex1 = hex(len(tmp) / 16)[-1:]
        hex2 = hex(len(tmp) % 16)[-1:]
        sendMessage("bh%s%s%s" % (hex1, hex2, tmp))
        cnt = 0
        while not "ba" in lastReceived and cnt < 50:
            sleep(0.1)
            cnt += 1
        if cnt == 50:
            putText("timeout(lastReceived=%s)" % lastReceived, "info")
            return
        lastReceived = ""
        putText("loaded %.2f percent" % (100 - 100.0 * len(content) / totalLength), "info")
    
# dummy function for receiving textArea key focus
def empty(event):
    return "break"
    
root = Tk()
root.title("ledDimmerTool")

textArea = Text(root)
textArea.pack(side=TOP)
textArea.bind("<Key>", empty)

textArea.tag_config("recv", background="yellow", foreground="blue")
textArea.tag_config("send", background="cyan", foreground="red")
textArea.tag_config("info", background="red", foreground="black")

messageField = Entry(root, width=64)
messageField.pack(side=LEFT)

send = Button(root, text="send", command=sendAction)
send.pack(side=RIGHT)

menubar = Menu(root)
bootloaderMenu = Menu(menubar, tearoff=0)
bootloaderMenu.add_command(label="Upload Master Firmware", command=lambda:thread.start_new_thread(bootloader, (1,)))
bootloaderMenu.add_command(label="Upload Slave Firmware", command=lambda:thread.start_new_thread(bootloader, (2,)))
#bootloaderMenu.add_separator()
menubar.add_cascade(label="Bootloader", menu=bootloaderMenu)
root.config(menu=menubar)

#ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
ser = serial.Serial('/dev/rfcomm0', 9600)

thread.start_new_thread(receive, ())

root.mainloop()
