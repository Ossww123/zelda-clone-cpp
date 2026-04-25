@echo off
docker compose up -d
timeout /t 2 /nobreak >nul
start cmd /k "docker compose exec redis redis-cli MONITOR"
