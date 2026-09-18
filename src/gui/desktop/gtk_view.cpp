#include "gui/desktop/gtk_view.h"

#include <utility>

namespace s21 {

GtkView::GtkView(InputFn input, StateFn state, Config config)
    : input_(std::move(input)),
      state_(std::move(state)),
      config_(std::move(config)) {
  field_.resize(kFieldHeight, std::vector<int>(kFieldWidth, 0));
  next_.resize(kNextSize, std::vector<int>(kNextSize, 0));
}

GtkView::~GtkView() {
  if (refresh_source_id_ != 0) {
    g_source_remove(refresh_source_id_);
  }
}

void GtkView::OnActivate(GtkApplication* app, gpointer user_data) {
  GtkWindow* window = gtk_application_get_active_window(app);
  if (window != nullptr) {
    gtk_window_present(window);
    return;
  }
  auto* startup = static_cast<Startup*>(user_data);
  auto* view = new GtkView(startup->input, startup->state, startup->config);
  view->BuildWindow(app);
}

void GtkView::BuildWindow(GtkApplication* app) {
  window_ = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window_), config_.window_title.c_str());
  gtk_window_set_default_size(GTK_WINDOW(window_), 560, 680);
  gtk_window_set_resizable(GTK_WINDOW(window_), FALSE);

  GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_container_set_border_width(GTK_CONTAINER(main_box), 8);
  gtk_container_add(GTK_CONTAINER(window_), main_box);

  GtkWidget* frame = gtk_frame_new(nullptr);
  gtk_box_pack_start(GTK_BOX(main_box), frame, FALSE, FALSE, 0);
  drawing_area_ = gtk_drawing_area_new();
  gtk_widget_set_size_request(drawing_area_, kCellSize * kFieldWidth,
                              kCellSize * kFieldHeight);
  gtk_container_add(GTK_CONTAINER(frame), drawing_area_);

  GtkWidget* sidebar = CreateSidebar();
  gtk_box_pack_start(GTK_BOX(main_box), sidebar, TRUE, TRUE, 0);

  g_signal_connect(drawing_area_, "draw", G_CALLBACK(OnDraw), this);
  g_signal_connect(window_, "key-press-event", G_CALLBACK(OnKeyPress), this);
  g_signal_connect(window_, "key-release-event", G_CALLBACK(OnKeyRelease),
                   this);
  g_signal_connect(window_, "focus-out-event", G_CALLBACK(OnFocusOut), this);
  g_signal_connect(window_, "destroy", G_CALLBACK(OnWindowDestroy), this);

  gtk_widget_set_can_focus(GTK_WIDGET(window_), TRUE);
  gtk_widget_add_events(GTK_WIDGET(window_),
                        GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  gtk_widget_show_all(window_);
  gtk_widget_grab_focus(GTK_WIDGET(window_));

  refresh_source_id_ = g_timeout_add(33, OnTick, this);
  UpdateSidebar();
}

GtkWidget* GtkView::CreateSidebar() {
  GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

  GtkWidget* title = gtk_label_new(nullptr);
  gtk_label_set_markup(
      GTK_LABEL(title),
      ("<b><big>" + config_.sidebar_title + "</big></b>").c_str());
  gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);

  score_label_ = gtk_label_new("Очки: 0");
  gtk_label_set_xalign(GTK_LABEL(score_label_), 0.0);
  gtk_box_pack_start(GTK_BOX(box), score_label_, FALSE, FALSE, 0);

  high_score_label_ = gtk_label_new("Рекорд: 0");
  gtk_label_set_xalign(GTK_LABEL(high_score_label_), 0.0);
  gtk_box_pack_start(GTK_BOX(box), high_score_label_, FALSE, FALSE, 0);

  level_label_ = gtk_label_new("Уровень: 1");
  gtk_label_set_xalign(GTK_LABEL(level_label_), 0.0);
  gtk_box_pack_start(GTK_BOX(box), level_label_, FALSE, FALSE, 0);

  speed_label_ = gtk_label_new("Скорость: 1");
  gtk_label_set_xalign(GTK_LABEL(speed_label_), 0.0);
  gtk_box_pack_start(GTK_BOX(box), speed_label_, FALSE, FALSE, 0);

  state_label_ = gtk_label_new("Состояние: Старт");
  gtk_label_set_xalign(GTK_LABEL(state_label_), 0.0);
  gtk_box_pack_start(GTK_BOX(box), state_label_, FALSE, FALSE, 0);

  GtkWidget* next_title = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(next_title), "<b>Следующая фигура:</b>");
  gtk_box_pack_start(GTK_BOX(box), next_title, FALSE, FALSE, 0);

  if (config_.show_next) {
    next_area_ = gtk_drawing_area_new();
    gtk_widget_set_size_request(next_area_, kNextCellSize * kNextSize,
                                kNextCellSize * kNextSize);
    gtk_box_pack_start(GTK_BOX(box), next_area_, FALSE, FALSE, 0);
    g_signal_connect(next_area_, "draw", G_CALLBACK(OnDrawNext), this);
  } else {
    GtkWidget* next_value = gtk_label_new("—");
    gtk_box_pack_start(GTK_BOX(box), next_value, FALSE, FALSE, 0);
  }

  GtkWidget* hint = gtk_label_new(
      "Управление:\n"
      "← ↑ → ↓ — направление\n"
      "Space — ускорение / поворот\n"
      "Enter — старт\n"
      "P — пауза\n"
      "Q / Esc — выход");
  gtk_label_set_justify(GTK_LABEL(hint), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(box), hint, TRUE, TRUE, 0);

  return box;
}

