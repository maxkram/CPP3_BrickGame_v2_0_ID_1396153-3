#include "controller.h"

namespace s21 {

Controller::Controller(Model* model) : model_(model) {}

void Controller::UserInput(UserAction_t action, bool hold) {
  model_->UserInput(action, hold);
}

GameInfo_t Controller::UpdateCurrentState() {
  return model_->UpdateCurrentState();
}

void Controller::Tick() { model_->Tick(); }

Model::State Controller::State() const { return model_->state(); }

bool Controller::IsGameOver() const { return model_->IsGameOver(); }

bool Controller::IsWin() const { return model_->IsWin(); }

bool Controller::IsLose() const {
  return model_->IsGameOver() && !model_->IsWin();
}

bool Controller::Paused() const { return model_->paused(); }

int Controller::Score() const { return model_->score(); }

int Controller::HighScore() const { return model_->high_score(); }

int Controller::Level() const { return model_->level(); }

int Controller::Speed() const { return model_->speed(); }

const std::deque<Model::Point>& Controller::Snake() const {
  return model_->snake();
}

Model::Point Controller::Apple() const { return model_->apple(); }

}  // namespace s21
