# PS2 플랫포머 - 소스 코드 구조 문서

## 프로젝트 개요

PlayStation 2용 2D 사이드스크롤 플랫포머 게임입니다.  
PS2SDK + gsKit을 사용하며, MIPS R5900 EE 크로스 컴파일러로 빌드합니다.

---

## 디렉토리 구조

```
ps2/
├── main.c                  # 엔트리 포인트 (게임 루프, 입력 처리)
├── Makefile                # 빌드 설정
├── env.sh                  # PS2DEV 환경변수 설정
├── audsrv.irx              # IOP 오디오 모듈
├── freesd.irx              # IOP 사운드 드라이버
├── sprites.pak             # 스프라이트 팩 (빌드 시 생성)
│
├── src/                    # 소스 모듈
│   ├── types.h             # 공통 타입, 상수, 구조체 정의
│   ├── system.h / .c       # PS2 시스템 (IOP 로딩, 파일 I/O)
│   ├── audio.h / .c        # BGM 재생 (WAV 파싱, audsrv 스트리밍)
│   ├── sprite.h / .c       # 스프라이트 로딩 (PS2TEX, PAK, VRAM 업로드)
│   ├── asset.h / .c        # 에셋 관리 (CD→RAM, RAM→VRAM 2단계)
│   ├── animation.h / .c    # 플레이어 애니메이션 상태 머신
│   ├── level.h / .c        # 레벨 파일 파싱, 타일 쿼리
│   ├── physics.h / .c      # 물리/충돌 (이동, 지면 판정)
│   └── render.h / .c       # 렌더링 (배경, 타일맵, 플레이어)
│
├── assets/                 # 에셋 파일
│   ├── src/                # PNG 원본 (빌드 시 PS2TEX로 변환)
│   ├── *.ps2tex            # 변환된 텍스처
│   └── README.txt          # 에셋 설명
│
├── levels/                 # 레벨 데이터
│   ├── levels.txt          # 레벨 목록
│   ├── level1.txt          # 레벨 1 데이터
│   └── level2.txt          # 레벨 2 데이터
│
├── tools/                  # 빌드 도구
│   ├── png_to_ps2tex.py    # PNG → PS2TEX 변환기
│   ├── make_spritepack.py  # 스프라이트 팩 생성기
│   ├── make_iso.sh         # ISO 이미지 생성
│   └── wav_convert.py      # WAV 오디오 변환
│
└── docs/                   # 문서
    └── ARCHITECTURE.md     # 이 파일
```

---

## 모듈 설명

### `types.h` — 공통 타입 정의
- 모든 모듈에서 사용하는 구조체, 열거형, 매크로 상수
- `Player`, `Level`, `GameTextures`, `AnimationClip`, `BgmPlayer` 등
- 화면 크기, 타일 크기, 물리 상수 등의 매크로

### `system` — PS2 시스템 유틸리티
| 함수 | 설명 |
|------|------|
| `make_cdrom_path()` | 일반 경로를 PS2 CDROM 형식으로 변환 |
| `iop_delay()` | IOP 안정화용 딜레이 루프 |
| `load_all_iop_modules()` | 패드, 오디오 IOP 모듈 일괄 로딩 |
| `open_binary_file()` | cdrom/host 다중 경로 시도 파일 오픈 |
| `open_level_file()` | 레벨/에셋 파일 오픈 (levels/ 경로 포함) |
| `read_u32_le()` / `read_u16_le()` | Little-endian 바이트 읽기 |
| `trim_line()` | 문자열 끝 공백/줄바꿈 제거 |

### `audio` — BGM 재생
| 함수 | 설명 |
|------|------|
| `load_wav_pcm()` | WAV 파일 파싱 → PCM 데이터 RAM 로딩 |
| `init_bgm_player()` | audsrv 초기화 + 포맷 설정 |
| `update_bgm_stream()` | 매 프레임 PCM 청크 스트리밍 |
| `shutdown_bgm_player()` | 오디오 정리 및 메모리 해제 |

- 8bit unsigned → 16bit signed 자동 변환 (SPU2 호환)
- 루프 재생 지원

