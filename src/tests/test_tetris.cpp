#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include "brick_game/tetris/tetris.h"

struct TetrisHighScoreIsolator {
  std::string test_home;
  TetrisHighScoreIsolator() {
    test_home = "/tmp/s21_tetris_tests_home";
    mkdir(test_home.c_str(), 0700);
    std::remove((test_home + "/.brickgame_tetris_highscore").c_str());
    setenv("HOME", test_home.c_str(), 1);
  }
};
TetrisHighScoreIsolator g_tetris_isolator;

namespace {

void StartFreshGame() {
  userInput(Terminate, false);
  userInput(Start, false);
  updateCurrentState();
}

int CountFieldCells(const GameInfo_t& info) {
  int count = 0;
  for (int row = 0; row < 20; ++row) {
    for (int col = 0; col < 10; ++col) {
      if (info.field[row][col] != 0) {
        ++count;
      }
    }
  }
  return count;
}

int CountNextCells(const GameInfo_t& info) {
  int count = 0;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (info.next[i][j] != 0) {
        ++count;
      }
    }
  }
  return count;
}

}  // namespace

// ---------------------------------------------------------------------
// Начальное состояние и переходы КА
// ---------------------------------------------------------------------

TEST(TetrisFsmTest, InitialState) {
  userInput(Terminate, false);
  const GameInfo_t info = updateCurrentState();
  EXPECT_EQ(isGameOver(), 0);
  EXPECT_EQ(info.pause, 0);
  EXPECT_NE(info.field, nullptr);
  EXPECT_NE(info.next, nullptr);
  for (int row = 0; row < 20; ++row) {
    for (int col = 0; col < 10; ++col) {
      EXPECT_EQ(info.field[row][col], 0);
    }
  }
}

TEST(TetrisFsmTest, StartSpawnsPiece) {
  StartFreshGame();
  const GameInfo_t info = updateCurrentState();
  EXPECT_EQ(CountNextCells(info), 4);
  EXPECT_GT(CountFieldCells(info), 0);
  EXPECT_EQ(isGameOver(), 0);
}

TEST(TetrisFsmTest, PauseToggles) {
  StartFreshGame();
  userInput(Pause, false);
  EXPECT_EQ(updateCurrentState().pause, 1);
  userInput(Pause, false);
  EXPECT_EQ(updateCurrentState().pause, 0);
}

TEST(TetrisFsmTest, TerminateEndsGameAndRestartWorks) {
  StartFreshGame();
  userInput(Terminate, false);
  EXPECT_EQ(isGameOver(), 1);
  // Рестарт по кнопке Start.
  StartFreshGame();
  EXPECT_EQ(isGameOver(), 0);
}

// ---------------------------------------------------------------------
// Движение, повороты, коллизии
// ---------------------------------------------------------------------

TEST(TetrisMoveTest, RenderingDoesNotAttachOrLeaveTrails) {
  StartFreshGame();
  tetrisTestSetPiece(0, 0);
  for (int step = 0; step < 3; ++step) {
    EXPECT_EQ(CountFieldCells(updateCurrentState()), 4);
    userInput(Right, false);
  }
  EXPECT_EQ(CountFieldCells(updateCurrentState()), 4);
  userInput(Down, false);
  EXPECT_EQ(CountFieldCells(updateCurrentState()), 8);
}

TEST(TetrisFsmTest, RestartClearsFieldAndPause) {
  StartFreshGame();
  userInput(Down, false);
  userInput(Pause, false);
  userInput(Terminate, false);
  EXPECT_EQ(updateCurrentState().pause, 0);
  userInput(Start, false);
  const GameInfo_t info = updateCurrentState();
  EXPECT_EQ(isGameOver(), 0);
  EXPECT_EQ(info.score, 0);
  EXPECT_EQ(info.pause, 0);
  EXPECT_EQ(CountFieldCells(info), 4);
}

TEST(TetrisMoveTest, HardDropAttachesPiece) {
  StartFreshGame();
  userInput(Down, false);
  userInput(Down, false);
  const GameInfo_t info = updateCurrentState();
  EXPECT_GE(CountFieldCells(info), 8);
  EXPECT_EQ(isGameOver(), 0);
}

TEST(TetrisMoveTest, RotationWorks) {
  StartFreshGame();
  tetrisTestSetPiece(0, 0);
  userInput(Action, false);
  const GameInfo_t info = updateCurrentState();
  EXPECT_EQ(isGameOver(), 0);
  EXPECT_GT(CountFieldCells(info), 0);
}

