#ifndef HYBRID_PUZZLE_CPP
#define HYBRID_PUZZLE_CPP

#include <vector>
#include <algorithm>
#include <iostream>
#include <random>
#include <functional>
#include "uniqueness.cpp"
#include "clues_config.hpp"

using namespace std;

// HybridPuzzleGenerator: Yeh generator diagonal symmetry aur random removal ka
// combination hai. Off-diagonal cells (r != c) diagonal symmetry se hataaye jaate hain
// (mirror across primary diagonal: (r,c) <-> (c,r)), aur diagonal cells (r == c)
// randomly hataaye jaate hain. Twodoku9 ke liye designed hai — primary diagonal
// woh hai jahan overlap grid hota hai.
template <int SIZE, int SUB_R, int SUB_C>
class HybridPuzzleGenerator {
public:
    int* gridMap = nullptr;
    bool* activeMap = nullptr;
    std::function<bool(const int*, const PuzzleConfig&)> customConstraints = nullptr;

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

    // satisfiesConstraints: Ensure karta hai ki puzzle saari config requirements poori karta hai
    bool satisfiesConstraints(const int* board, const PuzzleConfig& config) {
        if (customConstraints) return customConstraints(board, config);
        int clues = 0;
        int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0}, gridCounts[NUM_GRIDS] = {0};
        int diag1Count = 0, diag2Count = 0;

        for (int pos = 0; pos < SIZE * SIZE; pos++) {
            if (activeMap && !activeMap[pos]) continue;
            int r = pos / SIZE, c = pos % SIZE;
            if (board[pos] != 0) {
                clues++;
                rowCounts[r]++;
                colCounts[c]++;
                gridCounts[gridLookup[pos]]++;
                if (r == c) diag1Count++;
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
            if (config.exact_diagonal && (diag1Count != config.max_per_diagonal || diag2Count != config.max_per_diagonal)) return false;
        }

        return true;
    }

public:
    HybridPuzzleGenerator() : rng(random_device{}()) {}

