@echo off
echo ==============================================
echo  Building DesktopSnowNative (Native C++20)
echo ==============================================

g++ -O3 -mwindows -std=c++17 -static -static-libgcc -static-libstdc++ main.cpp -o DesktopSnowNative.exe -lgdi32 -luser32 -ldwmapi -lgdiplus -lcomctl32 -lwinmm -lshlwapi -lole32 -lshell32

if %ERRORLEVEL% EQU 0 (
    echo [OK] Build succeeded: DesktopSnowNative.exe
) else (
    echo [ERROR] Compilation failed!
)
