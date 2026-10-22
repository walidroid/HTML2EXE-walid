# ─────────────────────────────────────────────────────────────────────────────
# HTML2EXE Native WebView Host – Makefile
#
# Builds Webview.exe with ZERO .NET runtime dependency.
#
# Cross-compile on Ubuntu (GitHub Actions CI):
#   sudo apt-get install -y mingw-w64
#   make CC=x86_64-w64-mingw32-g++ RC=x86_64-w64-mingw32-windres
#
# Compile on Windows (MSYS2 / MinGW-w64):
#   make
# ─────────────────────────────────────────────────────────────────────────────

CC  ?= x86_64-w64-mingw32-g++
RC  ?= x86_64-w64-mingw32-windres

TARGET  = Webview.exe
SRC     = main.cpp
RES_SRC = resource.rc
RES_OBJ = resource.o

WV2_SDK ?= ../webview2-sdk
WV2_INC = $(WV2_SDK)/build/native/include

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
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built $(TARGET) successfully"

clean:
	rm -f $(RES_OBJ) $(TARGET)
