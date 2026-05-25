@echo off

if not exist build ( mkdir build )
cd build

del * /q

copy ..\..\apad_api_lib64\bin\*debug.* .

cl /nologo /w /I..\..\apad_api_lib64\source /Fe: notesboard /Od /Zi /std:c++17 ..\*.cpp *debug.lib

del *.ilk
del *.obj

REM Build rebuild_debug.bat
echo @echo off > rebuild_debug.bat
echo: >> rebuild_debug.bat
echo cd .. >> rebuild_debug.bat
echo call build_debug.bat >> rebuild_debug.bat