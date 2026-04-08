#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include <string>
#include "../clues/random_puzzle.cpp"

using namespace std;

const int SIZE = 12;
const int SUB_R = 3;
const int SUB_C = 4;

// Sudoku12: Yeah 12x12 badi grid ka sudoku hai, jisme 3x4 sub-blocks allow hote hain.
class Sudoku12 {
private:
    int board[SIZE * SIZE];
    
    // Bitmasks array: Fast uniqueness constraint checking ke liye mask use karte hain (O(1) time complexity)
    uint16_t rowMask[SIZE], colMask[SIZE], boxMask[SIZE];
    
    mt19937 rng;
    long long solveCounter; // Timeout mechanism setup

    void reset() {
        solveCounter = 0;
        for (int i = 0; i < SIZE; i++) {
            rowMask[i] = colMask[i] = boxMask[i] = 0;
        }
        for (int i = 0; i < SIZE * SIZE; i++) {
            board[i] = 0;
        }
    }

    // ALGORITHM: Ek diye gaye cell ke liye valid allowed numbers bitmask (0-11) calculate karta hai
    uint16_t getPossible(int r, int c) {
        // Block index math -> (r / 3) * 4 + (c / 4)
        uint16_t used = rowMask[r] | colMask[c] | boxMask[(r / SUB_R) * (SIZE / SUB_C) + (c / SUB_C)];
        // 0xFFF mask ka mtlb hota hai first 12 binary bits (i.e. number 1 se 12 valid limit set ho jaye)
        return (~used) & 0xFFF;
    }

    bool solve() {
        // Sudoku 12x12 me backtacking tree boht fail sakta hai, isliye 50,000 deep attempts ke baad
        // sidha false output dena behtar rehta hai time limits preserve krne ke lie
        if (++solveCounter > 50000) return false; 

        int bestR = -1, bestC = -1, minChoices = 14; 
        uint16_t bestMask = 0;

        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (board[pos] == 0) {
                    uint16_t mask = getPossible(r, c);
                    int choices = 0;
                    for (int i = 0; i < 12; i++) if (mask & (1 << i)) choices++;
                    
                    if (choices == 0) return false; // Dead end!
                    if (choices < minChoices) {
                        minChoices = choices; bestR = r; bestC = c; bestMask = mask;
                    }
                    if (minChoices == 1) goto found;
                }
            }
        }
        
    found:
        if (bestR == -1) return true;

        vector<int> nums;
        for (int i = 0; i < 12; i++) if (bestMask & (1 << i)) nums.push_back(i + 1);
        shuffle(nums.begin(), nums.end(), rng);

        int pos = bestR * SIZE + bestC;
        int bIdx = (bestR / SUB_R) * (SIZE / SUB_C) + (bestC / SUB_C);
        
        for (int n : nums) {
            uint16_t bit = 1 << (n - 1);
            board[pos] = n;
            rowMask[bestR] |= bit; colMask[bestC] |= bit; boxMask[bIdx] |= bit;

            if (solve()) return true;

            // Backtrack: False mile to mask set aur number values ko wapas utaro 
            rowMask[bestR] &= ~bit; colMask[bestC] &= ~bit; boxMask[bIdx] &= ~bit;
            board[pos] = 0;
        }
        return false;
    }

public:
    Sudoku12() : rng(random_device{}()) {}

    void generate() {
        PuzzleGenerator<SIZE, SUB_R, SUB_C> clueGen;
        bool success = false;
        while (!success) {
            reset();
            // Agar solve ho raha hai 50k timeout ke andar, tabhi final check me pass hoga
            if (solve()) {
                success = clueGen.generate(board, SUDOKU_12_CONFIG);
            }
        }
    }

    void printJSON() {
        cout << "{\"type\":\"standard\",\"size\":12,\"subRows\":3,\"subCols\":4,\"grid\":[";
        for (int r = 0; r < SIZE; r++) {
            if (r) cout << ",";
            cout << "[";
            for (int c = 0; c < SIZE; c++) {
                if (c) cout << ",";
                cout << board[r * SIZE + c];
            }
            cout << "]";
        }
        cout << "]}" << endl;
    }
};

int main() {
    Sudoku12 game;
    game.generate();
    game.printJSON();
    return 0;
}