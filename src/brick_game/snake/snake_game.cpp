#include "snake_game.h"

#include "model.h"

namespace {

s21::Model& GetModel() {
  static s21::Model model;
  return model;
}

}  // namespace

extern "C" void userInput(UserAction_t action, bool hold) {
  GetModel().UserInput(action, hold);
}

extern "C" GameInfo_t updateCurrentState() {
  return GetModel().UpdateCurrentState();
}

extern "C" int isGameOver() { return GetModel().IsGameOver() ? 1 : 0; }

extern "C" int isWin() { return GetModel().IsWin() ? 1 : 0; }
