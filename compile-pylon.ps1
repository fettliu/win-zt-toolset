# Compile pylon.exe (SOCKS5 + HTTP CONNECT proxy)
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Compiling pylon.exe" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Use g++ from PATH or environment variable
$GPP = if ($env:CXX) { $env:CXX } else { "g++" }

try {
    $null = Get-Command $GPP -ErrorAction Stop
} catch {
    Write-Host "[ERROR] G++ compiler not found in PATH" -ForegroundColor Red
    Write-Host "[INFO] Please install MinGW and add it to your PATH" -ForegroundColor Yellow
    Write-Host "[INFO] Or set CXX environment variable: `$env:CXX='g++'" -ForegroundColor Gray
    exit 1
}

Write-Host "[INFO] Using G++: $GPP" -ForegroundColor Green
Write-Host ""

# Compile pylon.c with g++ (because libzt.a is C++)
Write-Host "Step 1: Compiling pylon.c..." -ForegroundColor Yellow

& $GPP -o build/pylon.exe pylon.c `
    -DADD_EXPORTS `
    -I./build/include `
    ./build/libzt.a `
    -static-libgcc -static-libstdc++ `
    -lwsock32 -lws2_32 -liphlpapi -lshlwapi `
    -O2 `
    -Wall 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Compilation failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "✓✓✓ Compilation SUCCESS!" -ForegroundColor Green
Write-Host ""
Write-Host "Usage:" -ForegroundColor Cyan
Write-Host "  .\build\pylon.exe" -ForegroundColor White
Write-Host ""
Write-Host "Features:" -ForegroundColor Yellow
Write-Host "  - SOCKS5 proxy support" -ForegroundColor Gray
Write-Host "  - HTTP CONNECT tunnel support" -ForegroundColor Gray
Write-Host "  - Auto-detect protocol" -ForegroundColor Gray
Write-Host "  - Listen on port 5888" -ForegroundColor Gray
Write-Host ""
