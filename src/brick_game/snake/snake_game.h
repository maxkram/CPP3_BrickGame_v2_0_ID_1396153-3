#ifndef SRC_BRICK_GAME_SNAKE_SNAKE_GAME_H_
#define SRC_BRICK_GAME_SNAKE_SNAKE_GAME_H_

#include <stdbool.h>

#include "game_info.h"

#ifdef __cplusplus
extern "C" {
#endif

void userInput(UserAction_t action, bool hold);

GameInfo_t updateCurrentState(void);

int isGameOver(void);

int isWin(void);

#ifdef __cplusplus
}
#endif

#endif  // SRC_BRICK_GAME_SNAKE_SNAKE_GAME_H_
