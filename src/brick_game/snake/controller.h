#ifndef SRC_BRICK_GAME_SNAKE_CONTROLLER_H_
#define SRC_BRICK_GAME_SNAKE_CONTROLLER_H_

#include <deque>

#include "game_info.h"
#include "model.h"

namespace s21 {

class Controller {
 public:
  explicit Controller(Model* model);
  ~Controller() = default;

  Controller(const Controller&) = delete;
  Controller& operator=(const Controller&) = delete;

  void UserInput(UserAction_t action, bool hold);
  GameInfo_t UpdateCurrentState();
  void Tick();

  Model::State State() const;
  bool IsGameOver() const;
  bool IsWin() const;
  bool IsLose() const;
  bool Paused() const;
  int Score() const;
  int HighScore() const;
  int Level() const;
  int Speed() const;
  const std::deque<Model::Point>& Snake() const;
  Model::Point Apple() const;

 private:
  Model* model_;
};

}  // namespace s21

#endif  // SRC_BRICK_GAME_SNAKE_CONTROLLER_H_
