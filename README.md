# PS2 플랫폼 게임 (PS2SDK + gsKit)

PlayStation 2 홈브류 2D 플랫폼 게임 프로젝트입니다.

- 엔진: `PS2SDK`, `gsKit`, `dmaKit`, `audsrv`
- 대상: PS2 실기기, PCSX2
- 언어: C (EE 사이드)

## 주요 기능

- 타일 기반 횡스크롤 플랫폼 게임
- 텍스트 파일 기반 다중 레벨 로딩
- 플레이어 애니메이션 상태 머신 (idle/walk/run/jump/hurt/clear/dead)
- `audsrv` 기반 PCM WAV BGM 스트리밍
- 2단계 에셋 파이프라인:
  1. CD/host -> RAM
  2. RAM -> VRAM
- 스프라이트 팩(`sprites.pak`) 로딩 + 개별 `.ps2tex` 폴백 지원

## 프로젝트 구조

```text
.
|- main.c
|- build.sh
|- Makefile
|- tools/
|  |- make_iso.sh
|  |- png_to_ps2tex.py
|  |- make_spritepack.py
|- src/
|  |- types.h
|  |- system.c/.h
|  |- audio.c/.h
|  |- sprite.c/.h
|  |- asset.c/.h
|  |- animation.c/.h
|  |- level.c/.h
|  |- physics.c/.h
|  |- render.c/.h
|- assets/
|- levels/
`- docs/
   `- ARCHITECTURE.md
```

모듈 상세 문서: `docs/ARCHITECTURE.md`

## 빌드 (권장)

Windows 경로 문제(`C:\...` vs `/c/...`)를 피하려면 `build.sh` 사용을 권장합니다.

```bash
bash build.sh          # 빌드
bash build.sh iso      # 빌드 + ISO 생성
bash build.sh clean    # 클린
bash build.sh rebuild  # 클린 + 전체 재빌드
```

생성 결과:

- ELF: `game_engine.elf`
- ISO: `game.iso` (`tools/make_iso.sh` 기본값)

## 대체 빌드 (Makefile)

쉘 환경이 정상 설정되어 있다면 아래 방식도 사용 가능합니다.

```bash
source ./env.sh
make
make iso
```

## 조작법

- `Left / Right`: 이동
- `Cross (X)`: 달리기 (이동 중)
- `Circle (O)`: 점프
- `R1`: 다음 레벨
- `L1`: 이전 레벨
- `Start`: 재시작 (게임 오버 시)

## 레벨 포맷

레벨은 `levels/levels.txt`에 나열된 텍스트 파일을 사용합니다.

예시:

```text
width=40
height=14
spawn=2,10
data:
########################################
#......................................#
#..............G.......................#
#....###.....................X.........#
########################################
```

타일 규칙:

- `#`: 고체 블록
- `.`: 빈 공간
- `G`: 골
- `X`: 트랩

## 에셋

- 원본 PNG: `assets/src/*.png`
- 변환 텍스처: `assets/*.ps2tex`
- 패킹 결과: `sprites.pak`

에셋 변환은 `build.sh`에서 자동으로 처리됩니다.

## ISO 관련 참고

- ISO 생성 스크립트: `tools/make_iso.sh`
- PS2 호환을 위해 스테이징 파일은 평탄화(flat) + 대문자 변환
- `SYSTEM.CNF`는 CRLF 줄바꿈으로 생성

`game.iso`가 잠겨 있을 때(예: PCSX2에서 사용 중)는 앱을 종료하거나 다른 파일명으로 생성하세요.

```bash
bash tools/make_iso.sh game_new.iso
```

## 요구 사항

- `/c/Users/jm/Documents/ps2dev` 아래 PS2 툴체인 설치
  - `ps2sdk`
  - `gsKit`
  - EE 컴파일러 (`mips64r5900el-ps2-elf-gcc`)
- MSYS2 bash 환경
- Python 3 (에셋 변환 스크립트용)
- ISO 생성 도구 (`xorriso`, `mkisofs`, `genisoimage` 중 하나)

## 문제 해결

### `/usr/bin/bash: ... No such file or directory` + `C:\Users\...gcc` 오류

원인: Windows 백슬래시 경로가 bash 명령에서 깨지면서 발생합니다.

해결: `bash build.sh` 사용(권장), 또는 모든 툴체인 경로를 MSYS2 스타일(`/c/...`)로 통일하세요.

### ISO 생성 시 `Device or resource busy`

원인: 대상 ISO 파일이 현재 열려 있거나 마운트되어 있습니다.

해결: 해당 파일을 사용하는 에뮬레이터/프로그램을 종료하거나 다른 출력 파일명으로 생성하세요.