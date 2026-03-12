/**
 * @file main.c
 * @brief PS2 플랫포머 - 엔트리 포인트
 *
 * 초기화, 게임 루프, 입력 처리를 담당합니다.
 * 세부 기능은 src/ 하위 모듈로 분리되어 있습니다.
 */
#include <tamtypes.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <kernel.h>
#include <sbv_patches.h>
#include <debug.h>

#include "src/types.h"
#include "src/system.h"
#include "src/audio.h"
#include "src/sprite.h"
#include "src/asset.h"
#include "src/animation.h"
#include "src/level.h"
#include "src/physics.h"
#include "src/render.h"
#include "src/font.h"
#include "src/sfx.h"

/* ── 개발자 로그 헬퍼 ────────────────────────────── */
static void dev_log_push(DevLog *dl, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(dl->lines[dl->head], DEV_LOG_LEN, fmt, ap);
    va_end(ap);
    dl->head = (dl->head + 1) % DEV_LOG_LINES;
    if (dl->count < DEV_LOG_LINES) dl->count++;
}

/* ── 레벨 로딩 헬퍼 ─────────────────────────────── */
static void load_current_level(LevelList *ll, Level *level, GameWorld *world,
                               Player *player, int *gameOver, int *hurtTimer, int *clearTimer)
{
    if (parse_level_file(ll->paths[ll->current], level)) {
        memcpy(&world->level, level, sizeof(Level));
        parse_moving_entities(ll->paths[ll->current], world);
        reset_player_to_spawn(level, player);
        player->lives = PLAYER_INITIAL_LIVES;
        player->coins = 0;
        player->powered = 0;
        player->invincTimer = 0;
        *gameOver = 0;
        *hurtTimer = 0;
        *clearTimer = 0;
    }
}

