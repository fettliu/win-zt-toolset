# Build libzt with MinGW-GCC for Windows
# This script compiles the ZeroTier Sockets library

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Building libzt with MinGW-GCC" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Configuration - Use gcc/g++ from PATH or environment variable
$GCC = if ($env:CC) { $env:CC } else { "gcc" }
$GPP = if ($env:CXX) { $env:CXX } else { "g++" }
$CMAKE = "cmake"

# Check if compilers exist in PATH
try {
    $null = Get-Command $GCC -ErrorAction Stop
} catch {
    Write-Host "[ERROR] GCC compiler not found in PATH" -ForegroundColor Red
    Write-Host "[INFO] Please install MinGW and add it to your PATH, or set CC/CXX environment variables" -ForegroundColor Yellow
    Write-Host "[INFO] Example: `$env:CC='gcc'; `$env:CXX='g++'" -ForegroundColor Gray
    exit 1
}

try {
    $null = Get-Command $GPP -ErrorAction Stop
} catch {
    Write-Host "[ERROR] G++ compiler not found in PATH" -ForegroundColor Red
    Write-Host "[INFO] Please install MinGW and add it to your PATH, or set CXX environment variable" -ForegroundColor Yellow
    exit 1
}

Write-Host "[INFO] Using GCC: $GCC" -ForegroundColor Green
Write-Host "[INFO] Using G++: $GPP" -ForegroundColor Green
Write-Host ""

# Check if libzt submodule exists
if (-not (Test-Path "libzt/CMakeLists.txt")) {
    Write-Host "[ERROR] libzt submodule not found!" -ForegroundColor Red
    Write-Host "[INFO] Please run: git submodule update --init --recursive" -ForegroundColor Yellow
    exit 1
}

# Create build directory
$BUILD_DIR = "libzt-build"
if (Test-Path $BUILD_DIR) {
    Write-Host "[INFO] Cleaning existing build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BUILD_DIR
}
New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null

Write-Host "[INFO] Configuring CMake for MinGW..." -ForegroundColor Yellow
Set-Location $BUILD_DIR

# Configure CMake
& $CMAKE ..\libzt `
    -G "MinGW Makefiles" `
    -DCMAKE_C_COMPILER="$GCC" `
    -DCMAKE_CXX_COMPILER="$GPP" `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_STATIC_LIB=ON `
    -DBUILD_SHARED_LIB=OFF `
    -DBUILD_HOST_EXAMPLES=OFF `
    -DBUILD_HOST_SELFTEST=OFF `
    -DOMIT_JSON_SUPPORT=1 `
    -DZTS_ENABLE_STATS=1

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake configuration failed!" -ForegroundColor Red
    Set-Location ..
    exit 1
}

Write-Host ""
Write-Host "[INFO] Building libzt..." -ForegroundColor Yellow
& mingw32-make -j2

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Build failed!" -ForegroundColor Red
    Set-Location ..
    exit 1
}

# Return to project root
Set-Location ..

# Copy build artifacts
Write-Host ""
Write-Host "[INFO] Copying build artifacts..." -ForegroundColor Yellow

# Create build directory if not exists
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}
if (-not (Test-Path "build/include")) {
    New-Item -ItemType Directory -Path "build/include" | Out-Null
}

# Copy static library
$LIB_PATH = "$BUILD_DIR/lib/libzt-static.a"
if (Test-Path $LIB_PATH) {
    Copy-Item $LIB_PATH "build/libzt.a" -Force
    Write-Host "[OK] Copied libzt.a" -ForegroundColor Green
} else {
    Write-Host "[ERROR] libzt-static.a not found at $LIB_PATH" -ForegroundColor Red
    exit 1
}

# Copy header file
$HEADER_PATH = "libzt/include/ZeroTierSockets.h"
if (Test-Path $HEADER_PATH) {
    Copy-Item $HEADER_PATH "build/include/" -Force
    Write-Host "[OK] Copied ZeroTierSockets.h" -ForegroundColor Green
} else {
    Write-Host "[ERROR] ZeroTierSockets.h not found" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Build Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

# Show build artifacts
Get-Item "build/libzt.a" | Select-Object Name, @{N='Size(KB)';E={[math]::Round($_.Length/1KB,2)}}, LastWriteTime
Write-Host ""
Write-Host "Output files:" -ForegroundColor Cyan
Write-Host "  - build/libzt.a (static library)" -ForegroundColor White
Write-Host "  - build/include/ZeroTierSockets.h (header file)" -ForegroundColor White
Write-Host ""