TEST(TetrisMoveTest, PieceCannotLeaveField) {
  StartFreshGame();
  for (int i = 0; i < 20; ++i) {
    userInput(Left, false);
  }
  userInput(Down, false);
  const GameInfo_t info = updateCurrentState();
  EXPECT_EQ(CountFieldCells(info), 8);
  EXPECT_EQ(isGameOver(), 0);
}

// ---------------------------------------------------------------------
// Рекорд
// ---------------------------------------------------------------------

TEST(TetrisScoreTest, HighScoreFileCreatedOnGameOver) {
  char tmp_dir[] = "/tmp/s21_tetris_test_XXXXXX";
  char* dir = mkdtemp(tmp_dir);
  ASSERT_NE(dir, nullptr);
  setenv("HOME", dir, 1);

  StartFreshGame();
  userInput(Terminate, false); 

  const std::string path = std::string(dir) + "/.brickgame_tetris_highscore";
  FILE* file = fopen(path.c_str(), "r");
  ASSERT_NE(file, nullptr);
  fclose(file);

  std::remove(path.c_str());
  rmdir(dir);
  setenv("HOME", g_tetris_isolator.test_home.c_str(), 1);
}

// ---------------------------------------------------------------------
// C-API
// ---------------------------------------------------------------------

TEST(TetrisCApiTest, ApiFunctionsPresent) {
  userInput(Start, false);
  GameInfo_t info = updateCurrentState();
  EXPECT_NE(info.field, nullptr);
  EXPECT_NE(info.next, nullptr);
  EXPECT_EQ(info.level, 1);
  EXPECT_EQ(info.pause, 0);
  EXPECT_EQ(isGameOver(), 0);
  userInput(Terminate, false);
  EXPECT_EQ(isGameOver(), 1);
}

TEST(TetrisCApiTest, NextPieceAlwaysHasFourCells) {
  StartFreshGame();
  for (int i = 0; i < 10; ++i) {
    if (isGameOver()) {
      StartFreshGame();
    }
    userInput(Down, false);
    const GameInfo_t info = updateCurrentState();
    EXPECT_EQ(CountNextCells(info), 4);
  }
}

// ---------------------------------------------------------------------
// Очки, уровни, разрушение рядов (детерминированно через тестовый хук)
// ---------------------------------------------------------------------

namespace {

int DropAndAttach(int score_before) {
  userInput(Down, false);
  const GameInfo_t info = updateCurrentState();
  return info.score - score_before;
}

}  // namespace

TEST(TetrisScoreTest, OneRowIs100Points) {
  StartFreshGame();
  tetrisTestFillRow(19);
  const int delta = DropAndAttach(0);
  EXPECT_EQ(delta, 100);
  EXPECT_EQ(updateCurrentState().level, 1);
}

TEST(TetrisScoreTest, TwoRowsAre300Points) {
  StartFreshGame();
  tetrisTestFillRow(18);
  tetrisTestFillRow(19);
  const int delta = DropAndAttach(0);
  EXPECT_EQ(delta, 300);
}

TEST(TetrisScoreTest, ThreeRowsAre700Points) {
  StartFreshGame();
  tetrisTestFillRow(17);
  tetrisTestFillRow(18);
  tetrisTestFillRow(19);
  const int delta = DropAndAttach(0);
  EXPECT_EQ(delta, 700);
}

TEST(TetrisScoreTest, FourRowsAre1500Points) {
  StartFreshGame();
  tetrisTestFillRow(16);
  tetrisTestFillRow(17);
  tetrisTestFillRow(18);
  tetrisTestFillRow(19);
  const int delta = DropAndAttach(0);
  EXPECT_EQ(delta, 1500);
}

TEST(TetrisScoreTest, LevelGrowsEvery600Points) {
  StartFreshGame();
  tetrisTestFillRow(16);
  tetrisTestFillRow(17);
  tetrisTestFillRow(18);
  tetrisTestFillRow(19);
  DropAndAttach(0);
  EXPECT_EQ(updateCurrentState().level, 3);
  EXPECT_EQ(updateCurrentState().speed, 3);
}

TEST(TetrisScoreTest, LevelCappedAt10) {
  StartFreshGame();
  for (int round = 0; round < 4; ++round) {
    tetrisTestFillRow(16);
    tetrisTestFillRow(17);
    tetrisTestFillRow(18);
    tetrisTestFillRow(19);
    DropAndAttach(0);
  }
  EXPECT_EQ(updateCurrentState().level, 10);
}

TEST(TetrisScoreTest, HighScoreUpdatesDuringGame) {
  StartFreshGame();
  tetrisTestFillRow(19);
  DropAndAttach(0);
  const GameInfo_t info = updateCurrentState();
  EXPECT_GE(info.high_score, info.score);
}