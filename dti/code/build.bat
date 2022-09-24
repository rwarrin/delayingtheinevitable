@echo off

REM SET CompilerFlags=/nologo /Z7 /Od /Oi /fp:fast /FC
SET CompilerFlags=/nologo /Z7 /O1 /Ob0 /GS- /Oi /fp:fast /FC
REM SET CompilerFlags=/nologo /O2 /Oi /fp:fast /FC
SET LinkerFlags=/incremental:no

IF NOT EXIST build mkdir build

pushd build

cl.exe %CompilerFlags% ../dti/code/win32_dti.cpp /link %LinkerFlags% user32.lib gdi32.lib winmm.lib
cl.exe %CompilerFlags% ../dti/code/directsoundtest.cpp /link %LinkerFlags% user32.lib winmm.lib
cl.exe %CompilerFlags% ../dti/code/waveloadertest.cpp /link %LinkerFlags%

popd
