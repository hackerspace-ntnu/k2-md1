#!/usr/bin/python
from os import system
from sys import argv
if len(argv) < 2:
    print("Usage: ./run.py file.cpp")
else:
    command = "g++ %s -lX11 -lfreenect2 -Wl,-rpath=/home/johan/Johan/libfreenect2/build/lib -lOpenCL -O3 && sudo ./a.out"%argv[1]
    print(command)
    system(command)
