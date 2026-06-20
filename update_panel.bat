@echo off
chcp 65001 >nul
echo === Actualizador del Panel TC ===
echo.
C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File "%~dp0update_panel.ps1"
pause
