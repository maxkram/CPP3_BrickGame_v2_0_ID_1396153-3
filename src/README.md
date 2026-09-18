# BrickGame v2.0 — «Змейка»

```sh
cd src
make all         # сборка
make test        # unit-тесты (GTest): змейка + тетрис
make gcov_report # отчёт о покрытии -> build/report/index.html
```

Запуск:

```sh
./build/snake_gui          # десктопный интерфейс (GTK+ 3), змейка
./build/snake_gui tetris   # десктопный интерфейс (GTK+ 3), тетрис
./build/snake_console  # консольный интерфейс змейки (ncurses)
./build/tetris_cli     # терминальный тетрис (ncurses)
```

Управление:
- змейка: стрелки — направление, Space — ускорение, Enter — старт,
  P — пауза, Q/Esc — выход;
- тетрис: ←/→ — сдвиг, Space — поворот, ↓ — сброс вниз, Enter — старт,
  P — пауза, Q/Esc — выход.

## Документация

- [docs/README.md](docs/README.md) — полная техническая документация;
- [docs/defense.md](docs/defense.md) — вопросы и ответы для защиты;
- [docs/fsm.svg](docs/fsm.svg) — диаграмма КА змейки;
- [docs/fsm_tetris.svg](docs/fsm_tetris.svg) — диаграмма КА тетриса.
