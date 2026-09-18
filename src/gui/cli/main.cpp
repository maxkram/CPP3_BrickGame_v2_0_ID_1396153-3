#include <ncurses.h>

#include <clocale>

#include "brick_game/tetris/tetris.h"

namespace {

constexpr int kFieldWidth = 10;
constexpr int kFieldHeight = 20;
constexpr int kCellWidth = 2;
constexpr int kPieceSize = 4;

bool MapKey(int key, UserAction_t* action) {
  switch (key) {
    case KEY_LEFT:
      *action = Left;
      return true;
    case KEY_RIGHT:
      *action = Right;
      return true;
    case KEY_DOWN:
      *action = Down;
      return true;
    case ' ':
      *action = Action;
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
      if (info.field[row][col] != 0) {
        attron(COLOR_PAIR(2));
        mvprintw(row + 1, 1 + col * kCellWidth, "  ");
        attroff(COLOR_PAIR(2));
      }
    }
  }
}

void DrawSidebar(const GameInfo_t& info, int game_over) {
  const int x = kFieldWidth * kCellWidth + 4;
  int y = 1;
  mvprintw(y++, x, "Тетрис");
  mvprintw(y++, x, "-------");
  mvprintw(y++, x, "Очки: %d", info.score);
  mvprintw(y++, x, "Рекорд: %d", info.high_score);
  mvprintw(y++, x, "Уровень: %d", info.level);
  mvprintw(y++, x, "Скорость: %d", info.speed);
  y += 1;
  mvprintw(y++, x, "Следующая:");
  ++y;
  for (int i = 0; i < kPieceSize; ++i) {
    for (int j = 0; j < kPieceSize; ++j) {
      if (info.next[i][j] != 0) {
        attron(COLOR_PAIR(2));
        mvprintw(y + i, x + j * kCellWidth, "  ");
        attroff(COLOR_PAIR(2));
      }
    }
  }
  y += kPieceSize + 1;
  if (game_over) {
    attron(COLOR_PAIR(3));
    mvprintw(y++, x, "ИГРА ОКОНЧЕНА");
    attroff(COLOR_PAIR(3));
    mvprintw(y++, x, "Enter — заново");
  } else if (info.pause) {
    mvprintw(y++, x, "ПАУЗА");
  }
  y += 1;
  mvprintw(y++, x, "Управление:");
  mvprintw(y++, x, "Стрелки — движение");
  mvprintw(y++, x, "Space — поворот");
  mvprintw(y++, x, "Down — сброс вниз");
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
    init_pair(3, COLOR_RED, COLOR_BLACK);
  }

  bool running = true;
  while (running) {
    const int key = getch();
    if (key != ERR) {
      UserAction_t action = Action;
      if (MapKey(key, &action)) {
        userInput(action, false);
        if (action == Terminate) {
          running = false;
        }
      }
    }
    const GameInfo_t info = updateCurrentState();
    const int game_over = isGameOver();
    erase();
    DrawField(info);
    DrawSidebar(info, game_over);
    refresh();
  }

  endwin();
  return 0;
}