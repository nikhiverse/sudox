#ifndef MIXED_PUZZLE_CPP
#define MIXED_PUZZLE_CPP

#include <vector>
#include <algorithm>
#include <iostream>
#include <random>
#include "uniqueness.cpp"
#include "clues_config.hpp"

using namespace std;

// MixedPuzzleGenerator: Yeh generator adha puzzle symmetric banata hai 
// aur adha random jisse design natural aur beautiful dono lagein.
template <int SIZE, int SUB_R, int SUB_C>
class MixedPuzzleGenerator {
public:
    int* gridMap = nullptr;
    int* altGridMap = nullptr;

private:
    static constexpr int NUM_GRIDS = (SIZE / SUB_R) * (SIZE / SUB_C);
    mt19937 rng;
    int gridLookup[SIZE * SIZE];

    void computeGridLookup() {
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                gridLookup[pos] = gridMap ? gridMap[pos] : ((r / SUB_R) * (SIZE / SUB_C) + (c / SUB_C));
            }
        }
    }

    // satisfiesConstraints: Verify karta hai ki saari difficulty requirements poori hui hain.
    bool satisfiesConstraints(const int* board, const PuzzleConfig& config) {
        int clues = 0;
        int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0};
        int gridCounts[NUM_GRIDS] = {0}, altGridCounts[NUM_GRIDS] = {0};
        int diag1Count = 0, diag2Count = 0;
        int winCounts[4] = {0};

        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (board[pos] != 0) {
                    clues++;
                    rowCounts[r]++;
                    colCounts[c]++;
                    gridCounts[gridLookup[pos]]++;
                    if (altGridMap) {
                        altGridCounts[altGridMap[pos]]++;
                    }
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

        if (clues != config.target_clues) return false;

        for (int i = 0; i < SIZE; i++) {
            if (rowCounts[i] > config.max_per_row) return false;
            if (colCounts[i] > config.max_per_col) return false;
        }
        for (int i = 0; i < NUM_GRIDS; i++) {
            if (gridCounts[i] > config.max_per_grid) return false;
            if (altGridMap && altGridCounts[i] > config.max_per_grid) return false;
        }
        if (config.check_diagonals) {
            if (diag1Count > config.max_per_diagonal) return false;
            if (diag2Count > config.max_per_diagonal) return false;
        }
        if (config.check_windows) {
            for (int i = 0; i < 4; i++) {
                if (winCounts[i] > config.max_per_window) return false;
            }
        }

        return true;
    }

