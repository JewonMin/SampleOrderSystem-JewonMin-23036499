@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul

:: Change to batch file's own directory so relative paths always work
cd /d "%~dp0"

echo ============================================================
echo   SampleOrderSystem Build ^& Test
echo ============================================================

:: ?? Visual Studio environment setup (vswhere first, hardcoded fallbacks)
set "VCVARS="

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "usebackq tokens=*" %%i in (
        `"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`
    ) do (
        if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" (
            set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
        )
    )
)

if not defined VCVARS (
    for %%P in (
        "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) do (
        if exist %%P set "VCVARS=%%~P"
    )
)

if not defined VCVARS (
    echo [ERROR] Visual Studio C++ build tools not found.
    exit /b 1
)
call "!VCVARS!" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Failed to initialize Visual Studio environment
    exit /b 1
)
echo [OK] Visual Studio environment ready

:: ?? nlohmann/json download
if not exist "include\nlohmann\json.hpp" (
    echo [*] Downloading nlohmann/json...
    if not exist "include\nlohmann" mkdir include\nlohmann
    powershell -NoProfile -Command "Invoke-WebRequest -Uri 'https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp' -OutFile 'include\nlohmann\json.hpp'"
    if not exist "include\nlohmann\json.hpp" (
        echo [ERROR] Failed to download json.hpp
        exit /b 1
    )
    echo [OK] json.hpp downloaded
)

:: ?? Google Test download
if not exist "googletest\googletest\include\gtest\gtest.h" (
    echo [*] Downloading Google Test v1.14.0...
    powershell -NoProfile -Command "Invoke-WebRequest -Uri 'https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip' -OutFile 'gtest_dl.zip'"
    powershell -NoProfile -Command "Expand-Archive -Path 'gtest_dl.zip' -DestinationPath '.' -Force"
    if exist "googletest-1.14.0" rename "googletest-1.14.0" "googletest"
    if exist "gtest_dl.zip" del gtest_dl.zip
    if not exist "googletest\googletest\include\gtest\gtest.h" (
        echo [ERROR] Failed to download Google Test
        exit /b 1
    )
    echo [OK] Google Test downloaded
)

:: ?? Build directories
if not exist "build"    mkdir build
if not exist "build\p0" mkdir build\p0
if not exist "build\p1" mkdir build\p1
if not exist "build\p2" mkdir build\p2
if not exist "build\p3" mkdir build\p3
if not exist "build\p4" mkdir build\p4

:: ?? Compiler settings (single-line variables only)
set CL_FLAGS=/std:c++17 /EHsc /utf-8 /O2 /W3 /nologo
set INC=/I"include" /I"src"
set GT_INC=/I"googletest\googletest\include" /I"googletest\googletest"
set GT_SRC=googletest\googletest\src\gtest-all.cc
set REPO=src\repository\JsonRepository.cpp
set SVC_S=src\service\SampleService.cpp
set SVC_O=src\service\OrderService.cpp
set SVC_A=src\service\ApprovalService.cpp
set VIEW_S=src\view\SampleView.cpp
set VIEW_O=src\view\OrderView.cpp
set VIEW_A=src\view\ApprovalView.cpp

:: ?? App build
echo.
echo [*] Building app...
cl.exe %CL_FLAGS% %INC% src\main.cpp %REPO% %SVC_S% %SVC_O% %SVC_A% %VIEW_S% %VIEW_O% %VIEW_A% /Fe:SampleOrderSystem.exe /Fo:build\ >build\build_app.log 2>&1
if errorlevel 1 (
    echo [ERROR] App build failed:
    type build\build_app.log
    exit /b 1
)
echo [OK] SampleOrderSystem.exe

:: ?? Phase 0 test
echo.
echo [*] Building Phase 0 tests...
cl.exe %CL_FLAGS% %INC% %GT_INC% %GT_SRC% tests\test_phase0.cpp /Fe:build\test_phase0.exe /Fo:build\p0\ >build\build_test0.log 2>&1
if errorlevel 1 ( echo [ERROR] Phase 0 build failed: & type build\build_test0.log & exit /b 1 )
echo [OK] test_phase0.exe
build\test_phase0.exe
if errorlevel 1 ( echo [ERROR] Phase 0 tests FAILED & exit /b 1 )

:: ?? Phase 1 test
echo.
echo [*] Building Phase 1 tests...
cl.exe %CL_FLAGS% %INC% %GT_INC% %GT_SRC% %REPO% tests\test_phase1.cpp /Fe:build\test_phase1.exe /Fo:build\p1\ >build\build_test1.log 2>&1
if errorlevel 1 ( echo [ERROR] Phase 1 build failed: & type build\build_test1.log & exit /b 1 )
echo [OK] test_phase1.exe
build\test_phase1.exe
if errorlevel 1 ( echo [ERROR] Phase 1 tests FAILED & exit /b 1 )

:: ?? Phase 2 test
echo.
echo [*] Building Phase 2 tests...
cl.exe %CL_FLAGS% %INC% %GT_INC% %GT_SRC% %REPO% %SVC_S% tests\test_phase2.cpp /Fe:build\test_phase2.exe /Fo:build\p2\ >build\build_test2.log 2>&1
if errorlevel 1 ( echo [ERROR] Phase 2 build failed: & type build\build_test2.log & exit /b 1 )
echo [OK] test_phase2.exe
build\test_phase2.exe
if errorlevel 1 ( echo [ERROR] Phase 2 tests FAILED & exit /b 1 )

:: ?? Phase 3 test
echo.
echo [*] Building Phase 3 tests...
cl.exe %CL_FLAGS% %INC% %GT_INC% %GT_SRC% %REPO% %SVC_S% %SVC_O% tests\test_phase3.cpp /Fe:build\test_phase3.exe /Fo:build\p3\ >build\build_test3.log 2>&1
if errorlevel 1 ( echo [ERROR] Phase 3 build failed: & type build\build_test3.log & exit /b 1 )
echo [OK] test_phase3.exe
build\test_phase3.exe
if errorlevel 1 ( echo [ERROR] Phase 3 tests FAILED & exit /b 1 )

:: ?? Phase 4 test
echo.
echo [*] Building Phase 4 tests...
cl.exe %CL_FLAGS% %INC% %GT_INC% %GT_SRC% %REPO% %SVC_S% %SVC_O% %SVC_A% tests\test_phase4.cpp /Fe:build\test_phase4.exe /Fo:build\p4\ >build\build_test4.log 2>&1
if errorlevel 1 ( echo [ERROR] Phase 4 build failed: & type build\build_test4.log & exit /b 1 )
echo [OK] test_phase4.exe
build\test_phase4.exe
if errorlevel 1 ( echo [ERROR] Phase 4 tests FAILED & exit /b 1 )

echo.
echo ============================================================
echo   Build ^& Test ALL PASSED
echo ============================================================
exit /b 0
