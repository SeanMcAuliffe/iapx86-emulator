#!/usr/bin/bash

mkdir -p bin

g++ $@ -Wall -fno-exceptions -std=c++20 src/*.cpp -o bin/emulator.exe

rc=$?

if [ $rc -eq 0 ]; then
  ./run.sh
else
  echo Compilation failed!
  exit $rc
fi
