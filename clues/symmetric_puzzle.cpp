#ifndef SYMMETRIC_CPP
#define SYMMETRIC_CPP

#include <vector>
#include <algorithm>
#include <iostream>
#include <random>
#include "uniqueness.cpp"
#include "clues_config.hpp"

using namespace std;

// SymmetricPuzzleGenerator: Yeh puzzles mein clues ko 180 degrees centrally
// rotational (inverse point) symmetry par locate/remove karta hai.
template <int SIZE, int SUB_R, int SUB_C>
class SymmetricPuzzleGenerator {
public:
    int* gridMap = nullptr;

private:
    static constexpr int NUM_GRIDS = (SIZE / SUB_R) * (SIZE / SUB_C);
    mt19937 rng;
    int gridLookup[SIZE * SIZE];

    void computeGridLookup() {
        for (int pos = 0; pos < SIZE * SIZE; pos++) {
            int r = pos / SIZE, c = pos % SIZE;
            gridLookup[pos] = gridMap ? gridMap[pos] : ((r / SUB_R) * (SIZE / SUB_C) + (c / SUB_C));
        }
    }

    // satisfiesConstraints: Ensure karta hai ki exact parameters count match ho rahi ho
    bool satisfiesConstraints(const int* board, const PuzzleConfig& config) {
        int clues = 0;
        int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0}, gridCounts[NUM_GRIDS] = {0};
        int winCounts[4] = {0};

