/**
 * @file asset.c
 * @brief 에셋 관리 구현
 */
#include "asset.h"
#include "sprite.h"
#include "system.h"

#include <string.h>
#include <stdio.h>
#include <debug.h>
#include <kernel.h>

void loading_msg(int step, int total, const char *name)
{
    scr_printf("[%2d/%2d] Loading %s ...\n", step, total, name);
}

void load_all_assets_from_cd(AssetBank *bank)
{
    SpritePack pack;
    int step = 0;
    const int totalSteps = 12;
    static const char *animNames[PLAYER_ANIM_COUNT] = {
        "player_idle", "player_walk", "player_run", "player_jump",
        "player_clear", "player_dead", "player_hurt"
    };
    int a;

    memset(bank, 0, sizeof(*bank));

    init_scr();
    scr_printf("=== PS2 Game by Jeong Jimin - Loading ===\n\n");

    /* 고정 대기 없이 즉시 로딩 시작 */
    FlushCache(0);

    scr_printf("Opening SPRITES.PAK ...\n");

    /* SPRITES.PAK 로딩 재시도 (노후 드라이브에서 첫 읽기가 실패할 수 있음) */
    {
        int pakAttempt;
        int pakOk = 0;
        for (pakAttempt = 0; pakAttempt < 3; pakAttempt++) {
            if (pakAttempt > 0) {
                scr_printf("  Retry %d/3 ...\n", pakAttempt + 1);
                FlushCache(0);
            }
            if (load_spritepack("sprites.pak", &pack)) {
                pakOk = 1;
                break;
            }
        }
        if (!pakOk) {
        scr_printf("  SPRITES.PAK FAILED! Trying individual files...\n\n");

        loading_msg(++step, totalSteps, "tile_solid");
        if (!read_sprite_data("tile_solid.ps2tex", &bank->tileSolid))
            read_sprite_data("tile_soild.ps2tex", &bank->tileSolid);
        scr_printf("  -> %s\n", bank->tileSolid.valid ? "OK" : "MISSING");

        loading_msg(++step, totalSteps, "tile_trap");
        read_sprite_data("tile_trap.ps2tex", &bank->tileTrap);
        scr_printf("  -> %s\n", bank->tileTrap.valid ? "OK" : "MISSING");

        loading_msg(++step, totalSteps, "tile_goal");
        read_sprite_data("tile_goal.ps2tex", &bank->tileGoal);
        scr_printf("  -> %s\n", bank->tileGoal.valid ? "OK" : "MISSING");

        loading_msg(++step, totalSteps, "background");
        read_sprite_data("bg.ps2tex", &bank->background);
        scr_printf("  -> %s\n", bank->background.valid ? "OK" : "MISSING");

        loading_msg(++step, totalSteps, "menu_background");
        read_sprite_data("bg_menu.ps2tex", &bank->menuBackground);
        scr_printf("  -> %s\n", bank->menuBackground.valid ? "OK" : "MISSING");

        loading_msg(++step, totalSteps, "tile_coinblock");
        read_sprite_data("tile_coinblock.ps2tex", &bank->tileCoinBlock);
        scr_printf("  -> %s\n", bank->tileCoinBlock.valid ? "OK" : "MISSING");

        loading_msg(++step, totalSteps, "tile_emptyblock");
        read_sprite_data("tile_emptyblock.ps2tex", &bank->tileEmptyBlock);
        scr_printf("  -> %s\n", bank->tileEmptyBlock.valid ? "OK" : "MISSING");

        loading_msg(++step, totalSteps, "tile_coin");
        read_sprite_data("tile_coin.ps2tex", &bank->tileCoin);
        scr_printf("  -> %s\n", bank->tileCoin.valid ? "OK" : "MISSING");

        loading_msg(++step, totalSteps, "tile_mushroom");
        read_sprite_data("tile_mushroom.ps2tex", &bank->tileMushroom);
        scr_printf("  -> %s\n", bank->tileMushroom.valid ? "OK" : "MISSING");

        loading_msg(++step, totalSteps, "tile_1up");
        read_sprite_data("tile_1up.ps2tex", &bank->tile1up);
        scr_printf("  -> %s\n", bank->tile1up.valid ? "OK" : "MISSING");

        loading_msg(++step, totalSteps, "player");
        read_sprite_data("player.ps2tex", &bank->player);
        scr_printf("  -> %s\n", bank->player.valid ? "OK" : "MISSING");

        for (a = 0; a < PLAYER_ANIM_COUNT; a++) {
            loading_msg(++step, totalSteps, animNames[a]);
            bank->animFrameCount[a] = read_anim_clip_data(animNames[a], bank->animFrames[a], MAX_ANIM_FRAMES);
            scr_printf("  -> %d frames\n", bank->animFrameCount[a]);
        }

        scr_printf("\nAll assets read to RAM. Initializing video...\n");
        return;
    } /* pakOk */
    } /* pakAttempt retry block */

    scr_printf("  -> %d sprites in pack\n\n", pack.count);

    loading_msg(++step, totalSteps, "tile_solid");
    if (!read_sprite_from_pack(&pack, "TILE_SOLID.PS2TEX", &bank->tileSolid))
        read_sprite_from_pack(&pack, "TILE_SOILD.PS2TEX", &bank->tileSolid);
    scr_printf("  -> %s\n", bank->tileSolid.valid ? "OK" : "MISSING");

    loading_msg(++step, totalSteps, "tile_trap");
    read_sprite_from_pack(&pack, "TILE_TRAP.PS2TEX", &bank->tileTrap);
    scr_printf("  -> %s\n", bank->tileTrap.valid ? "OK" : "MISSING");

    loading_msg(++step, totalSteps, "tile_goal");
    read_sprite_from_pack(&pack, "TILE_GOAL.PS2TEX", &bank->tileGoal);
    scr_printf("  -> %s\n", bank->tileGoal.valid ? "OK" : "MISSING");

    loading_msg(++step, totalSteps, "background");
    read_sprite_from_pack(&pack, "BG.PS2TEX", &bank->background);
    scr_printf("  -> %s\n", bank->background.valid ? "OK" : "MISSING");

    loading_msg(++step, totalSteps, "menu_background");
    read_sprite_from_pack(&pack, "BG_MENU.PS2TEX", &bank->menuBackground);
    scr_printf("  -> %s\n", bank->menuBackground.valid ? "OK" : "MISSING");

    loading_msg(++step, totalSteps, "tile_coinblock");
    read_sprite_from_pack(&pack, "TILE_COINBLOCK.PS2TEX", &bank->tileCoinBlock);
    scr_printf("  -> %s\n", bank->tileCoinBlock.valid ? "OK" : "MISSING");

    loading_msg(++step, totalSteps, "tile_emptyblock");
    read_sprite_from_pack(&pack, "TILE_EMPTYBLOCK.PS2TEX", &bank->tileEmptyBlock);
    scr_printf("  -> %s\n", bank->tileEmptyBlock.valid ? "OK" : "MISSING");

    loading_msg(++step, totalSteps, "tile_coin");
    read_sprite_from_pack(&pack, "TILE_COIN.PS2TEX", &bank->tileCoin);
    scr_printf("  -> %s\n", bank->tileCoin.valid ? "OK" : "MISSING");

    loading_msg(++step, totalSteps, "tile_mushroom");
    read_sprite_from_pack(&pack, "TILE_MUSHROOM.PS2TEX", &bank->tileMushroom);
    scr_printf("  -> %s\n", bank->tileMushroom.valid ? "OK" : "MISSING");

    loading_msg(++step, totalSteps, "tile_1up");
    read_sprite_from_pack(&pack, "TILE_1UP.PS2TEX", &bank->tile1up);
    scr_printf("  -> %s\n", bank->tile1up.valid ? "OK" : "MISSING");

    loading_msg(++step, totalSteps, "player");
    read_sprite_from_pack(&pack, "PLAYER.PS2TEX", &bank->player);
    scr_printf("  -> %s\n", bank->player.valid ? "OK" : "n/a");

    for (a = 0; a < PLAYER_ANIM_COUNT; a++) {
        char name[64];
        int i, loaded = 0;
        loading_msg(++step, totalSteps, animNames[a]);
        for (i = 0; i < MAX_ANIM_FRAMES; i++) {
            snprintf(name, sizeof(name), "%s_%02d.ps2tex", animNames[a], i);
            if (!read_sprite_from_pack(&pack, name, &bank->animFrames[a][i]))
                break;
            loaded++;
        }
        if (loaded == 0) {
            snprintf(name, sizeof(name), "%s.ps2tex", animNames[a]);
            if (read_sprite_from_pack(&pack, name, &bank->animFrames[a][0]))
                loaded = 1;
        }
        bank->animFrameCount[a] = loaded;
        scr_printf("  -> %d frames\n", loaded);
    }

    free_spritepack(&pack);
    scr_printf("\nAll assets read to RAM. Initializing video...\n");
}

