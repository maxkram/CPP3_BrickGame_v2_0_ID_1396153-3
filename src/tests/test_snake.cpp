#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "brick_game/snake/controller.h"
#include "brick_game/snake/model.h"
#include "brick_game/snake/snake_game.h"

namespace s21 {
class HighScoreReset : public testing::EmptyTestEventListener {
 public:
  void OnTestStart(const testing::TestInfo&) override {
    std::remove(
        (std::string(std::getenv("HOME")) + "/.brickgame_snake_highscore")
            .c_str());
  }
};

struct HighScoreIsolator {
  std::string test_home;
  HighScoreIsolator() {
    char path[] = "/tmp/s21_snake_tests_XXXXXX";
    char* dir = mkdtemp(path);
    if (dir == nullptr) {
      std::abort();
    }
    test_home = dir;
    setenv("HOME", test_home.c_str(), 1);
    testing::UnitTest::GetInstance()->listeners().Append(new HighScoreReset);
  }
  ~HighScoreIsolator() {
    std::remove((test_home + "/.brickgame_snake_highscore").c_str());
    rmdir(test_home.c_str());
  }
};
HighScoreIsolator g_isolator;
}  // namespace s21
using s21::g_isolator;

namespace s21 {
class SnakeModelTest {
 public:
  static void SetSnake(s21::Model& model,
                       const std::vector<s21::Model::Point>& cells) {
    model.snake_.clear();
    for (const auto& cell : cells) {
      model.snake_.push_back(cell);
    }
  }

  static void SetApple(s21::Model& model, const s21::Model::Point& apple) {
    model.apple_ = apple;
    model.apple_spawned_ = true;
  }

  static void SetDirection(s21::Model& model, s21::Model::Direction direction) {
    model.direction_ = direction;
  }

  static void SetScore(s21::Model& model, int score) { model.score_ = score; }

  static void SetLevel(s21::Model& model, int level) {
    model.level_ = level;
    model.speed_ = level;
  }

  static void MoveLastTickBack(s21::Model& model, long long ms) {
    model.last_tick_ =
        std::chrono::steady_clock::now() - std::chrono::milliseconds(ms);
  }

  static bool Hold(const s21::Model& model) { return model.hold_; }

  static s21::Model::Point Head(const s21::Model& model) {
    return model.snake_.front();
  }
};
}  // namespace s21

using s21::SnakeModelTest;

constexpr int kStartHeadRow = s21::Model::kFieldHeight / 2;
constexpr int kStartHeadCol = s21::Model::kFieldWidth / 2;

// ---------------------------------------------------------------------
// Начальное состояние и переходы конечного автомата
// ---------------------------------------------------------------------

TEST(SnakeModelTest, InitialState) {
  s21::Model model;
  EXPECT_EQ(model.state(), s21::Model::State::kStart);
  EXPECT_EQ(model.score(), 0);
  EXPECT_EQ(model.level(), 1);
  EXPECT_EQ(model.speed(), 1);
  EXPECT_EQ(model.high_score(), 0);
  EXPECT_FALSE(model.IsGameOver());
  EXPECT_TRUE(model.snake().empty());
  EXPECT_FALSE(model.paused());

  const GameInfo_t info = model.UpdateCurrentState();
  EXPECT_EQ(info.pause, 0);
  EXPECT_EQ(info.next, nullptr);
  ASSERT_NE(info.field, nullptr);
  for (int row = 0; row < s21::Model::kFieldHeight; ++row) {
    for (int col = 0; col < s21::Model::kFieldWidth; ++col) {
      EXPECT_EQ(info.field[row][col], s21::Model::kEmptyValue);
    }
  }
}

