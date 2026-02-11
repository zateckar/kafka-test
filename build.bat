@echo off
REM Build script for Kafka CLI Tool
REM Uses MinGW/GCC to compile the application with dynamic linking to librdkafka

setlocal EnableDelayedExpansion

echo ==========================================
echo Kafka CLI Tool - Build Script
echo ==========================================
echo.

REM Configuration
set SRC_DIR=src
set BUILD_DIR=build
set LIBRDKAFKA_DIR=librdkafka
set SOURCE_FILE=%SRC_DIR%\kafka_cli.c
set OUTPUT_FILE=%BUILD_DIR%\kafka_cli.exe

REM Compiler settings
set CC=gcc
set CFLAGS=-Wall -Wextra -O2 -std=c99 -D_CRT_SECURE_NO_WARNINGS
set INCLUDES=-I%LIBRDKAFKA_DIR%
set LIBS=-L%LIBRDKAFKA_DIR% -lrdkafka -lws2_32 -lsecur32 -lcrypt32

REM Create build directory if it doesn't exist
if not exist %BUILD_DIR% (
    echo Creating build directory...
    mkdir %BUILD_DIR%
)

REM Check if source file exists
if not exist %SOURCE_FILE% (
    echo ERROR: Source file not found: %SOURCE_FILE%
    exit /b 1
)

echo Source: %SOURCE_FILE%
echo Output: %OUTPUT_FILE%
echo.

REM Clean previous build
echo Cleaning previous build...
if exist %OUTPUT_FILE% del %OUTPUT_FILE%

REM Compile
echo Compiling...
echo Command: %CC% %CFLAGS% %INCLUDES% %SOURCE_FILE% -o %OUTPUT_FILE% %LIBS%
echo.

%CC% %CFLAGS% %INCLUDES% %SOURCE_FILE% -o %OUTPUT_FILE% %LIBS%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ==========================================
    echo BUILD FAILED
echo ==========================================
    exit /b 1
)

echo.
echo ==========================================
echo BUILD SUCCESSFUL
echo ==========================================
echo.
echo Executable: %OUTPUT_FILE%
echo.
echo Copying required DLLs to build directory...

REM Copy required DLLs to build directory
for %%D in (%LIBRDKAFKA_DIR%\*.dll) do (
    echo   Copying %%~nxD
    copy "%%D" "%BUILD_DIR%\" >nul
)

echo.
echo ==========================================
echo Build complete! You can now run:
echo   %OUTPUT_FILE%
echo ==========================================

endlocal
