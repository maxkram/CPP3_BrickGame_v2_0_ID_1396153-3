#ifndef SRC_BRICK_GAME_TETRIS_TETRIS_H_
#define SRC_BRICK_GAME_TETRIS_TETRIS_H_

#include <stdbool.h>

#include "brick_game/snake/game_info.h"

#ifdef __cplusplus
extern "C" {
#endif

void userInput(UserAction_t action, bool hold);

GameInfo_t updateCurrentState(void);

int isGameOver(void);

void tetrisTestSetPiece(int type, int rotation);

void tetrisTestFillRow(int row);

#ifdef __cplusplus
}
#endif

#endif  // SRC_BRICK_GAME_TETRIS_TETRIS_H_