#ifndef UNIQUENESS_CPP
#define UNIQUENESS_CPP
#include <utility>
#include <vector>
#include <cstdint>

// UniquenessChecker class: Yeh ensure karta hai ki puzzle ka sirf aur sirf 1 hi valid solution ho.
// Multiple solutions allow nahi kiye jaate hain.
using namespace std;
template <int SIZE, int SUB_R, int SUB_C>
class UniquenessChecker {
private:
    static constexpr int NUM_GRIDS = (SIZE / SUB_R) * (SIZE / SUB_C);
    static constexpr int CELLS_PER_GRID = SUB_R * SUB_C;

    int board[SIZE * SIZE];
    int gridLookup[SIZE * SIZE];
    int gridCell[NUM_GRIDS][CELLS_PER_GRID];
    
    // Bitmasks array: Fast uniqueness constraint checking ke liye mask use karte hain (O(1) time complexity)
    uint16_t rowMask[SIZE], colMask[SIZE], gridMasks[NUM_GRIDS], altGridMasks[NUM_GRIDS];
    uint16_t diag1Mask, diag2Mask, winMasks[4];
    int solutionCount;
    int evalCount;

public:
    // Game variants modifiers
    bool check_diagonals = false;
    bool check_windows = false;
    int* gridMap = nullptr;
    int* altGridMap = nullptr;
    bool* activeMap = nullptr;

private:

    // solveMRV (Minimum Remaining Values): Yeh function un cells ko pick karta hai
    // jiske paas sabse kam options bache hain. Uske baad backtracking recursively us par try karta hai.
    void solveMRV() {
        if (++evalCount > 250000) return; // Prevent exponential time in 12x12 unique check
        // Agar pehle hi 1 se zyada solution mil gaye toh aage solve karne ka fayda nahi
        if (solutionCount > 1) return;

        int bestPos = -1, minChoices = SIZE + 1;
        uint16_t bestMask = 0;
        uint16_t cellMask[SIZE * SIZE] = {};

        // MRV loop: Sabhi empty cells check karo taaki sabse kam choice wala cell mile
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                // 1D Array indexing: r (row) ko SIZE se multiply karke column add kar dete hain.
                int pos = r * SIZE + c;
                if (activeMap && !activeMap[pos]) continue;
                if (board[pos] == 0) {
                    
                    // Bitwise OR karke already used numbers ka ek combined mask banate hain
                    uint16_t used = rowMask[r] | colMask[c] | gridMasks[gridLookup[pos]];
                    if (altGridMap) used |= altGridMasks[altGridMap[pos]];

                    if (check_diagonals) {
                        if (r == c) used |= diag1Mask;
                        if (r + c == SIZE - 1) used |= diag2Mask;
                    }
                    if (check_windows) {
                        const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                        for (int w = 0; w < 4; w++) {
                            if (r >= windows[w].first && r < windows[w].first + 3 &&
                                c >= windows[w].second && c < windows[w].second + 3) used |= winMasks[w];
                        }
                    }

                    // Mask inversion (~): Used mask ko invert karke valid (allowed) choices nikal rahe hain
                    uint16_t mask = (~used) & ((1 << SIZE) - 1);
                    cellMask[pos] = mask;
                    
                    // builtin_popcount CPU instruction use karta hai direct set bits calculate karne ke liye (fast)
                    int choices = __builtin_popcount(mask);

                    // Agar koi choice nahi bachi, toh iska matlab dead end aa gaya hai
                    if (choices == 0) return;
                    
                    // Sabse kam choices wale cell ko best cell maan kar track karte hain
                    if (choices < minChoices) {
                        minChoices = choices; bestPos = pos; bestMask = mask;
                    }
                    // Optimization: Agar ek hi choice hai toh aage loop chalana faltu hai, break kar do!
                    if (minChoices == 1) goto found;
                }
            }
        }

        // Hidden Singles Optimization: 
        // Aisa number find karo jo kisi poori row/col/box mein sirf ek hi specific cell mein fit ho sakta hai
        {
            const uint16_t fullMask = (1 << SIZE) - 1;
            // Rows check karte hain
            for (int r = 0; r < SIZE; r++) {
                uint16_t unplaced = (~rowMask[r]) & fullMask;
                for (uint16_t vm = unplaced; vm; ) {
                    // builtin_ctz bitwise command lowest common bit ki counting jaldi karta hai
                    int val = __builtin_ctz(vm);
                    vm &= vm - 1;
                    uint16_t valBit = (1 << val);
                    int cnt = 0; int lastPos = -1;
                    for (int c = 0; c < SIZE; c++) {
                        int pos = r * SIZE + c;
                        if (cellMask[pos] & valBit) { cnt++; lastPos = pos; if (cnt > 1) break; }
                    }
                    // Agar counter 1 aaya iska matlab voh value exactly is ek lastPos location pe jayegi (hidden single)
                    if (cnt == 1) { bestPos = lastPos; bestMask = valBit; goto found; }
                }
            }
            // Columns check karte hain
            for (int c = 0; c < SIZE; c++) {
                uint16_t unplaced = (~colMask[c]) & fullMask;
                for (uint16_t vm = unplaced; vm; ) {
                    int val = __builtin_ctz(vm);
                    vm &= vm - 1;
                    uint16_t valBit = (1 << val);
                    int cnt = 0; int lastPos = -1;
                    for (int r = 0; r < SIZE; r++) {
                        int pos = r * SIZE + c;
                        if (cellMask[pos] & valBit) { cnt++; lastPos = pos; if (cnt > 1) break; }
                    }
                    if (cnt == 1) { bestPos = lastPos; bestMask = valBit; goto found; }
                }
            }
            // Grids/Boxes check karte hain
            for (int g = 0; g < NUM_GRIDS; g++) {
                uint16_t unplaced = (~gridMasks[g]) & fullMask;
                for (uint16_t vm = unplaced; vm; ) {
                    int val = __builtin_ctz(vm);
                    vm &= vm - 1;
                    uint16_t valBit = (1 << val);
                    int cnt = 0; int lastPos = -1;
                    for (int i = 0; i < CELLS_PER_GRID && cnt <= 1; i++) {
                        int pos = gridCell[g][i];
                        if (cellMask[pos] & valBit) { cnt++; lastPos = pos; }
                    }
                    if (cnt == 1) { bestPos = lastPos; bestMask = valBit; goto found; }
                }
            }
        }

    found:
        // Agar bestPos abhi bhi -1 hai, toh iska matlab saare cell bhar gaye aur humein ek valid solution mil gaya
        if (bestPos == -1) {
            solutionCount++;
            return;
        }

        // Available candidate constraints try karo
        for (uint16_t m = bestMask; m; ) {
            int n = __builtin_ctz(m);
            m &= m - 1;
            uint16_t bit = (1 << n);
            
            // 1D array value se row-column revert nikaalte hain
            int r = bestPos / SIZE;
            int c = bestPos % SIZE;
            
            // Board pe value place kar rahe hain
            board[bestPos] = n + 1;
            
            // Placement masks track kar rahe hain set bits update karke
            rowMask[r] |= bit; colMask[c] |= bit;
            int g = gridLookup[bestPos];
            int ag = altGridMap ? altGridMap[bestPos] : -1;
            gridMasks[g] |= bit;
            if (ag != -1) altGridMasks[ag] |= bit;
            if (check_diagonals) {
                if (r == c) diag1Mask |= bit;
                if (r + c == SIZE - 1) diag2Mask |= bit;
            }
            if (check_windows) {
                const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                for (int w = 0; w < 4; w++) {
                    if (r >= windows[w].first && r < windows[w].first + 3 &&
                        c >= windows[w].second && c < windows[w].second + 3) winMasks[w] |= bit;
                }
            }

            // Recursive backtrack call: next tree branch try karo
            solveMRV();

            // Recursion wapas aagayi, ab changes revert (UNDO) karo 
            // saare states ussi halat mein waapis chhod do previous node par.
            if (check_windows) {
                const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                for (int w = 0; w < 4; w++) {
                    if (r >= windows[w].first && r < windows[w].first + 3 &&
                        c >= windows[w].second && c < windows[w].second + 3) winMasks[w] &= ~bit;
                }
            }
            if (check_diagonals) {
                if (r == c) diag1Mask &= ~bit;
                if (r + c == SIZE - 1) diag2Mask &= ~bit;
            }
            if (ag != -1) altGridMasks[ag] &= ~bit;
            gridMasks[g] &= ~bit;
            colMask[c] &= ~bit; rowMask[r] &= ~bit;
            board[bestPos] = 0;
            if (solutionCount > 1) return;
        }
    }

