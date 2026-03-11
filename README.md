# PS2 홈브류 게임

PlayStation 2 홈브류 2D 플랫폼 게임 프로젝트입니다.

## 빌드

```bash
bash build.sh          # 빌드
bash build.sh iso      # 빌드 + ISO 생성
bash build.sh clean    # 클린
bash build.sh rebuild  # 클린 + 전체 재빌드
```

생성 결과:

- ELF: `game_engine.elf`
- ISO: `SLUS_209.99.PS2_PLATFORMER.iso` (`tools/make_iso.sh` 기본값)

## 조작법

- `D-Pad`: 이동
- `X (B)`: 달리기 (이동 중)
- `O (A)`: 점프
- `R1`: 다음 레벨 (개발자 모드 전용)
- `L1`: 이전 레벨 (개발자 모드 전용)
- `Start`: 재시작 (게임 오버 시)
- `△ (Y)`: 메뉴로 돌아가기

## 레벨 에디터 (GUI)

전용 GUI 에디터로 편집하는것을 추천합니다.

```bash
python tools/level_editor.py levels/level1.txt
```

## ISO 관련 참고

- ISO 생성 스크립트: `tools/make_iso.sh`
- PS2 호환을 위해 스테이징 파일은 평탄화(flat) + 대문자 변환
- `SYSTEM.CNF`는 CRLF 줄바꿈으로 생성
- 기본 부팅 파일명은 실게임 스타일의 ID 형식(`SLUS_209.99`)으로 생성
- 실기 CD 구동시 CD-R(DVD 불가)에 1x~8x의 저배속으로 굽기  
- 실기 CD 구동시 모드칩 설치기 필요합니다

게임 ID/타이틀 지정:

## 요구 사항

- `ps2sdk`
- `MSYS2`
- Python3
- `xorriso`
- ImgBurn (선택)