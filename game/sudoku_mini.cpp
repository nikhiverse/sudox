#include "../clues/random_puzzle.cpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

using namespace std;

const int SIZE = 6;
const int SUB_R = 2; // Rows per block (ek 2x3 grid hogi)
const int SUB_C = 3; // Columns per block

// Sudoku6x6: Mini sudoku (6x6). Isme numbers 1-6 hain aur 2x3 block structure hai.
class Sudoku6x6 {
private:
  int board[SIZE * SIZE];

  // Bitmasks for rows, columns, and 2x3 blocks array setup
  uint8_t rowMask[SIZE];
  uint8_t colMask[SIZE];
  uint8_t blockMask[SIZE]; // 6 total blocks

  mt19937 rng;

  void reset() {
    for (int i = 0; i < SIZE; i++) {
        rowMask[i] = colMask[i] = blockMask[i] = 0;
    }
    for (int i = 0; i < SIZE * SIZE; i++) {
        board[i] = 0;
    }
  }

  // Box index calculate karne ka mathematical logic
  int getBlockIdx(int r, int c) {
    // blockRow (0-2) * 2 + blockCol (0-1) formula se 6 blocks specific index return karega
    return (r / SUB_R) * (SIZE / SUB_C) + (c / SUB_C);
  }

  // ALGORITHM: Isme bina MRV ke simple order me (from top left to bottom right) solve kiya jaana ka pattern hai.
  // 6x6 itna small hota hai ki MRV heuristic ki itni sakht zaroorat nahi parti.
  bool solve(int r, int c) {
    // Agar hum end board ki rows complete kar chuke hain, then puzzle is solved
    if (r == SIZE)
      return true;

    // Next cell kahan hai calculate karo
    // Agar line complete hui (c == 5) to row change karo!
    int nextR = (c == SIZE - 1) ? r + 1 : r;
    int nextC = (c == SIZE - 1) ? 0 : c + 1;

    // Try numbers 1 se 6 random order me daalne ki koshish kari jaye
    vector<int> nums = {1, 2, 3, 4, 5, 6};
    shuffle(nums.begin(), nums.end(), rng);

    int pos = r * SIZE + c;
    int bIdx = getBlockIdx(r, c);

    for (int n : nums) {
      uint8_t bit = 1 << (n - 1);

      // Constraints verification via AND masking (Check if bit is NOT set)
      if (!(rowMask[r] & bit) && !(colMask[c] & bit) &&
          !(blockMask[bIdx] & bit)) {

        // Valid conditions pe place number 
        board[pos] = n;
        rowMask[r] |= bit;
        colMask[c] |= bit;
        blockMask[bIdx] |= bit;

        if (solve(nextR, nextC))
          return true;

        // Backtrack: Galat nikla toh clean state par revert karo
        board[pos] = 0;
        rowMask[r] &= ~bit;
        colMask[c] &= ~bit;
        blockMask[bIdx] &= ~bit;
      }
    }
    return false;
  }

public:
  Sudoku6x6() : rng(random_device{}()) {}

  void generate() {
    PuzzleGenerator<SIZE, SUB_R, SUB_C> clueGen;
    bool success = false;
    while (!success) {
      reset();
      solve(0, 0); // 0,0 location se suru honge simply
      success = clueGen.generate(board, SUDOKU_MINI_CONFIG);
    }
  }

  void printJSON() {
    cout << "{\"type\":\"standard\",\"size\":6,\"subRows\":2,\"subCols\":3,"
            "\"grid\":[";
    for (int r = 0; r < SIZE; r++) {
      if (r)
        cout << ",";
      cout << "[";
      for (int c = 0; c < SIZE; c++) {
        if (c)
          cout << ",";
        cout << board[r * SIZE + c];
      }
      cout << "]";
    }
    cout << "]}" << endl;
  }
};

int main() {
  Sudoku6x6 game;
  game.generate();
  game.printJSON();
  return 0;
}