int main(int argc, char *argv[])
{
    const int padPort = 0;
    const int padSlot = 0;
    struct padButtonStatus buttons;
    unsigned int padData = 0;
    unsigned int prevPadData = 0;
    static char *padBuf = NULL;
    int padReady = 0;
    int lastPadState = -1;
    unsigned int frameCount = 0;

    Level level;
    LevelList levelList;
    Player player;
    GameWorld world;
    float cameraX = 0.0f;
    int gameOver = 0;
    int hurtTimer = 0;
    int clearTimer = 0;
    GameTextures textures;
    AnimationClip playerAnims[PLAYER_ANIM_COUNT];
    PlayerAnimator animator;
    BgmPlayer bgm;
    BitmapFont uiFont;
    int padModulesOk = 0;
    int audioModulesOk = 0;

    /* 신규: 씬 상태, 메뉴, 설정, 개발자 로그 */
    GameScene scene = SCENE_MENU;
    GameSettings settings;
    DevLog devLog;
    int menuSel = 0;         /* 메인 메뉴 커서 */
    int settingsSel = 0;     /* 설정 메뉴 커서 */

    GSGLOBAL *gsGlobal;
    AssetBank assetBank;

    LOG("main start");
    memset(&textures, 0, sizeof(textures));
    memset(&world, 0, sizeof(world));
    memset(&uiFont, 0, sizeof(uiFont));
    memset(&bgm, 0, sizeof(bgm));
    memset(&devLog, 0, sizeof(devLog));
    (void)argc; (void)argv;
    settings.soundOn = 1;
    settings.devMode = 0;
    animator_init(&animator);

    sceSifInitRpc(0);

    /* SBV 패치: 실제 PS2에서 cdrom0: IOP 모듈 로딩에 필요 */
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();

    load_all_iop_modules(&padModulesOk, &audioModulesOk);

    if (padModulesOk) {
        if (padInit(0) != 0) {
            LOG("padInit ok");
        } else {
            padModulesOk = 0;
        }
    }

    if (padModulesOk) {
        padBuf = (char *)memalign(64, 256);
        if (padBuf != NULL && padPortOpen(padPort, padSlot, padBuf) != 0) {
            padReady = 1;
            LOG("padPortOpen ok (port=%d slot=%d)", padPort, padSlot);
        }
    }

    /* ── Phase 1: CD → RAM 에셋 로딩 (디버그 스크린) ── */
    load_all_assets_from_cd(&assetBank);

    if (!load_level_list("levels.txt", &levelList)) {
        scr_printf("ERROR: cannot load level list!\n");
        LOG("cannot continue without level list");
        while (1) { }
    }

    if (!parse_level_file(levelList.paths[levelList.current], &level)) {
        scr_printf("ERROR: cannot load level: %s\n", levelList.paths[levelList.current]);
        LOG("cannot load initial level: %s", levelList.paths[levelList.current]);
        while (1) { }
    }

    memcpy(&world.level, &level, sizeof(Level));
    parse_moving_entities(levelList.paths[levelList.current], &world);

    reset_player_to_spawn(&level, &player);
    player.lives = PLAYER_INITIAL_LIVES;
    player.coins = 0;
    player.powered = 0;
    player.invincTimer = 0;

    /* ── Phase 2: GS 초기화 + RAM → VRAM 업로드 ── */
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsGlobal = gsKit_init_global();
    gsGlobal->Mode = GS_MODE_NTSC;
    gsGlobal->Width = SCREEN_W;
    gsGlobal->Height = SCREEN_H;
    gsGlobal->PSM = GS_PSM_CT32;
    gsGlobal->PSMZ = GS_PSMZ_16S;
    gsGlobal->ZBuffering = GS_SETTING_OFF;
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    gsGlobal->PABE = GS_SETTING_OFF;

    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_ONESHOT);

    gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_set_test(gsGlobal, GS_ATEST_ON);
    gsKit_sync_flip(gsGlobal);

    upload_all_assets_to_vram(gsGlobal, &assetBank, &textures, playerAnims);

    /* ── 폰트 로딩 ── */
    if (load_font_metrics("font_ui.fnt", &uiFont)) {
        load_font_texture(gsGlobal, "font_ui.ps2tex", &uiFont);
    }

    if (!textures.hasPlayer) {
        LOG("fallback to debug rectangle player (missing assets/player.ps2tex)");
    }

    /* ── BGM 초기화 (RAM 기반, 하나만 로드 → 씬 전환 시 교체) ── */
    if (!init_bgm_player(&bgm, MENU_BGM_FILE_PATH, audioModulesOk)) {
        LOG("Menu BGM disabled (put a PCM WAV at %s)", MENU_BGM_FILE_PATH);
    }
    if (!settings.soundOn) bgm_set_volume(0);

    /* ── SFX 초기화 ── */
    if (audioModulesOk) {
        sfx_init();
        if (!settings.soundOn) sfx_set_volume(0);
    }

    /* ── 게임 루프 ── */
    while (1) {
        /* BGM 스트리밍 (RAM → audsrv) */
        update_bgm_stream(&bgm);

        /* 패드 입력 */
        if (padReady) {
            int state = padGetState(padPort, padSlot);

            if (state != lastPadState) {
                LOG("pad state changed: %d", state);
                lastPadState = state;
            }

            if (state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) {
                if (padRead(padPort, padSlot, &buttons) != 0) {
                    padData = 0xFFFF ^ buttons.btns;
                }
            }
        }

        /* ── 씬 분기 ── */
        switch (scene) {

        /* ───────────────────────────────────────────
         * SCENE_MENU
         * ───────────────────────────────────────────*/
        case SCENE_MENU:
            if ((padData & PAD_UP) && !(prevPadData & PAD_UP)) {
                menuSel = (menuSel + 1) % 2;
            }
            if ((padData & PAD_DOWN) && !(prevPadData & PAD_DOWN)) {
                menuSel = (menuSel + 1) % 2;
            }
            if (((padData & PAD_CROSS) && !(prevPadData & PAD_CROSS)) ||
                ((padData & PAD_CIRCLE) && !(prevPadData & PAD_CIRCLE))) {
                if (menuSel == 0) {
                    /* 게임 시작 */
                    if (settings.soundOn) sfx_play(SFX_SELECT);
                    scene = SCENE_PLAY;
                    /* BGM 전환: 메뉴→플레이 (로딩 화면 표시) */
                    render_loading_screen(gsGlobal, &textures, &uiFont);
                    bgm_swap(&bgm, BGM_FILE_PATH);
                    if (!settings.soundOn) bgm_set_volume(0);
                    /* 레벨 로드 */
                    load_current_level(&levelList, &level, &world, &player,
                                       &gameOver, &hurtTimer, &clearTimer);
                } else {
                    if (settings.soundOn) sfx_play(SFX_SELECT);
                    scene = SCENE_SETTINGS;
                    settingsSel = 0;
                }
            }

            gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x20, 0x20, 0x40, 0x80, 0x00));
            render_main_menu(gsGlobal, &textures, &uiFont, menuSel);
            break;

        /* ───────────────────────────────────────────
         * SCENE_SETTINGS
         * ───────────────────────────────────────────*/
        case SCENE_SETTINGS:
            if ((padData & PAD_UP) && !(prevPadData & PAD_UP)) {
                settingsSel = (settingsSel - 1 + 3) % 3;
            }
            if ((padData & PAD_DOWN) && !(prevPadData & PAD_DOWN)) {
                settingsSel = (settingsSel + 1) % 3;
            }
            if (((padData & PAD_CROSS) && !(prevPadData & PAD_CROSS)) ||
                ((padData & PAD_CIRCLE) && !(prevPadData & PAD_CIRCLE))) {
                if (settingsSel == 0) {
                    settings.soundOn = !settings.soundOn;
                    bgm_set_volume(settings.soundOn ? 80 : 0);
                    sfx_set_volume(settings.soundOn ? MAX_VOLUME : 0);
                } else if (settingsSel == 1) {
                    settings.devMode = !settings.devMode;
                } else {
                    /* 뒤로 */
                    if (settings.soundOn) sfx_play(SFX_SELECT);
                    scene = SCENE_MENU;
                }
            }
            if ((padData & PAD_TRIANGLE) && !(prevPadData & PAD_TRIANGLE)) {
                if (settings.soundOn) sfx_play(SFX_SELECT);
                scene = SCENE_MENU;
            }

            gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x20, 0x20, 0x40, 0x80, 0x00));
            render_settings_menu(gsGlobal, &textures, &uiFont, &settings, settingsSel);
            break;

        /* ───────────────────────────────────────────
         * SCENE_PLAY
         * ───────────────────────────────────────────*/
        case SCENE_PLAY:
            /* 게임 로직 */
            if (!gameOver && clearTimer == 0 && hurtTimer == 0) {
                char collected;
                int bumpTx, bumpTy;
                int spawnedType;
                int fellOutThisFrame = 0;

                {
                    float targetSpeed = (padData & PAD_CROSS) ? PLAYER_RUN_SPEED : PLAYER_WALK_SPEED;

                    player.vx = 0.0f;
                    if (padData & PAD_LEFT) {
                        player.vx = -targetSpeed;
                        player.facingRight = 0;
                    }
                    if (padData & PAD_RIGHT) {
                        player.vx = targetSpeed;
                        player.facingRight = 1;
                    }
                }

                if ((padData & PAD_CIRCLE) && !(prevPadData & PAD_CIRCLE)) {
                    player.jumpBufferTimer = JUMP_BUFFER_FRAMES;
                } else if (player.jumpBufferTimer > 0) {
                    player.jumpBufferTimer--;
                }

                player.onGround = check_grounded(&level, &player);
                if (!player.onGround) {
                    player.onGround = check_on_moving_platform(&world, &player);
                }
                if (player.onGround) {
                    player.coyoteTimer = COYOTE_FRAMES;
                    if (player.vy > 0.0f) {
                        player.vy = 0.0f;
                    }
                } else if (player.coyoteTimer > 0) {
                    player.coyoteTimer--;
                }

                if (player.jumpBufferTimer > 0 && player.coyoteTimer > 0) {
                    player.vy = JUMP_VELOCITY;
                    player.onGround = 0;
                    player.coyoteTimer = 0;
                    player.jumpBufferTimer = 0;
                    if (settings.soundOn) sfx_play(SFX_JUMP);
                }

                player.vy += GRAVITY;
                if (player.vy > MAX_FALL_SPEED) {
                    player.vy = MAX_FALL_SPEED;
                }

                move_player_x(&level, &player);

                /* 코인 블록 머리 부딪힘 */
                if (check_head_bump_coin_block(&level, &player, &bumpTx, &bumpTy)) {
                    spawn_item_from_block(&world, bumpTx, bumpTy, 'C');
                    player.coins++;
                    if (settings.soundOn) sfx_play(SFX_COIN);
                    if (player.coins >= COINS_PER_1UP) {
                        player.coins -= COINS_PER_1UP;
                        player.lives++;
                        if (settings.soundOn) sfx_play(SFX_1UP);
                        if (settings.devMode)
                            dev_log_push(&devLog, "100coins -> 1UP! lives=%d", player.lives);
                    }
                    if (settings.devMode)
                        dev_log_push(&devLog, "coinblock (%d,%d) coins=%d", bumpTx, bumpTy, player.coins);
                }

                move_player_y(&level, &player);

                if (!player.onGround) {
                    player.onGround = check_grounded(&level, &player);
                    if (!player.onGround) {
                        player.onGround = check_on_moving_platform(&world, &player);
                    }
                }

                /* 낙사와 아이템 수집이 같은 프레임에 중복 처리되지 않도록
                 * 낙사 판정을 먼저 수행하고 해당 프레임 로직을 종료합니다. */
                if (player.y > (float)(level.height * TILE_SIZE + 96)) {
                    if (!settings.devMode) {
                        player.lives--;
                    }
                    if (settings.soundOn) sfx_play(SFX_DEATH);
                    if (settings.devMode)
                        dev_log_push(&devLog, "fell, lives=%d", player.lives);
                    if (!settings.devMode && player.lives <= 0) {
                        gameOver = 1;
                    } else {
                        parse_level_file(levelList.paths[levelList.current], &level);
                        memcpy(&world.level, &level, sizeof(Level));
                        parse_moving_entities(levelList.paths[levelList.current], &world);
                        reset_player_to_spawn(&level, &player);
                        player.powered = 0;
                        player.invincTimer = INVINCIBILITY_FRAMES;
                    }
                    fellOutThisFrame = 1;
                }

                if (fellOutThisFrame) {
                    break;
                }

                /* 맵 직접 배치 아이템 수집 */
                collected = intersects_collectible(&level, &player);
                if (collected == 'C') {
                    player.coins++;
                    if (settings.soundOn) sfx_play(SFX_COIN);
                    if (player.coins >= COINS_PER_1UP) {
                        player.coins -= COINS_PER_1UP;
                        player.lives++;
                        if (settings.soundOn) sfx_play(SFX_1UP);
                        if (settings.devMode)
                            dev_log_push(&devLog, "100coins -> 1UP! lives=%d", player.lives);
                    }
                } else if (collected == 'M') {
                    player.powered = 1;
                    if (settings.soundOn) sfx_play(SFX_POWERUP);
                    if (settings.devMode)
                        dev_log_push(&devLog, "mushroom collected!");
                } else if (collected == '1') {
                    player.lives++;
                    if (settings.soundOn) sfx_play(SFX_1UP);
                    if (settings.devMode)
                        dev_log_push(&devLog, "1UP! lives=%d", player.lives);
                }

                /* 스폰된 아이템 수집 */
                spawnedType = collect_spawned_item(&world, &player);
                if (spawnedType == 'M') {
                    player.powered = 1;
                    if (settings.soundOn) sfx_play(SFX_POWERUP);
                } else if (spawnedType == '1') {
                    player.lives++;
                    if (settings.soundOn) sfx_play(SFX_1UP);
                }

                /* 무적 타이머 감소 */
                if (player.invincTimer > 0) {
                    player.invincTimer--;
                }

                /* 트랩 교차 (타일 + 이동 함정) */
                if (player.invincTimer == 0 &&
                    (intersects_trap(&level, &player) || intersects_moving_trap(&world, &player))) {
                    if (player.powered) {
                        player.powered = 0;
                        player.invincTimer = INVINCIBILITY_FRAMES;
                        player.vy = -3.0f;
                        if (settings.soundOn) sfx_play(SFX_HURT);
                    } else {
                        hurtTimer = HURT_FRAMES;
                        player.vx = 0.0f;
                        player.vy = -2.0f;
                        if (settings.soundOn) sfx_play(SFX_HURT);
                        if (settings.devMode)
                            dev_log_push(&devLog, "player hurt by trap");
                    }
                }

                if (intersects_goal(&level, &player)) {
                    clearTimer = CLEAR_FRAMES;
                    player.vx = 0.0f;
                    player.vy = 0.0f;
                    if (settings.soundOn) sfx_play(SFX_CLEAR);
                    if (settings.devMode)
                        dev_log_push(&devLog, "level clear!");
                }
            } else if (hurtTimer > 0) {
                hurtTimer--;
                player.vy += GRAVITY;
                if (player.vy > MAX_FALL_SPEED) {
                    player.vy = MAX_FALL_SPEED;
                }
                move_player_x(&level, &player);
                move_player_y(&level, &player);

                if (hurtTimer == 0) {
                    if (!settings.devMode) {
                        player.lives--;
                    }
                    if (!settings.devMode && player.lives <= 0) {
                        gameOver = 1;
                        if (settings.soundOn) sfx_play(SFX_DEATH);
                    } else {
                        reset_player_to_spawn(&level, &player);
                        player.powered = 0;
                        player.invincTimer = INVINCIBILITY_FRAMES;
                    }
                }
            } else if (clearTimer > 0) {
                clearTimer--;
                if (clearTimer == 0) {
                    levelList.current = (levelList.current + 1) % levelList.count;
                    if (parse_level_file(levelList.paths[levelList.current], &level)) {
                        memcpy(&world.level, &level, sizeof(Level));
                        parse_moving_entities(levelList.paths[levelList.current], &world);
                        reset_player_to_spawn(&level, &player);
                    }
                }
            } else {
                /* gameOver 상태 */
                if ((padData & PAD_START) && !(prevPadData & PAD_START)) {
                    load_current_level(&levelList, &level, &world, &player,
                                       &gameOver, &hurtTimer, &clearTimer);
                }
            }

            /* 레벨 전환 (R1/L1) — 개발자 모드 전용 */
            if (settings.devMode) {
                if ((padData & PAD_R1) && !(prevPadData & PAD_R1)) {
                    levelList.current = (levelList.current + 1) % levelList.count;
                    load_current_level(&levelList, &level, &world, &player,
                                       &gameOver, &hurtTimer, &clearTimer);
                    dev_log_push(&devLog, ">> level %d", levelList.current);
                }
                if ((padData & PAD_L1) && !(prevPadData & PAD_L1)) {
                    levelList.current = (levelList.current - 1 + levelList.count) % levelList.count;
                    load_current_level(&levelList, &level, &world, &player,
                                       &gameOver, &hurtTimer, &clearTimer);
                    dev_log_push(&devLog, "<< level %d", levelList.current);
                }
            }

            /* TRIANGLE: 메인 메뉴로 복귀 */
            if ((padData & PAD_TRIANGLE) && !(prevPadData & PAD_TRIANGLE)) {
                scene = SCENE_MENU;
                /* BGM 전환: 플레이→메뉴 (로딩 화면 표시) */
                render_loading_screen(gsGlobal, &textures, &uiFont);
                bgm_swap(&bgm, MENU_BGM_FILE_PATH);
                if (!settings.soundOn) bgm_set_volume(0);
                break;
            }

            /* 이동 엔티티 + 스폰 아이템 업데이트 */
            update_moving_entities(&world);
            update_spawned_items(&world);

            /* 애니메이션 + 카메라 */
            animator_step(&animator, playerAnims, &player, padData, gameOver, hurtTimer, clearTimer);

            {
                float mapPixelW = (float)(level.width * TILE_SIZE);
                float maxCamX = mapPixelW - (float)SCREEN_W;
                if (maxCamX < 0.0f) {
                    maxCamX = 0.0f;
                }
                cameraX = clampf(player.x + (PLAYER_W * 0.5f) - ((float)SCREEN_W * 0.5f), 0.0f, maxCamX);
            }

            if ((frameCount % 300) == 0) {
                LOG("frame=%u level=%d pos=(%.1f, %.1f) camX=%.1f", frameCount, levelList.current, player.x, player.y, cameraX);
            }
            frameCount++;

            /* 렌더링 */
            gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x70, 0xB5, 0xFF, 0x80, 0x00));
            render_background(gsGlobal, &textures);
            render_level(gsGlobal, &level, cameraX, &textures);
            render_moving_entities(gsGlobal, &world, cameraX, &textures);
            render_spawned_items(gsGlobal, &world, cameraX, &textures);
            render_player(gsGlobal, &player, cameraX, &textures, playerAnims, animator.state, animator.frame, player.facingRight);
            render_hud(gsGlobal, &player, &uiFont);

            if (gameOver) {
                gsKit_prim_sprite(gsGlobal, 0.0f, 0.0f, (float)SCREEN_W, (float)SCREEN_H, 3,
                                  GS_SETREG_RGBAQ(0x80, 0x00, 0x00, 0x40, 0x00));
                char buf[32];
                render_text(gsGlobal, &uiFont, (SCREEN_W / 2) - 64, (SCREEN_H / 2) - 8, "GAME OVER",
                             GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x80, 0x00));
                render_text(gsGlobal, &uiFont, (SCREEN_W / 2) - 96, (SCREEN_H / 2) + 16, "Press START to retry",
                             GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x80, 0x00));
            }

            if (settings.devMode) {
                render_dev_log(gsGlobal, &uiFont, &devLog);
            }
            break;

        } /* switch (scene) */

        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);

        prevPadData = padData;
    }

    sfx_shutdown();
    shutdown_bgm_player(&bgm);

    return 0;
}
