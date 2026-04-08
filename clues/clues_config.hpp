#ifndef CLUES_CONFIG_HPP
#define CLUES_CONFIG_HPP

struct PuzzleConfig {
  int target_clues;
  int max_per_row;
  int max_per_col;
  int max_per_grid;
  int max_per_diagonal;
  bool check_diagonals;
  bool exact_diagonal;
  int max_per_window;
  bool check_windows;
  bool exact_window;
};

// Sudoku Mini: 6x6 puzzle. Total clues = 15, max per row/col/grid = 3
const PuzzleConfig SUDOKU_MINI_CONFIG = {15,    3,     3, 3,     6,
                                         false, false, 6, false, false};

// Sudoku Easy: 9x9 puzzle. Total clues = 45, max per row/col/grid = 5
const PuzzleConfig SUDOKU_EASY_CONFIG = {45,    5,     5, 5,     9,
                                         false, false, 9, false, false};

// Sudoku 9: 9x9 puzzle. Total clues = 36, max per row/col/grid = 5
const PuzzleConfig SUDOKU_9_CONFIG = {36,    5,     5, 5,     9,
                                      false, false, 9, false, false};

// Sudoku 9: 9x9 puzzle. Total clues = 27, max per row/col/grid = 4
const PuzzleConfig SUDOKU_A_CONFIG = {27,    4,     4, 4,     9,
                                      false, false, 9, false, false};

// Sudoku X: 9x9 puzzle. Total clues = 40, max per row/col/grid/diag = 5
const PuzzleConfig SUDOKU_X_CONFIG = {40,   5,    5, 5,     5,
                                      true, true, 9, false, false};

// Windoku: 9x9 puzzle. Total clues = 40, max per row/col/grid/window = 5
const PuzzleConfig WINDOKU_CONFIG = {40,    5,     5, 5,    9,
                                     false, false, 5, true, true};

// Windoku X: 9x9 puzzle. Total clues = 36, max 4 on diagonals and windows
const PuzzleConfig WINDOKU_X_CONFIG = {36,   4,    4, 4,    4,
                                       true, true, 4, true, true};

// Jigsaw 8: 8x8 puzzle. Total clues = 32, max per row/col/grid = 5
const PuzzleConfig JIGSAW_8_CONFIG = {32,    5,     5, 5,     8,
                                      false, false, 8, false, false};

// Jigsaw 9: 9x9 puzzle. Total clues = 40, max per row/col/grid = 5
const PuzzleConfig JIGSAW_9_CONFIG = {40,    5,     5, 5,     9,
                                      false, false, 9, false, false};

// Jigsaw X: 9x9 puzzle. Total clues = 40, max per row/col/grid/diag = 5
const PuzzleConfig JIGSAW_X_CONFIG = {40,   5,    5, 5,     5,
                                      true, true, 9, false, false};

// Windoku Jigsaw: 9x9 puzzle. Total clues = 40, max per row/col/grid/window = 5
const PuzzleConfig WINDOKU_JIGSAW_CONFIG = {40,    5,     5, 5,    9,
                                            false, false, 5, true, true};

const PuzzleConfig SUDOKU_12_CONFIG = {72,    7,     7,  7,     12,
                                       false, false, 12, false, false};

// Dozaku: 12x12 with 3x4 and 4x3 grids. Total clues = 72, max per row/col/grid
// = 7
const PuzzleConfig DOZAKU_CONFIG = {72,    7,     7,  7,     12,
                                    false, false, 12, false, false};

const PuzzleConfig TWODOKU_MINI_CONFIG = {30,    3,     3, 3,     6,
                                          false, false, 6, false, false};

const PuzzleConfig TWODOKU_8_CONFIG = {56,    5,     5, 5,     8,
                                       false, false, 8, false, false};

const PuzzleConfig TWODOKU_9_CONFIG = {81,    5,     5, 5,     9,
                                       false, false, 9, false, false};

#endif
