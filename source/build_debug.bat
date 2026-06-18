@echo off

if not exist build ( mkdir build )
cd build

del *.dll /q
del *.pdb /q
copy ..\..\apad_api_lib64\bin\*.dll .
copy ..\..\apad_api_lib64\bin\*.pdb .

cl /nologo /w /I..\..\apad_api_lib64\source /Fe: bolapad /Od /Zi /std:c++17 ..\*.cpp /link ..\..\apad_api_lib64\bin\*.lib opengl32.lib user32.lib

del *.ilk
del *.obj

REM Build rebuild_debug.bat
echo @echo off > rebuild_debug.bat
echo: >> rebuild_debug.bat
echo cd .. >> rebuild_debug.bat
echo call build_debug.bat >> rebuild_debug.bat