#include "model.h"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <random>
#include <vector>

namespace s21 {

namespace {
constexpr long long kBaseIntervalMs = 500;
constexpr long long kIntervalStepMs = 40;
constexpr long long kMinIntervalMs = 100;
constexpr char kHighScoreFileName[] = ".brickgame_snake_highscore";
}  // namespace

std::string Model::GetHighScoreFilePath() {
  const char* home = std::getenv("HOME");
  const std::string base = (home != nullptr) ? home : ".";
  return base + "/" + kHighScoreFileName;
}

Model::Model() : high_score_file_(GetHighScoreFilePath()) {
  for (int row = 0; row < kFieldHeight; ++row) {
    field_rows_[row] = field_data_[row];
  }
  info_.field = field_rows_;
  info_.next = nullptr;
  LoadHighScore();
  info_.score = score_;
  info_.high_score = high_score_;
  info_.level = level_;
  info_.speed = speed_;
  info_.pause = pause_;
}

void Model::UserInput(UserAction_t action, bool hold) {
  switch (action) {
    case Start:
      if (state_ == State::kStart || state_ == State::kGameOver) {
        StartGame();
      }
      break;
    case Pause:
      if (state_ == State::kMoving) {
        pause_ = 1;
        SetState(State::kPause);
      } else if (state_ == State::kPause) {
        pause_ = 0;
        SetState(State::kMoving);
      }
      break;
    case Terminate:
      break;
    case Left:
      TryChangeDirection(Direction::kLeft);
      break;
    case Right:
      TryChangeDirection(Direction::kRight);
      break;
    case Up:
      TryChangeDirection(Direction::kUp);
      break;
    case Down:
      TryChangeDirection(Direction::kDown);
      break;
    case Action:
      hold_ = hold;
      break;
  }
}

GameInfo_t Model::UpdateCurrentState() {
  const auto now = std::chrono::steady_clock::now();
  if (state_ == State::kSpawn) {
    SetState(State::kMoving);
  } else if (state_ == State::kMoving) {
    long long interval = TickIntervalMs();
    if (hold_) {
      interval /= 2; 
    }
    const long long elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_)
            .count();
    if (elapsed >= interval) {
      last_tick_ = now;
      Tick();
    }
  }
  RefreshField();
  info_.score = score_;
  info_.high_score = high_score_;
  info_.level = level_;
  info_.speed = speed_;
  info_.pause = pause_;
  return info_;
}

long long Model::TickIntervalMs() const {
  long long interval = kBaseIntervalMs - (level_ - 1) * kIntervalStepMs;
  if (interval < kMinIntervalMs) {
    interval = kMinIntervalMs;
  }
  return interval;
}

void Model::Tick() {
  if (state_ != State::kMoving) {
    return;
  }
  move_dir_ = direction_;
  const Point& head = snake_.front();
  int row = head.first;
  int col = head.second;
  switch (direction_) {
    case Direction::kUp:
      --row;
      break;
    case Direction::kDown:
      ++row;
      break;
    case Direction::kLeft:
      --col;
      break;
    case Direction::kRight:
      ++col;
      break;
  }

  if (row < 0 || row >= kFieldHeight || col < 0 || col >= kFieldWidth) {
    GameOver(false);
    return;
  }

  for (auto cell = snake_.begin(); cell != std::prev(snake_.end()); ++cell) {
    if (cell->first == row && cell->second == col) {
      GameOver(false);
      return;
    }
  }

  snake_.push_front({row, col});

  if (row == apple_.first && col == apple_.second) {
    ++score_;
    if (score_ > high_score_) {
      high_score_ = score_;
      SaveHighScore();
    }
    const int new_level = score_ / kLevelUpScoreStep + 1;
    level_ = (new_level > kMaxLevel) ? kMaxLevel : new_level;
    speed_ = level_;
    if (static_cast<int>(snake_.size()) >= kMaxLength) {
      GameOver(true);
      return;
    }
    SpawnApple();
  } else {
    snake_.pop_back();
  }
}

void Model::SetState(State state) {
  state_ = state;
  last_tick_ = std::chrono::steady_clock::now();
}

void Model::StartGame() {
  score_ = 0;
  level_ = 1;
  speed_ = 1;
  pause_ = 0;
  win_ = false;
  hold_ = false;
  apple_spawned_ = false;
  snake_.clear();
  const int head_row = kFieldHeight / 2;
  const int head_col = kFieldWidth / 2;
  for (int i = 0; i < kStartLength; ++i) {
    snake_.push_back({head_row, head_col - i});
  }
  direction_ = Direction::kRight;
  move_dir_ = Direction::kRight;
  SpawnApple();
  SetState(State::kSpawn);
}

void Model::SpawnApple() {
  std::vector<Point> free_cells;
  free_cells.reserve(kFieldHeight * kFieldWidth);
  for (int row = 0; row < kFieldHeight; ++row) {
    for (int col = 0; col < kFieldWidth; ++col) {
      if (!IsSnakeCell(row, col)) {
        free_cells.push_back({row, col});
      }
    }
  }
  if (free_cells.empty()) {
    apple_spawned_ = false;
    return;
  }
  static std::mt19937 generator(std::random_device{}());
  std::uniform_int_distribution<size_t> distribution(0, free_cells.size() - 1);
  apple_ = free_cells[distribution(generator)];
  apple_spawned_ = true;
}

void Model::TryChangeDirection(Direction direction) {
  if (state_ != State::kMoving) {
    return;
  }
  if (direction == direction_ || IsOpposite(move_dir_, direction)) {
    return;
  }
  direction_ = direction;
}

void Model::GameOver(bool win) {
  win_ = win;
  SetState(State::kGameOver);
  if (score_ > high_score_) {
    high_score_ = score_;
    SaveHighScore();
  }
}

void Model::RefreshField() {
  for (int row = 0; row < kFieldHeight; ++row) {
    for (int col = 0; col < kFieldWidth; ++col) {
      field_data_[row][col] = kEmptyValue;
    }
  }
  if (apple_spawned_) {
    field_data_[apple_.first][apple_.second] = kAppleValue;
  }
  for (const auto& cell : snake_) {
    field_data_[cell.first][cell.second] = kSnakeValue;
  }
}

void Model::LoadHighScore() {
  std::ifstream input(high_score_file_);
  int value = 0;
  if (input >> value && value >= 0) {
    high_score_ = value;
  }
}

void Model::SaveHighScore() {
  std::ofstream output(high_score_file_);
  if (output.is_open()) {
    output << high_score_ << '\n';
  }
}

bool Model::IsSnakeCell(int row, int col) const {
  for (const auto& cell : snake_) {
    if (cell.first == row && cell.second == col) {
      return true;
    }
  }
  return false;
}

bool Model::IsOpposite(Direction lhs, Direction rhs) const {
  return (lhs == Direction::kUp && rhs == Direction::kDown) ||
         (lhs == Direction::kDown && rhs == Direction::kUp) ||
         (lhs == Direction::kLeft && rhs == Direction::kRight) ||
         (lhs == Direction::kRight && rhs == Direction::kLeft);
}

}  // namespace s21
