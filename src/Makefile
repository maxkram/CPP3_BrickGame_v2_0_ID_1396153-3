.DEFAULT_GOAL := all

CXX      := $(shell command -v g++-13 2>/dev/null || \
               command -v g++-14 2>/dev/null || \
               command -v g++ 2>/dev/null || echo g++)
CC       := $(shell command -v gcc-13 2>/dev/null || \
               command -v gcc-14 2>/dev/null || \
               command -v gcc 2>/dev/null || echo gcc)
GCOV     := $(shell command -v gcov-13 2>/dev/null || \
               command -v gcov-14 2>/dev/null || \
               command -v gcov 2>/dev/null || echo gcov)
AR       ?= ar
RM       := rm -rf

STD      := -std=c++20
CSTD     := -std=c11
WARN     := -Wall -Wextra -Werror -pedantic
CXXFLAGS := $(STD) $(WARN) -I. -MMD -MP
CFLAGS   := $(CSTD) $(WARN) -D_POSIX_C_SOURCE=199309L -I. -MMD -MP

# --- Каталоги --------------------------------------------------------
BUILD_DIR    := build
LIB_DIR      := $(BUILD_DIR)/lib
COV_DIR      := $(BUILD_DIR)/cov
TESTS_DIR    := $(BUILD_DIR)/tests
REPORT_DIR   := $(BUILD_DIR)/report
DVI_DIR      := $(BUILD_DIR)/dvi
INSTALL_DIR  ?= $(HOME)/brickgame_v2_0
DIST_NAME    := brickgame_v2_0

# --- Внешние библиотеки ---------------------------------------------
GTK_CFLAGS   = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS     = $(shell pkg-config --libs gtk+-3.0)
NCURSES_CFLAGS = $(shell pkg-config --cflags ncursesw)
NCURSES_LIBS   = $(shell pkg-config --libs ncursesw)
GTEST_LIBS   := -lgtest -lgtest_main -pthread

# --- Источники -------------------------------------------------------
SNAKE_SRCS   := brick_game/snake/model.cpp \
                brick_game/snake/controller.cpp \
                brick_game/snake/snake_game.cpp
TETRIS_SRCS  := brick_game/tetris/tetris.c
GUI_SRCS     := gui/desktop/main.cpp gui/desktop/gtk_view.cpp
CONSOLE_SRCS := gui/console/main.cpp
CLI_SRCS     := gui/cli/main.cpp
TEST_SRC     := tests/test_snake.cpp
TETRIS_TEST_SRC := tests/test_tetris.cpp

LIB_OBJS     := $(patsubst %.cpp,$(LIB_DIR)/%.o,$(SNAKE_SRCS))
TETRIS_OBJS  := $(patsubst %.c,$(LIB_DIR)/%.o,$(TETRIS_SRCS))
GUI_OBJS     := $(patsubst %.cpp,$(LIB_DIR)/%.o,$(GUI_SRCS))
CONSOLE_OBJS := $(patsubst %.cpp,$(LIB_DIR)/%.o,$(CONSOLE_SRCS))
CLI_OBJS     := $(patsubst %.cpp,$(LIB_DIR)/%.o,$(CLI_SRCS))
SNAKE_COV_OBJS := $(patsubst %.cpp,$(COV_DIR)/%.o,$(SNAKE_SRCS)) \
                  $(COV_DIR)/tests/test_snake.o
TETRIS_COV_OBJS := $(patsubst %.c,$(COV_DIR)/%.o,$(TETRIS_SRCS)) \
                   $(COV_DIR)/tests/test_tetris.o
COV_OBJS       := $(SNAKE_COV_OBJS) $(TETRIS_COV_OBJS)

DEPS         := $(LIB_OBJS:.o=.d) $(TETRIS_OBJS:.o=.d) $(GUI_OBJS:.o=.d) \
                $(CONSOLE_OBJS:.o=.d) $(CLI_OBJS:.o=.d) $(COV_OBJS:.o=.d)
-include $(DEPS)

LIB          := $(LIB_DIR)/libs21_snake.a
TETRIS_LIB   := $(LIB_DIR)/libs21_tetris.a
GUI_BIN      := $(BUILD_DIR)/snake_gui
CONSOLE_BIN  := $(BUILD_DIR)/snake_console
CLI_BIN      := $(BUILD_DIR)/tetris_cli
TEST_BIN     := $(TESTS_DIR)/test_snake
TETRIS_TEST_BIN := $(TESTS_DIR)/test_tetris

.PHONY: all clean test gcov_report dvi dist install uninstall style style_fix valgrind

# ======================================================================
#  Сборка
# ======================================================================

all: $(GUI_BIN) $(CONSOLE_BIN) $(CLI_BIN)
	@echo "Сборка завершена:"
	@echo "  $(GUI_BIN)     — десктопный интерфейс (GTK+ 3), змейка и тетрис"
	@echo "  $(CONSOLE_BIN) — консольный интерфейс (ncurses), змейка"
	@echo "  $(CLI_BIN)     — терминальный интерфейс (ncurses), тетрис"