TEST(SnakeModelTest, StartTransitionsToSpawnThenMoving) {
  s21::Model model;
  model.UserInput(Start, false);
  EXPECT_EQ(model.state(), s21::Model::State::kSpawn);
  EXPECT_EQ(model.snake().size(), 4u);

  const GameInfo_t info = model.UpdateCurrentState();
  EXPECT_EQ(model.state(), s21::Model::State::kMoving);
  EXPECT_EQ(model.snake().size(), 4u);
  EXPECT_EQ(info.level, 1);
  EXPECT_EQ(info.speed, 1);

  int snake_cells = 0;
  int apples = 0;
  for (int row = 0; row < s21::Model::kFieldHeight; ++row) {
    for (int col = 0; col < s21::Model::kFieldWidth; ++col) {
      if (info.field[row][col] == s21::Model::kSnakeValue) {
        ++snake_cells;
      } else if (info.field[row][col] == s21::Model::kAppleValue) {
        ++apples;
      }
    }
  }
  EXPECT_EQ(snake_cells, 4);
  EXPECT_EQ(apples, 1);

  for (const auto& cell : model.snake()) {
    EXPECT_NE(model.apple(), cell);
  }
}

TEST(SnakeModelTest, StartIgnoredDuringMoving) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();
  const size_t length = model.snake().size();
  const auto head = SnakeModelTest::Head(model);

  model.UserInput(Start, false);
  EXPECT_EQ(model.state(), s21::Model::State::kMoving);
  EXPECT_EQ(model.snake().size(), length);
  EXPECT_EQ(SnakeModelTest::Head(model), head);
}

TEST(SnakeModelTest, PauseTransitions) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();

  model.UserInput(Pause, false);
  EXPECT_EQ(model.state(), s21::Model::State::kPause);
  EXPECT_TRUE(model.paused());
  EXPECT_EQ(model.UpdateCurrentState().pause, 1);

  SnakeModelTest::MoveLastTickBack(model, 1000);
  const auto head = SnakeModelTest::Head(model);
  model.Tick();
  EXPECT_EQ(SnakeModelTest::Head(model), head);
  EXPECT_EQ(model.state(), s21::Model::State::kPause);

  model.UserInput(Pause, false);
  EXPECT_EQ(model.state(), s21::Model::State::kMoving);
  EXPECT_FALSE(model.paused());
  EXPECT_EQ(model.UpdateCurrentState().pause, 0);
}

TEST(SnakeModelTest, TerminateIgnoredInModel) {
  s21::Model model;
  EXPECT_EQ(model.state(), s21::Model::State::kStart);
  model.UserInput(Terminate, false);
  EXPECT_EQ(model.state(), s21::Model::State::kStart);
}

TEST(SnakeModelTest, RestartFromGameOver) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();
  SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
  SnakeModelTest::SetSnake(model, {{5, 9}, {5, 8}, {5, 7}, {5, 6}});
  SnakeModelTest::SetApple(model, {0, 0});
  model.Tick();
  EXPECT_TRUE(model.IsGameOver());
  EXPECT_FALSE(model.IsWin());

  model.UserInput(Start, false);
  EXPECT_EQ(model.state(), s21::Model::State::kSpawn);
  model.UpdateCurrentState();
  EXPECT_EQ(model.state(), s21::Model::State::kMoving);
  EXPECT_EQ(model.snake().size(), 4u);
  EXPECT_EQ(model.score(), 0);
  EXPECT_EQ(model.level(), 1);
}

// ---------------------------------------------------------------------
// Движение и управление
// ---------------------------------------------------------------------

TEST(SnakeModelTest, MovementForward) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();
  EXPECT_EQ(SnakeModelTest::Head(model),
            s21::Model::Point(kStartHeadRow, kStartHeadCol));

  SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
  const auto head = SnakeModelTest::Head(model);
  model.Tick();
  EXPECT_EQ(SnakeModelTest::Head(model),
            s21::Model::Point(head.first, head.second + 1));
}

