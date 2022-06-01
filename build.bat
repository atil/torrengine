@echo off

cls

if not exist bin (
    mkdir bin
)
if not exist obj (
    mkdir obj
)

set Paths=/Ideps /Fo.\obj\ /Fe.\bin\
set Libs=lib/glew32s.lib lib/glfw3dll.lib lib/openal32.lib opengl32.lib 
set OtherFlags=/nologo /Wall /Zi /Qspectre /EHsc /std:c++17

cl %OtherFlags% %Paths% src/main.cpp %Libs%

if not "%1" == "-b" ( REM Build only switch
    if errorlevel 0 if not errorlevel 1 .\bin\main.exe
)
