@echo off
setlocal

:: ============================================================================
:: Local MinGW convenience build script for Smart Hospital System (CS2305)
:: Uses machine-specific compiler path C:\MinGW\bin\g++.exe.
:: Note: CMake/CTest and memory diagnostics remain unverified on this system.
:: ============================================================================

:: Anchor working directory to the script's own directory
cd /d "%~dp0"

:: Set path prioritizing the local MinGW compiler and standard Windows system folders
set "PATH=C:\MinGW\bin;C:\Windows\System32;C:\Windows;C:\Windows\System32\Wbem"

echo ================================================================================
echo   Building Smart Hospital System (CS2305) - Milestones 1 to 8 (Complete System)
echo   Local MinGW convenience build script
echo ================================================================================

:: Create output directory and clean previous binaries so stale builds never execute
if not exist bin mkdir bin
if exist bin\SmartHospitalSystem.exe del /f /q bin\SmartHospitalSystem.exe
if exist bin\TestRunner.exe del /f /q bin\TestRunner.exe

:: Check if g++ exists at the expected machine-specific MinGW path
if not exist "C:\MinGW\bin\g++.exe" (
    echo [ERROR] MinGW C++ compiler not found at machine-specific path C:\MinGW\bin\g++.exe
    exit /b 1
)

echo [TOOLCHAIN] Compiling with MinGW GCC (C++17, -Wall -Wextra -pedantic)...

echo [1/2] Compiling main application: bin\SmartHospitalSystem.exe ...
g++ -std=c++1z -Wall -Wextra -pedantic -I include src\models\Patient.cpp src\models\DispatchResource.cpp src\services\DataLoader.cpp src\main.cpp -o bin\SmartHospitalSystem.exe
if errorlevel 1 (
    echo [ERROR] Compilation of SmartHospitalSystem.exe failed.
    exit /b 1
)

echo [2/2] Compiling automated test suite: bin\TestRunner.exe ...
g++ -std=c++1z -Wall -Wextra -pedantic -I include src\models\Patient.cpp src\models\DispatchResource.cpp src\services\DataLoader.cpp tests\TestRunner.cpp -o bin\TestRunner.exe
if errorlevel 1 (
    echo [ERROR] Compilation of TestRunner.exe failed.
    exit /b 1
)

echo.
echo ================================================================================
echo   Executing Milestones 1 to 7 Automated Tests
echo ================================================================================
bin\TestRunner.exe data/
if errorlevel 1 (
    echo [TEST FAILED] Automated test suite returned non-zero exit code.
    exit /b 1
)

echo.
echo ================================================================================
echo   Executing Main Application Deterministic Demonstration (--demo)
echo ================================================================================
bin\SmartHospitalSystem.exe --demo data/
if errorlevel 1 (
    echo [APP FAILED] Main executable returned non-zero exit code.
    exit /b 1
)

echo.
echo ================================================================================
echo [SUCCESS] Milestones 1 to 8 (Complete System) built and verified cleanly.
echo ================================================================================
exit /b 0
