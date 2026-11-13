@echo off
setlocal
cd /d "%~dp0"

where g++ >nul 2>nul
if errorlevel 1 (
    echo g++ was not found. Install MinGW-w64 and add g++ to PATH.
    pause
    exit /b 1
)

if not exist build mkdir build

echo Building the LEX-style generator...
g++ -std=c++17 -Wall -Wextra -pedantic src\lexgen_v2.cpp -o build\lexgen_v2.exe
if errorlevel 1 goto failed

echo Generating the scanner from the active rules...
build\lexgen_v2.exe tests\lex_style_sample.lex src\lex_style_scanner.cpp
if errorlevel 1 goto failed

echo Building the generated scanner...
g++ -std=c++17 -Wall -Wextra -pedantic src\lex_style_scanner.cpp -o build\lex_style_scanner.exe
if errorlevel 1 goto failed

start "" /min cmd /c "timeout /t 2 /nobreak >nul & start http://127.0.0.1:8000"
echo Starting the local web application...
python server.py
exit /b %ERRORLEVEL%

:failed
echo Build failed. Review the error above.
pause
exit /b 1
