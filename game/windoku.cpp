#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include "../clues/random_puzzle.cpp"

using namespace std;

const int SIZE = 9;

// Windoku Variant: Yeh 9x9 sudoku ke alawa 4 extra internal 3x3 blocks (windows) bhi valid rakhti hai
// Jo ki standard box logic pe overlap karte hue constraints badhate hain.
class Windoku {
private:
  int board[SIZE * SIZE];

  // Bitmasks for fast validation constraints (Memory flags)
  uint16_t rowMask[SIZE], colMask[SIZE], boxMask[SIZE], winMask[4];

  // Window offsets: Top-Left cell coordinates of each internal 3x3 windoku window mapping
  const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};

  mt19937 rng;
  long long solveCounter;

  void reset() {
    solveCounter = 0;
    for (int i = 0; i < SIZE; i++) {
        rowMask[i] = colMask[i] = boxMask[i] = 0;
        if (i < 4) winMask[i] = 0; // Windoku ki sirf 4 hi windows hoti hain
    }
    for (int i = 0; i < SIZE * SIZE; i++) {
        board[i] = 0;
    }
  }

  // Helper function: Identify mapping cell on a window (-1 if nahi fall karta, warna id index 0-3 ret karta hai)
  int getWinIdx(int r, int c) {
    for (int i = 0; i < 4; i++) {
      if (r >= windows[i].first && r < windows[i].first + 3 &&
          c >= windows[i].second && c < windows[i].second + 3) // Window range check
        return i;
    }
    return -1;
  }

  // ALGORITHM: Calculate allowed numbers limit constraint bitwise map setup
  uint16_t getPossible(int r, int c) {
    // Basic rules OR filter combination check
    uint16_t used = rowMask[r] | colMask[c] | boxMask[(r / 3) * 3 + (c / 3)];
    int w = getWinIdx(r, c); // Optional windoku array mapping fetch index call check 
    if (w != -1) // Agar point overlap element me range boundary constraint fulfill kar rha he toh rule override mapping set logic validation
      used |= winMask[w];
    return (~used) & 0x1FF; // Mask bits validation extraction lowest limits map mapping index list (1-9 bits flag checks only inverted value bitwise complement memory)
  }

  // ALGORITHM: MRV (Minimum Remaining Values) Solver algorithm heuristics loop limits validation algorithm mapping structure bounds
  bool solve() {
    if (++solveCounter > 10000)
      return false; // Fail-safe limit array index limit parameter algorithm bound failure revert backtrack protection loop parameter validation setup mechanism restart boundary break timeout execution loop limits index array variable check mapping

    int bestR = -1, bestC = -1, minChoices = 10;
    uint16_t bestMask = 0;

    for (int r = 0; r < SIZE; r++) {
      for (int c = 0; c < SIZE; c++) {
        int pos = r * SIZE + c;
        if (board[pos] == 0) {
          uint16_t mask = getPossible(r, c);
          int count = 0;
          for (int i = 0; i < 9; i++)
            if (mask & (1 << i))
              count++;

          if (count == 0)
            return false; // Point limit failed map array parameter bounds check (Dead end layout)
          // Tight mapping points limits selection process setup criteria ranking map criteria parameters checks limits level parameter
          if (count < minChoices) {
            minChoices = count;
            bestR = r;
            bestC = c;
            bestMask = mask;
          }
          if (minChoices == 1) // First map optimization early short-circuit stop rule break out array map parameter bounds array checks bypass validation step criteria parameters step mechanism flag logic execution rules algorithm logic loop skip sequence
            goto found;
        }
      }
    }

  found:
    if (bestR == -1) // All items limit map points criteria checks array boundaries index pointers level array execution success parameters return step algorithm mapped logic array
      return true; 

    vector<int> nums;
    for (int i = 0; i < 9; i++)
      if (bestMask & (1 << i))
        nums.push_back(i + 1); // Extract numbers tracking array variables random selection setup limits vector limits value mapping points assignment layout pattern memory pointer
    shuffle(nums.begin(), nums.end(), rng);

    int pos = bestR * SIZE + bestC;
    int bIdx = (bestR / 3) * 3 + (bestC / 3);
    int wIdx = getWinIdx(bestR, bestC);

    for (int n : nums) {
      uint16_t bit = 1 << (n - 1);
      board[pos] = n;
      // Constraints assign criteria variables map layout memory logic
      rowMask[bestR] |= bit;
      colMask[bestC] |= bit;
      boxMask[bIdx] |= bit;
      if (wIdx != -1)
        winMask[wIdx] |= bit; // Extra map logic variable check

      if (solve())
        return true; 

      // Backtracking validation sequence limit step pointer state restore logic mapped array 
      if (wIdx != -1)
        winMask[wIdx] &= ~bit;
      boxMask[bIdx] &= ~bit;
      colMask[bestC] &= ~bit;
      rowMask[bestR] &= ~bit;
      board[pos] = 0; // Empty limit mapped criteria pointers index level state logic variable parameter list validation checks validation loop setup parameter pointer loop algorithm
    }
    return false;
  }

public:
  Windoku() : rng(random_device{}()) {}

  void generate() {
    PuzzleGenerator<SIZE, 3, 3> clueGen;
    bool success = false;
    while (!success) {
      reset(); // Pattern reset variable checks algorithm
      solve(); // Map execution algorithm criteria level loops sequence limit bounds validation setup step array parameter
      
      // Sudoku Windoku configuration ke through layout remove pass setup map execution sequence step criteria checks arrays loop
      success = clueGen.generate(board, WINDOKU_CONFIG); 
    }
  }

  void printJSON() {
    cout << "{\"type\":\"standard\",\"size\":9,\"subRows\":3,\"subCols\":3,\"windows\":[[1,1],[1,5],[5,1],[5,5]],\"grid\":[";
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
  Windoku game;
  game.generate();
  game.printJSON();
  return 0;
}