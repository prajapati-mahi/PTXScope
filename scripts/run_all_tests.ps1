# PTXScope Master Test Runner
$ErrorActionPreference = "Continue"

Write-Host ""
Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host "           PTXScope Master Test Runner                 " -ForegroundColor Cyan
Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host ""

$localAppData = $env:LOCALAPPDATA
$llvmBin = "$localAppData\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260908-ucrt-x86_64\bin"
if (Test-Path $llvmBin) {
    $env:Path = "$llvmBin;$env:Path"
}

$failed = 0

# Step 1: C++ Engine Tests
Write-Host "[1/3] Running C++ Compiler Engine Unit Tests..." -ForegroundColor Yellow
$engineTests = ".\build\engine\tests\ptxscope_tests.exe"
if (Test-Path $engineTests) {
    & $engineTests
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[FAIL] C++ Engine tests failed!" -ForegroundColor Red
        $failed++
    } else {
        Write-Host "[PASS] C++ Engine tests passed!" -ForegroundColor Green
    }
} else {
    Write-Host "[FAIL] Engine tests binary not found. Run ./scripts/build_engine.ps1 first." -ForegroundColor Red
    $failed++
}

# Step 2: Backend API Tests
Write-Host ""
Write-Host "[2/3] Running FastAPI Backend Integration Tests..." -ForegroundColor Yellow
$env:PYTHONPATH = "backend"
python -m pytest backend/tests -v
if ($LASTEXITCODE -ne 0) {
    Write-Host "[FAIL] Backend API tests failed!" -ForegroundColor Red
    $failed++
} else {
    Write-Host "[PASS] Backend API tests passed!" -ForegroundColor Green
}

# Step 3: Frontend Build & Typecheck
Write-Host ""
Write-Host "[3/3] Running Frontend Typecheck & Build..." -ForegroundColor Yellow
Push-Location frontend
npm run build
if ($LASTEXITCODE -ne 0) {
    Write-Host "[FAIL] Frontend build failed!" -ForegroundColor Red
    $failed++
} else {
    Write-Host "[PASS] Frontend build succeeded!" -ForegroundColor Green
}
Pop-Location

Write-Host ""
Write-Host "=======================================================" -ForegroundColor Cyan
if ($failed -eq 0) {
    Write-Host "   ALL TESTS PASSED! PTXScope is 100% verified.       " -ForegroundColor Green
} else {
    Write-Host "   $failed TEST SUITE(S) FAILED.                     " -ForegroundColor Red
}
Write-Host "=======================================================" -ForegroundColor Cyan
Write-Host ""

exit $failed
