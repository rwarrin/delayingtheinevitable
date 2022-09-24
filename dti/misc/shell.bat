@echo off

set wd=%cd:~,-8%

SUBST U: /D
SUBST U: %wd%

pushd U:

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
start gvim.exe
