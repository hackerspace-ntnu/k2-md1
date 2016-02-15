#!/usr/bin/python
from os import system
from sys import argv
if len(argv) < 2:
    print ("Usage: ./run.py file.cpp")
else:
    command = "g++ %s -lX11 -Wl,-rpath=/home/hackerspace/Documents/k2-md1/deps/libfreenect2/build/lib -L /home/hackerspace/Documents/k2-md1/deps/libfreenect2/build/lib -lfreenect2 -I /home/hackerspace/Documents/k2-md1/deps/libfreenect2/include -O3 && ./a.out"%argv[1]
    print (command)
    system(command)
