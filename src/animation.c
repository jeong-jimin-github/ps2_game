/**
 * @file animation.c
 * @brief 플레이어 애니메이션 구현
 */
#include "animation.h"

#include <string.h>
#include <libpad.h>

AnimationClip *get_player_clip_safe(AnimationClip clips[PLAYER_ANIM_COUNT], PlayerAnimState state)
{
    int i;
    AnimationClip *clip = &clips[(int)state];
    if (clip->count > 0) {
        return clip;
    }

    clip = &clips[(int)PLAYER_ANIM_IDLE];
    if (clip->count > 0) {
        return clip;
    }

    for (i = 0; i < PLAYER_ANIM_COUNT; i++) {
        if (clips[i].count > 0) {
            return &clips[i];
        }
    }

    return NULL;
}

void animator_init(PlayerAnimator *anim)
{
    memset(anim, 0, sizeof(*anim));
    anim->state = PLAYER_ANIM_IDLE;
    anim->pendingState = PLAYER_ANIM_IDLE;
}

PlayerAnimState pick_player_anim_state(const Player *p,
                                       unsigned int padData,
                                       int gameOver,
                                       int hurtTimer,
                                       int clearTimer,
                                       int *groundedGrace)
{
    int left = (padData & PAD_LEFT) != 0;
    int right = (padData & PAD_RIGHT) != 0;
    int moveInput = (left ^ right);
    int runInput = moveInput && ((padData & PAD_CROSS) != 0);

    if (p->onGround) {
        *groundedGrace = 3;
    } else if (*groundedGrace > 0) {
        (*groundedGrace)--;
    }

    if (gameOver) {
        return PLAYER_ANIM_DEAD;
    }
    if (hurtTimer > 0) {
        return PLAYER_ANIM_HURT;
    }
    if (clearTimer > 0) {
        return PLAYER_ANIM_CLEAR;
    }

    if (!p->onGround && *groundedGrace == 0) {
        return PLAYER_ANIM_JUMP;
    }
    if (runInput) {
        return PLAYER_ANIM_RUN;
    }
    if (moveInput) {
        return PLAYER_ANIM_WALK;
    }
    return PLAYER_ANIM_IDLE;
}

void animator_step(PlayerAnimator *anim,
                   AnimationClip clips[PLAYER_ANIM_COUNT],
                   const Player *p,
                   unsigned int padData,
                   int gameOver,
                   int hurtTimer,
                   int clearTimer)
{
    PlayerAnimState candidate = pick_player_anim_state(p, padData, gameOver, hurtTimer, clearTimer, &anim->groundedGrace);
    int requiredStable = (candidate == PLAYER_ANIM_JUMP || candidate == PLAYER_ANIM_HURT ||
                          candidate == PLAYER_ANIM_DEAD || candidate == PLAYER_ANIM_CLEAR)
                             ? 1
                             : 3;

    if (candidate != anim->state) {
        if (anim->pendingState != candidate) {
            anim->pendingState = candidate;
            anim->pendingTicks = 1;
        } else {
            anim->pendingTicks++;
        }

        if (anim->pendingTicks >= requiredStable) {
            anim->state = candidate;
            anim->frame = 0;
            anim->tick = 0;
            anim->pendingTicks = 0;
        }
    } else {
        anim->pendingState = candidate;
        anim->pendingTicks = 0;
    }

    {
        AnimationClip *clip = get_player_clip_safe(clips, anim->state);
        int frameDelay = 1;
        if (clip == NULL || clip->count <= 0) {
            return;
        }

        if (clip->frameDelay > 0) {
            frameDelay = clip->frameDelay;
        }

        anim->tick++;
        if (anim->tick >= frameDelay) {
            anim->tick = 0;
            if (clip->loop) {
                anim->frame = (anim->frame + 1) % clip->count;
            } else if (anim->frame < clip->count - 1) {
                anim->frame++;
            }
        }
    }
}
