/**
 * @file types.h
 * @brief PS2 플랫포머 - 공통 타입 정의 및 상수
 *
 * 모든 모듈에서 공유하는 구조체, 열거형, 매크로 상수를 정의합니다.
 */
#ifndef TYPES_H
#define TYPES_H

#include <tamtypes.h>
#include <gsKit.h>
#include <audsrv.h>

/* ── 디버그 로그 ─────────────────────────────────── */
#define LOG(fmt, ...) printf("[DBG] " fmt "\n", ##__VA_ARGS__)

/* ── 화면 설정 ───────────────────────────────────── */
#define SCREEN_W 640
#define SCREEN_H 448

/* ── 타일/레벨 ───────────────────────────────────── */
#define TILE_SIZE 32
#define MAX_LEVEL_WIDTH 128
#define MAX_LEVEL_HEIGHT 32
#define MAX_LEVELS 32
#define MAX_PATH_LEN 128

/* ── 플레이어 물리 ───────────────────────────────── */
#define PLAYER_W 24.0f
#define PLAYER_H 30.0f
#define PLAYER_WALK_SPEED 2.3f
#define PLAYER_RUN_SPEED 4.0f
#define GRAVITY 0.28f
#define MAX_FALL_SPEED 6.2f
#define JUMP_VELOCITY -8.4f
#define COYOTE_FRAMES 6
#define JUMP_BUFFER_FRAMES 6

/* ── 스프라이트/팩 ───────────────────────────────── */
#define SPRITE_MAGIC 0x58543250u /* 'P2TX' */
#define PAK_MAGIC    0x4B505332u /* '2SPK' */
#define PAK_NAME_LEN 32
#define MAX_ANIM_FRAMES 16

/* ── 게임 상태 ───────────────────────────────────── */
#define HURT_FRAMES 20
#define CLEAR_FRAMES 50
#define BGM_FILE_PATH "bgm.wav"
#define MENU_BGM_FILE_PATH "bgm_menu.wav"
#define BGM_STREAM_CHUNK 4096
#define COINS_PER_1UP 100

/* ── 게임 상태 (씬) ──────────────────────────────── */
typedef enum {
    SCENE_MENU = 0,
    SCENE_SETTINGS,
    SCENE_PLAY
} GameScene;

/* ── 설정 ────────────────────────────────────────── */
typedef struct {
    int soundOn;     /* 1=on, 0=off */
    int devMode;     /* 1=developer mode */
} GameSettings;

/* ── 개발자 로그 링 버퍼 ─────────────────────────── */
#define DEV_LOG_LINES 6
#define DEV_LOG_LEN   80
typedef struct {
    char lines[DEV_LOG_LINES][DEV_LOG_LEN];
    int head;
    int count;
} DevLog;

/* ── 개발자 HUD 시스템 정보 ─────────────────────── */
typedef struct {
    char systemName[32];
    char romVersion[32];
    int totalMemBytes;
    int heapTotalBytes;
    int heapUsedBytes;
    int heapFreeBytes;
    int maxFreeBlockBytes;
} DevHudInfo;

/* ── 아이템 / 엔티티 ────────────────────────────── */
#define MAX_MOVING_ENTITIES 32
#define MAX_SPAWNED_ITEMS 32
#define COIN_BOUNCE_FRAMES 16
#define INVINCIBILITY_FRAMES 60
#define ITEM_RISE_SPEED -1.5f
#define MOVING_ENTITY_DEFAULT_SPEED 1.0f
#define PLAYER_INITIAL_LIVES 3

/* ── 애니메이션 상태 열거형 ──────────────────────── */
typedef enum {
    PLAYER_ANIM_IDLE = 0,
    PLAYER_ANIM_WALK,
    PLAYER_ANIM_RUN,
    PLAYER_ANIM_JUMP,
    PLAYER_ANIM_CLEAR,
    PLAYER_ANIM_DEAD,
    PLAYER_ANIM_HURT,
    PLAYER_ANIM_COUNT
} PlayerAnimState;

/* ── 스프라이트 헤더 (파일 포맷) ─────────────────── */
typedef struct {
    u32 magic;
    u16 width;
    u16 height;
    s16 offsetX;
    s16 offsetY;
} SpriteHeader;

/* ── 스프라이트 데이터 (RAM에 읽은 상태) ─────────── */
typedef struct {
    void *pixels;
    u16 width;
    u16 height;
    s16 offsetX;
    s16 offsetY;
    int valid;
} SpriteData;

/* ── 레벨 ────────────────────────────────────────── */
typedef struct {
    int width;
    int height;
    char tiles[MAX_LEVEL_HEIGHT][MAX_LEVEL_WIDTH + 1];
    float spawnX;
    float spawnY;
} Level;

/* ── 이동 엔티티 (플랫폼/함정) ───────────────────── */
typedef struct {
    float x, y;
    float startX, startY;
    float rangePixels;
    float speed;
    float prevX, prevY;
    int dirHorizontal;  /* 1 = horizontal, 0 = vertical */
    int isTrap;         /* 0 = platform, 1 = trap */
    int active;
    int forward;
    float width, height;
} MovingEntity;

