# PTXScope Engine Build Script (Windows PowerShell)
$ErrorActionPreference = "Stop"

Write-Host "=== Building PTXScope C++ Compiler Engine ===" -ForegroundColor Cyan

# Setup Toolchain Environment
$localAppData = $env:LOCALAPPDATA
$llvmBin = "$localAppData\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260908-ucrt-x86_64\bin"
$pyScripts = "C:\Users\mahii\AppData\Roaming\Python\Python312\Scripts"

if (Test-Path $llvmBin) {
    $env:Path = "$llvmBin;$pyScripts;$env:Path"
}

cmake -B build -G "Ninja" -DCMAKE_CXX_COMPILER=clang++
cmake --build build

Write-Host "=== Engine build complete ===" -ForegroundColor Green
Write-Host "Executable: build/engine/ptxscope.exe" -ForegroundColor Yellow
Write-Host "Tests:      build/engine/tests/ptxscope_tests.exe" -ForegroundColor Yellow
