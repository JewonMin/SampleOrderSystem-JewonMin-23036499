@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul

pushd "%~dp0."

echo ============================================================
echo   SampleOrderSystem Build
echo ============================================================

:: ── VS 환경 ─────────────────────────────────────────────────────────────
set "VCVARS="
for %%P in (
    "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) do (
    if exist %%P set "VCVARS=%%~P"
)

if not defined VCVARS (
    echo [ERROR] Visual Studio not found. Check the paths in build.bat.
    pause & exit /b 1
)
call "!VCVARS!" >nul 2>&1
echo [OK] VS: !VCVARS!

:: ── nlohmann/json ────────────────────────────────────────────────────────
if not exist "include\nlohmann\json.hpp" (
    echo [*] Downloading nlohmann/json...
    if not exist "include\nlohmann" mkdir include\nlohmann
    powershell -NoProfile -Command "Invoke-WebRequest -Uri 'https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp' -OutFile 'include\nlohmann\json.hpp'"
    if not exist "include\nlohmann\json.hpp" ( echo [ERROR] Download failed. & pause & exit /b 1 )
    echo [OK] json.hpp
)

:: ── Build dirs ───────────────────────────────────────────────────────────
if not exist "build" mkdir build

:: ── Compile ──────────────────────────────────────────────────────────────
set CL_FLAGS=/std:c++17 /EHsc /utf-8 /O2 /W3 /nologo
set INC=/I"include" /I"src"
set SRCS=src\main.cpp src\repository\JsonRepository.cpp src\service\SampleService.cpp src\service\OrderService.cpp src\service\ApprovalService.cpp src\service\ProductionService.cpp src\service\ReleaseService.cpp src\service\MonitorService.cpp src\view\SampleView.cpp src\view\OrderView.cpp src\view\ApprovalView.cpp src\view\ProductionView.cpp src\view\ReleaseView.cpp src\view\MonitorView.cpp

echo.
echo [*] Compiling...
cl.exe %CL_FLAGS% %INC% %SRCS% /Fe:SampleOrderSystem.exe /Fo:build\ 2>&1
if errorlevel 1 (
    echo.
    echo [ERROR] Build FAILED.
    pause & exit /b 1
)

echo.
echo ============================================================
echo   Build PASSED  --  SampleOrderSystem.exe
echo ============================================================

:: ── Tests (opt-in) ───────────────────────────────────────────────────────
set RUN_TEST=N
if /i "%~1"=="/test"  set RUN_TEST=Y
if /i "%~1"=="--test" set RUN_TEST=Y
if /i "!RUN_TEST!" NEQ "Y" ( pause & exit /b 0 )

:: Google Test
if not exist "build\p0" mkdir build\p0
if not exist "build\p1" mkdir build\p1
if not exist "build\p2" mkdir build\p2
if not exist "build\p3" mkdir build\p3
if not exist "build\p4" mkdir build\p4
if not exist "build\p5" mkdir build\p5
if not exist "build\p6" mkdir build\p6
if not exist "build\p7" mkdir build\p7
if not exist "build\p8" mkdir build\p8

if not exist "googletest\googletest\include\gtest\gtest.h" (
    echo [*] Downloading Google Test...
    powershell -NoProfile -Command "Invoke-WebRequest -Uri 'https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip' -OutFile 'gtest_dl.zip'"
    powershell -NoProfile -Command "Expand-Archive -Path 'gtest_dl.zip' -DestinationPath '.' -Force"
    if exist "googletest-1.14.0" rename "googletest-1.14.0" "googletest"
    if exist "gtest_dl.zip" del gtest_dl.zip
    if not exist "googletest\googletest\include\gtest\gtest.h" ( echo [ERROR] GTest download failed. & pause & exit /b 1 )
    echo [OK] Google Test
)

set GT_INC=/I"googletest\googletest\include" /I"googletest\googletest"
set GT_OBJ=build\gtest-all.obj
if not exist "%GT_OBJ%" (
    echo [*] Building gtest-all.obj...
    cl.exe %CL_FLAGS% %GT_INC% /c googletest\googletest\src\gtest-all.cc /Fo:%GT_OBJ% >nul 2>&1
    if errorlevel 1 ( echo [ERROR] gtest-all failed. & pause & exit /b 1 )
)

set REPO=src\repository\JsonRepository.cpp
set SVC_S=src\service\SampleService.cpp
set SVC_O=src\service\OrderService.cpp
set SVC_A=src\service\ApprovalService.cpp
set SVC_P=src\service\ProductionService.cpp
set SVC_R=src\service\ReleaseService.cpp
set SVC_M=src\service\MonitorService.cpp

for %%T in (0 1 2 3 4 5 6 7 8) do (
    echo [*] Phase %%T test...
    if %%T==0 ( set T_SRC= )
    if %%T==1 ( set T_SRC=%REPO% )
    if %%T==2 ( set T_SRC=%REPO% %SVC_S% )
    if %%T==3 ( set T_SRC=%REPO% %SVC_S% %SVC_O% )
    if %%T==4 ( set T_SRC=%REPO% %SVC_S% %SVC_O% %SVC_A% )
    if %%T==5 ( set T_SRC=%REPO% %SVC_S% %SVC_O% %SVC_A% %SVC_P% )
    if %%T==6 ( set T_SRC=%REPO% %SVC_R% )
    if %%T==7 ( set T_SRC=%REPO% %SVC_M% )
    if %%T==8 ( set T_SRC=%REPO% %SVC_S% %SVC_O% %SVC_A% %SVC_P% %SVC_R% %SVC_M% )
    cl.exe %CL_FLAGS% %INC% %GT_INC% %GT_OBJ% !T_SRC! tests\test_phase%%T.cpp /Fe:build\test_phase%%T.exe /Fo:build\p%%T\ >nul 2>&1
    if errorlevel 1 ( echo [ERROR] Phase %%T build failed. & pause & exit /b 1 )
    build\test_phase%%T.exe
    if errorlevel 1 ( echo [ERROR] Phase %%T FAILED. & pause & exit /b 1 )
)

echo.
echo ============================================================
echo   Build ^& Test ALL PASSED
echo ============================================================
pause
exit /b 0
