#!/usr/bin/python
from os import system
from sys import argv
if len(argv) < 2:
    print "Usage: ./run.py file.cpp"
else:
    command = "g++ %s -lX11 -lfreenect2 -L ../../libfreenect2/lib/ -I ../../libfreenect2/include -O3 && a.out"%argv[1]
    print command
    system(command)
