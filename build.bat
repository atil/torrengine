@echo off

cls

if not exist bin (
    mkdir bin
)
if not exist obj (
    mkdir obj
)

set Paths=/Ideps /Fo.\obj\ /Fe.\bin\main.exe
set Libs=lib/glew32s.lib lib/glfw3dll.lib lib/openal32.lib opengl32.lib 
set OtherFlags=/nologo /Wall /Zi /Qspectre /EHsc /std:c++17

cl %OtherFlags% %Paths% src/*.cpp %Libs%

if %errorlevel% neq 0 (
    echo.
    echo ***Build failed***
    exit /B 1
)

if not "%1" == "-b" ( REM Build only switch
    .\bin\main.exe
)
