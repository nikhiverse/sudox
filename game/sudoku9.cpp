#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include "../clues/symmetric_puzzle.cpp"

using namespace std;

const int SIZE = 9;

// Sudoku9x9: Ye standard 9x9 backend generator hai.
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
                int pos = r * SIZE + c; // 1D array indexing
                if (board[pos] == 0) {
                    uint16_t mask = getPossible(r, c);
                    
                    // Optimized: Count fast set bits using CPU instruction
                    int choices = __builtin_popcount(mask);
                    
                    // Dead end mil gaya! Yahan koi number nahi aa sakta. Return false hoke backtrack ho jayega.
                    if (choices == 0) return false; 
                    
                    if (choices < minChoices) {
                        minChoices = choices;
                        bestR = r; bestC = c;
                        bestMask = mask;
                    }
                    // Hidden Single optimization: Agar bas 1 hi choice bachi hai toh direct placement karo, loop mat chalao
                    if (minChoices == 1) goto found; 
                }
            }
        }
        
    found:
        // Agar bestR update nahi hua, toh equation solved! (Pure 81 cells bhar chuke hain)
        if (bestR == -1) return true; 

        // Valid options nikalkar randomize karte hain taaki har baar valid par random puzzle bane
        vector<int> nums;
        for (int i = 0; i < 9; i++) if (bestMask & (1 << i)) nums.push_back(i + 1);
        shuffle(nums.begin(), nums.end(), rng);

        int pos = bestR * SIZE + bestC;
        int bIdx = (bestR / 3) * 3 + (bestC / 3); // 3x3 block index nikalo
        
        for (int n : nums) {
            uint16_t bit = 1 << (n - 1); // 1-based digit ko bit shift convert karo
            
            // Placement step
            board[pos] = n;
            rowMask[bestR] |= bit; colMask[bestC] |= bit; boxMask[bIdx] |= bit;

            // Recursion level deep jayega 
            if (solve()) return true;

            // Backtrack: Agar solution valid nahi laata toh UNDO karo
            rowMask[bestR] &= ~bit; colMask[bestC] &= ~bit; boxMask[bIdx] &= ~bit;
            board[pos] = 0;
        }
        return false;
    }

public:
    Sudoku9x9() : rng(random_device{}()) {}

    // generate(): Yeh continuously solve -> eliminate -> check loop tab tak chalata hai
    // jab tak target clues na mil jayein.
    // PRODUCTION UPDATE: Added a maximum attempt limit to prevent infinite server hangs.
    bool generate() {
        SymmetricPuzzleGenerator<SIZE, 3, 3> clueGen;
        bool success = false;
        int attempts = 0;
        const int MAX_ATTEMPTS = 1000;

        while (!success && attempts < MAX_ATTEMPTS) {
            attempts++;
            reset(); // Har attempt per full reset
            solve(); // Ek naya random completely solved board banao
            // Clue generator use karke numbers gayab karo rules ke sath (180 deg symmetric type)
            success = clueGen.generate(board, SUDOKU_9_CONFIG); 
        }
        
        return success; // Returns false if it hits 1000 attempts without a valid board
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
    // Check if generation was successful before printing
    if (game.generate()) {
        game.printJSON();
    } else {
        cerr << "{\"error\": \"Generation timed out. Adjust config or check constraints.\"}" << endl;
        return 1;
    }
    return 0;
}