/* ── 스폰된 아이템 (블록에서 나온 것) ────────────── */
typedef struct {
    float x, y;
    float vy;
    int type;       /* 'C' = coin, 'M' = mushroom, '1' = 1up */
    int active;
    int timer;
    float targetY;
} SpawnedItem;

/* ── 플레이어 ────────────────────────────────────── */
typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    int onGround;
    int facingRight;
    int coyoteTimer;
    int jumpBufferTimer;
    int lives;
    int coins;
    int powered;        /* 1 = has mushroom shield */
    int invincTimer;    /* invincibility frames after hit */
} Player;

/* ── 레벨 목록 ───────────────────────────────────── */
typedef struct {
    int count;
    int current;
    char paths[MAX_LEVELS][MAX_PATH_LEN];
} LevelList;

/* ── 텍스처 집합 (VRAM 업로드 후) ────────────────── */
typedef struct {
    GSTEXTURE player;
    GSTEXTURE tileSolid;
    GSTEXTURE tileTrap;
    GSTEXTURE tileGoal;
    GSTEXTURE background;
    GSTEXTURE menuBackground;
    GSTEXTURE tileCoinBlock;
    GSTEXTURE tileEmptyBlock;
    GSTEXTURE tileCoin;
    GSTEXTURE tileMushroom;
    GSTEXTURE tile1up;
    void *playerPixels;
    void *tileSolidPixels;
    void *tileTrapPixels;
    void *tileGoalPixels;
    void *backgroundPixels;
    void *menuBackgroundPixels;
    void *tileCoinBlockPixels;
    void *tileEmptyBlockPixels;
    void *tileCoinPixels;
    void *tileMushroomPixels;
    void *tile1upPixels;
    int hasPlayer;
    int hasTileSolid;
    int hasTileTrap;
    int hasTileGoal;
    int hasBackground;
    int hasMenuBackground;
    int hasTileCoinBlock;
    int hasTileEmptyBlock;
    int hasTileCoin;
    int hasTileMushroom;
    int hasTile1up;
} GameTextures;

/* ── 애니메이션 클립 ─────────────────────────────── */
typedef struct {
    GSTEXTURE frames[MAX_ANIM_FRAMES];
    void *pixels[MAX_ANIM_FRAMES];
    s16 offsetX[MAX_ANIM_FRAMES];
    s16 offsetY[MAX_ANIM_FRAMES];
    int count;
    int frameDelay;
    int loop;
} AnimationClip;

/* ── 애니메이터 ──────────────────────────────────── */
typedef struct {
    PlayerAnimState state;
    int frame;
    int tick;
    PlayerAnimState pendingState;
    int pendingTicks;
    int groundedGrace;
} PlayerAnimator;

/* ── BGM 플레이어 ────────────────────────────────── */
typedef struct {
    unsigned char *pcmData;
    int pcmSize;
    int cursor;
    audsrv_fmt_t format;
    int audioReady;
} BgmPlayer;

/* ── BGM CD 스트리밍 플레이어 ────────────────────── */
#define BGM_SBUF_SIZE    (48 * 1024)
#define BGM_SREAD_CHUNK  (8 * 1024)

typedef struct {
    void *fp;               /* FILE* (cdrom handle, stored as void*) */
    unsigned char *buf;     /* read buffer */
    int  bufLen;            /* valid bytes in buf */
    long dataStart;         /* file seek offset for PCM data start */
    int  dataSize;          /* total PCM data bytes */
    int  dataPos;           /* bytes read from file in current loop */
    audsrv_fmt_t format;
    int  opened;            /* WAV parsed, buf allocated, audsrv init'd */
    int  playing;           /* currently feeding audsrv */
} BgmStream;

/* ── 스프라이트 팩 ───────────────────────────────── */
typedef struct {
    char name[PAK_NAME_LEN];
    u32 offset;
    u32 size;
} PakEntry;

typedef struct {
    PakEntry *entries;
    unsigned char *dataSection;
    int count;
} SpritePack;

/* ── 에셋 뱅크 (CD→RAM 로딩 단계) ───────────────── */
typedef struct {
    SpriteData tileSolid;
    SpriteData tileTrap;
    SpriteData tileGoal;
    SpriteData background;
    SpriteData menuBackground;
    SpriteData player;
    SpriteData tileCoinBlock;
    SpriteData tileEmptyBlock;
    SpriteData tileCoin;
    SpriteData tileMushroom;
    SpriteData tile1up;
    SpriteData animFrames[PLAYER_ANIM_COUNT][MAX_ANIM_FRAMES];
    int animFrameCount[PLAYER_ANIM_COUNT];
    BgmPlayer bgm;
} AssetBank;

/* ── 게임 월드 (레벨 + 동적 엔티티) ─────────────── */
typedef struct {
    Level level;
    MovingEntity movingEnts[MAX_MOVING_ENTITIES];
    int movingEntCount;
    SpawnedItem items[MAX_SPAWNED_ITEMS];
    int itemCount;
} GameWorld;

#endif /* TYPES_H */
