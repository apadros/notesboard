@echo off

if %1.==. (
	echo:
	echo Usage compile file.cpp
	exit /b
)

if not exist build ( mkdir build )
pushd build

cl /I..\..\apad_api_lib64\source /c /w /nologo /std:c++17 ..\%1

:Exit
popd
exit /b