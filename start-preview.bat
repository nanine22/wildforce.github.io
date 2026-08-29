@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "PORT=5501"
set "LOCAL_URL=http://localhost:%PORT%/preview.html"

where py >nul 2>&1
if not errorlevel 1 goto start_server_py
where python >nul 2>&1
if not errorlevel 1 goto start_server_python

echo Python was not found. Install Python or use VS Code Live Server.
pause
exit /b 1

:start_server_py
for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "(Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.254.*' } | Select-Object -First 1 -ExpandProperty IPAddress) 2>$null"`) do set "LAN_IP=%%i"
if not defined LAN_IP set "LAN_IP=localhost"
set "LAN_URL=http://%LAN_IP%:%PORT%/preview.html"

echo Starting local dashboard + API server...
echo Local URL: %LOCAL_URL%
echo Remote URL: %LAN_URL%
start "Wildforce Preview" "%LAN_URL%"
py "%~dp0tools\local_preview_server.py"
exit /b %errorlevel%

:start_server_python
for /f "usebackq delims=" %%i in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "(Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.254.*' } | Select-Object -First 1 -ExpandProperty IPAddress) 2>$null"`) do set "LAN_IP=%%i"
if not defined LAN_IP set "LAN_IP=localhost"
set "LAN_URL=http://%LAN_IP%:%PORT%/preview.html"

echo Starting local dashboard + API server...
echo Local URL: %LOCAL_URL%
echo Remote URL: %LAN_URL%
start "Wildforce Preview" "%LAN_URL%"
python "%~dp0tools\local_preview_server.py"
exit /b %errorlevel%
