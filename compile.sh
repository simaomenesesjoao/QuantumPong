#!/bin/bash

g++ -lOpenCL -lX11 opencl.cpp main.cpp x11.cpp -o qp
