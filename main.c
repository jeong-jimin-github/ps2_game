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
    float cameraX = 0.0f;
    int gameOver = 0;
    int hurtTimer = 0;
    int clearTimer = 0;
    GameTextures textures;
    AnimationClip playerAnims[PLAYER_ANIM_COUNT];
    PlayerAnimator animator;
    BgmPlayer bgm;
    int padModulesOk = 0;
    int audioModulesOk = 0;

    GSGLOBAL *gsGlobal;
    AssetBank assetBank;

    LOG("main start");
    memset(&textures, 0, sizeof(textures));
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

    reset_player_to_spawn(&level, &player);

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

    if (!textures.hasPlayer) {
        LOG("fallback to debug rectangle player (missing assets/player.ps2tex)");
    }

    if (!init_bgm_player(&bgm, BGM_FILE_PATH, audioModulesOk)) {
        LOG("BGM disabled (put a PCM WAV at %s)", BGM_FILE_PATH);
    }

    /* ── 게임 루프 ── */
    while (1) {
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

        /* 게임 로직 */
        if (!gameOver && clearTimer == 0 && hurtTimer == 0) {
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
            }

            player.vy += GRAVITY;
            if (player.vy > MAX_FALL_SPEED) {
                player.vy = MAX_FALL_SPEED;
            }

            move_player_x(&level, &player);
            move_player_y(&level, &player);

            if (!player.onGround) {
                player.onGround = check_grounded(&level, &player);
            }

            if (intersects_trap(&level, &player)) {
                hurtTimer = HURT_FRAMES;
                player.vx = 0.0f;
                player.vy = -2.0f;
                LOG("player hurt by trap");
            }

            if (intersects_goal(&level, &player)) {
                clearTimer = CLEAR_FRAMES;
                player.vx = 0.0f;
                player.vy = 0.0f;
                LOG("level clear animation");
            }

            if (player.y > (float)(level.height * TILE_SIZE + 96)) {
                gameOver = 1;
                LOG("GAME OVER: fell into hole");
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
                gameOver = 1;
                LOG("GAME OVER: hurt finished");
            }
        } else if (clearTimer > 0) {
            clearTimer--;
            if (clearTimer == 0) {
                levelList.current = (levelList.current + 1) % levelList.count;
                if (parse_level_file(levelList.paths[levelList.current], &level)) {
                    reset_player_to_spawn(&level, &player);
                }
            }
        } else {
            if ((padData & PAD_START) && !(prevPadData & PAD_START)) {
                reset_player_to_spawn(&level, &player);
                gameOver = 0;
                hurtTimer = 0;
                clearTimer = 0;
                LOG("restart level");
            }
        }

        /* 레벨 전환 (R1/L1) */
        if ((padData & PAD_R1) && !(prevPadData & PAD_R1)) {
            levelList.current = (levelList.current + 1) % levelList.count;
            if (parse_level_file(levelList.paths[levelList.current], &level)) {
                reset_player_to_spawn(&level, &player);
                gameOver = 0;
                hurtTimer = 0;
                clearTimer = 0;
            }
        }

        if ((padData & PAD_L1) && !(prevPadData & PAD_L1)) {
            levelList.current = (levelList.current - 1 + levelList.count) % levelList.count;
            if (parse_level_file(levelList.paths[levelList.current], &level)) {
                reset_player_to_spawn(&level, &player);
                gameOver = 0;
                hurtTimer = 0;
                clearTimer = 0;
            }
        }

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
        render_player(gsGlobal, &player, cameraX, &textures, playerAnims, animator.state, animator.frame, player.facingRight);

        if (gameOver) {
            gsKit_prim_sprite(gsGlobal, 0.0f, 0.0f, (float)SCREEN_W, (float)SCREEN_H, 3,
                              GS_SETREG_RGBAQ(0x80, 0x00, 0x00, 0x40, 0x00));
        }

        gsKit_queue_exec(gsGlobal);
        gsKit_sync_flip(gsGlobal);

        prevPadData = padData;
    }

    shutdown_bgm_player(&bgm);

    return 0;
}