void GtkView::Refresh() {
  const GameInfo_t info = state_();
  for (int row = 0; row < kFieldHeight; ++row) {
    for (int col = 0; col < kFieldWidth; ++col) {
      field_[row][col] = info.field[row][col];
    }
  }
  if (config_.show_next && info.next != nullptr) {
    for (int row = 0; row < kNextSize; ++row) {
      for (int col = 0; col < kNextSize; ++col) {
        next_[row][col] = info.next[row][col];
      }
    }
  }
  score_ = info.score;
  high_score_ = info.high_score;
  level_ = info.level;
  speed_ = info.speed;
  paused_ = info.pause != 0;
  game_over_ = config_.game_over && config_.game_over() != 0;
  win_ = config_.win && config_.win() != 0;
  UpdateSidebar();
  if (next_area_ != nullptr) {
    gtk_widget_queue_draw(next_area_);
  }
  gtk_widget_queue_draw(drawing_area_);
}

void GtkView::UpdateSidebar() {
  gtk_label_set_text(GTK_LABEL(score_label_),
                     ("Очки: " + std::to_string(score_)).c_str());
  gtk_label_set_text(GTK_LABEL(high_score_label_),
                     ("Рекорд: " + std::to_string(high_score_)).c_str());
  gtk_label_set_text(GTK_LABEL(level_label_),
                     ("Уровень: " + std::to_string(level_)).c_str());
  gtk_label_set_text(GTK_LABEL(speed_label_),
                     ("Скорость: " + std::to_string(speed_)).c_str());
  std::string state = "Состояние: Игра";
  if (game_over_) {
    state = win_ ? "Состояние: Победа!" : "Состояние: Игра окончена";
  } else if (paused_) {
    state = "Состояние: Пауза";
  }
  gtk_label_set_text(GTK_LABEL(state_label_), state.c_str());
}

gboolean GtkView::OnTick(gpointer user_data) {
  auto* view = static_cast<GtkView*>(user_data);
  view->Refresh();
  return G_SOURCE_CONTINUE;
}

gboolean GtkView::OnDraw(GtkWidget*, cairo_t* cr, gpointer user_data) {
  auto* view = static_cast<GtkView*>(user_data);
  view->DrawField(cr);
  return FALSE;
}
gboolean GtkView::OnDrawNext(GtkWidget*, cairo_t* cr, gpointer user_data) {
  auto* view = static_cast<GtkView*>(user_data);
  view->DrawNext(cr);
  return FALSE;
}

gboolean GtkView::OnKeyPress(GtkWidget*, GdkEventKey* event,
                             gpointer user_data) {
  auto* view = static_cast<GtkView*>(user_data);
  switch (event->keyval) {
    case GDK_KEY_Left:
      view->input_(Left, false);
      return TRUE;
    case GDK_KEY_Right:
      view->input_(Right, false);
      return TRUE;
    case GDK_KEY_Up:
      view->input_(Up, false);
      return TRUE;
    case GDK_KEY_Down:
      view->input_(Down, false);
      return TRUE;
    case GDK_KEY_space:
      view->input_(Action, true);
      return TRUE;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
      view->input_(Start, false);
      return TRUE;
    case GDK_KEY_p:
    case GDK_KEY_P:
    case GDK_KEY_Pause:
      view->input_(Pause, false);
      return TRUE;
    case GDK_KEY_q:
    case GDK_KEY_Q:
    case GDK_KEY_Escape:
      gtk_widget_destroy(view->window_);
      return TRUE;
    default:
      return FALSE;
  }
}

