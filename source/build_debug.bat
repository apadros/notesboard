@echo off

if not exist build ( mkdir build )
cd build

del *.dll /q
del *.pdb /q
copy ..\..\apad_api_lib64\bin\*.dll .
copy ..\..\apad_api_lib64\bin\*.pdb .

cl /nologo /w /I..\..\apad_api_lib64\source /Fe: bolapad.exe /Od /Zi /std:c++17 ..\*.cpp /link ..\..\apad_api_lib64\bin\*.lib opengl32.lib user32.lib

del *.ilk
del *.obj

REM Build rebuild.bat
echo @echo off > rebuild.bat
echo cd .. >> rebuild.bat
echo call build_debug.bat >> rebuild.bat