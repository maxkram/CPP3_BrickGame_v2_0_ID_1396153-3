#include <gtk/gtk.h>

#include <iostream>
#include <memory>
#include <string>

#include "brick_game/snake/controller.h"
#include "brick_game/snake/model.h"
#include "brick_game/tetris/tetris.h"
#include "gui/desktop/gtk_view.h"

int main(int argc, char** argv) {
  const std::string game = (argc > 1) ? argv[1] : "snake";

  if (argc > 2 || (game != "snake" && game != "tetris")) {
    std::cerr << "Usage: " << argv[0] << " [snake|tetris]\n";
    return 1;
  }
  s21::GtkView::Startup startup_data;
  auto* startup = &startup_data;

  if (game == "tetris") {
    startup->config.window_title = "BrickGame v2.0 — Тетрис";
    startup->config.sidebar_title = "Тетрис";
    startup->config.show_next = true;
    startup->config.send_action_release = false;
    startup->config.game_over = [] { return isGameOver(); };
    startup->input = [](UserAction_t action, bool hold) {
      userInput(action, hold);
    };
    startup->state = [] { return updateCurrentState(); };
  } else {
    static s21::Model model;
    static s21::Controller controller(&model);
    startup->config.window_title = "BrickGame v2.0 — Змейка";
    startup->config.sidebar_title = "Змейка";
    startup->config.show_next = false;
    startup->config.game_over = [] { return controller.IsGameOver(); };
    startup->config.win = [] { return controller.IsWin(); };
    startup->input = [](UserAction_t action, bool hold) {
      controller.UserInput(action, hold);
    };
    startup->state = [] { return controller.UpdateCurrentState(); };
  }

  std::unique_ptr<GtkApplication, decltype(&g_object_unref)> app(
      gtk_application_new("edu.school21.brickgame.desktop",
                          G_APPLICATION_NON_UNIQUE),
      &g_object_unref);
  g_signal_connect(app.get(), "activate", G_CALLBACK(s21::GtkView::OnActivate),
                   startup);
  char* gtk_argv[] = {argv[0], nullptr};
  return g_application_run(G_APPLICATION(app.get()), 1, gtk_argv);
}