TEST(SnakeModelTest, TurnLeftAndRightOnly) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();

  SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
  model.UserInput(Left, false);
  const auto head = SnakeModelTest::Head(model);
  model.Tick();
  EXPECT_EQ(SnakeModelTest::Head(model),
            s21::Model::Point(head.first, head.second + 1));

  model.UserInput(Up, false);
  const auto head2 = SnakeModelTest::Head(model);
  model.Tick();
  EXPECT_EQ(SnakeModelTest::Head(model),
            s21::Model::Point(head2.first - 1, head2.second));

  model.UserInput(Down, false);
  const auto head3 = SnakeModelTest::Head(model);
  model.Tick();
  EXPECT_EQ(SnakeModelTest::Head(model),
            s21::Model::Point(head3.first - 1, head3.second));

  model.UserInput(Right, false);
  const auto head4 = SnakeModelTest::Head(model);
  model.Tick();
  EXPECT_EQ(SnakeModelTest::Head(model),
            s21::Model::Point(head4.first, head4.second + 1));
}

TEST(SnakeModelTest, DoubleTurnWithinOneTickCannotReverse) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();

  const auto head = SnakeModelTest::Head(model);
  model.UserInput(Up, false);
  model.UserInput(Left, false);
  model.Tick();

  EXPECT_EQ(SnakeModelTest::Head(model),
            s21::Model::Point(head.first - 1, head.second));
  EXPECT_FALSE(model.IsGameOver());
}

TEST(SnakeModelTest, DirectionIgnoredOutsideMoving) {
  s21::Model model;
  model.UserInput(Up, false);
  model.UserInput(Left, false);
  model.Tick();
  EXPECT_EQ(model.state(), s21::Model::State::kStart);
  EXPECT_TRUE(model.snake().empty());
}

TEST(SnakeModelTest, ActionHoldAccelerates) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();
  SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
  const auto head = SnakeModelTest::Head(model);

  model.UserInput(Action, false);
  EXPECT_FALSE(SnakeModelTest::Hold(model));
  SnakeModelTest::MoveLastTickBack(model, 300);
  model.UpdateCurrentState();
  EXPECT_EQ(SnakeModelTest::Head(model), head);

  model.UserInput(Action, true);
  EXPECT_TRUE(SnakeModelTest::Hold(model));
  SnakeModelTest::MoveLastTickBack(model, 300);
  model.UpdateCurrentState();
  EXPECT_NE(SnakeModelTest::Head(model), head);
}

TEST(SnakeModelTest, HoldKeptOnDirectionInput) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();

  model.UserInput(Action, true);
  EXPECT_TRUE(SnakeModelTest::Hold(model));

  model.UserInput(Up, false);
  EXPECT_TRUE(SnakeModelTest::Hold(model));
  model.UserInput(Left, false);
  EXPECT_TRUE(SnakeModelTest::Hold(model));

  model.UserInput(Action, false);
  EXPECT_FALSE(SnakeModelTest::Hold(model));
}

// ---------------------------------------------------------------------
// Механики: яблоко, очки, уровни, столкновения, победа
// ---------------------------------------------------------------------

TEST(SnakeModelTest, EatAppleGrowsAndScores) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();

  SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
  SnakeModelTest::SetSnake(model, {{5, 5}, {5, 4}, {5, 3}, {5, 2}});
  SnakeModelTest::SetApple(model, {5, 6});

  EXPECT_EQ(model.snake().size(), 4u);
  model.Tick();

  EXPECT_EQ(model.snake().size(), 5u);
  EXPECT_EQ(model.score(), 1);
  EXPECT_EQ(model.high_score(), 1);
  EXPECT_EQ(model.level(), 1);
  EXPECT_FALSE(model.IsGameOver());

  const GameInfo_t info = model.UpdateCurrentState();
  int apples = 0;
  for (int row = 0; row < s21::Model::kFieldHeight; ++row) {
    for (int col = 0; col < s21::Model::kFieldWidth; ++col) {
      if (info.field[row][col] == s21::Model::kAppleValue) {
        ++apples;
      }
    }
  }
  EXPECT_EQ(apples, 1);
}

