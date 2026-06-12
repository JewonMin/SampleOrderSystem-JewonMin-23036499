@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul

echo ============================================================
echo   SampleOrderSystem Build ^& Test
echo ============================================================

:: ── Visual Studio 환경 설정 (vswhere 우선, 하드코딩 경로 fallback)
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
        if exist %%P set "VCVARS=%%P"
    )
)

if not defined VCVARS (
    echo [ERROR] Visual Studio C++ 빌드 도구를 찾을 수 없습니다.
    exit /b 1
)
call !VCVARS! >nul 2>&1
echo [OK] Visual Studio 환경 설정 완료

:: ── nlohmann/json 다운로드
if not exist "include\nlohmann\json.hpp" (
    echo [*] nlohmann/json 다운로드 중...
    if not exist "include\nlohmann" mkdir include\nlohmann
    powershell -NoProfile -Command ^
        "Invoke-WebRequest -Uri 'https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp' -OutFile 'include\nlohmann\json.hpp'"
    if not exist "include\nlohmann\json.hpp" (
        echo [ERROR] json.hpp 다운로드 실패
        exit /b 1
    )
    echo [OK] json.hpp 다운로드 완료
)

:: ── Google Test 다운로드
if not exist "googletest\googletest\include\gtest\gtest.h" (
    echo [*] Google Test v1.14.0 다운로드 중...
    powershell -NoProfile -Command ^
        "Invoke-WebRequest -Uri 'https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip' -OutFile 'gtest_dl.zip'"
    powershell -NoProfile -Command ^
        "Expand-Archive -Path 'gtest_dl.zip' -DestinationPath '.' -Force"
    if exist "googletest-1.14.0" rename "googletest-1.14.0" "googletest"
    if exist "gtest_dl.zip" del gtest_dl.zip
    if not exist "googletest\googletest\include\gtest\gtest.h" (
        echo [ERROR] Google Test 다운로드 실패
        exit /b 1
    )
    echo [OK] Google Test 다운로드 완료
)

:: ── 빌드 폴더 생성
if not exist "build" mkdir build

set GTEST_ROOT=googletest\googletest
set CL_FLAGS=/std:c++17 /EHsc /utf-8 /O2 /W3 /nologo
set INCLUDES=/I"include" /I"src"
set GTEST_INCLUDES=/I"%GTEST_ROOT%\include" /I"%GTEST_ROOT%"

:: ── 공통 소스 (Phase별 추가)
set APP_SRC=src\main.cpp ^
    src\repository\JsonRepository.cpp ^
    src\service\SampleService.cpp ^
    src\service\OrderService.cpp ^
    src\view\SampleView.cpp ^
    src\view\OrderView.cpp

set SHARED_SRC=src\repository\JsonRepository.cpp ^
    src\service\SampleService.cpp ^
    src\service\OrderService.cpp

:: ── 메인 앱 빌드
echo.
echo [*] 앱 빌드 중...
cl.exe %CL_FLAGS% %INCLUDES% ^
    %APP_SRC% ^
    /Fe:SampleOrderSystem.exe ^
    /Fo:build\ >build\build_app.log 2>&1
if errorlevel 1 (
    echo [ERROR] 앱 빌드 실패:
    type build\build_app.log
    exit /b 1
)
echo [OK] SampleOrderSystem.exe 빌드 완료

:: ── Phase 0 테스트 빌드 및 실행
if not exist "build\p0" mkdir build\p0
echo.
echo [*] Phase 0 테스트 빌드 중...
cl.exe %CL_FLAGS% %INCLUDES% %GTEST_INCLUDES% ^
    "%GTEST_ROOT%\src\gtest-all.cc" ^
    tests\test_phase0.cpp ^
    /Fe:build\test_phase0.exe ^
    /Fo:build\p0\ >build\build_test0.log 2>&1
if errorlevel 1 (
    echo [ERROR] Phase 0 테스트 빌드 실패:
    type build\build_test0.log
    exit /b 1
)
echo [OK] test_phase0.exe 빌드 완료

echo [*] Phase 0 테스트 실행 중...
build\test_phase0.exe
if errorlevel 1 (
    echo [ERROR] Phase 0 테스트 실패
    exit /b 1
)

:: ── Phase 1 테스트 빌드 및 실행
if not exist "build\p1" mkdir build\p1
echo.
echo [*] Phase 1 테스트 빌드 중...
cl.exe %CL_FLAGS% %INCLUDES% %GTEST_INCLUDES% ^
    "%GTEST_ROOT%\src\gtest-all.cc" ^
    %SHARED_SRC% ^
    tests\test_phase1.cpp ^
    /Fe:build\test_phase1.exe ^
    /Fo:build\p1\ >build\build_test1.log 2>&1
if errorlevel 1 (
    echo [ERROR] Phase 1 테스트 빌드 실패:
    type build\build_test1.log
    exit /b 1
)
echo [OK] test_phase1.exe 빌드 완료

echo [*] Phase 1 테스트 실행 중...
build\test_phase1.exe
if errorlevel 1 (
    echo [ERROR] Phase 1 테스트 실패
    exit /b 1
)

:: ── Phase 2 테스트 빌드 및 실행
if not exist "build\p2" mkdir build\p2
echo.
echo [*] Phase 2 테스트 빌드 중...
cl.exe %CL_FLAGS% %INCLUDES% %GTEST_INCLUDES% ^
    "%GTEST_ROOT%\src\gtest-all.cc" ^
    %SHARED_SRC% ^
    tests\test_phase2.cpp ^
    /Fe:build\test_phase2.exe ^
    /Fo:build\p2\ >build\build_test2.log 2>&1
if errorlevel 1 (
    echo [ERROR] Phase 2 테스트 빌드 실패:
    type build\build_test2.log
    exit /b 1
)
echo [OK] test_phase2.exe 빌드 완료

echo [*] Phase 2 테스트 실행 중...
build\test_phase2.exe
if errorlevel 1 (
    echo [ERROR] Phase 2 테스트 실패
    exit /b 1
)

:: ── Phase 3 테스트 빌드 및 실행
if not exist "build\p3" mkdir build\p3
echo.
echo [*] Phase 3 테스트 빌드 중...
cl.exe %CL_FLAGS% %INCLUDES% %GTEST_INCLUDES% ^
    "%GTEST_ROOT%\src\gtest-all.cc" ^
    %SHARED_SRC% ^
    tests\test_phase3.cpp ^
    /Fe:build\test_phase3.exe ^
    /Fo:build\p3\ >build\build_test3.log 2>&1
if errorlevel 1 (
    echo [ERROR] Phase 3 테스트 빌드 실패:
    type build\build_test3.log
    exit /b 1
)
echo [OK] test_phase3.exe 빌드 완료

echo [*] Phase 3 테스트 실행 중...
build\test_phase3.exe
if errorlevel 1 (
    echo [ERROR] Phase 3 테스트 실패
    exit /b 1
)

echo.
echo ============================================================
echo   Build ^& Test 완료
echo ============================================================
exit /b 0