    bool generate(int* board, const PuzzleConfig& config) {
        int original[SIZE * SIZE];
        for (int pos = 0; pos < SIZE * SIZE; pos++) original[pos] = board[pos];

        computeGridLookup();

        UniquenessChecker<SIZE, SUB_R, SUB_C> checker;
        checker.gridMap = this->gridMap;
        checker.activeMap = this->activeMap;
        checker.check_diagonals = config.check_diagonals;

        int num_grids = NUM_GRIDS;
        int min_row = config.target_clues - (SIZE - 1) * config.max_per_row;
        if (min_row < 0) min_row = 0;
        int min_col = config.target_clues - (SIZE - 1) * config.max_per_col;
        if (min_col < 0) min_col = 0;
        int min_grid = config.target_clues - (num_grids - 1) * config.max_per_grid;
        if (min_grid < 0) min_grid = 0;

        int min_diag = config.exact_diagonal ? config.max_per_diagonal : 0;

        int attempts = 0;

        while (attempts < 1000) {
            attempts++;
            // Board reset to full state
            for (int pos = 0; pos < SIZE * SIZE; pos++) board[pos] = original[pos];

            int currentClues = SIZE * SIZE;
            int stuckCount = 0;

            // ===== PHASE 1: Diagonal Symmetric Removal =====
            // Off-diagonal cells (r != c) ko diagonal symmetry se hataate hain
            // Mirror: (r,c) <-> (c,r) primary diagonal ke across
            while (currentClues > config.target_clues && stuckCount < 5) {
                int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0}, gridCounts[NUM_GRIDS] = {0};
                int diag1Count = 0, diag2Count = 0;
                for (int pos = 0; pos < SIZE * SIZE; pos++) {
                    if (activeMap && !activeMap[pos]) continue;
                    int r = pos / SIZE, c = pos % SIZE;
                    if (board[pos] != 0) {
                        rowCounts[r]++;
                        colCounts[c]++;
                        gridCounts[gridLookup[pos]]++;
                        if (r == c) diag1Count++;
                        if (r + c == SIZE - 1) diag2Count++;
                    }
                }

                // Phase 1 candidates: off-diagonal cells only (r != c), upper triangle
                vector<int> candidates;
                for (int r = 0; r < SIZE; r++) {
                    for (int c = r + 1; c < SIZE; c++) { // Only upper triangle (c > r)
                        int pos = r * SIZE + c;
                        if (activeMap && (!activeMap[pos] || !activeMap[c * SIZE + r])) continue; // Need both mirror cells active
                        if (board[pos] != 0) {
                            candidates.push_back(pos);
                        }
                    }
                }

                if (candidates.empty()) break; // No more off-diagonal candidates

                shuffle(candidates.begin(), candidates.end(), rng);
                stable_sort(candidates.begin(), candidates.end(), [&](int pA, int pB) {
                    auto getScore = [&](int pos) {
                        int r = pos / SIZE, c = pos % SIZE;
                        int s = rowCounts[r] + colCounts[c] + gridCounts[gridLookup[pos]];
                        return s;
                    };
                    
                    int rA = pA / SIZE, cA = pA % SIZE;
                    int scoreA = getScore(pA);
                    scoreA += getScore(cA * SIZE + rA); // Mirror score bhi add karo
                    
                    int rB = pB / SIZE, cB = pB % SIZE;
                    int scoreB = getScore(pB);
                    scoreB += getScore(cB * SIZE + rB);

                    return scoreA > scoreB;
                });

                bool removedAny = false;
                for (int pos : candidates) {
                    int r = pos / SIZE;
                    int c = pos % SIZE;
                    // Primary diagonal mirror: (r,c) <-> (c,r)
                    int r_sym = c;
                    int c_sym = r;
                    int pos_sym = r_sym * SIZE + c_sym;

                    // Symmetric pair always removes 2 (since r != c)
                    int removed_count = 2;

                    if (currentClues - removed_count < config.target_clues) continue;

                    // Check mirror cell is also filled
                    if (board[pos_sym] == 0) continue;

                    // Validate min counts won't be violated
                    int gIdx = gridLookup[pos];
                    int gIdx_sym = gridLookup[pos_sym];
                    
                    bool valid_counts = true;
                    if (rowCounts[r] - 1 < min_row || rowCounts[r_sym] - 1 < min_row) valid_counts = false;
                    if (colCounts[c] - 1 < min_col || colCounts[c_sym] - 1 < min_col) valid_counts = false;
                    
                    if (gIdx == gIdx_sym) {
                        if (gridCounts[gIdx] - 2 < min_grid) valid_counts = false;
                    } else {
                        if (gridCounts[gIdx] - 1 < min_grid || gridCounts[gIdx_sym] - 1 < min_grid) valid_counts = false;
                    }
                    
                    if (!valid_counts) continue;

                    int backup1 = board[pos];
                    int backup2 = board[pos_sym];
                    
                    board[pos] = 0;
                    board[pos_sym] = 0;

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
                    // Try single off-diagonal removal as fallback
                    vector<int> singles;
                    for (int pos = 0; pos < SIZE * SIZE; pos++) {
                        if (activeMap && !activeMap[pos]) continue;
                        int r = pos / SIZE, c = pos % SIZE;
                        if (r != c && board[pos] != 0) singles.push_back(pos);
                    }
                    shuffle(singles.begin(), singles.end(), rng);
                    for (int pos : singles) {
                        int r = pos / SIZE, c = pos % SIZE;
                        int gIdx = gridLookup[pos];
                        if (rowCounts[r] - 1 < min_row) continue;
                        if (colCounts[c] - 1 < min_col) continue;
                        if (gridCounts[gIdx] - 1 < min_grid) continue;

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
                        for (int pos = 0; pos < SIZE * SIZE; pos++) {
                            if (activeMap && !activeMap[pos]) continue;
                            if (board[pos] == 0 && original[pos] != 0)
                                emptyCells.push_back(pos);
                        }
                        if (emptyCells.size() < 2) break;
                        shuffle(emptyCells.begin(), emptyCells.end(), rng);
                        int putBack = min(4, (int)emptyCells.size());
                        for (int i = 0; i < putBack; i++) {
                            board[emptyCells[i]] = original[emptyCells[i]];
                            currentClues++;
                        }
                    }
                }
            }

            // ===== PHASE 2: Random Removal on Diagonal =====
            // Primary diagonal cells (r == c) se randomly clues hataate hain
            stuckCount = 0;
            while (currentClues > config.target_clues && stuckCount < 5) {
                int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0}, gridCounts[NUM_GRIDS] = {0};
                for (int pos = 0; pos < SIZE * SIZE; pos++) {
                    if (activeMap && !activeMap[pos]) continue;
                    int r = pos / SIZE, c = pos % SIZE;
                    if (board[pos] != 0) {
                        rowCounts[r]++;
                        colCounts[c]++;
                        gridCounts[gridLookup[pos]]++;
                    }
                }

                // Diagonal candidates: r == c
                vector<int> diagCandidates;
                for (int i = 0; i < SIZE; i++) {
                    int pos = i * SIZE + i;
                    if (activeMap && !activeMap[pos]) continue;
                    if (board[pos] != 0) {
                        diagCandidates.push_back(pos);
                    }
                }

                // Also consider any remaining off-diagonal cells
                for (int pos = 0; pos < SIZE * SIZE; pos++) {
                    if (activeMap && !activeMap[pos]) continue;
                    int r = pos / SIZE, c = pos % SIZE;
                    if (r != c && board[pos] != 0) {
                        diagCandidates.push_back(pos);
                    }
                }

                shuffle(diagCandidates.begin(), diagCandidates.end(), rng);
                // Sort by density score
                stable_sort(diagCandidates.begin(), diagCandidates.end(), [&](int pA, int pB) {
                    int rA = pA / SIZE, cA = pA % SIZE;
                    int scoreA = rowCounts[rA] + colCounts[cA] + gridCounts[gridLookup[pA]];
                    int rB = pB / SIZE, cB = pB % SIZE;
                    int scoreB = rowCounts[rB] + colCounts[cB] + gridCounts[gridLookup[pB]];
                    return scoreA > scoreB;
                });

                bool removedAny = false;
                for (int pos : diagCandidates) {
                    int r = pos / SIZE, c = pos % SIZE;
                    int gIdx = gridLookup[pos];
                    if (rowCounts[r] - 1 < min_row) continue;
                    if (colCounts[c] - 1 < min_col) continue;
                    if (gridCounts[gIdx] - 1 < min_grid) continue;

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
                return true;
            }
        }

        return false;
    }
};

#endif