$(LIB): $(LIB_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(TETRIS_LIB): $(TETRIS_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(LIB_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(GTK_CFLAGS) $(NCURSES_CFLAGS) -c $< -o $@

$(LIB_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(GUI_BIN): $(GUI_OBJS) $(filter-out %/snake_game.o,$(LIB_OBJS)) $(TETRIS_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(GTK_CFLAGS) $^ -o $@ $(GTK_LIBS)

$(CONSOLE_BIN): $(CONSOLE_OBJS) $(LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(NCURSES_CFLAGS) $^ -o $@ $(NCURSES_LIBS)

$(CLI_BIN): $(CLI_OBJS) $(TETRIS_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(NCURSES_CFLAGS) $^ -o $@ $(NCURSES_LIBS)

# ======================================================================
#  Тестирование и покрытие
# ======================================================================

test: $(TEST_BIN) $(TETRIS_TEST_BIN)
	@find $(COV_DIR) -name '*.gcda' -delete 2>/dev/null || true
	$(TEST_BIN)
	$(TETRIS_TEST_BIN)

$(COV_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(STD) -I. --coverage -O0 -MMD -MP -c $< -o $@

$(COV_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CSTD) -D_POSIX_C_SOURCE=199309L -I. --coverage -O0 -MMD -MP -c $< -o $@

$(TEST_BIN): $(SNAKE_COV_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) --coverage $(STD) -I. $^ -o $@ $(GTEST_LIBS)

$(TETRIS_TEST_BIN): $(TETRIS_COV_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) --coverage $(STD) -I. $^ -o $@ $(GTEST_LIBS)
# Отчёт о покрытии (lcov + genhtml).
gcov_report: test
	@mkdir -p $(REPORT_DIR)
	lcov --capture --directory $(COV_DIR) \
	     --gcov-tool $(GCOV) \
	     --ignore-errors mismatch \
	     --rc geninfo_unexecuted_blocks=1 \
	     --output-file $(COV_DIR)/coverage.info
	lcov --extract $(COV_DIR)/coverage.info 'brick_game' \
	     --output-file $(COV_DIR)/coverage_filtered.info
	genhtml $(COV_DIR)/coverage_filtered.info \
	     --output-directory $(REPORT_DIR)
	lcov --extract $(COV_DIR)/coverage_filtered.info 'brick_game/snake' \
	     --output-file $(COV_DIR)/snake.info
valgrind: test
	valgrind --leak-check=full --error-exitcode=1 $(TEST_BIN)
	valgrind --leak-check=full --error-exitcode=1 $(TETRIS_TEST_BIN)

# ======================================================================
#  Документация (dvi)
# ======================================================================

dvi:
	@mkdir -p $(DVI_DIR)
	cp -r docs/* $(DVI_DIR)/
	@if command -v pandoc >/dev/null 2>&1; then \
		pandoc docs/README.md -o $(DVI_DIR)/README.html; \
	else \
		cp docs/README.md $(DVI_DIR)/README.txt; \
	fi
	@echo "Документация: $(DVI_DIR)/README.md (HTML: $(DVI_DIR)/README.html)"

# ======================================================================
#  Установка и удаление
# ======================================================================

install: all
	@mkdir -p $(INSTALL_DIR)
	install -m 755 $(GUI_BIN) $(INSTALL_DIR)/brickgame_gui
	install -m 755 $(CONSOLE_BIN) $(INSTALL_DIR)/brickgame_console
	install -m 755 $(CLI_BIN) $(INSTALL_DIR)/brickgame_tetris_cli
	@echo "Программа установлена в: $(INSTALL_DIR)"

uninstall:
	$(RM) $(INSTALL_DIR)
	@echo "Программа удалена: $(INSTALL_DIR)"

# ======================================================================
#  Архив исходников (dist)
# ======================================================================

dist:
	@mkdir -p $(BUILD_DIR)/dist/$(DIST_NAME)
	cp -r brick_game gui tests docs \
	    Makefile .clang-format README.md \
	    $(BUILD_DIR)/dist/$(DIST_NAME)/
	tar -czf $(DIST_NAME).tar.gz -C $(BUILD_DIR)/dist $(DIST_NAME)
	@echo "Архив: $(DIST_NAME).tar.gz"

# ======================================================================
#  Стиль (clang-format, Google Style)
# ======================================================================

STYLE_FILES := $(shell find brick_game gui tests -name '*.cpp' -o -name '*.h' -o -name '*.c')

style:
	clang-format -n -Werror $(STYLE_FILES)

style_fix:
	clang-format -i $(STYLE_FILES)

# ======================================================================
#  Очистка
# ======================================================================

clean:
	$(RM) $(BUILD_DIR)
	$(RM) $(DIST_NAME).tar.gz
	@echo "Проект очищен"

