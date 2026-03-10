EE_BIN = game_engine.elf
EE_OBJS = main.o src/system.o src/audio.o src/sprite.o src/asset.o \
         src/animation.o src/level.o src/physics.o src/render.o

SHELL := /usr/bin/bash

ENV_SH := $(abspath ./env.sh)
ifneq ($(wildcard $(ENV_SH)),)
export BASH_ENV := $(ENV_SH)
PS2DEV ?= $(shell bash -lc '. "$(ENV_SH)" >/dev/null 2>&1; printf "%s" "$$PS2DEV"')
PS2SDK ?= $(shell bash -lc '. "$(ENV_SH)" >/dev/null 2>&1; printf "%s" "$$PS2SDK"')
PYTHON ?= $(shell bash -lc '. "$(ENV_SH)" >/dev/null 2>&1; printf "%s" "$$PYTHON"')
endif

PYTHON ?= python3

# On Windows cmd, PATH separator is ';' and ':'-based overrides can break lookup.
# Prefer explicit tool prefix so gcc/as/ld are resolved without relying on PATH shape.
ifeq ($(OS),Windows_NT)
EE_TOOL_PREFIX ?= $(PS2DEV)/ee/bin/mips64r5900el-ps2-elf-
endif

# 1. gsKit이 설치된 경로를 변수로 지정 (MSYS2 방식 경로)
GSKIT = $(PS2DEV)/gsKit

# 2. 헤더 파일 경로 추가 (여러 개일 경우 한 칸 띄우고 추가)
# -I 옵션 뒤에 경로를 붙입니다.
EE_INCS += -I$(GSKIT)/include -I$(GSKIT)/ee/include -I$(PS2SDK)/ports/include -I.

# 3. 라이브러리 파일 경로 추가
# -L 옵션 뒤에 경로를 붙입니다.
EE_LDFLAGS += -L$(GSKIT)/lib -L$(GSKIT)/ee/lib -L$(PS2SDK)/ports/lib

# 4. 링크할 라이브러리 목록
EE_LIBS = -lgskit -ldmakit -lpad -laudsrv -lpatches -lc -ldebug

.PHONY: all clean assets convert-sprites iso

all: $(EE_BIN)

iso: $(EE_BIN)
	bash tools/make_iso.sh

$(EE_OBJS): assets

convert-sprites:
	@if command -v $(PYTHON) >/dev/null 2>&1; then \
		$(PYTHON) tools/png_to_ps2tex.py --all assets/src assets; \
	elif command -v python >/dev/null 2>&1; then \
		python tools/png_to_ps2tex.py --all assets/src assets; \
	elif command -v py >/dev/null 2>&1; then \
		py -3 tools/png_to_ps2tex.py --all assets/src assets; \
	else \
		echo "No Python interpreter found (tried: $(PYTHON), python, py -3)"; \
		exit 127; \
	fi

assets: convert-sprites
	@if command -v $(PYTHON) >/dev/null 2>&1; then \
		$(PYTHON) tools/make_spritepack.py assets sprites.pak; \
	elif command -v python >/dev/null 2>&1; then \
		python tools/make_spritepack.py assets sprites.pak; \
	elif command -v py >/dev/null 2>&1; then \
		py -3 tools/make_spritepack.py assets sprites.pak; \
	fi

clean:
	rm -f $(EE_BIN) $(EE_OBJS) assets/*.ps2tex sprites.pak

# PS2SDK 기본 규칙 포함
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal