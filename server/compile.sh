#!/bin/bash

g++ -lOpenCL -lX11 opencl.cpp main.cpp x11.cpp -o qp

# g++ -Wall *.cpp -o server  && ./server
# g++ *.cpp -o client  `sdl2-config --libs --cflags` -Wall -lm && ./client
# a=$(ps aux | grep "./client" | head -n 1 | awk '{print $2}'); echo $a; kill -9 $a
