@echo off

if not exist build ( mkdir build )
cd build

del apad_api* /q
copy ..\..\apad_api_lib64\bin\*debug.* .

cl /nologo /w /I..\..\apad_api_lib64\source /Fe: bolapad /Od /Zi /std:c++17 ..\*.cpp *debug.lib opengl32.lib

del *.ilk
del *.obj

REM Build rebuild_debug.bat
echo @echo off > rebuild_debug.bat
echo: >> rebuild_debug.bat
echo cd .. >> rebuild_debug.bat
echo call build_debug.bat >> rebuild_debug.bat