# ─────────────────────────────────────────────────────────────────────────────
# HTML2EXE Native WebView Host – Makefile
#
# Targets:
#   all    – build Webview.exe (default)
#   clean  – remove build artefacts
#
# Cross-compile on Ubuntu (CI):
#   sudo apt-get install -y mingw-w64
#   make CC=x86_64-w64-mingw32-g++ RC=x86_64-w64-mingw32-windres
#
# Compile on Windows (MSYS2/MinGW-w64):
#   make
# ─────────────────────────────────────────────────────────────────────────────

CC  ?= x86_64-w64-mingw32-g++
RC  ?= x86_64-w64-mingw32-windres

TARGET  = Webview.exe
SRC     = main.cpp
RES_SRC = resource.rc
RES_OBJ = resource.o

# WebView2 SDK downloaded by the CI publish.yml step.
# Local build: set WV2_SDK to the extracted NuGet package directory.
WV2_SDK ?= webview2-sdk

WV2_INC      = $(WV2_SDK)/build/native/include
WV2_IMPLIB   = $(WV2_SDK)/build/native/x64/WebView2Loader.dll.lib

CXXFLAGS = \
  -std=c++17 \
  -O2 \
  -Wall \
  -mwindows \
  -DUNICODE \
  -D_UNICODE \
  -DWINVER=0x0601 \
  -D_WIN32_WINNT=0x0601 \
  -I$(WV2_INC)

# Pass the .dll.lib (MSVC-style import library) as a positional linker arg.
# MinGW can read MSVC import libs directly when given as a path argument.
LDFLAGS = \
  -mwindows \
  -static-libgcc \
  -static-libstdc++ \
  -lole32 \
  -loleaut32 \
  -lshell32 \
  -luuid \
  -lshlwapi

.PHONY: all clean

all: $(TARGET)

$(RES_OBJ): $(RES_SRC) app.manifest
	$(RC) -o $@ $<

$(TARGET): $(SRC) $(RES_OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(WV2_IMPLIB) $(LDFLAGS)
	@echo "Built $@ (size: $$(stat -c%s $@ 2>/dev/null || stat -f%z $@) bytes)"

clean:
	rm -f $(RES_OBJ) $(TARGET)
