@echo off
setlocal

where qmake >nul 2>nul
if errorlevel 1 (
  echo qmake not found. Install Qt 5.15 and run this from a Qt developer prompt.
  exit /b 1
)

pushd "%~dp0legacy\original-server"
qmake HHServer.pro
if errorlevel 1 exit /b 1
where nmake >nul 2>nul
if errorlevel 1 (mingw32-make) else (nmake)
if errorlevel 1 exit /b 1
popd

pushd "%~dp0legacy\original-client"
qmake HHClient.pro
if errorlevel 1 exit /b 1
where nmake >nul 2>nul
if errorlevel 1 (mingw32-make) else (nmake)
if errorlevel 1 exit /b 1
popd

echo Legacy server and client build completed.
