#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include "../clues/random_puzzle.cpp"

using namespace std;

const int SIZE = 12;

// DoubleConstraintSudoku / Dozaku Variant
// Ye 12x12 variant pe run karta hai, jiske elements 3x4 block size mein aate hai AND DOOSRA 4x3 block size overlap parallel matrix limit rules array loop map logic rules Array map tracking limit Bounds checks pointers Matrix execution limits layout pointer Mapping bounds pointer
class DoubleConstraintSudoku {
private:
    int board[SIZE * SIZE]; 
    int altGridMap[SIZE * SIZE]; 

    // Yha rules 4 matrices me checks limits boundaries Map indices Evaluation Array mapping Execution pointer maps Limit matrix sequence Map step Sequence layout variables
    uint16_t rowM[SIZE], colM[SIZE], b34M[12], b43M[12];
    mt19937 rng;
    long long steps;

    void reset() {
        steps = 0;
        for (int i = 0; i < SIZE; i++) {
            rowM[i] = colM[i] = 0;
            if (i < 12) b34M[i] = b43M[i] = 0; 
        }
        for (int i = 0; i < SIZE * SIZE; i++) {
            board[i] = 0; 
        }
    }

    // Mathematical coordinate block pointer map evaluation layout Map bounds matrices arrays step pointer array structure Arrays mapping logic sequence
    inline int get34(int r, int c) { return (r / 3) * 3 + (c / 4); } // primary grid check
    inline int get43(int r, int c) { return (r / 4) * 4 + (c / 3); } // 4x3 alt grid check

    uint16_t getPoss(int r, int c) {
        // Validation parameters limits structure indices array variables loop checks variables arrays indices Map tracking algorithm boundaries
        uint16_t u = rowM[r] | colM[c] | b34M[get34(r, c)] | b43M[get43(r, c)];
        return (~u) & 0xFFF; // Limits validation limits pointer Limit Step algorithm pointers
    }

    bool solve() {
        if (++steps > 20000) return false; 

        int bR = -1, bC = -1, minP = 13;
        uint16_t bM = 0;

        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (board[pos] == 0) {
                    uint16_t m = getPoss(r, c);
                    int count = 0;
                    for (int i = 0; i < 12; i++) if (m & (1 << i)) count++;
                    if (count == 0) return false;
                    
                    if (count < minP) { minP = count; bR = r; bC = c; bM = m; }
                    if (minP == 1) goto found;
                }
            }
        }
        found:
        if (bR == -1) return true;

        vector<int> cands;
        for (int i = 0; i < 12; i++) if (bM & (1 << i)) cands.push_back(i + 1);
        shuffle(cands.begin(), cands.end(), rng);

        int pos = bR * SIZE + bC;
        int i34 = get34(bR, bC), i43 = get43(bR, bC);
        for (int v : cands) {
            uint16_t bit = (1 << (v - 1));
            board[pos] = v;
            
            // Constraint arrays pointers mapping execution array step Validation state Matrix pointers
            rowM[bR] |= bit; colM[bC] |= bit; b34M[i34] |= bit; b43M[i43] |= bit;
            
            if (solve()) return true;

            b34M[i34] &= ~bit; b43M[i43] &= ~bit; colM[bC] &= ~bit; rowM[bR] &= ~bit;
            board[pos] = 0; 
        }
        return false;
    }

public:
    DoubleConstraintSudoku() : rng(random_device{}()) {
        // Init altGridMap for Uniqueness Checker
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                altGridMap[pos] = get43(r, c);
            }
        }
    }

    void generate() {
        // Apply Mixed Puzzle Generator for Dozaku Configs (72 clues, 7 max row/col, grid limit 12 setup)
        PuzzleGenerator<12, 3, 4> clueGen;
        clueGen.altGridMap = altGridMap; // Dozaku requires the UniquenessChecker to test the 4x3 alternate blocks too!

        bool success = false;
        while (!success) {
            reset();
            if (solve()) {
                success = clueGen.generate(board, DOZAKU_CONFIG);
            }
        }
    }

    void printJSON() {
        cout << "{\"type\":\"standard\",\"size\":12,\"subRows\":4,\"subCols\":3,\"altSubRows\":3,\"altSubCols\":4,\"grid\":[";
        for (int r = 0; r < SIZE; r++) {
            if (r) cout << ",";
            cout << "[";
            for (int c = 0; c < SIZE; c++) {
                if (c) cout << ",";
                int pos = r * SIZE + c;
                cout << board[pos];
            }
            cout << "]";
        }
        cout << "]}" << endl;
    }
};

int main() {
    DoubleConstraintSudoku game;
    game.generate();
    game.printJSON();
    return 0;
}