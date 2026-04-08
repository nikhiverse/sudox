#ifndef DIAGONAL_PUZZLE_CPP
#define DIAGONAL_PUZZLE_CPP

#include <vector>
#include <algorithm>
#include <iostream>
#include <random>
#include "uniqueness.cpp"
#include "clues_config.hpp"

using namespace std;

// DiagonalPuzzleGenerator: Yeh puzzles mein clues ko primary(\) and secondary(/) 
// diagonals ke across mirror image (symmetric) rakhta hai.
template <int SIZE, int SUB_R, int SUB_C>
class DiagonalPuzzleGenerator {
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

    // satisfiesConstraints: Ensure karta hai ki diagonal count requirements bhi pure hain puzzle mein.
    bool satisfiesConstraints(const int* board, const PuzzleConfig& config) {
        int clues = 0;
        int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0}, gridCounts[NUM_GRIDS] = {0};
        int diag1Count = 0, diag2Count = 0;

        for (int pos = 0; pos < SIZE * SIZE; pos++) {
            int r = pos / SIZE, c = pos % SIZE;
            if (board[pos] != 0) {
                clues++;
                rowCounts[r]++;
                colCounts[c]++;
                gridCounts[gridLookup[pos]]++;
                // Check if element lies on \ diagonal (primary)
                if (r == c) diag1Count++;
                // Check if element lies on / diagonal (secondary)
                if (r + c == SIZE - 1) diag2Count++;
            }
        }

        if (clues != config.target_clues) return false;

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
            // Agar specifically diagonals exact length mangti hai tabhi extra conditions lagengi
            if (config.exact_diagonal && (diag1Count != config.max_per_diagonal || diag2Count != config.max_per_diagonal)) return false;
        }

        return true;
    }

public:
    DiagonalPuzzleGenerator() : rng(random_device{}()) {}

    bool generate(int* board, const PuzzleConfig& config) {
        int original[SIZE * SIZE];
        for(int pos = 0; pos < SIZE * SIZE; pos++) original[pos] = board[pos];

        computeGridLookup();

        UniquenessChecker<SIZE, SUB_R, SUB_C> checker;
        checker.gridMap = this->gridMap;
        checker.check_diagonals = config.check_diagonals;

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

        while (attempts < 1000) {
            attempts++;
            // Reset board completely
            for(int pos = 0; pos < SIZE * SIZE; pos++) board[pos] = original[pos];

            int currentClues = SIZE * SIZE;
            int stuckCount = 0;

            while (currentClues > config.target_clues && stuckCount < 5) {
                // Compute current counts dynamically loop ke andar
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
                                    c >= windows[w].second && c < windows[w].second + 3) winCounts[w]++;
                            }
                        }
                    }
                }

                vector<int> candidates;
                for (int r = 0; r < SIZE; r++) {
                    // Agar gridMap custom layout hai, toh pure row traverse karna padta hai
                    // nahi toh just ek half traverse karke hi uski reflection find ki ja sakti hai (r, c) => (c, r)
                    int start_c = (gridMap != nullptr) ? 0 : r;
                    for (int c = start_c; c < SIZE; c++) {
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
                    if (rA != cA) scoreA += getScore(cA * SIZE + rA);
                    
                    int rB = pB / SIZE, cB = pB % SIZE;
                    int scoreB = getScore(pB);
                    if (rB != cB) scoreB += getScore(cB * SIZE + rB);

                    return scoreA > scoreB;
                });

                bool removedAny = false;
                for (int pos : candidates) {
                    int r = pos / SIZE;
                    int c = pos % SIZE;
                    // Primary diagonal (c=r line) pe flip karke mirror points: r_sym = c, c_sym = r
                    int r_sym = c;
                    int c_sym = r;
                    
                    int pos_sym = r_sym * SIZE + c_sym;

                    int removed_count = (r == r_sym && c == c_sym) ? 1 : 2;
                    
                    if (gridMap != nullptr) { // Custom irregular shapes pe symmetric reflection disable kar dete hain aam taur pe
                        r_sym = r;
                        c_sym = c;
                        pos_sym = pos;
                        removed_count = 1;
                    }

                    if (currentClues - removed_count < config.target_clues) continue;

                    // Verify boundaries and min-count logic (Taki puzzle unsolvable na ban jaye)
                    int gIdx = gridLookup[pos];
                    int gIdx_sym = gridLookup[pos_sym];
                    
                    bool valid_counts = true;
                    if (removed_count == 1) {
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
                    } else {
                        // Jab 2 entries simultaneously hatayi jaati hain mirror pair ki wajah se
                        if (rowCounts[r] - 1 < min_row || rowCounts[r_sym] - 1 < min_row) valid_counts = false;
                        if (colCounts[c] - 1 < min_col || colCounts[c_sym] - 1 < min_col) valid_counts = false;
                        
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
                    
                    // Dono points remove karo symmetry banane ke liye
                    board[pos] = 0;
                    board[pos_sym] = 0;

                    if (checker.count(board) == 1) { // Check that there's still ONLY 1 solution!
                        currentClues -= removed_count;
                        removedAny = true;
                        break; 
                    } else {
                        // Undo karo aur agli location try karo!
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
                        if (checker.count(board) == 1) { // Verify validity
                            currentClues--;
                            removedAny = true;
                            break;
                        }
                        board[pos] = backup; // Undo
                    }
                    
                    if (!removedAny) {
                        stuckCount++;
                        vector<int> emptyCells;
                        for (int pos = 0; pos < SIZE * SIZE; pos++)
                            if (board[pos] == 0 && original[pos] != 0)
                                emptyCells.push_back(pos);
                        if (emptyCells.size() < 2) break;
                        shuffle(emptyCells.begin(), emptyCells.end(), rng); // Perturb randomly to get un-stuck
                        int putBack = min(4, (int)emptyCells.size());
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
