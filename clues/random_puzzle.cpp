#ifndef RANDOM_PUZZLE_CPP
#define RANDOM_PUZZLE_CPP

#include <vector>
#include <algorithm>
#include <iostream>
#include <random>
#include <functional>
#include "uniqueness.cpp"
#include "clues_config.hpp"

using namespace std;

// PuzzleGenerator: Random Sudoku puzzle generate karta hai constraints ko follow karke
template <int SIZE, int SUB_R, int SUB_C>
class PuzzleGenerator {
public:
    int* gridMap = nullptr;
    int* altGridMap = nullptr;
    bool* activeMap = nullptr;
    std::function<bool(const int*, const PuzzleConfig&)> customConstraints = nullptr;

private:
    static constexpr int NUM_GRIDS = (SIZE / SUB_R) * (SIZE / SUB_C);
    mt19937 rng;
    int gridLookup[SIZE * SIZE];

    // Compute Grid Lookup: Taaki baar baar arithmetic division calculations na karni padein
    void computeGridLookup() {
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                gridLookup[pos] = gridMap ? gridMap[pos] : ((r / SUB_R) * (SIZE / SUB_C) + (c / SUB_C));
            }
        }
    }

    // satisfiesConstraints: Yeh check karta hai ki generated puzzle hamaare predefined configs me fit baithta hai ya nahi
    bool satisfiesConstraints(const int* board, const PuzzleConfig& config) {
        if (customConstraints) return customConstraints(board, config);
        int clues = 0;
        int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0}, gridCounts[NUM_GRIDS] = {0};
        int diag1Count = 0, diag2Count = 0;
        int winCounts[4] = {0};

        // Board ko ghoom ke metrics gather kar rahe hain
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (activeMap && !activeMap[pos]) continue;
                if (board[pos] != 0) {
                    clues++;
                    rowCounts[r]++;
                    colCounts[c]++;
                    gridCounts[gridLookup[pos]]++;
                    if (config.check_diagonals) {
                        if (r == c) diag1Count++;
                        if (r + c == SIZE - 1) diag2Count++;
                    }
                    if (config.check_windows) {
                        const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                        for (int w = 0; w < 4; w++) {
                            if (r >= windows[w].first && r < windows[w].first + 3 &&
                                c >= windows[w].second && c < windows[w].second + 3) {
                                winCounts[w]++;
                            }
                        }
                    }
                }
            }
        }

        // Agar actual bache hue clues target se match nahi hote toh return false
        if (clues != config.target_clues) return false;

        // Constraints violation checking
        for (int i = 0; i < SIZE; i++) {
            if (rowCounts[i] > config.max_per_row) return false;
            if (colCounts[i] > config.max_per_col) return false;
        }
        for (int i = 0; i < NUM_GRIDS; i++) {
            if (gridCounts[i] > config.max_per_grid) return false;
        }
        if (config.check_diagonals) {
            if (diag1Count > config.max_per_diagonal) return false;
            if (diag2Count > config.max_per_diagonal) return false;
            if (config.exact_diagonal && (diag1Count != config.max_per_diagonal || diag2Count != config.max_per_diagonal)) return false;
        }
        if (config.check_windows) {
            for (int i = 0; i < 4; i++) {
                if (winCounts[i] > config.max_per_window) return false;
                if (config.exact_window && winCounts[i] != config.max_per_window) return false;
            }
        }

        return true; // Saari shartein poori ho gayi!
    }

