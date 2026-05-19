# ZeroTier Tools Makefile
# Usage: make [target]
# Targets: all, zt-join, zt-forward, pylon, zt-ping, clean, libzt

# Use gcc/g++ from PATH (can be overridden by environment variables CC/CXX)
CC ?= gcc
CXX ?= g++

CFLAGS = -DADD_EXPORTS -O2 -Wall
INCLUDES = -Ibuild/include
LDFLAGS = -Wl,--defsym,__imp__snprintf=_snprintf -Lbuild -lzt \
          -static-libgcc -static-libstdc++ -lpthread \
          -lws2_32 -lwsock32 -liphlpapi -lshlwapi

.PHONY: all clean libzt zt-join zt-forward pylon zt-ping check-compiler

all: check-compiler build/libzt.a zt-join zt-forward pylon zt-ping
	@echo ""
	@echo "========================================"
	@echo "All tools built successfully!"
	@echo "========================================"

check-compiler:
	@which $(CC) > /dev/null 2>&1 || (echo "[ERROR] GCC compiler not found in PATH" && echo "[INFO] Please install MinGW and add it to your PATH" && exit 1)
	@which $(CXX) > /dev/null 2>&1 || (echo "[ERROR] G++ compiler not found in PATH" && echo "[INFO] Please install MinGW G++ and add it to your PATH" && exit 1)
	@echo "[INFO] Using CC: $(CC)"
	@echo "[INFO] Using CXX: $(CXX)"

libzt: build/libzt.a

build/libzt.a:
	@echo "Building libzt..."
	powershell -ExecutionPolicy Bypass -File ./build-libzt.ps1

zt-join: build/libzt.a
	@echo "Building zt-join.exe..."
	$(CXX) $(CFLAGS) $(INCLUDES) -o build/zt-join.exe zt-join.c $(LDFLAGS)
	@echo "[OK] zt-join.exe built"

zt-forward: build/libzt.a
	@echo "Building zt-forward.exe..."
	$(CXX) $(CFLAGS) $(INCLUDES) -o build/zt-forward.exe zt-forward.c $(LDFLAGS)
	@echo "[OK] zt-forward.exe built"

pylon: build/libzt.a
	@echo "Building pylon.exe..."
	$(CXX) $(CFLAGS) $(INCLUDES) -o build/pylon.exe pylon.c $(LDFLAGS)
	@echo "[OK] pylon.exe built"

zt-ping: build/libzt.a
	@echo "Building zt-ping.exe..."
	$(CXX) $(CFLAGS) $(INCLUDES) -o build/zt-ping.exe zt-ping.c $(LDFLAGS)
	@echo "[OK] zt-ping.exe built"

clean:
	@echo "Cleaning build directory..."
	rm -rf build
	@echo "[OK] Cleaned"

help:
	@echo "Available targets:"
	@echo "  all        - Build all tools (default)"
	@echo "  libzt      - Build libzt library only"
	@echo "  zt-join    - Build network join tool"
	@echo "  zt-forward - Build port forwarder"
	@echo "  pylon      - Build SOCKS5/HTTP proxy"
	@echo "  zt-ping    - Build ping tool"
	@echo "  clean      - Clean build directory"
	@echo "  help       - Show this help"
