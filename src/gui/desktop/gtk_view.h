#ifndef SRC_GUI_DESKTOP_GTK_VIEW_H_
#define SRC_GUI_DESKTOP_GTK_VIEW_H_

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <vector>

#include "brick_game/snake/game_info.h"

namespace s21 {

class GtkView {
 public:
  using InputFn = std::function<void(UserAction_t, bool)>;
  using StateFn = std::function<GameInfo_t(void)>;
  using FlagFn = std::function<int(void)>;

  struct Config {
    std::string window_title;
    std::string sidebar_title;
    bool show_next{false};
    bool send_action_release{true};
    FlagFn game_over;
    FlagFn win;
  };

  struct Startup {
    InputFn input;
    StateFn state;
    Config config;
  };

  GtkView(InputFn input, StateFn state, Config config);
  ~GtkView();

  GtkView(const GtkView&) = delete;
  GtkView& operator=(const GtkView&) = delete;

  static void OnActivate(GtkApplication* app, gpointer user_data);

 private:
  void BuildWindow(GtkApplication* app);
  GtkWidget* CreateSidebar();
  void Refresh();
  void UpdateSidebar();

  static gboolean OnTick(gpointer user_data);
  static gboolean OnDraw(GtkWidget* widget, cairo_t* cr, gpointer user_data);
  static gboolean OnDrawNext(GtkWidget* widget, cairo_t* cr,
                             gpointer user_data);
  static gboolean OnKeyPress(GtkWidget* widget, GdkEventKey* event,
                             gpointer user_data);
  static gboolean OnKeyRelease(GtkWidget* widget, GdkEventKey* event,
                               gpointer user_data);
  static gboolean OnFocusOut(GtkWidget* widget, GdkEventFocus* event,
                             gpointer user_data);
  static void OnWindowDestroy(GtkWidget* widget, gpointer user_data);

  void DrawField(cairo_t* cr);
  void DrawBackground(cairo_t* cr);
  void DrawCell(cairo_t* cr, int row, int col, double red, double green,
                double blue);
  void DrawNext(cairo_t* cr);
  void DrawOverlay(cairo_t* cr, const std::string& text);

  InputFn input_;
  StateFn state_;
  Config config_;

  GtkWidget* window_{nullptr};
  GtkWidget* drawing_area_{nullptr};
  GtkWidget* next_area_{nullptr};
  GtkWidget* score_label_{nullptr};
  GtkWidget* high_score_label_{nullptr};
  GtkWidget* level_label_{nullptr};
  GtkWidget* speed_label_{nullptr};
  GtkWidget* state_label_{nullptr};
  guint refresh_source_id_{0};

  std::vector<std::vector<int>> field_;
  std::vector<std::vector<int>> next_;
  int score_{0};
  int high_score_{0};
  int level_{1};
  int speed_{1};
  bool paused_{false};
  bool game_over_{false};
  bool win_{false};

  static constexpr int kCellSize = 30;
  static constexpr int kNextCellSize = 22;
  static constexpr int kNextSize = 4;
  static constexpr int kFieldWidth = 10;
  static constexpr int kFieldHeight = 20;
};

}  // namespace s21

#endif  // SRC_GUI_DESKTOP_GTK_VIEW_H_