public:
    MixedPuzzleGenerator() : rng(random_device{}()) {}

    bool generate(int* board, const PuzzleConfig& config) {
        int original[SIZE * SIZE];
        for (int i = 0; i < SIZE * SIZE; i++) original[i] = board[i];

        computeGridLookup();

        UniquenessChecker<SIZE, SUB_R, SUB_C> checker;
        checker.gridMap = this->gridMap;
        checker.altGridMap = this->altGridMap;
        checker.check_diagonals = config.check_diagonals;
        checker.check_windows = config.check_windows;

        int num_grids = NUM_GRIDS;
        int min_row = config.target_clues - (SIZE - 1) * config.max_per_row;
        if (min_row < 0) min_row = 0;
        int min_col = config.target_clues - (SIZE - 1) * config.max_per_col;
        if (min_col < 0) min_col = 0;
        int min_grid = config.target_clues - (num_grids - 1) * config.max_per_grid;
        if (min_grid < 0) min_grid = 0;
        
        int min_diag = config.exact_diagonal ? config.max_per_diagonal : 0;
        int min_win = config.exact_window ? config.max_per_window : 0;

        int totalToRemove = (SIZE * SIZE) - config.target_clues;
        int symmetricTargetCount = totalToRemove / 2; // Exact aadhe cells rotationally remove honge

        int attempts = 0;
        while (attempts < 50) {
            attempts++;
            for (int i = 0; i < SIZE * SIZE; i++) board[i] = original[i];

            int currentClues = SIZE * SIZE;
            int removedSoFar = 0;
            int stuckCount = 0;

            // Phase 1: Symmetric Removal - Pairs mein hatate hain
            while (removedSoFar < symmetricTargetCount && stuckCount < 5) {
                vector<int> candidates;
                // Sirf board ke upper half se candidates uthate hain
                for (int r = 0; r < SIZE / 2; r++) {
                    for (int c = 0; c < SIZE; c++) {
                        int pos = r * SIZE + c;
                        if (board[pos] != 0) candidates.push_back(pos);
                    }
                }
                // Agar board odd size ka hai(eg. 9x9), toh middle row ka half track karte hain
                if (SIZE % 2 != 0) {
                    int r = SIZE / 2;
                    for (int c = 0; c < SIZE / 2; c++) {
                        int pos = r * SIZE + c;
                        if (board[pos] != 0) candidates.push_back(pos);
                    }
                }

                int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0}, gridCounts[NUM_GRIDS] = {0}, altGridCounts[NUM_GRIDS] = {0};
                for (int r = 0; r < SIZE; r++) {
                    for (int c = 0; c < SIZE; c++) {
                        int pos = r * SIZE + c;
                        if (board[pos] != 0) {
                            rowCounts[r]++;
                            colCounts[c]++;
                            gridCounts[gridLookup[pos]]++;
                            if (altGridMap) altGridCounts[altGridMap[pos]]++;
                        }
                    }
                }
                
                shuffle(candidates.begin(), candidates.end(), rng);
                stable_sort(candidates.begin(), candidates.end(), [&](int pA, int pB) {
                    auto getScore = [&](int pos) {
                        int r = pos / SIZE, c = pos % SIZE;
                        int s = rowCounts[r] + colCounts[c] + gridCounts[gridLookup[pos]];
                        if (altGridMap) s += altGridCounts[altGridMap[pos]];
                        return s;
                    };
                    
                    int scoreA = getScore(pA);
                    int r2A = SIZE - 1 - (pA / SIZE);
                    int c2A = SIZE - 1 - (pA % SIZE);
                    scoreA += getScore(r2A * SIZE + c2A);

                    int scoreB = getScore(pB);
                    int r2B = SIZE - 1 - (pB / SIZE);
                    int c2B = SIZE - 1 - (pB % SIZE);
                    scoreB += getScore(r2B * SIZE + c2B);

                    return scoreA > scoreB;
                });
                bool removed = false;
                for (int pos1 : candidates) {
                    int r1 = pos1 / SIZE;
                    int c1 = pos1 % SIZE;
                    // Mirror image coordinate find karo
                    int r2 = SIZE - 1 - r1;
                    int c2 = SIZE - 1 - c1;
                    if (r1 == r2 && c1 == c2) continue; // Center pivot cell skip kardo
                    
                    int pos2 = r2 * SIZE + c2;
                    int backup1 = board[pos1], backup2 = board[pos2];
                    board[pos1] = board[pos2] = 0;
                    
                    // Uniqueness check: Dono hata ke dekho
                    if (checker.count(board) == 1) {
                        removedSoFar += 2; currentClues -= 2; removed = true; break;
                    } else {
                        board[pos1] = backup1; board[pos2] = backup2; // Wapis chipkao
                    }
                }
                
                if (!removed) {
                    // Agar aur jodiyan nahi mil rahi symmetry ki, tab perturb/undo karo
                    stuckCount++;
                    vector<int> emptyCells;
                    for (int pos = 0; pos < SIZE * SIZE; pos++) {
                        if (board[pos] == 0 && original[pos] != 0) emptyCells.push_back(pos);
                    }
                    if (emptyCells.size() < 2) break;
                    shuffle(emptyCells.begin(), emptyCells.end(), rng);
                    int putBack = min(3, (int)emptyCells.size());
                    for (int i = 0; i < putBack; i++) {
                        board[emptyCells[i]] = original[emptyCells[i]];
                        currentClues++;
                        removedSoFar--; // Backup counter decrement kar rahe hain kyunki values insert hui hain
                    }
                }
            }

            // Phase 2: Random Removal - Baki bache numbers normal randomize hoke hateinge
            stuckCount = 0;
            while (currentClues > config.target_clues && stuckCount < 5) {
                int rowCounts[SIZE] = {0}, colCounts[SIZE] = {0}, gridCounts[NUM_GRIDS] = {0}, altGridCounts[NUM_GRIDS] = {0};
                vector<int> candidates;
                for (int pos = 0; pos < SIZE * SIZE; pos++) {
                    if (board[pos] != 0) {
                        candidates.push_back(pos);
                        int r = pos / SIZE, c = pos % SIZE;
                        rowCounts[r]++;
                        colCounts[c]++;
                        gridCounts[gridLookup[pos]]++;
                        if (altGridMap) altGridCounts[altGridMap[pos]]++;
                    }
                }
                shuffle(candidates.begin(), candidates.end(), rng);
                stable_sort(candidates.begin(), candidates.end(), [&](int pA, int pB) {
                    int rA = pA / SIZE, cA = pA % SIZE;
                    int scoreA = rowCounts[rA] + colCounts[cA] + gridCounts[gridLookup[pA]];
                    if (altGridMap) scoreA += altGridCounts[altGridMap[pA]];
                    
                    int rB = pB / SIZE, cB = pB % SIZE;
                    int scoreB = rowCounts[rB] + colCounts[cB] + gridCounts[gridLookup[pB]];
                    if (altGridMap) scoreB += altGridCounts[altGridMap[pB]];
                    
                    return scoreA > scoreB;
                });
                bool removed = false;
                for (int pos : candidates) {
                    int backup = board[pos];
                    board[pos] = 0;
                    if (checker.count(board) == 1) {
                        currentClues--; removed = true; break;
                    } else {
                        board[pos] = backup;
                    }
                }
                if (!removed) {
                    stuckCount++;
                    vector<int> emptyCells;
                    for (int pos = 0; pos < SIZE * SIZE; pos++) {
                        if (board[pos] == 0 && original[pos] != 0) emptyCells.push_back(pos);
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
