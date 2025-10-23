@echo off
REM === Проверяем, установлен ли Python ===
where python >nul 2>nul
if errorlevel 1 (
    echo Python не найден. Установи Python с https://www.python.org/downloads/
    pause
    exit /b
)

REM === Запускаем локальный сервер на порту 8080 ===
echo Запуск локального сервера...
start "" "http://localhost:8080/index.html"
python -m http.server 8080
