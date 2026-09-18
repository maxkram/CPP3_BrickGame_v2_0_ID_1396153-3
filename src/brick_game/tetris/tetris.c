#include "brick_game/tetris/tetris.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FIELD_WIDTH 10
#define FIELD_HEIGHT 20
#define PIECE_SIZE 4
#define PIECE_COUNT 7
#define MAX_LEVEL 10
#define BASE_INTERVAL_MS 500L
#define INTERVAL_STEP_MS 40L
#define MIN_INTERVAL_MS 100L
#define HIGH_SCORE_FILE_NAME ".brickgame_tetris_highscore"

typedef enum {
  kStateStart,
  kStateSpawn,
  kStateMoving,
  kStatePause,
  kStateGameOver
} TetrisState;

static int g_field[FIELD_HEIGHT][FIELD_WIDTH];
static int g_display[FIELD_HEIGHT][FIELD_WIDTH];
static int g_cur[PIECE_SIZE][PIECE_SIZE];
static int g_next[PIECE_SIZE][PIECE_SIZE];
static int g_cur_row;
static int g_cur_col;

static TetrisState g_state = kStateStart;
static int g_score = 0;
static int g_high_score = 0;
static int g_level = 1;
static int g_speed = 1;
static int g_pause = 0;
static struct timespec g_last_tick;

static int *g_field_rows[FIELD_HEIGHT];
static int *g_next_rows[PIECE_SIZE];
static GameInfo_t g_info;

