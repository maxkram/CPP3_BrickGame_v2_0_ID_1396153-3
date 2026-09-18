#include <ncurses.h>

#include <chrono>
#include <clocale>

#include "brick_game/snake/snake_game.h"

namespace {

constexpr int kFieldWidth = 10;
constexpr int kFieldHeight = 20;
constexpr int kCellWidth = 2;
constexpr int kHoldReleaseMs = 300;

bool MapKey(int key, UserAction_t* action, bool* hold) {
  switch (key) {
    case KEY_LEFT:
      *action = Left;
      return true;
    case KEY_RIGHT:
      *action = Right;
      return true;
    case KEY_UP:
      *action = Up;
      return true;
    case KEY_DOWN:
      *action = Down;
      return true;
    case ' ':
      *action = Action;
      *hold = true;
      return true;
    case '\n':
    case KEY_ENTER:
      *action = Start;
      return true;
    case 'p':
    case 'P':
      *action = Pause;
      return true;
    case 'q':
    case 'Q':
    case 27:  // ESC
      *action = Terminate;
      return true;
    default:
      return false;
  }
}

void DrawField(const GameInfo_t& info) {
  const int width = kFieldWidth * kCellWidth + 2;
  for (int col = 0; col < width; ++col) {
    mvaddch(0, col, '=');
    mvaddch(kFieldHeight + 1, col, '=');
  }
  for (int row = 0; row <= kFieldHeight + 1; ++row) {
    mvaddch(row, 0, '|');
    mvaddch(row, width - 1, '|');
  }
  for (int row = 0; row < kFieldHeight; ++row) {
    for (int col = 0; col < kFieldWidth; ++col) {
      const int y = row + 1;
      const int x = 1 + col * kCellWidth;
      const int value = info.field[row][col];
      if (value == 1) {
        attron(COLOR_PAIR(2));
        mvprintw(y, x, "  ");
        attroff(COLOR_PAIR(2));
      } else if (value == 2) {
        attron(COLOR_PAIR(3));
        mvprintw(y, x, "  ");
        attroff(COLOR_PAIR(3));
      }
    }
  }
}

void DrawSidebar(const GameInfo_t& info, int game_over, int win) {
  const int x = kFieldWidth * kCellWidth + 4;
  int y = 1;
  mvprintw(y++, x, "Змейка");
  mvprintw(y++, x, "-------");
  mvprintw(y++, x, "Очки: %d", info.score);
  mvprintw(y++, x, "Рекорд: %d", info.high_score);
  mvprintw(y++, x, "Уровень: %d", info.level);
  mvprintw(y++, x, "Скорость: %d", info.speed);
  if (game_over) {
    attron(COLOR_PAIR(4));
    mvprintw(y++, x, win ? "ПОБЕДА!" : "ИГРА ОКОНЧЕНА");
    attroff(COLOR_PAIR(4));
    mvprintw(y++, x, "Enter — заново");
  } else if (info.pause) {
    mvprintw(y++, x, "ПАУЗА");
  }
  y += 1;
  mvprintw(y++, x, "Управление:");
  mvprintw(y++, x, "Стрелки — направление");
  mvprintw(y++, x, "Space — ускорение");
  mvprintw(y++, x, "Enter — старт");
  mvprintw(y++, x, "P — пауза");
  mvprintw(y++, x, "Q — выход");
}

}  // namespace

int main() {
  setlocale(LC_ALL, "");

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  timeout(50);

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_GREEN, COLOR_GREEN);
    init_pair(3, COLOR_RED, COLOR_RED);
    init_pair(4, COLOR_RED, COLOR_BLACK);
  }

  bool running = true;
  bool holding = false;
  auto last_hold_press = std::chrono::steady_clock::now();

  while (running) {
    const int key = getch();
    if (key != ERR) {
      UserAction_t action = Action;
      bool hold = false;
      if (MapKey(key, &action, &hold)) {
        userInput(action, hold);
        if (action == Action && hold) {
          holding = true;
          last_hold_press = std::chrono::steady_clock::now();
        }
        if (action == Terminate) {
          running = false;
        }
      }
    }
    if (holding && std::chrono::steady_clock::now() - last_hold_press >
                       std::chrono::milliseconds(kHoldReleaseMs)) {
      userInput(Action, false);
      holding = false;
    }
    const GameInfo_t info = updateCurrentState();
    const int game_over = isGameOver();
    const int win = isWin();
    erase();
    DrawField(info);
    DrawSidebar(info, game_over, win);
    refresh();
  }

  endwin();
  return 0;
}