        for (int pos = 0; pos < SIZE * SIZE; pos++) {
            int r = pos / SIZE, c = pos % SIZE;
            if (board[pos] != 0) {
                clues++;
                rowCounts[r]++;
                colCounts[c]++;
                gridCounts[gridLookup[pos]]++;
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

        if (clues != config.target_clues) return false;

        for (int i = 0; i < SIZE; i++) {
            // Agar limit exceed kar gayi, toh fail! Backtrack wahi se karna padega.
            if (rowCounts[i] > config.max_per_row) return false;
            if (colCounts[i] > config.max_per_col) return false;
        }
        for (int i = 0; i < NUM_GRIDS; i++) {
            if (gridCounts[i] > config.max_per_grid) return false;
        }
        if (config.check_windows) {
            for (int i = 0; i < 4; i++) {
                if (winCounts[i] > config.max_per_window) return false;
                if (config.exact_window && winCounts[i] != config.max_per_window) return false;
            }
        }

        return true;
    }

public:
    SymmetricPuzzleGenerator() : rng(random_device{}()) {}

    bool generate(int* board, const PuzzleConfig& config) {
        int original[SIZE * SIZE];
        for (int pos = 0; pos < SIZE * SIZE; pos++) original[pos] = board[pos];

        computeGridLookup();

        UniquenessChecker<SIZE, SUB_R, SUB_C> checker;

        int num_grids = NUM_GRIDS;
        int min_row = config.target_clues - (SIZE - 1) * config.max_per_row;
        if (min_row < 0) min_row = 0;
        int min_col = config.target_clues - (SIZE - 1) * config.max_per_col;
        if (min_col < 0) min_col = 0;
        int min_grid = config.target_clues - (num_grids - 1) * config.max_per_grid;
        if (min_grid < 0) min_grid = 0;
        
        int min_diag = config.exact_diagonal ? config.max_per_diagonal : 0;
        int min_win = config.exact_window ? config.max_per_window : 0;

        int attempts = 0;

        checker.gridMap = this->gridMap;
        checker.check_windows = config.check_windows;

        while (attempts < 1000) {
            attempts++;
            for (int pos = 0; pos < SIZE * SIZE; pos++) board[pos] = original[pos];

            int currentClues = SIZE * SIZE;
            int stuckCount = 0;

            while (currentClues > config.target_clues && stuckCount < 5) {
                int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0}, gridCounts[NUM_GRIDS] = {0};
                int diag1Count = 0, diag2Count = 0;
                int winCounts[4] = {0};
                for (int pos = 0; pos < SIZE * SIZE; pos++) {
                    int r = pos / SIZE, c = pos % SIZE;
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

                vector<int> candidates;
                for (int r = 0; r < SIZE; r++) {
                    for (int c = 0; c < SIZE; c++) {
                        // Consider cells only in the "first half" to avoid duplicating pairs
                        // Warna symmetry mirror duplicate pairs array ko load kardegi
                        if (r > SIZE / 2 || (r == SIZE / 2 && c > SIZE / 2)) continue;
                        int pos = r * SIZE + c;
                        if (board[pos] != 0) {
                            candidates.push_back(pos);
                        }
                    }
                }
                shuffle(candidates.begin(), candidates.end(), rng);
                stable_sort(candidates.begin(), candidates.end(), [&](int pA, int pB) {
                    auto getScore = [&](int pos) {
                        int r = pos / SIZE, c = pos % SIZE;
                        int s = rowCounts[r] + colCounts[c] + gridCounts[gridLookup[pos]];
                        if (config.check_diagonals) {
                            if (r == c) s += diag1Count;
                            if (r + c == SIZE - 1) s += diag2Count;
                        }
                        if (config.check_windows) {
                            const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                            for (int w = 0; w < 4; w++) {
                                if (r >= windows[w].first && r < windows[w].first + 3 && c >= windows[w].second && c < windows[w].second + 3) s += winCounts[w];
                            }
                        }
                        return s;
                    };
                    
                    int rA = pA / SIZE, cA = pA % SIZE;
                    int scoreA = getScore(pA);
                    
                    // Rotational Symmetry inverse coordinate mapping (180 degree rotation mapping)
                    int rA_sym = SIZE - 1 - rA, cA_sym = SIZE - 1 - cA;
                    if (rA != rA_sym || cA != cA_sym) scoreA += getScore(rA_sym * SIZE + cA_sym);
                    
                    int rB = pB / SIZE, cB = pB % SIZE;
                    int scoreB = getScore(pB);
                    int rB_sym = SIZE - 1 - rB, cB_sym = SIZE - 1 - cB;
                    if (rB != rB_sym || cB != cB_sym) scoreB += getScore(rB_sym * SIZE + cB_sym);

                    return scoreA > scoreB;
                });

                bool removedAny = false;
                for (int pos : candidates) {
                    int r = pos / SIZE;
                    int c = pos % SIZE;
                    
                    // Center se inverted mirror calculate karte hain array index
                    int r_sym = SIZE - 1 - r;
                    int c_sym = SIZE - 1 - c;
                    
                    int pos_sym = r_sym * SIZE + c_sym;

                    int removed_count = (r == r_sym && c == c_sym) ? 1 : 2;

                    // Do not overshoot the target clues
                    if (currentClues - removed_count < config.target_clues) continue;

                    int gIdx = gridLookup[pos];
                    int gIdx_sym = gridLookup[pos_sym];
                    
                    bool valid_counts = true;
                    if (removed_count == 1) { // Center point akela eliminate hoga 1 hi cell pe
                        if (rowCounts[r] - 1 < min_row) valid_counts = false;
                        if (colCounts[c] - 1 < min_col) valid_counts = false;
                        if (gridCounts[gIdx] - 1 < min_grid) valid_counts = false;
                        if (config.check_diagonals) {
                            if (r == c && diag1Count - 1 < min_diag) valid_counts = false;
                            if (r + c == SIZE - 1 && diag2Count - 1 < min_diag) valid_counts = false;
                            if (config.exact_diagonal && r == c && r + c == SIZE - 1) {
                                if (config.max_per_diagonal % 2 != 0) valid_counts = false;
                            }
                        }
                        if (config.check_windows) {
                            const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                            for (int w = 0; w < 4; w++) {
                                if (r >= windows[w].first && r < windows[w].first + 3 && c >= windows[w].second && c < windows[w].second + 3) {
                                    if (winCounts[w] - 1 < min_win) valid_counts = false;
                                }
                            }
                        }
                    } else { // Nahi toh do reflection points ikkatthe erase honge 
                        if (r == r_sym) {
                            if (rowCounts[r] - 2 < min_row) valid_counts = false;
                        } else {
                            if (rowCounts[r] - 1 < min_row || rowCounts[r_sym] - 1 < min_row) valid_counts = false;
                        }
                        
                        if (c == c_sym) {
                            if (colCounts[c] - 2 < min_col) valid_counts = false;
                        } else {
                            if (colCounts[c] - 1 < min_col || colCounts[c_sym] - 1 < min_col) valid_counts = false;
                        }
                        
                        if (gIdx == gIdx_sym) {
                            if (gridCounts[gIdx] - 2 < min_grid) valid_counts = false;
                        } else {
                            if (gridCounts[gIdx] - 1 < min_grid || gridCounts[gIdx_sym] - 1 < min_grid) valid_counts = false;
                        }
                        
                        if (config.check_diagonals) {
                            int d1_rem = (r == c ? 1 : 0) + (r_sym == c_sym ? 1 : 0);
                            int d2_rem = (r + c == SIZE - 1 ? 1 : 0) + (r_sym + c_sym == SIZE - 1 ? 1 : 0);
                            if (diag1Count - d1_rem < min_diag) valid_counts = false;
                            if (diag2Count - d2_rem < min_diag) valid_counts = false;
                        }
                        
                        if (config.check_windows) {
                            const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                            for (int w = 0; w < 4; w++) {
                                int w_rem = 0;
                                if (r >= windows[w].first && r < windows[w].first + 3 && c >= windows[w].second && c < windows[w].second + 3) w_rem++;
                                if (r_sym >= windows[w].first && r_sym < windows[w].first + 3 && c_sym >= windows[w].second && c_sym < windows[w].second + 3) w_rem++;
                                if (winCounts[w] - w_rem < min_win) valid_counts = false;
                            }
                        }
                    }
                    
                    if (!valid_counts) continue;

                    int backup1 = board[pos];
                    int backup2 = board[pos_sym];
                    
                    board[pos] = 0;
                    board[pos_sym] = 0;

                    // Uniqueness checker confirms ki abhi bhi multiple solution exist toh nahi karte?
                    if (checker.count(board) == 1) {
                        currentClues -= removed_count;
                        removedAny = true;
                        break; 
                    } else {
                        board[pos] = backup1;
                        board[pos_sym] = backup2;
                    }
                }

                if (!removedAny) {
                    vector<int> singles;
                    for (int pos = 0; pos < SIZE * SIZE; pos++) {
                        if (board[pos] != 0) singles.push_back(pos);
                    }
                    shuffle(singles.begin(), singles.end(), rng);
                    for (int pos : singles) {
                        int r = pos / SIZE;
                        int c = pos % SIZE;

                        int gIdx = gridLookup[pos];
                        if (rowCounts[r] - 1 < min_row) continue;
                        if (colCounts[c] - 1 < min_col) continue;
                        if (gridCounts[gIdx] - 1 < min_grid) continue;
                        
                        bool skip = false;
                        if (config.check_diagonals) {
                            if (r == c && diag1Count - 1 < min_diag) skip = true;
                            if (r + c == SIZE - 1 && diag2Count - 1 < min_diag) skip = true;
                        }
                        if (config.check_windows) {
                            const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                            for (int w = 0; w < 4; w++) {
                                if (r >= windows[w].first && r < windows[w].first + 3 && c >= windows[w].second && c < windows[w].second + 3) {
                                    if (winCounts[w] - 1 < min_win) skip = true;
                                }
                            }
                        }
                        if (skip) continue;

                        int backup = board[pos];
                        board[pos] = 0;
                        if (checker.count(board) == 1) {
                            currentClues--;
                            removedAny = true;
                            break;
                        }
                        board[pos] = backup;
                    }
                    
                    if (!removedAny) {
                        stuckCount++;
                        vector<int> emptyCells;
                        for (int pos = 0; pos < SIZE * SIZE; pos++)
                            if (board[pos] == 0 && original[pos] != 0)
                                emptyCells.push_back(pos);
                        if (emptyCells.size() < 2) break;
                        shuffle(emptyCells.begin(), emptyCells.end(), rng);
                        int putBack = min(4, (int)emptyCells.size());
                        // Stuck hone par wapas board restore ho jayega backup memory loop me randomly
                        for (int i = 0; i < putBack; i++) {
                            board[emptyCells[i]] = original[emptyCells[i]];
                            currentClues++;
                        }
                    }
                }
            }

            if (stuckCount < 5 && satisfiesConstraints(board, config)) {
                return true; 
            }
        }

        return false;
    }
};

#endif