public:
    PuzzleGenerator() : rng(random_device{}()) {}

    bool generate(int* board, const PuzzleConfig& config) {
        int original[SIZE * SIZE];
        for (int i = 0; i < SIZE * SIZE; i++) original[i] = board[i];

        computeGridLookup();

        UniquenessChecker<SIZE, SUB_R, SUB_C> checker;

        int num_grids = NUM_GRIDS;
        
        int min_row = 0;
        int min_col = 0;
        int min_grid = 0;
        int min_diag = 0;
        int min_win = 0;

        if (!customConstraints) {
            min_row = config.target_clues - (SIZE - 1) * config.max_per_row;
            if (min_row < 0) min_row = 0;
            min_col = config.target_clues - (SIZE - 1) * config.max_per_col;
            if (min_col < 0) min_col = 0;
            min_grid = config.target_clues - (num_grids - 1) * config.max_per_grid;
            if (min_grid < 0) min_grid = 0;
            
            min_diag = config.exact_diagonal ? config.max_per_diagonal : 0;
            min_win = config.exact_window ? config.max_per_window : 0;
        }

        int attempts = 0;

        // Uniqueness checker ke params set kar rahe hain
        checker.gridMap = this->gridMap;
        checker.altGridMap = this->altGridMap;
        checker.activeMap = this->activeMap;
        checker.check_diagonals = config.check_diagonals;
        checker.check_windows = config.check_windows;

        // Generator ko 250 chances denge retry karne ke liye
        while (attempts < 250) {
            attempts++;
            // Board ko complete filled state me reset karo
            for (int i = 0; i < SIZE * SIZE; i++) board[i] = original[i];

            int currentClues = SIZE * SIZE;
            int stuckCount = 0;

            // Jab tak clues desired number se zyada hain, hum cells hataane ki koshish karenge
            while (currentClues > config.target_clues && stuckCount < 5) {
                // Compute current constraint counts
                int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0}, gridCounts[NUM_GRIDS] = {0};
                int diag1Count = 0, diag2Count = 0;
                int winCounts[4] = {0};
                for (int r = 0; r < SIZE; r++) {
                    for (int c = 0; c < SIZE; c++) {
                        int pos = r * SIZE + c;
                        if (activeMap && !activeMap[pos]) continue;
                        if (board[pos] != 0) {
                            rowCounts[r]++;
                            colCounts[c]++;
                            gridCounts[gridLookup[pos]]++;
                            if (config.check_diagonals) {
                                if (r == c) diag1Count++;
                                if (r + c == SIZE - 1) diag2Count++;
                            }
                            if (config.check_windows) {
                                const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                                for (int w = 0; w < 4; w++) {
                                    if (r >= windows[w].first && r < windows[w].first + 3 &&
                                        c >= windows[w].second && c < windows[w].second + 3) {
                                        winCounts[w]++;
                                    }
                                }
                            }
                        }
                    }
                }

                // Un candidates ko gather karo jinko easily (safely) remove kiya jaa sake
                vector<int> candidates;
                for (int r = 0; r < SIZE; r++) {
                    for (int c = 0; c < SIZE; c++) {
                        int pos = r * SIZE + c;
                        if (activeMap && !activeMap[pos]) continue;
                        if (board[pos] != 0) {
                            bool can_remove = (rowCounts[r] > min_row && colCounts[c] > min_col && gridCounts[gridLookup[pos]] > min_grid);
                            
                            if (can_remove && config.check_diagonals) {
                                if (r == c && diag1Count <= min_diag) can_remove = false;
                                if (r + c == SIZE - 1 && diag2Count <= min_diag) can_remove = false;
                            }
                            if (can_remove && config.check_windows) {
                                const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                                for (int w = 0; w < 4; w++) {
                                    if (r >= windows[w].first && r < windows[w].first + 3 && c >= windows[w].second && c < windows[w].second + 3) {
                                        if (winCounts[w] <= min_win) can_remove = false;
                                    }
                                }
                            }
                            
                            if (can_remove) candidates.push_back(pos);
                        }
                    }
                }

                // Randomness dalne ke baad un candidates ko sort karenge jaha par sabse zyada values jama hain
                shuffle(candidates.begin(), candidates.end(), rng);
                stable_sort(candidates.begin(), candidates.end(), [&](int pA, int pB) {
                    int rA = pA / SIZE, cA = pA % SIZE;
                    int rB = pB / SIZE, cB = pB % SIZE;
                    int scoreA = rowCounts[rA] + colCounts[cA] + gridCounts[gridLookup[pA]];
                    int scoreB = rowCounts[rB] + colCounts[cB] + gridCounts[gridLookup[pB]];
                    
                    if (config.check_diagonals) {
                        if (rA == cA) scoreA += diag1Count;
                        if (rA + cA == SIZE - 1) scoreA += diag2Count;
                        if (rB == cB) scoreB += diag1Count;
                        if (rB + cB == SIZE - 1) scoreB += diag2Count;
                    }
                    if (config.check_windows) {
                        const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                        for (int w = 0; w < 4; w++) {
                            if (rA >= windows[w].first && rA < windows[w].first + 3 && cA >= windows[w].second && cA < windows[w].second + 3) scoreA += winCounts[w];
                            if (rB >= windows[w].first && rB < windows[w].first + 3 && cB >= windows[w].second && cB < windows[w].second + 3) scoreB += winCounts[w];
                        }
                    }
                    return scoreA > scoreB;
                });

                bool removedAny = false;
                for (int pos : candidates) {
                    int backup = board[pos];
                    board[pos] = 0;

                    // Checker use karte hain ensure karne ke liye ki puzzle uniqueness loose toh nahi kiya...
                    if (checker.count(board) == 1) {
                        currentClues--;
                        removedAny = true;
                        break; // Loop break karo taaki counts phir se recompute ho sakein
                    } else {
                        board[pos] = backup; // Cannot remove without breaking uniqueness, Undo karo!
                    }
                }

                if (!removedAny) {
                    // Stuck recovery (Perturbation): Agar aur cell nahi hat pa rahe hain,
                    // toh aakhri haal mein kuj randomly cells wapas daal dete hain aur 
                    // aage se ek naya alternative pathway dhundhte hain.
                    stuckCount++;
                    vector<int> emptyCells;
                    for (int pos = 0; pos < SIZE * SIZE; pos++) {
                        if (activeMap && !activeMap[pos]) continue;
                        if (board[pos] == 0 && original[pos] != 0)
                            emptyCells.push_back(pos);
                    }
                    if (emptyCells.size() < 2) break;
                    shuffle(emptyCells.begin(), emptyCells.end(), rng);
                    int putBack = min(3, (int)emptyCells.size());
                    for (int i = 0; i < putBack; i++) {
                        board[emptyCells[i]] = original[emptyCells[i]];
                        currentClues++;
                    }
                }
            }

            if (currentClues == config.target_clues && satisfiesConstraints(board, config)) {
                return true; // Success! Puzzle ready hai
            }
        }

        // Return false: Caller function signal catch karke puzzle dobara random seed se suru karega
        return false;
    }
};
#endif
