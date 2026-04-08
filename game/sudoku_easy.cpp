#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include "../clues/random_puzzle.cpp"

using namespace std;

const int SIZE = 9;

// Sudoku9x9 Easy Variant: Yeh normal sudoku generator hai par SUDOKU_EASY_CONFIG parameters 
// follow karta hai jisse puzzle beginners ke liye aasan ho.
class Sudoku9x9 {
private:
    int board[SIZE * SIZE];
    
    // Bitmasks array: Fast uniqueness constraint checking ke liye mask use karte hain (O(1) time complexity)
    uint16_t rowMask[SIZE], colMask[SIZE], boxMask[SIZE];
    
    mt19937 rng;

    void reset() {
        for (int i = 0; i < SIZE; i++) {
            rowMask[i] = colMask[i] = boxMask[i] = 0;
        }
        for (int i = 0; i < SIZE * SIZE; i++) {
            board[i] = 0;
        }
    }

    // ALGORITHM: Ek diye gaye cell ke liye valid allowed numbers bitmask calculate karta hai
    uint16_t getPossible(int r, int c) {
        // Row, Col aur Box ke bitmasks ko OR bitwise operation se combine karke existing set numbers decode karna
        uint16_t used = rowMask[r] | colMask[c] | boxMask[(r / 3) * 3 + (c / 3)];
        // Mask inversion (~): Used mask ko invert karke valid choices nikalte hain.
        // 0x1FF ka matlab hai sirf lowest 9 bits (1 se 9 numbers) consider karo.
        return (~used) & 0x1FF;
    }

    // ALGORITHM: MRV (Minimum Remaining Values)
    // MRV heuristic: Hum us cell ko find karenge jisme sabse kam valid choices bachi hain.
    // Taa ki humari speed zyada rahe aur faltu mein backtracking ghalat level pe na jaaye.
    bool solve() {
        int bestR = -1, bestC = -1, minChoices = 10;
        uint16_t bestMask = 0;

        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (board[pos] == 0) {
                    uint16_t mask = getPossible(r, c);
                    
                    int choices = 0;
                    for (int i = 0; i < 9; i++) if (mask & (1 << i)) choices++;
                    
                    // Dead end mil gaya! Yahan koi valid allowed number fit hi nahi ho sakta. Backtrack!
                    if (choices == 0) return false; 
                    
                    if (choices < minChoices) {
                        minChoices = choices;
                        bestR = r; bestC = c;
                        bestMask = mask;
                    }
                    // Hidden Single optimization: Agar specific cell mein sirif 1 option hai toh turant fill kardo
                    if (minChoices == 1) goto found; 
                }
            }
        }
        
    found:
        // Agar bestR update nahi hua, iska matlab koi empty cell nahi mila (solved!)
        if (bestR == -1) return true; 

        vector<int> nums;
        for (int i = 0; i < 9; i++) if (bestMask & (1 << i)) nums.push_back(i + 1);
        shuffle(nums.begin(), nums.end(), rng);

        int pos = bestR * SIZE + bestC;
        int bIdx = (bestR / 3) * 3 + (bestC / 3);
        
        for (int n : nums) {
            uint16_t bit = 1 << (n - 1);
            
            // Placement memory update kar rahe hain
            board[pos] = n;
            rowMask[bestR] |= bit; colMask[bestC] |= bit; boxMask[bIdx] |= bit;

            // Agla raasta traverse karenge
            if (solve()) return true;

            // Backtrack: Restore memory to original state and continue loop finding
            rowMask[bestR] &= ~bit; colMask[bestC] &= ~bit; boxMask[bIdx] &= ~bit;
            board[pos] = 0;
        }
        return false; // Agar kisi bhi n pe answer na mile
    }

public:
    Sudoku9x9() : rng(random_device{}()) {}

    void generate() {
        PuzzleGenerator<SIZE, 3, 3> clueGen;
        bool success = false;
        while (!success) {
            reset();
            solve();
            success = clueGen.generate(board, SUDOKU_EASY_CONFIG);
        }
    }

    void printJSON() {
        cout << "{\"type\":\"standard\",\"size\":9,\"subRows\":3,\"subCols\":3,\"grid\":[";
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
    Sudoku9x9 game;
    game.generate();
    game.printJSON();
    return 0;
}