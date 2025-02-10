@echo off

mkdir bin
pushd bin
cd

cl %* /W4 /EHsc /std:c++20 ..\src\*.cpp /Feemulator.exe

popd

if %ERRORLEVEL% NEQ 0 (
  echo Compilation failed!
  exit /b %ERRORLEVEL%
) else (
  run.bat
)
