@echo off
setlocal

set "BIN_DIR=%~dp0"
set "SCRIPT=%BIN_DIR%GenNetworkFunc.py"
set "LOG=%BIN_DIR%GenNetworkFunc.log"

if not exist "%SCRIPT%" (
  echo [ERROR] Script not found: "%SCRIPT%"
  pause
  exit /b 1
)

python "%SCRIPT%" > "%LOG%" 2>&1
if errorlevel 1 (
  echo [ERROR] Failed. See log: "%LOG%"
  type "%LOG%"
  pause
  exit /b 1
)

echo [OK] Done. Log: "%LOG%"
type "%LOG%"
pause
endlocal