TEST(SnakeModelTest, LevelUpEveryFivePoints) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();

  SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
  SnakeModelTest::SetScore(model, 4);
  SnakeModelTest::SetSnake(model, {{5, 5}, {5, 4}, {5, 3}, {5, 2}});
  SnakeModelTest::SetApple(model, {5, 6});
  model.Tick();
  EXPECT_EQ(model.score(), 5);
  EXPECT_EQ(model.level(), 2);
  EXPECT_EQ(model.speed(), 2);
}

TEST(SnakeModelTest, LevelCapAtTen) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();

  SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
  SnakeModelTest::SetScore(model, 49);
  SnakeModelTest::SetSnake(model, {{5, 5}, {5, 4}, {5, 3}, {5, 2}});
  SnakeModelTest::SetApple(model, {5, 6});
  model.Tick();
  EXPECT_EQ(model.score(), 50);
  EXPECT_EQ(model.level(), s21::Model::kMaxLevel);
  EXPECT_EQ(model.speed(), s21::Model::kMaxLevel);
}

TEST(SnakeModelTest, IntervalDecreasesWithLevel) {
  s21::Model model;
  EXPECT_EQ(model.TickIntervalMs(), 500);
  SnakeModelTest::SetLevel(model, 5);
  EXPECT_EQ(model.TickIntervalMs(), 340);
  SnakeModelTest::SetLevel(model, s21::Model::kMaxLevel);
  EXPECT_EQ(model.TickIntervalMs(), 140);
  SnakeModelTest::SetLevel(model, 100);
  EXPECT_EQ(model.TickIntervalMs(), 100);
}

TEST(SnakeModelTest, WallCollisionLose) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();

  SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
  SnakeModelTest::SetSnake(model, {{5, 9}, {5, 8}, {5, 7}, {5, 6}});
  SnakeModelTest::SetApple(model, {0, 0});
  model.Tick();

  EXPECT_TRUE(model.IsGameOver());
  EXPECT_FALSE(model.IsWin());
}

TEST(SnakeModelTest, SelfCollisionLose) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();

  SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
  SnakeModelTest::SetSnake(model, {{5, 4}, {5, 5}, {6, 5}, {6, 4}});
  SnakeModelTest::SetApple(model, {0, 0});
  model.Tick();

  EXPECT_TRUE(model.IsGameOver());
  EXPECT_FALSE(model.IsWin());
}

TEST(SnakeModelTest, WinAtMaxLength) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();

  std::vector<s21::Model::Point> cells;
  cells.push_back({10, 4});
  for (int row = 0; row < s21::Model::kFieldHeight; ++row) {
    for (int col = 0; col < s21::Model::kFieldWidth; ++col) {
      if ((row == 10 && col == 4) || (row == 10 && col == 5)) {
        continue;
      }
      cells.push_back({row, col});
    }
  }
  ASSERT_EQ(cells.size(), 199u);
  SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
  SnakeModelTest::SetSnake(model, cells);
  SnakeModelTest::SetApple(model, {10, 5});
  model.Tick();

  EXPECT_TRUE(model.IsGameOver());
  EXPECT_TRUE(model.IsWin());
}

TEST(SnakeModelTest, HighScoreSavedToFile) {
  char tmp_dir[] = "/tmp/s21_snake_test_XXXXXX";
  char* dir = mkdtemp(tmp_dir);
  ASSERT_NE(dir, nullptr);
  setenv("HOME", dir, 1);

  {
    s21::Model model;
    model.UserInput(Start, false);
    model.UpdateCurrentState();
    SnakeModelTest::SetDirection(model, s21::Model::Direction::kRight);
    SnakeModelTest::SetSnake(model, {{5, 5}, {5, 4}, {5, 3}, {5, 2}});
    SnakeModelTest::SetApple(model, {5, 6});
    model.Tick();
    EXPECT_EQ(model.high_score(), 1);
  }
  {
    s21::Model model;
    EXPECT_EQ(model.high_score(), 1);
  }

  const std::string file = std::string(dir) + "/.brickgame_snake_highscore";
  std::ifstream input(file);
  int value = 0;
  input >> value;
  EXPECT_EQ(value, 1);

  std::remove(file.c_str());
  rmdir(dir);
  setenv("HOME", g_isolator.test_home.c_str(), 1);
}

