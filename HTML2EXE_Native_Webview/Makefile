# ─────────────────────────────────────────────────────────────────────────────
# HTML2EXE Native WebView Host – Makefile
#
# Builds Webview.exe with ZERO .NET runtime dependency.
#
# Cross-compile on Ubuntu (GitHub Actions CI):
#   sudo apt-get install -y mingw-w64
#   make CXX=x86_64-w64-mingw32-g++ CC=x86_64-w64-mingw32-gcc RC=x86_64-w64-mingw32-windres
#
# Compile on Windows (MSYS2 / MinGW-w64):
#   make
# ─────────────────────────────────────────────────────────────────────────────

CXX ?= x86_64-w64-mingw32-g++
CC  ?= x86_64-w64-mingw32-gcc
RC  ?= x86_64-w64-mingw32-windres

TARGET    = Webview.exe
SRC       = main.cpp
MINIZ_SRC = miniz.c
MINIZ_OBJ = miniz.o
RES_SRC   = resource.rc
RES_OBJ   = resource.o

WV2_SDK ?= ../webview2-sdk
WV2_INC = $(WV2_SDK)/build/native/include

CFLAGS = \
  -O2 \
  -Wall \
  -Wno-unused-function \
  -Wno-unused-variable \
  -D_FILE_OFFSET_BITS=64

CXXFLAGS = \
  -std=c++17 \
  -O2 \
  -Wall \
  -Wno-unknown-pragmas \
  -mwindows \
  -municode \
  -DUNICODE \
  -D_UNICODE \
  -DWINVER=0x0601 \
  -D_WIN32_WINNT=0x0601 \
  -I. \
  -I$(WV2_INC)

LDFLAGS = \
  -mwindows \
  -municode \
  -static-libgcc \
  -static-libstdc++ \
  -lole32 \
  -loleaut32 \
  -lshell32 \
  -luuid \
  -lshlwapi

.PHONY: all clean

all: $(TARGET)

$(MINIZ_OBJ): $(MINIZ_SRC) miniz.h
	$(CC) $(CFLAGS) -c $(MINIZ_SRC) -o $@

$(RES_OBJ): $(RES_SRC) app.manifest
	$(RC) -o $@ $<

$(TARGET): $(SRC) $(MINIZ_OBJ) $(RES_OBJ) EventToken.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(MINIZ_OBJ) $(RES_OBJ) $(LDFLAGS)
	@echo "Built $(TARGET) successfully"

clean:
	rm -f $(RES_OBJ) $(MINIZ_OBJ) $(TARGET)
