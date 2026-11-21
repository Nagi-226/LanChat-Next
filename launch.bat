@echo off
cd /d "%~dp0"
set "LANCHAT_EXE=%LOCALAPPDATA%\LanChat-Qt\lanchat_qt.exe"
if not exist "%LANCHAT_EXE%" (
  powershell -ExecutionPolicy Bypass -File "scripts\deploy-qt.ps1"
  if errorlevel 1 exit /b 1
)
start "" /D "%LOCALAPPDATA%\LanChat-Qt" "%LANCHAT_EXE%" %*
