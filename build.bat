@echo off

mkdir bin
pushd bin
cd

cl %* /W4 /EHsc /std:c++20 ..\src\*.cpp /Feemulator.exe

popd

run.bat
