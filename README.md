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

## 빌드 (권장)

```bash
bash build.sh          # 빌드
bash build.sh iso      # 빌드 + ISO 생성
bash build.sh clean    # 클린
bash build.sh rebuild  # 클린 + 전체 재빌드
```

생성 결과:

- ELF: `game_engine.elf`
- ISO: `SLUS_209.99.PS2_PLATFORMER.iso` (`tools/make_iso.sh` 기본값)

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

## 레벨 에디터 (GUI)

`tools/level_editor.py`로 레벨 텍스트를 시각적으로 편집할 수 있습니다.

```bash
python tools/level_editor.py levels/level1.txt
```

주요 기능:

- 타일 페인팅 (`#`, `.`, `G`, `X`)
- 마우스로 스폰 좌표(`spawn=x,y`) 지정
- 레벨 크기(`width`, `height`) 조정
- `levels/*.txt` 형식으로 저장

## ISO 관련 참고

- ISO 생성 스크립트: `tools/make_iso.sh`
- PS2 호환을 위해 스테이징 파일은 평탄화(flat) + 대문자 변환
- `SYSTEM.CNF`는 CRLF 줄바꿈으로 생성
- 기본 부팅 파일명은 실게임 스타일의 ID 형식(`SLUS_209.99`)으로 생성
- 기본 ISO 파일명은 OPL 친화 형식(`GAMEID.TITLE.iso`)으로 생성

게임 ID/타이틀 지정:

```bash
PS2_GAME_ID=SLUS_209.99 PS2_GAME_TITLE="SUPER BLOCK ADVENTURE" bash build.sh iso
```

- `PS2_GAME_ID` 형식: `AAAA_123.45`
- `PS2_GAME_TITLE`은 OPL 파일명 규칙에 맞게 자동 정규화(공백/특수문자 -> `_`)

생성 대상 ISO가 잠겨 있을 때(예: PCSX2에서 사용 중)는 앱을 종료하거나 다른 파일명으로 생성하세요.

```bash
bash tools/make_iso.sh game_new.iso
```

## 요구 사항

- `ps2sdk`
- `MSYS2`
- Python3
- `xorriso`