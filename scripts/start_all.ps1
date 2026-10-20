# PTXScope Full-Stack Launcher (Windows PowerShell)
Write-Host "=== Starting PTXScope Local Workbench ===" -ForegroundColor Cyan

# 1. Start FastAPI Backend in background job
Write-Host "Starting FastAPI Backend on http://127.0.0.1:8000 ..." -ForegroundColor Yellow
$backendJob = Start-Job -ScriptBlock {
    param($root)
    Set-Location $root
    $env:PYTHONPATH = "backend"
    python -m uvicorn app.main:app --host 127.0.0.1 --port 8000
} -ArgumentList $PSScriptRoot\..

Start-Sleep -Seconds 2

# 2. Start Vite Frontend in background job
Write-Host "Starting Vite Frontend on http://localhost:5173 ..." -ForegroundColor Yellow
$frontendJob = Start-Job -ScriptBlock {
    param($root)
    Set-Location "$root\frontend"
    npm run dev
} -ArgumentList $PSScriptRoot\..

Write-Host "`nPTXScope is running!" -ForegroundColor Green
Write-Host "Frontend:  http://localhost:5173" -ForegroundColor Cyan
Write-Host "Backend:   http://127.0.0.1:8000" -ForegroundColor Cyan
Write-Host "API Docs:  http://127.0.0.1:8000/docs" -ForegroundColor Cyan
Write-Host "`nPress Ctrl+C to terminate both servers.`n"

try {
    while ($true) {
        Receive-Job $backendJob -ErrorAction SilentlyContinue | Write-Host
        Receive-Job $frontendJob -ErrorAction SilentlyContinue | Write-Host
        Start-Sleep -Seconds 1
    }
} finally {
    Write-Host "`nStopping servers..." -ForegroundColor Yellow
    Stop-Job $backendJob -ErrorAction SilentlyContinue
    Stop-Job $frontendJob -ErrorAction SilentlyContinue
    Remove-Job $backendJob -ErrorAction SilentlyContinue
    Remove-Job $frontendJob -ErrorAction SilentlyContinue
    Write-Host "Servers stopped." -ForegroundColor Green
}
