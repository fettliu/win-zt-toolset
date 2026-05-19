# Unified Build Script for ZeroTier Tools
# Usage: .\build.ps1 [target]
# Targets: all, zt-join, zt-forward, pylon, zt-ping, clean, libzt

param(
    [string]$Target = "all"
)

# Use gcc/g++ from PATH or environment variables
$GCC = if ($env:CC) { $env:CC } else { "gcc" }
$GPP = if ($env:CXX) { $env:CXX } else { "g++" }

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "ZeroTier Tools Build System" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check compilers
try {
    $null = Get-Command $GCC -ErrorAction Stop
} catch {
    Write-Host "[ERROR] GCC compiler not found in PATH" -ForegroundColor Red
    Write-Host "[INFO] Please install MinGW and add it to your PATH" -ForegroundColor Yellow
    Write-Host "[INFO] Or set environment variables: CC=gcc, CXX=g++" -ForegroundColor Gray
    exit 1
}

try {
    $null = Get-Command $GPP -ErrorAction Stop
} catch {
    Write-Host "[ERROR] G++ compiler not found in PATH" -ForegroundColor Red
    Write-Host "[INFO] Please install MinGW G++ and add it to your PATH" -ForegroundColor Yellow
    exit 1
}

Write-Host "[INFO] Using GCC: $GCC" -ForegroundColor Green
Write-Host "[INFO] Using G++: $GPP" -ForegroundColor Green
Write-Host ""

function Build-LibZT {
    Write-Host "Building libzt..." -ForegroundColor Yellow
    if (Test-Path ".\build-libzt.ps1") {
        & ".\build-libzt.ps1"
        return $LASTEXITCODE -eq 0
    } else {
        Write-Host "[ERROR] build-libzt.ps1 not found" -ForegroundColor Red
        return $false
    }
}

function Build-ZTJoin {
    Write-Host "Building zt-join.exe..." -ForegroundColor Yellow
    
    $CFLAGS = @("-DADD_EXPORTS", "-Os", "-Wall")
    $INCLUDES = @("-Ibuild\include")
    $LDFLAGS = @("-Wl,--defsym,__imp__snprintf=_snprintf", "-Lbuild", "-lzt", 
                 "-static-libgcc", "-static-libstdc++", "-lpthread",
                 "-lws2_32", "-lwsock32", "-liphlpapi", "-lshlwapi")
    
    & $GPP @CFLAGS @INCLUDES -o "build\zt-join.exe" "zt-join.c" @LDFLAGS
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] zt-join.exe built successfully" -ForegroundColor Green
        return $true
    } else {
        Write-Host "[ERROR] Failed to build zt-join.exe" -ForegroundColor Red
        return $false
    }
}

function Build-ZTForward {
    Write-Host "Building zt-forward.exe..." -ForegroundColor Yellow
    
    $CFLAGS = @("-DADD_EXPORTS", "-Os", "-Wall")
    $INCLUDES = @("-Ibuild\include")
    $LDFLAGS = @("-Wl,--defsym,__imp__snprintf=_snprintf", "-Lbuild", "-lzt",
                 "-static-libgcc", "-static-libstdc++", "-lpthread",
                 "-lws2_32", "-lwsock32", "-liphlpapi", "-lshlwapi")
    
    & $GPP @CFLAGS @INCLUDES -o "build\zt-forward.exe" "zt-forward.c" @LDFLAGS
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] zt-forward.exe built successfully" -ForegroundColor Green
        return $true
    } else {
        Write-Host "[ERROR] Failed to build zt-forward.exe" -ForegroundColor Red
        return $false
    }
}

function Build-Pylon {
    Write-Host "Building pylon.exe..." -ForegroundColor Yellow
    
    $CFLAGS = @("-DADD_EXPORTS", "-Os", "-Wall")
    $INCLUDES = @("-Ibuild\include")
    $LDFLAGS = @("-Wl,--defsym,__imp__snprintf=_snprintf", "-Lbuild", "-lzt",
                 "-static-libgcc", "-static-libstdc++", "-lpthread",
                 "-lws2_32", "-lwsock32", "-liphlpapi", "-lshlwapi")
    
    & $GPP @CFLAGS @INCLUDES -o "build\pylon.exe" "pylon.c" @LDFLAGS
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] pylon.exe built successfully" -ForegroundColor Green
        return $true
    } else {
        Write-Host "[ERROR] Failed to build pylon.exe" -ForegroundColor Red
        return $false
    }
}

function Build-ZTPing {
    Write-Host "Building zt-ping.exe..." -ForegroundColor Yellow
    
    $CFLAGS = @("-DADD_EXPORTS", "-Os", "-Wall")
    $INCLUDES = @("-Ibuild\include")
    $LDFLAGS = @("-Wl,--defsym,__imp__snprintf=_snprintf", "-Lbuild", "-lzt",
                 "-static-libgcc", "-static-libstdc++", "-lpthread",
                 "-lws2_32", "-lwsock32", "-liphlpapi", "-lshlwapi")
    
    & $GPP @CFLAGS @INCLUDES -o "build\zt-ping.exe" "zt-ping.c" @LDFLAGS
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] zt-ping.exe built successfully" -ForegroundColor Green
        return $true
    } else {
        Write-Host "[ERROR] Failed to build zt-ping.exe" -ForegroundColor Red
        return $false
    }
}

function Clean-Build {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path "build") {
        Remove-Item -Recurse -Force "build"
        Write-Host "[OK] Build directory cleaned" -ForegroundColor Green
    }
}

# Main logic
switch ($Target.ToLower()) {
    "libzt" {
        Build-LibZT
    }
    "zt-join" {
        if (-not (Test-Path "build\libzt.a")) {
            Build-LibZT
        }
        Build-ZTJoin
    }
    "zt-forward" {
        if (-not (Test-Path "build\libzt.a")) {
            Build-LibZT
        }
        Build-ZTForward
    }
    "pylon" {
        if (-not (Test-Path "build\libzt.a")) {
            Build-LibZT
        }
        Build-Pylon
    }
    "zt-ping" {
        if (-not (Test-Path "build\libzt.a")) {
            Build-LibZT
        }
        Build-ZTPing
    }
    "clean" {
        Clean-Build
    }
    "all" {
        if (-not (Test-Path "build\libzt.a")) {
            if (-not (Build-LibZT)) {
                exit 1
            }
        }
        
        $success = $true
        $success = $success -and (Build-ZTJoin)
        $success = $success -and (Build-ZTForward)
        $success = $success -and (Build-Pylon)
        $success = $success -and (Build-ZTPing)
        
        if ($success) {
            Write-Host ""
            Write-Host "========================================" -ForegroundColor Green
            Write-Host "All tools built successfully!" -ForegroundColor Green
            Write-Host "========================================" -ForegroundColor Green
        } else {
            Write-Host ""
            Write-Host "Some builds failed. Check errors above." -ForegroundColor Red
            exit 1
        }
    }
    default {
        Write-Host "[ERROR] Unknown target: $Target" -ForegroundColor Red
        Write-Host "Available targets: all, zt-join, zt-forward, pylon, zt-ping, clean, libzt" -ForegroundColor Yellow
        exit 1
    }
}
