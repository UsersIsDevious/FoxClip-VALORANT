@echo off
setlocal

cd /d "%~dp0"

set "PORT=55555"
set "PASS=xxxxxxxxxxxxxxxxxxxxxx"
set "ENDPOINT=help"
set "ENDPOINT_FILENAME=%ENDPOINT:/=_%"

for /f %%i in ('powershell -NoP -C "(Get-Date).ToString(\"yyyyMMdd-HHmmss\")"') do set "STAMP=%%i"

if not exist "%ENDPOINT_FILENAME%" (
  mkdir "%ENDPOINT_FILENAME%"
)

curl.exe -ks "https://127.0.0.1:%PORT%/%ENDPOINT%" ^
  -u riot:%PASS% ^
  -H "Origin: https://127.0.0.1" > "%ENDPOINT_FILENAME%/%STAMP%_%ENDPOINT_FILENAME%.json"

endlocal
