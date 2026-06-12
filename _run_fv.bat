@echo off
setlocal EnableDelayedExpansion
pushd "C:\reviewer\260612_CRA_DAY3\SampleOrderSystem"

set "VCVARS="
for %%P in (
    "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) do ( if exist %%P set "VCVARS=%%~P" )

if not defined VCVARS ( echo [ERROR] VS not found & exit /b 1 )
call "!VCVARS!" >nul 2>&1

set CL_FLAGS=/std:c++17 /EHsc /utf-8 /O2 /W3 /nologo
set GT_INC=/I"googletest\googletest\include" /I"googletest\googletest"
set GT_OBJ=build\gtest-all.obj

if not exist build mkdir build
if not exist build\fv mkdir build\fv

if not exist %GT_OBJ% (
    echo Building gtest-all.obj...
    cl.exe %CL_FLAGS% %GT_INC% /c googletest\googletest\src\gtest-all.cc /Fo:%GT_OBJ%
    if errorlevel 1 ( echo [ERROR] gtest-all build failed & exit /b 1 )
)

set INC=/I"include" /I"src" %GT_INC%
set SRCS=src\repository\JsonRepository.cpp src\service\SampleService.cpp src\service\OrderService.cpp src\service\ApprovalService.cpp src\service\ProductionService.cpp src\service\ReleaseService.cpp src\service\MonitorService.cpp

echo Building test_final_verify.exe...
cl.exe %CL_FLAGS% %INC% %GT_OBJ% %SRCS% tests\test_final_verify.cpp /Fe:build\test_final_verify.exe /Fo:build\fv\
if errorlevel 1 ( echo [ERROR] Compile failed & exit /b 1 )

echo [OK] Build success
build\test_final_verify.exe --gtest_output=xml:test_final_verify.xml
exit /b %ERRORLEVEL%