TEST(SnakeModelTest, AppleNotOnSnakeAfterMove) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();
  for (int i = 0; i < 20; ++i) {
    model.Tick();
  }
  for (const auto& cell : model.snake()) {
    EXPECT_NE(model.apple(), cell);
  }
}

// ---------------------------------------------------------------------
// C-API и контроллер
// ---------------------------------------------------------------------

TEST(SnakeModelTest, CanEnterVacatedTailCell) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();
  SnakeModelTest::SetSnake(model, {{5, 5}, {5, 4}, {6, 4}, {6, 5}});
  SnakeModelTest::SetDirection(model, s21::Model::Direction::kDown);
  SnakeModelTest::SetApple(model, {0, 0});
  model.Tick();
  EXPECT_FALSE(model.IsGameOver());
  EXPECT_EQ(model.snake().front(), s21::Model::Point(6, 5));
  EXPECT_EQ(model.snake().size(), 4u);
}

TEST(SnakeModelTest, ReleaseRestoresNormalInterval) {
  s21::Model model;
  model.UserInput(Start, false);
  model.UpdateCurrentState();
  SnakeModelTest::SetApple(model, {0, 0});
  const auto head = model.snake().front();
  model.UserInput(Action, true);
  model.UserInput(Action, false);
  SnakeModelTest::MoveLastTickBack(model, 300);
  model.UpdateCurrentState();
  EXPECT_EQ(model.snake().front(), head);
  SnakeModelTest::MoveLastTickBack(model, 600);
  model.UpdateCurrentState();
  EXPECT_EQ(model.snake().front(),
            s21::Model::Point(head.first, head.second + 1));
}

TEST(SnakeModelTest, NegativeHighScoreIsIgnored) {
  {
    std::ofstream output(g_isolator.test_home + "/.brickgame_snake_highscore");
    output << -10;
  }
  s21::Model model;
  EXPECT_EQ(model.high_score(), 0);
}

TEST(SnakeCApiTest, UserInputAndUpdateCurrentState) {
  userInput(Start, false);
  GameInfo_t info = updateCurrentState();
  EXPECT_EQ(info.level, 1);
  EXPECT_NE(info.field, nullptr);
  EXPECT_EQ(info.next, nullptr);
  EXPECT_EQ(info.pause, 0);

  userInput(Pause, false);
  info = updateCurrentState();
  EXPECT_EQ(info.pause, 1);

  userInput(Pause, false);
  info = updateCurrentState();
  EXPECT_EQ(info.pause, 0);

  userInput(Terminate, false);
  EXPECT_EQ(updateCurrentState().pause, 0);
}

TEST(ControllerTest, DelegatesToModel) {
  s21::Model model;
  s21::Controller controller(&model);

  EXPECT_EQ(controller.State(), s21::Model::State::kStart);
  controller.UserInput(Start, false);
  controller.UpdateCurrentState();
  EXPECT_EQ(controller.State(), s21::Model::State::kMoving);
  EXPECT_EQ(controller.Score(), 0);
  EXPECT_EQ(controller.Level(), 1);
  EXPECT_EQ(controller.Speed(), 1);
  EXPECT_FALSE(controller.Paused());
  EXPECT_FALSE(controller.IsGameOver());
  EXPECT_FALSE(controller.IsWin());
  EXPECT_FALSE(controller.IsLose());
  EXPECT_EQ(controller.Snake().size(), 4u);
  EXPECT_EQ(controller.Apple(), model.apple());

  const GameInfo_t info = controller.UpdateCurrentState();
  EXPECT_EQ(info.score, controller.Score());
}
