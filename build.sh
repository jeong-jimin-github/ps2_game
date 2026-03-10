#!/bin/bash
# ============================================================
#  PS2 플랫포머 - 올인원 빌드 & ISO 생성 스크립트
#  사용법:  bash build.sh          (빌드만)
#           bash build.sh iso      (빌드 + ISO)
#           bash build.sh clean    (클린)
#           bash build.sh rebuild  (클린 + 빌드)
# ============================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ── 환경 설정 (MSYS2 유닉스 경로) ──────────────────────────
export PS2DEV=/c/Users/jm/Documents/ps2dev
export PS2SDK=$PS2DEV/ps2sdk
export GSKIT=$PS2DEV/gsKit
export PATH="$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2DEV/dvp/bin:$PS2SDK/bin:$PATH"
export PATH="/c/msys64/mingw32/bin:/c/msys64/mingw64/bin:$PATH"

# Python 찾기
PYTHON=""
if command -v /c/Users/jm/AppData/Local/Programs/Python/Python310/python.exe &>/dev/null; then
    PYTHON="/c/Users/jm/AppData/Local/Programs/Python/Python310/python.exe"
elif command -v python3 &>/dev/null; then
    PYTHON="python3"
elif command -v python &>/dev/null; then
    PYTHON="python"
elif command -v py &>/dev/null; then
    PYTHON="py -3"
fi

# ── 도구 경로 ──────────────────────────────────────────────
CC="$PS2DEV/ee/bin/mips64r5900el-ps2-elf-gcc"
AS="$PS2DEV/ee/bin/mips64r5900el-ps2-elf-as"
AR="$PS2DEV/ee/bin/mips64r5900el-ps2-elf-ar"
STRIP="$PS2DEV/ee/bin/mips64r5900el-ps2-elf-strip"

# 컴파일러 존재 확인
if [ ! -f "$CC" ] && [ ! -f "${CC}.exe" ]; then
    echo "ERROR: EE compiler not found at $CC"
    echo "       PS2DEV=$PS2DEV 경로를 확인하세요."
    exit 1
fi

# ── 컴파일 플래그 ──────────────────────────────────────────
ELF_NAME="game_engine.elf"

CFLAGS="-D_EE -G0 -O2 -Wall -gdwarf-2 -gz"
CFLAGS="$CFLAGS -I$PS2SDK/ee/include"
CFLAGS="$CFLAGS -I$PS2SDK/common/include"
CFLAGS="$CFLAGS -I."
CFLAGS="$CFLAGS -I$GSKIT/include"
CFLAGS="$CFLAGS -I$GSKIT/ee/include"
CFLAGS="$CFLAGS -I$PS2SDK/ports/include"

LDFLAGS="-T$PS2SDK/ee/startup/linkfile -O2"
LDFLAGS="$LDFLAGS -L$PS2SDK/ee/lib -Wl,-zmax-page-size=128"
LDFLAGS="$LDFLAGS -L$GSKIT/lib -L$GSKIT/ee/lib"
LDFLAGS="$LDFLAGS -L$PS2SDK/ports/lib"

LIBS="-lgskit -ldmakit -lpad -laudsrv -lpatches -lc -ldebug"

# CRT 오브젝트 (PS2SDK 링크에 필요)
CRT0="$PS2SDK/ee/startup/crt0.o"
LINKFILE="$PS2SDK/ee/startup/linkfile"

# ── 소스 파일 목록 ────────────────────────────────────────
SRCS=(
    main.c
    src/system.c
    src/audio.c
    src/sprite.c
    src/asset.c
    src/animation.c
    src/level.c
    src/physics.c
    src/render.c
)

# ── 함수 정의 ─────────────────────────────────────────────

do_clean() {
    echo "=== Clean ==="
    rm -f "$ELF_NAME"
    rm -f assets/*.ps2tex sprites.pak
    for src in "${SRCS[@]}"; do
        rm -f "${src%.c}.o"
    done
    echo "Done."
}

do_assets() {
    echo "=== Converting sprites ==="
    if [ -z "$PYTHON" ]; then
        echo "WARNING: Python not found, skipping asset conversion."
        echo "         수동으로 에셋을 변환하세요."
        return
    fi

    # PNG → PS2TEX
    if [ -d "assets/src" ] && ls assets/src/*.png &>/dev/null 2>&1; then
        $PYTHON tools/png_to_ps2tex.py --all assets/src assets
    else
        echo "  (assets/src에 PNG 없음, 스킵)"
    fi

    # 스프라이트 팩
    if ls assets/*.ps2tex &>/dev/null 2>&1; then
        $PYTHON tools/make_spritepack.py assets sprites.pak
    else
        echo "  (ps2tex 파일 없음, 팩 생성 스킵)"
    fi
}

do_compile() {
    echo "=== Building $ELF_NAME ==="

    OBJS=()
    for src in "${SRCS[@]}"; do
        obj="${src%.c}.o"
        OBJS+=("$obj")

        # 소스가 오브젝트보다 새로우면 재컴파일
        if [ "$src" -nt "$obj" ] 2>/dev/null || [ ! -f "$obj" ]; then
            echo "  CC  $src"
            "$CC" $CFLAGS -c "$src" -o "$obj"
        else
            echo "  (skip) $src (up to date)"
        fi
    done

    echo "  LD  $ELF_NAME"
    "$CC" $LDFLAGS -o "$ELF_NAME" "${OBJS[@]}" $LIBS

    echo "=== Build OK: $ELF_NAME ==="
}

do_iso() {
    echo ""
    echo "=== Creating ISO ==="
    bash tools/make_iso.sh
}

# ── 메인 ──────────────────────────────────────────────────
case "${1:-build}" in
    clean)
        do_clean
        ;;
    rebuild)
        do_clean
        do_assets
        do_compile
        ;;
    iso)
        do_assets
        do_compile
        do_iso
        ;;
    build|"")
        do_assets
        do_compile
        ;;
    *)
        echo "Usage: bash build.sh [build|iso|clean|rebuild]"
        exit 1
        ;;
esac