static const int kShapes[PIECE_COUNT][PIECE_SIZE][PIECE_SIZE] = {
    // I
    {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // O
    {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // T
    {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // S
    {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // Z
    {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // J
    {{1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // L
    {{0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
};

static void RotateShape(int src[PIECE_SIZE][PIECE_SIZE],
                        int dst[PIECE_SIZE][PIECE_SIZE]) {
  for (int i = 0; i < PIECE_SIZE; ++i) {
    for (int j = 0; j < PIECE_SIZE; ++j) {
      dst[i][j] = src[PIECE_SIZE - 1 - j][i];
    }
  }
}

static int CanPlace(int shape[PIECE_SIZE][PIECE_SIZE], int row, int col) {
  for (int i = 0; i < PIECE_SIZE; ++i) {
    for (int j = 0; j < PIECE_SIZE; ++j) {
      if (shape[i][j] == 0) {
        continue;
      }
      const int r = row + i;
      const int c = col + j;
      if (c < 0 || c >= FIELD_WIDTH || r >= FIELD_HEIGHT) {
        return 0;
      }
      if (r >= 0 && g_field[r][c] != 0) {
        return 0;
      }
    }
  }
  return 1;
}

static void Shift(int delta) {
  if (CanPlace(g_cur, g_cur_row, g_cur_col + delta)) {
    g_cur_col += delta;
  }
}

static void RotateCurrent(void) {
  int rotated[PIECE_SIZE][PIECE_SIZE];
  RotateShape(g_cur, rotated);
  if (CanPlace(rotated, g_cur_row, g_cur_col)) {
    memcpy(g_cur, rotated, sizeof(g_cur));
  }
}

static void HardDrop(void) {
  while (CanPlace(g_cur, g_cur_row + 1, g_cur_col)) {
    ++g_cur_row;
  }
}

static int ScoreForRows(int rows) {
  switch (rows) {
    case 1:
      return 100;
    case 2:
      return 300;
    case 3:
      return 700;
    case 4:
      return 1500;
    default:
      return 0;
  }
}

static void ClearLines(void) {
  int rows_cleared = 0;
  for (int row = FIELD_HEIGHT - 1; row >= 0; --row) {
    int full = 1;
    for (int col = 0; col < FIELD_WIDTH; ++col) {
      if (g_field[row][col] == 0) {
        full = 0;
        break;
      }
    }
    if (full) {
      ++rows_cleared;
      for (int r = row; r > 0; --r) {
        for (int c = 0; c < FIELD_WIDTH; ++c) {
          g_field[r][c] = g_field[r - 1][c];
        }
      }
      for (int c = 0; c < FIELD_WIDTH; ++c) {
        g_field[0][c] = 0;
      }
      ++row;
    }
  }
  if (rows_cleared > 0) {
    g_score += ScoreForRows(rows_cleared);
    const int new_level = g_score / 600 + 1;
    g_level = (new_level > MAX_LEVEL) ? MAX_LEVEL : new_level;
    g_speed = g_level;
  }
}

static void HighScoreFilePath(char *path, size_t size) {
  const char *home = getenv("HOME");
  snprintf(path, size, "%s/%s", (home != NULL) ? home : ".",
           HIGH_SCORE_FILE_NAME);
}

static void LoadHighScore(void) {
  char path[1024];
  HighScoreFilePath(path, sizeof(path));
  FILE *file = fopen(path, "r");
  if (file != NULL) {
    int value = 0;
    if (fscanf(file, "%d", &value) == 1) {
      g_high_score = value;
    }
    fclose(file);
  }
}

static void SaveHighScore(void) {
  char path[1024];
  HighScoreFilePath(path, sizeof(path));
  FILE *file = fopen(path, "w");
  if (file != NULL) {
    fprintf(file, "%d\n", g_high_score);
    fclose(file);
  }
}

static long long ElapsedMs(const struct timespec *since) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return (now.tv_sec - since->tv_sec) * 1000LL +
         (now.tv_nsec - since->tv_nsec) / 1000000LL;
}

static long TickIntervalMs(void) {
  long interval = BASE_INTERVAL_MS - (long)(g_level - 1) * INTERVAL_STEP_MS;
  if (interval < MIN_INTERVAL_MS) {
    interval = MIN_INTERVAL_MS;
  }
  return interval;
}

static void UpdateHighScore(void) {
  if (g_score > g_high_score) {
    g_high_score = g_score;
    SaveHighScore();
  }
}

static void SpawnPiece(void) {
  static int next_initialized = 0;
  if (!next_initialized) {
    const int type = rand() % PIECE_COUNT;
    memcpy(g_next, kShapes[type], sizeof(g_next));
    next_initialized = 1;
  }
  memcpy(g_cur, g_next, sizeof(g_cur));
  const int type = rand() % PIECE_COUNT;
  memcpy(g_next, kShapes[type], sizeof(g_next));
  g_cur_row = 0;
  g_cur_col = 3;
  if (!CanPlace(g_cur, g_cur_row, g_cur_col)) {
    g_state = kStateGameOver;
    UpdateHighScore();
  }
}

static void AttachPiece(void) {
  for (int i = 0; i < PIECE_SIZE; ++i) {
    for (int j = 0; j < PIECE_SIZE; ++j) {
      if (g_cur[i][j] != 0) {
        const int r = g_cur_row + i;
        const int c = g_cur_col + j;
        if (r >= 0 && r < FIELD_HEIGHT && c >= 0 && c < FIELD_WIDTH) {
          g_field[r][c] = 1;
        }
      }
    }
  }
  ClearLines();
  UpdateHighScore();
}

static void RefreshInfo(void) {
  for (int row = 0; row < FIELD_HEIGHT; ++row) {
    for (int col = 0; col < FIELD_WIDTH; ++col) {
      g_info.field[row][col] = g_field[row][col];
    }
  }
  if (g_state == kStateMoving || g_state == kStatePause) {
    for (int i = 0; i < PIECE_SIZE; ++i) {
      for (int j = 0; j < PIECE_SIZE; ++j) {
        if (g_cur[i][j] != 0) {
          const int r = g_cur_row + i;
          const int c = g_cur_col + j;
          if (r >= 0 && r < FIELD_HEIGHT && c >= 0 && c < FIELD_WIDTH) {
            g_info.field[r][c] = 1;
          }
        }
      }
    }
  }
  for (int i = 0; i < PIECE_SIZE; ++i) {
    for (int j = 0; j < PIECE_SIZE; ++j) {
      g_info.next[i][j] = g_next[i][j];
    }
  }
  g_info.score = g_score;
  g_info.high_score = g_high_score;
  g_info.level = g_level;
  g_info.speed = g_speed;
  g_info.pause = g_pause;
}

static void StartGame(void) {
  g_state = kStateSpawn;
  memset(g_field, 0, sizeof(g_field));
  g_score = 0;
  g_level = 1;
  g_speed = 1;
  g_pause = 0;
  srand((unsigned)time(NULL));
  clock_gettime(CLOCK_MONOTONIC, &g_last_tick);
  SpawnPiece();
  if (g_state != kStateGameOver) {
    g_state = kStateMoving;
  }
}

static void EnsureInfoInit(void) {
  static int initialized = 0;
  if (!initialized) {
    for (int row = 0; row < FIELD_HEIGHT; ++row) {
      g_field_rows[row] = g_display[row];
    }
    for (int i = 0; i < PIECE_SIZE; ++i) {
      g_next_rows[i] = g_next[i];
    }
    g_info.field = g_field_rows;
    g_info.next = g_next_rows;
    LoadHighScore();
    initialized = 1;
  }
}

void userInput(UserAction_t action, bool hold) {
  EnsureInfoInit();
  (void)hold;
  switch (action) {
    case Start:
      if (g_state == kStateStart || g_state == kStateGameOver) {
        StartGame();
      }
      break;
    case Pause:
      if (g_state == kStateMoving) {
        g_pause = 1;
        g_state = kStatePause;
      } else if (g_state == kStatePause) {
        g_pause = 0;
        clock_gettime(CLOCK_MONOTONIC, &g_last_tick);
        g_state = kStateMoving;
      }
      break;
    case Terminate:
      if (g_state == kStateMoving || g_state == kStatePause) {
        g_state = kStateGameOver;
        g_pause = 0;
        UpdateHighScore();
        SaveHighScore();
      }
      break;
    case Left:
      if (g_state == kStateMoving) {
        Shift(-1);
      }
      break;
    case Right:
      if (g_state == kStateMoving) {
        Shift(1);
      }
      break;
    case Up:
      break;
    case Down:
      if (g_state == kStateMoving) {
        HardDrop();
        AttachPiece();
        clock_gettime(CLOCK_MONOTONIC, &g_last_tick);
        SpawnPiece();
      }
      break;
    case Action:
      if (g_state == kStateMoving) {
        RotateCurrent();
      }
      break;
  }
}

GameInfo_t updateCurrentState(void) {
  EnsureInfoInit();
  if (g_state == kStateSpawn) {
    g_state = kStateMoving;
  } else if (g_state == kStateMoving && g_pause == 0) {
    const long long elapsed = ElapsedMs(&g_last_tick);
    if (elapsed >= (long long)TickIntervalMs()) {
      if (CanPlace(g_cur, g_cur_row + 1, g_cur_col)) {
        ++g_cur_row;
      } else {
        AttachPiece();
        SpawnPiece();
      }
      clock_gettime(CLOCK_MONOTONIC, &g_last_tick);
    }
  }
  RefreshInfo();
  return g_info;
}

int isGameOver(void) { return g_state == kStateGameOver ? 1 : 0; }

void tetrisTestSetPiece(int type, int rotation) {
  if (type < 0 || type >= PIECE_COUNT) {
    return;
  }
  int shape[PIECE_SIZE][PIECE_SIZE];
  memcpy(shape, kShapes[type], sizeof(shape));
  for (int r = 0; r < rotation % 4; ++r) {
    int rotated[PIECE_SIZE][PIECE_SIZE];
    RotateShape(shape, rotated);
    memcpy(shape, rotated, sizeof(shape));
  }
  memcpy(g_cur, shape, sizeof(g_cur));
}

void tetrisTestFillRow(int row) {
  if (row < 0 || row >= FIELD_HEIGHT) {
    return;
  }
  for (int col = 0; col < FIELD_WIDTH; ++col) {
    g_field[row][col] = 1;
  }
}
