#coding:utf-8
import socket
import json
import time
import pyaudio

p = pyaudio.PyAudio()
s = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
s.connect(( "192.168.39.184", 3000))
stream = p.open(format=pyaudio.paInt16,
                channels=2,
                rate=8000,
                output=True)
while True:
    a=s.recv(1024)
    stream.write(a)
s.close()
