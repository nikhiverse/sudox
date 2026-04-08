#include "../clues/symmetric_puzzle.cpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace std;

const int SIZE = 8;

struct Block {
  int r, c, w, h;
};

// TilingSudoku / Jigsaw8: 8x8 matrix par custom varying rectangles overlap karte hain
// Generally block sizes (2x4) aur (4x2) ko combine mix krke lagaya jata hai
class TilingSudoku {
private:
  int gridMap[SIZE * SIZE];
  int numberMap[SIZE * SIZE];
  vector<Block> blocks;

  // Bitmasks for fast verification checking setup
  uint16_t rowMask[SIZE], colMask[SIZE], blockMask[SIZE];

  mt19937 rng;
  long long solveCounter;

  void reset() {
    blocks.clear();
    solveCounter = 0; // infinite loops bachane ke liye counter zaroori hai
    for (int i = 0; i < SIZE; i++) {
        rowMask[i] = colMask[i] = blockMask[i] = 0;
    }
    for (int pos = 0; pos < SIZE * SIZE; pos++) {
        gridMap[pos] = -1; // -1 khali jagah dikha raha hai
        numberMap[pos] = 0;
    }
  }

  // --- STEP 1: Generate Geometry (Tiling Phase) ---
  // Yeah loop try karta hai grid par rectangles fit karne ki completely tile hone tak
  bool solveGeometry(int hRem, int vRem) {
    int r = -1, c = -1;
    for (int i = 0; i < SIZE; i++) {
      for (int j = 0; j < SIZE; j++) {
        if (gridMap[i * SIZE + j] == -1) {
          r = i; // Pehla khula location (empty cell) identify kara
          c = j;
          break;
        }
      }
      if (r != -1)
        break;
    }

    // Saari jagah bhar gayi toh return True
    if (r == -1)
      return true;

    // Type of blocks: h (horizontal flat) ya v (vertical long) blocks
    struct Choice {
      char type;
      int w, h; // Width aur Height
    };
    vector<Choice> choices;
    // Condition of quotas left checks limit constraint count
    if (hRem > 0)
      choices.push_back({'h', 4, 2});
    if (vRem > 0)
      choices.push_back({'v', 2, 4});
    shuffle(choices.begin(), choices.end(), rng);

    // Iterating randomly choices over remaining slots
    for (auto &ch : choices) {
      if (r + ch.h <= SIZE && c + ch.w <= SIZE) {
        bool fits = true;
        // Collision check algorithm
        for (int i = r; i < r + ch.h; i++) {
          for (int j = c; j < c + ch.w; j++) {
            // Agar pehle se occupy ho toh overlap nahi kar sakte
            if (gridMap[i * SIZE + j] != -1) {
              fits = false;
              break;
            }
          }
        }

        if (fits) {
          int blockId = blocks.size(); // Naya block id generate hua list number ke relative
          for (int i = r; i < r + ch.h; i++) {
            for (int j = c; j < c + ch.w; j++)
              gridMap[i * SIZE + j] = blockId; // Region pe painting ID marking
          }
          blocks.push_back({r, c, ch.w, ch.h});

          // Check for continuous nested iteration success recursion
          if (solveGeometry(ch.type == 'h' ? hRem - 1 : hRem,
                            ch.type == 'v' ? vRem - 1 : vRem))
            return true;

          // Backtrack step for collision fallback map unpaint
          blocks.pop_back();
          for (int i = r; i < r + ch.h; i++) {
            for (int j = c; j < c + ch.w; j++)
              gridMap[i * SIZE + j] = -1;
          }
        }
      }
    }
    return false;
  }

  // --- STEP 2: MRV Solver (Number Placement Phase) ---
  uint16_t getPossible(int r, int c) {
    // blockMask index dynamically gridMap array se pass out nikal rha hai naaki calculation se (like r/3)
    uint16_t used = rowMask[r] | colMask[c] | blockMask[gridMap[r * SIZE + c]];
    return (~used) & 0xFF; // Only bits for allowed numbers 1-8
  }

  bool solveNumbers() {
    if (++solveCounter > 10000)
      return false; // Fail-safe for tight restricted shapes aur unsolvable geometries

    int bR = -1, bC = -1, minChoices = 10;
    uint16_t bMask = 0;

    for (int r = 0; r < SIZE; r++) {
      for (int c = 0; c < SIZE; c++) {
        int pos = r * SIZE + c;
        if (numberMap[pos] == 0) { // Cell abhi khali hai
          uint16_t mask = getPossible(r, c);
          int count = 0;
          for (int i = 0; i < 8; i++)
            if (mask & (1 << i))
              count++;
          if (count == 0)
            return false; // dead path
          if (count < minChoices) {
            minChoices = count;
            bR = r; // Row store update
            bC = c; // Column store update
            bMask = mask;
          }
        }
      }
    }

    if (bR == -1) // Sub complete!
      return true;

    vector<int> nums;
    for (int i = 0; i < 8; i++)
      if (bMask & (1 << i)) // Only options valid available 
        nums.push_back(i + 1);
    shuffle(nums.begin(), nums.end(), rng); // Shuffle for randomization effect

    int pos = bR * SIZE + bC;
    int bIdx = gridMap[pos]; // Region id lookup layout
    for (int n : nums) {
      uint16_t bit = 1 << (n - 1);
      
      numberMap[pos] = n; // value daaldo
      
      // Masks limits check set bits memory array logic update
      rowMask[bR] |= bit;
      colMask[bC] |= bit;
      blockMask[bIdx] |= bit;

      if (solveNumbers())
        return true;

      // Unassign for checking next configuration combinations
      rowMask[bR] &= ~bit;
      colMask[bC] &= ~bit;
      blockMask[bIdx] &= ~bit;
      numberMap[pos] = 0;
    }
    return false;
  }

  bool isB(int r, int c, int dr, int dc) {
    int nr = r + dr, nc = c + dc;
    if (nr < 0 || nr >= SIZE || nc < 0 || nc >= SIZE)
      return true; // Edge coordinates limits boundary detection true map function validation
    return gridMap[r * SIZE + c] != gridMap[nr * SIZE + nc];
  }

public:
  TilingSudoku() : rng(random_device{}()) {}

  // Main puzzle constructor trigger method
  void generate() {
    SymmetricPuzzleGenerator<SIZE, 2, 4> clueGen; // Symmetric generation variant lagaya hai
    bool success = false;
    while (!success) {
      reset();
      // Total 8 blocks allocate karte hain (4 horizontal + 4 vertical = 8 shapes x 8 cells each = 64 cells target completely packed)
      if (solveGeometry(4, 4)) {
        if (solveNumbers()) {
          clueGen.gridMap = gridMap;
          success = clueGen.generate(numberMap, JIGSAW_8_CONFIG);
        }
      }
    }
  }

  void printJSON() {
    cout << "{\"type\":\"jigsaw\",\"size\":8,\"grid\":[";
    for (int r = 0; r < SIZE; r++) {
      if (r)
        cout << ",";
      cout << "[";
      for (int c = 0; c < SIZE; c++) {
        if (c)
          cout << ",";
        cout << numberMap[r * SIZE + c];
      }
      cout << "]";
    }
    cout << "],\"groups\":[";
    for (int r = 0; r < SIZE; r++) {
      if (r)
        cout << ",";
      cout << "[";
      for (int c = 0; c < SIZE; c++) {
        if (c)
          cout << ",";
        cout << gridMap[r * SIZE + c];
      }
      cout << "]";
    }
    cout << "]}" << endl;
  }
};

int main() {
  TilingSudoku game;
  game.generate();
  game.printJSON();
  return 0;
}