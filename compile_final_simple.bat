@echo off
echo Compiling Enhanced Final Simple UI keyboard simulator...

REM Try to find Visual Studio
if exist "D:\VS\Common7\Tools\VsDevCmd.bat" (
    echo Found Visual Studio at D:\VS
    call "D:\VS\Common7\Tools\VsDevCmd.bat"
    goto compile
)

echo Visual Studio environment not found, trying direct compilation...

:compile
echo Starting compilation...
cl /EHsc /utf-8 final_simple_ui.cpp /Fe:final_simple_ui.exe user32.lib gdi32.lib comctl32.lib /link /SUBSYSTEM:WINDOWS

if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
    echo Starting the Enhanced Final Simple UI keyboard simulator...
    start final_simple_ui.exe
) else (
    echo Compilation failed!
)

echo Press any key to continue...
pause >nul