public:
    // Board solution count ginraha hai 
    int count(const int* inputBoard) {
        diag1Mask = diag2Mask = 0;
        for(int i=0; i<4; i++) winMasks[i] = 0;

        // Pre-compute grid lookup table: taaki calculation repeat na karni padhe
        for(int r=0; r<SIZE; r++) {
            for(int c=0; c<SIZE; c++) {
                int pos = r * SIZE + c;
                gridLookup[pos] = gridMap ? gridMap[pos] : ((r / SUB_R) * (SIZE / SUB_C) + (c / SUB_C));
            }
        }

        // Hidden singles heuristic track karne ke liye group-based layout memory pre-fill kar rahe hain
        int gIdx[NUM_GRIDS] = {};
        for(int r=0; r<SIZE; r++) {
            for(int c=0; c<SIZE; c++) {
                int pos = r * SIZE + c;
                if (activeMap && !activeMap[pos]) continue;
                int g = gridLookup[pos];
                int idx = gIdx[g]++;
                gridCell[g][idx] = pos;
            }
        }

        for(int i=0; i<SIZE; i++) rowMask[i] = colMask[i] = 0;
        for(int i=0; i<NUM_GRIDS; i++) gridMasks[i] = altGridMasks[i] = 0;

        for(int i=0; i<SIZE; i++) {
            for(int j=0; j<SIZE; j++) {
                int pos = i * SIZE + j;
                board[pos] = inputBoard[pos];
                if (board[pos] != 0 && (!activeMap || activeMap[pos])) {
                    uint16_t bit = (1 << (board[pos] - 1));
                    rowMask[i] |= bit;
                    colMask[j] |= bit;
                    gridMasks[gridLookup[pos]] |= bit;
                    if (altGridMap) altGridMasks[altGridMap[pos]] |= bit;
                    if (check_diagonals) {
                        if (i == j) diag1Mask |= bit;
                        if (i + j == SIZE - 1) diag2Mask |= bit;
                    }
                    if (check_windows) {
                        const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
                        for (int w = 0; w < 4; w++) {
                            if (i >= windows[w].first && i < windows[w].first + 3 &&
                                j >= windows[w].second && j < windows[w].second + 3) winMasks[w] |= bit;
                        }
                    }
                }
            }
        }
        solutionCount = 0;
        evalCount = 0;
        solveMRV();
        return solutionCount;
    }
};

#endif
