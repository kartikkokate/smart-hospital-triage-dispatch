@echo off
setlocal

:: ============================================================================
:: Builds the REST API server (bin\HospitalServer.exe) with MinGW g++.
:: Does not touch or rebuild the original console app or test runner.
:: ============================================================================

cd /d "%~dp0.."

:: Prefer a g++ already on PATH; fall back to the MinGW path used by build_and_test.bat
where g++ >nul 2>nul
if errorlevel 1 (
    if exist "C:\MinGW\bin\g++.exe" (
        set "PATH=C:\MinGW\bin;%PATH%"
    ) else (
        echo [ERROR] g++ not found on PATH or at C:\MinGW\bin\g++.exe
        exit /b 1
    )
)

if not exist bin mkdir bin
if exist bin\HospitalServer.exe del /f /q bin\HospitalServer.exe

echo Compiling bin\HospitalServer.exe (this takes about 30 seconds)...
g++ -std=c++17 -Wall -Wextra -pedantic -O1 -I include -I server\third_party -I server\src ^
    server\src\server_main.cpp server\src\http_server.cpp server\src\api.cpp ^
    src\models\Patient.cpp src\models\DispatchResource.cpp src\services\DataLoader.cpp ^
    -o bin\HospitalServer.exe -lws2_32
if errorlevel 1 (
    echo [ERROR] Server compilation failed.
    exit /b 1
)

echo [SUCCESS] Built bin\HospitalServer.exe
echo Run it from the project root:  bin\HospitalServer.exe
exit /b 0
