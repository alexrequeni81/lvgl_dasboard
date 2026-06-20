@echo off
chcp 65001 >nul
echo === Actualizador del Panel TC ===
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0update_panel.ps1"
pause
