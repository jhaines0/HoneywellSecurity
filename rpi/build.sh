#!/bin/sh
g++ -o honeywell --std=c++11 digitalDecoder.cpp analogDecoder.cpp main.cpp -lrtlsdr
