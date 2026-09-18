#ifndef SRC_BRICK_GAME_SNAKE_GAME_INFO_H_
#define SRC_BRICK_GAME_SNAKE_GAME_INFO_H_

#include <stdbool.h>

#ifdef __cplusplus  // предохранитель от ошибки переопределения типов
extern "C" {
#endif

typedef enum {
  Start,
  Pause,
  Terminate,
  Left,
  Right,
  Up,
  Down,
  Action
} UserAction_t;

typedef struct {
  int **field;
  int **next; 
  int score;
  int high_score;
  int level;
  int speed;
  int pause;
} GameInfo_t;

#ifdef __cplusplus
}
#endif

#endif  // SRC_BRICK_GAME_SNAKE_GAME_INFO_H_