void upload_all_assets_to_vram(GSGLOBAL *gsGlobal, const AssetBank *bank,
                               GameTextures *tex,
                               AnimationClip playerAnims[PLAYER_ANIM_COUNT])
{
    static const int animDelays[PLAYER_ANIM_COUNT] = { 12, 7, 5, 6, 6, 8, 6 };
    static const int animLoops[PLAYER_ANIM_COUNT]  = {  1, 1, 1, 0, 1, 0, 0 };
    int a, f;

    memset(tex, 0, sizeof(*tex));

    if (bank->tileSolid.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->tileSolid, &tex->tileSolid, &tex->tileSolidPixels, NULL, NULL);
        tex->hasTileSolid = 1;
    }
    if (bank->tileTrap.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->tileTrap, &tex->tileTrap, &tex->tileTrapPixels, NULL, NULL);
        tex->hasTileTrap = 1;
    }
    if (bank->tileGoal.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->tileGoal, &tex->tileGoal, &tex->tileGoalPixels, NULL, NULL);
        tex->hasTileGoal = 1;
    }
    if (bank->background.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->background, &tex->background, &tex->backgroundPixels, NULL, NULL);
        tex->hasBackground = 1;
    }
    if (bank->menuBackground.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->menuBackground, &tex->menuBackground, &tex->menuBackgroundPixels, NULL, NULL);
        tex->hasMenuBackground = 1;
    }
    if (bank->player.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->player, &tex->player, &tex->playerPixels, NULL, NULL);
        tex->hasPlayer = 1;
    }
    if (bank->tileCoinBlock.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->tileCoinBlock, &tex->tileCoinBlock, &tex->tileCoinBlockPixels, NULL, NULL);
        tex->hasTileCoinBlock = 1;
    }
    if (bank->tileEmptyBlock.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->tileEmptyBlock, &tex->tileEmptyBlock, &tex->tileEmptyBlockPixels, NULL, NULL);
        tex->hasTileEmptyBlock = 1;
    }
    if (bank->tileCoin.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->tileCoin, &tex->tileCoin, &tex->tileCoinPixels, NULL, NULL);
        tex->hasTileCoin = 1;
    }
    if (bank->tileMushroom.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->tileMushroom, &tex->tileMushroom, &tex->tileMushroomPixels, NULL, NULL);
        tex->hasTileMushroom = 1;
    }
    if (bank->tile1up.valid) {
        upload_sprite_to_vram(gsGlobal, &bank->tile1up, &tex->tile1up, &tex->tile1upPixels, NULL, NULL);
        tex->hasTile1up = 1;
    }

    for (a = 0; a < PLAYER_ANIM_COUNT; a++) {
        memset(&playerAnims[a], 0, sizeof(playerAnims[a]));
        playerAnims[a].frameDelay = animDelays[a];
        playerAnims[a].loop = animLoops[a];
        playerAnims[a].count = bank->animFrameCount[a];
        for (f = 0; f < bank->animFrameCount[a]; f++) {
            upload_sprite_to_vram(gsGlobal, &bank->animFrames[a][f],
                                  &playerAnims[a].frames[f],
                                  &playerAnims[a].pixels[f],
                                  &playerAnims[a].offsetX[f],
                                  &playerAnims[a].offsetY[f]);
        }
    }

    LOG("all assets uploaded to VRAM");
}
