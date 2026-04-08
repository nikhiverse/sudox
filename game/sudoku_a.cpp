#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include "../clues/random_puzzle.cpp"

using namespace std;

const int SIZE = 9;

// Sudoku9x9 A-variant: Ye standard random 9x9 layout generator hai
// Sudoku A "Asymmetric" ke liye bhi jana jata hai jisme random puzzles generate hote hain bina symmetry rule ke.
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
        // Row, Col aur Box ke bitmasks ko OR bitwise operation se combine karke existing numbers check karte hain
        uint16_t used = rowMask[r] | colMask[c] | boxMask[(r / 3) * 3 + (c / 3)];
        // Mask inversion (~): Used mask ko invert karke valid choices nikalte hain.
        // 0x1FF ka matlab hai sirf lowest 9 bits (1 se 9 numbers) consider karo.
        return (~used) & 0x1FF;
    }

    // ALGORITHM: MRV (Minimum Remaining Values)
    // MRV heuristic: Hum us cell ko find karenge jisme sabse kam valid choices bachi hain.
    // Isse backtracking tree narrow ho jata hai aur speed badhti hai.
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
                    
                    // Dead end mil gaya! Yahan koi number nahi aa sakta. Return false hoke backtrack ho jayega.
                    if (choices == 0) return false; 
                    
                    if (choices < minChoices) {
                        minChoices = choices;
                        bestR = r; bestC = c;
                        bestMask = mask;
                    }
                    // Hidden Single optimization: Agar bas 1 hi choice bachi hai toh direct placement karo, loop break!!
                    if (minChoices == 1) goto found; 
                }
            }
        }
        
    found:
        // Agar bestR update nahi hua, toh equation solved!
        if (bestR == -1) return true; 

        // Valid options nikalkar randomize karte hain recursion branches spread karne ke liye
        vector<int> nums;
        for (int i = 0; i < 9; i++) if (bestMask & (1 << i)) nums.push_back(i + 1);
        shuffle(nums.begin(), nums.end(), rng);

        int pos = bestR * SIZE + bestC;
        int bIdx = (bestR / 3) * 3 + (bestC / 3);
        
        for (int n : nums) {
            uint16_t bit = 1 << (n - 1);
            
            // Placement step
            board[pos] = n;
            rowMask[bestR] |= bit; colMask[bestC] |= bit; boxMask[bIdx] |= bit;

            // Recursively baqi grid solve karo
            if (solve()) return true;

            // Backtrack: Agar agla raasta useless nikla toh Undo kardo
            rowMask[bestR] &= ~bit; colMask[bestC] &= ~bit; boxMask[bIdx] &= ~bit;
            board[pos] = 0;
        }
        return false;
    }

public:
    Sudoku9x9() : rng(random_device{}()) {}

    // generate(): Yeh continuously full grid fill aur clues randomly hatane ka kaam trigger karta hai
    void generate() {
        PuzzleGenerator<SIZE, 3, 3> clueGen;
        bool success = false;
        while (!success) {
            reset();
            solve();
            // RamdomPuzzleGenerator object (clueGen) ko call karte hain random layout remove pattern create karne
            success = clueGen.generate(board, SUDOKU_A_CONFIG);
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
