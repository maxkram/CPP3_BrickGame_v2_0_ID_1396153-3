#ifndef SRC_BRICK_GAME_SNAKE_MODEL_H_
#define SRC_BRICK_GAME_SNAKE_MODEL_H_

#include <chrono>
#include <deque>
#include <string>
#include <utility>

#include "game_info.h"

namespace s21 {

class SnakeModelTest;

class Model {
 public:
  using Point = std::pair<int, int>; //using=typedef

  static constexpr int kFieldWidth =
      10;  // constexpr значение известно на этапе компиляции
  static constexpr int kFieldHeight = 20;
  static constexpr int kMaxLength = 200;
  static constexpr int kMaxLevel = 10;
  static constexpr int kStartLength = 4;
  static constexpr int kLevelUpScoreStep = 5;

  static constexpr int kEmptyValue = 0;
  static constexpr int kSnakeValue = 1;
  static constexpr int kAppleValue = 2;

  enum class Direction { kUp, kDown, kLeft, kRight };  // scoped enum
  enum class State {
    kStart,
    kSpawn,
    kMoving,
    kPause,
    kGameOver
  };  // default member initializer

  Model();
  ~Model() = default;

  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;

  void UserInput(UserAction_t action, bool hold);

  GameInfo_t UpdateCurrentState();

  void Tick();

  State state() const { return state_; }
  bool paused() const { return pause_ != 0; }
  bool IsWin() const { return win_; }
  bool IsGameOver() const { return state_ == State::kGameOver; }
  int score() const { return score_; }
  int high_score() const { return high_score_; }
  int level() const { return level_; }
  int speed() const { return speed_; }
  const std::deque<Point>& snake() const { return snake_; }
  Point apple() const { return apple_; }
  long long TickIntervalMs() const;

 private:
  friend class SnakeModelTest;

  void SetState(State state);
  void StartGame();
  void SpawnApple();
  void TryChangeDirection(Direction direction);
  void GameOver(bool win);
  void RefreshField();
  void LoadHighScore();
  void SaveHighScore();
  bool IsSnakeCell(int row, int col) const;
  bool IsOpposite(Direction lhs, Direction rhs) const;

  static std::string GetHighScoreFilePath();

  State state_{State::kStart};
  Direction direction_{Direction::kRight};
  Direction move_dir_{Direction::kRight};
  std::deque<Point> snake_;
  Point apple_{};
  bool apple_spawned_{false};
  bool hold_{false};
  bool win_{false};
  int score_{0};
  int high_score_{0};
  int level_{1};
  int speed_{1};
  int pause_{0};

  std::chrono::steady_clock::time_point last_tick_{};

  int field_data_[kFieldHeight][kFieldWidth]{};
  int* field_rows_[kFieldHeight]{};
  GameInfo_t info_{};
  std::string high_score_file_;
};

}  // namespace s21

#endif  // SRC_BRICK_GAME_SNAKE_MODEL_H_