gboolean GtkView::OnKeyRelease(GtkWidget*, GdkEventKey* event,
                               gpointer user_data) {
  auto* view = static_cast<GtkView*>(user_data);
  if (event->keyval == GDK_KEY_space && view->config_.send_action_release) {
    view->input_(Action, false);
    return TRUE;
  }
  return FALSE;
}

gboolean GtkView::OnFocusOut(GtkWidget*, GdkEventFocus*, gpointer user_data) {
  auto* view = static_cast<GtkView*>(user_data);
  if (view->config_.send_action_release) {
    view->input_(Action, false);
  }
  return FALSE;
}

void GtkView::OnWindowDestroy(GtkWidget*, gpointer user_data) {
  auto* view = static_cast<GtkView*>(user_data);
  view->input_(Terminate, false);
  delete view;
}

void GtkView::DrawField(cairo_t* cr) {
  DrawBackground(cr);
  for (int row = 0; row < kFieldHeight; ++row) {
    for (int col = 0; col < kFieldWidth; ++col) {
      switch (field_[row][col]) {
        case 1:
          DrawCell(cr, row, col, 0.15, 0.75, 0.25);
          break;
        case 2:
          DrawCell(cr, row, col, 0.90, 0.15, 0.15);
          break;
        default:
          break;
      }
    }
  }
  if (paused_) {
    DrawOverlay(cr, "ПАУЗА");
  } else if (game_over_) {
    DrawOverlay(cr, win_ ? "ПОБЕДА!" : "ИГРА ОКОНЧЕНА");
  }
}

void GtkView::DrawNext(cairo_t* cr) {
  cairo_set_source_rgb(cr, 0.10, 0.10, 0.12);
  cairo_paint(cr);
  for (int row = 0; row < kNextSize; ++row) {
    for (int col = 0; col < kNextSize; ++col) {
      if (next_[row][col] != 0) {
        const double x = col * kNextCellSize;
        const double y = row * kNextCellSize;
        cairo_set_source_rgb(cr, 0.30, 0.55, 0.95);
        cairo_rectangle(cr, x + 1, y + 1, kNextCellSize - 2, kNextCellSize - 2);
        cairo_fill(cr);
      }
    }
  }
}

void GtkView::DrawBackground(cairo_t* cr) {
  cairo_set_source_rgb(cr, 0.10, 0.10, 0.12);
  cairo_paint(cr);

  cairo_set_source_rgb(cr, 0.22, 0.22, 0.26);
  cairo_set_line_width(cr, 1.0);
  for (int row = 0; row <= kFieldHeight; ++row) {
    cairo_move_to(cr, 0, row * kCellSize);
    cairo_line_to(cr, kFieldWidth * kCellSize, row * kCellSize);
    cairo_stroke(cr);
  }
  for (int col = 0; col <= kFieldWidth; ++col) {
    cairo_move_to(cr, col * kCellSize, 0);
    cairo_line_to(cr, col * kCellSize, kFieldHeight * kCellSize);
    cairo_stroke(cr);
  }
}

void GtkView::DrawCell(cairo_t* cr, int row, int col, double red, double green,
                       double blue) {
  const double x = col * kCellSize;
  const double y = row * kCellSize;
  cairo_set_source_rgb(cr, red, green, blue);
  cairo_rectangle(cr, x + 1, y + 1, kCellSize - 2, kCellSize - 2);
  cairo_fill(cr);
}

void GtkView::DrawOverlay(cairo_t* cr, const std::string& text) {
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.65);
  cairo_rectangle(cr, 0, 0, kFieldWidth * kCellSize, kFieldHeight * kCellSize);
  cairo_fill(cr);

  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, 26);
  cairo_text_extents_t extents;
  cairo_text_extents(cr, text.c_str(), &extents);
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_move_to(cr, (kFieldWidth * kCellSize - extents.width) / 2.0,
                (kFieldHeight * kCellSize) / 2.0);
  cairo_show_text(cr, text.c_str());
}

}  // namespace s21
