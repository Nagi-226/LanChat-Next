@echo off
cd /d "%~dp0"
echo LanChat-Next Launcher
echo.
echo   Visible mode (this window) — shows server + Tauri logs
echo   Silent mode — double-click launch.vbs to run with no console
echo.
powershell -ExecutionPolicy Bypass -File "scripts\launch.ps1"
pause
