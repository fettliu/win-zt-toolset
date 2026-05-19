# Compile zt-join.exe with MinGW g++
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Compiling zt-join.exe" -ForegroundColor Cyan
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

# Check if libzt.a exists
if (-not (Test-Path "build\libzt.a")) {
    Write-Host "[ERROR] libzt.a not found in build directory" -ForegroundColor Red
    Write-Host "[INFO] Please run .\build-libzt.ps1 first" -ForegroundColor Yellow
    exit 1
}

# Link zt-join.exe with g++ (because libzt.a is C++)
Write-Host "Linking zt-join.exe..." -ForegroundColor Yellow
$CFLAGS = @("-DADD_EXPORTS", "-std=c++11")
$INCLUDES = @("-Ibuild\include")
$LDFLAGS = @("-Wl,--defsym,__imp__snprintf=_snprintf", "-Lbuild", "-lzt", 
             "-static-libgcc", "-static-libstdc++", "-lpthread",
             "-lws2_32", "-lwsock32", "-liphlpapi", "-lshlwapi")

& $MINGW_GPP @CFLAGS @INCLUDES -o "build\zt-join.exe" "zt-join.c" @LDFLAGS

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "✓✓✓ Compilation SUCCESS!" -ForegroundColor Green
    Write-Host ""
    Get-Item "build\zt-join.exe" | Select-Object Name, @{N='Size(KB)';E={[math]::Round($_.Length/1KB,2)}}, LastWriteTime
} else {
    Write-Host ""
    Write-Host "✗✗✗ Compilation FAILED (exit code: $LASTEXITCODE)" -ForegroundColor Red
}

Write-Host ""