### `sprite` — 스프라이트 로딩
| 함수 | 설명 |
|------|------|
| `read_sprite_data()` | PS2TEX 파일 → SpriteData (RAM) |
| `upload_sprite_to_vram()` | SpriteData → GS VRAM 텍스처 |
| `read_anim_clip_data()` | 연번 스프라이트 일괄 로딩 (`_00`, `_01`...) |
| `load_spritepack()` | SPRITES.PAK 파일 로딩 |
| `find_in_pack()` | 팩 내 이름 검색 |
| `read_sprite_from_pack()` | 팩에서 개별 스프라이트 추출 |
| `free_spritepack()` | 팩 메모리 해제 |

- PS2TEX 포맷: 매직 `P2TX` + 크기 + 오프셋 + RGBA32 픽셀
- PAK 포맷: 매직 `2SPK` + 인덱스 + 데이터 섹션
- 투명 픽셀 전처리 (alpha=0 → RGB=0)

### `asset` — 에셋 관리
| 함수 | 설명 |
|------|------|
| `load_all_assets_from_cd()` | Phase 1: CD/host → RAM 전체 에셋 로딩 |
| `upload_all_assets_to_vram()` | Phase 2: RAM → GS VRAM 업로드 |

- 2단계 로딩: CD 읽기(디버그 스크린) → 비디오 초기화 후 VRAM 업로드
- PAK 우선, 실패 시 개별 파일 폴백
- 타일(solid, trap, goal), 배경, 플레이어, 7종 애니메이션 클립

### `animation` — 플레이어 애니메이션
| 함수 | 설명 |
|------|------|
| `animator_init()` | 애니메이터 초기화 |
| `pick_player_anim_state()` | 입력/게임 상태 → 애니메이션 상태 결정 |
| `animator_step()` | 매 프레임 상태 전환 + 프레임 진행 |
| `get_player_clip_safe()` | 안전한 클립 검색 (폴백 포함) |

- 7가지 상태: Idle, Walk, Run, Jump, Clear, Dead, Hurt
- 안정성 필터: 즉시 전환(jump/hurt) vs 3프레임 안정(walk/idle)

### `level` — 레벨 시스템
| 함수 | 설명 |
|------|------|
| `parse_level_file()` | 텍스트 레벨 파일 파싱 |
| `load_level_list()` | levels.txt 목록 로딩 |
| `tile_at()` | 좌표 → 타일 문자 |
| `is_solid()` / `is_goal()` / `is_trap()` | 타일 유형 판별 |

- 레벨 형식: `width=`, `height=`, `spawn=x,y`, `data:` + 타일 행
- 타일: `#` 솔리드, `.` 빈 공간, `G` 골, `X` 트랩

### `physics` — 물리/충돌
| 함수 | 설명 |
|------|------|
| `move_player_x()` | X축 이동 + 벽 충돌 보정 |
| `move_player_y()` | Y축 이동 + 바닥/천장 충돌 보정 |
| `check_grounded()` | 지면 접촉 여부 판정 |
| `intersects_goal()` | 골 타일 겹침 판정 |
| `intersects_trap()` | 트랩 타일 겹침 판정 |
| `reset_player_to_spawn()` | 스폰 위치 초기화 |
| `clampf()` | float 값 범위 제한 |

- 코요테 타임 (6프레임) + 점프 버퍼링 (6프레임)
- AABB 기반 타일맵 충돌

### `render` — 렌더링
| 함수 | 설명 |
|------|------|
| `render_background()` | 전체 화면 배경 텍스처 |
| `render_level()` | 카메라 범위 내 타일맵 렌더링 |
| `render_player()` | 플레이어 애니메이션/스프라이트 렌더링 |

- 텍스처 없을 시 색상 사각형 폴백
- 좌우 반전 지원 (UV 플립)

---

## 빌드 방법

```bash
# 환경 설정
source ./env.sh

# 빌드
make

# ISO 이미지 생성
make iso

# 클린
make clean
```

---

## 조작법

| 버튼 | 동작 |
|------|------|
| ← → | 이동 |
| × (Cross) | 달리기 (이동 중) |
| ○ (Circle) | 점프 |
| START | 리스타트 (게임 오버 시) |
| R1 | 다음 레벨 |
| L1 | 이전 레벨 |

---

## 의존 관계

```
main.c
 ├── src/types.h      (모든 모듈 공통)
 ├── src/system        (IOP, 파일 I/O)
 ├── src/audio         ← system
 ├── src/sprite        ← system
 ├── src/asset         ← sprite, system
 ├── src/animation     (독립)
 ├── src/level         ← system
 ├── src/physics       ← level
 └── src/render        ← animation, level
```
