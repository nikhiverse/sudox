#include "../clues/random_puzzle.cpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

using namespace std;

const int TS = 15;
const int GS = 9;
const int OO = 6;

class SamuraiSudoku {
private:
  int board[TS * TS];
  mt19937 rng;

  bool inG1(int r, int c) { return r >= 0 && r < 9 && c >= 0 && c < 9; }
  bool inG2(int r, int c) { return r >= 6 && r < 15 && c >= 6 && c < 15; }
  bool isActive(int r, int c) { return inG1(r, c) || inG2(r, c); }

  void initBoard() {
    for (int i = 0; i < TS * TS; i++)
      board[i] = 0;
  }

  // ===== Full-board solution generator =====
  bool isValidGen(int r, int c, int num) {
    if (inG1(r, c)) {
      for (int k = 0; k < 9; k++) {
        if (board[r * TS + k] == num || board[k * TS + c] == num)
          return false;
      }
      int sr = (r / 3) * 3, sc = (c / 3) * 3;
      for (int i = sr; i < sr + 3; i++)
        for (int j = sc; j < sc + 3; j++)
          if (board[i * TS + j] == num)
            return false;
    }
    if (inG2(r, c)) {
      for (int k = 0; k < 9; k++) {
        if (board[r * TS + (OO + k)] == num || board[(OO + k) * TS + c] == num)
          return false;
      }
      int lr = r - OO, lc = c - OO;
      int sr = OO + (lr / 3) * 3, sc = OO + (lc / 3) * 3;
      for (int i = sr; i < sr + 3; i++)
        for (int j = sc; j < sc + 3; j++)
          if (board[i * TS + j] == num)
            return false;
    }
    return true;
  }

  bool solveGen(int index) {
    if (index == TS * TS)
      return true;
    int r = index / TS, c = index % TS;
    if (!isActive(r, c))
      return solveGen(index + 1);
    vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    shuffle(nums.begin(), nums.end(), rng);
    for (int n : nums) {
      if (isValidGen(r, c, n)) {
        board[index] = n;
        if (solveGen(index + 1))
          return true;
        board[index] = 0;
      }
    }
    return false;
  }

  // ===== Full-board MRV uniqueness checker =====
  uint16_t g1RowM[9], g1ColM[9], g1BoxM[9];
  uint16_t g2RowM[9], g2ColM[9], g2BoxM[9];
  int fSolCount, fEvalCount;
  static const int EVAL_LIMIT = 100000;

  void initFullMasks() {
    for (int i = 0; i < 9; i++) {
      g1RowM[i] = g1ColM[i] = g1BoxM[i] = 0;
      g2RowM[i] = g2ColM[i] = g2BoxM[i] = 0;
    }
    for (int r = 0; r < 9; r++)
      for (int c = 0; c < 9; c++) {
        int v = board[r * TS + c];
        if (v) {
          uint16_t b = 1 << (v - 1);
          g1RowM[r] |= b;
          g1ColM[c] |= b;
          g1BoxM[(r / 3) * 3 + c / 3] |= b;
        }
      }
    for (int r = 0; r < 9; r++)
      for (int c = 0; c < 9; c++) {
        int v = board[(r + OO) * TS + (c + OO)];
        if (v) {
          uint16_t b = 1 << (v - 1);
          g2RowM[r] |= b;
          g2ColM[c] |= b;
          g2BoxM[(r / 3) * 3 + c / 3] |= b;
        }
      }
  }

  uint16_t fGetAvail(int r, int c) {
    uint16_t used = 0;
    if (inG1(r, c))
      used |= g1RowM[r] | g1ColM[c] | g1BoxM[(r / 3) * 3 + c / 3];
    if (inG2(r, c)) {
      int lr = r - OO, lc = c - OO;
      used |= g2RowM[lr] | g2ColM[lc] | g2BoxM[(lr / 3) * 3 + lc / 3];
    }
    return (~used) & 0x1FF;
  }

  void fPlace(int r, int c, uint16_t bit) {
    if (inG1(r, c)) {
      g1RowM[r] |= bit;
      g1ColM[c] |= bit;
      g1BoxM[(r / 3) * 3 + c / 3] |= bit;
    }
    if (inG2(r, c)) {
      int lr = r - OO, lc = c - OO;
      g2RowM[lr] |= bit;
      g2ColM[lc] |= bit;
      g2BoxM[(lr / 3) * 3 + lc / 3] |= bit;
    }
  }

  void fRemove(int r, int c, uint16_t bit) {
    if (inG1(r, c)) {
      g1RowM[r] &= ~bit;
      g1ColM[c] &= ~bit;
      g1BoxM[(r / 3) * 3 + c / 3] &= ~bit;
    }
    if (inG2(r, c)) {
      int lr = r - OO, lc = c - OO;
      g2RowM[lr] &= ~bit;
      g2ColM[lc] &= ~bit;
      g2BoxM[(lr / 3) * 3 + lc / 3] &= ~bit;
    }
  }

  void fSolveMRV() {
    if (fSolCount > 1)
      return;
    if (++fEvalCount > EVAL_LIMIT)
      return;
    int bR = -1, bC = -1, minCh = 10;
    uint16_t bM = 0;
    for (int r = 0; r < TS; r++)
      for (int c = 0; c < TS; c++) {
        if (!isActive(r, c) || board[r * TS + c] != 0)
          continue;
        uint16_t av = fGetAvail(r, c);
        int ch = __builtin_popcount(av);
        if (ch == 0)
          return;
        if (ch < minCh) {
          minCh = ch;
          bR = r;
          bC = c;
          bM = av;
        }
        if (minCh == 1)
          goto found;
      }
  found:
    if (bR == -1) {
      fSolCount++;
      return;
    }
    {
      int pos = bR * TS + bC;
      for (uint16_t m = bM; m;) {
        int n = __builtin_ctz(m);
        m &= m - 1;
        uint16_t bit = 1 << n;
        board[pos] = n + 1;
        fPlace(bR, bC, bit);
        fSolveMRV();
        fRemove(bR, bC, bit);
        board[pos] = 0;
        if (fSolCount > 1)
          return;
      }
    }
  }

  int countSolutions() {
    fSolCount = 0;
    fEvalCount = 0;
    initFullMasks();
    fSolveMRV();
    return fSolCount;
  }

  // ===== Grid helpers =====
  void extractGrid(int *g, int oR, int oC) {
    for (int r = 0; r < 9; r++)
      for (int c = 0; c < 9; c++)
        g[r * 9 + c] = board[(r + oR) * TS + (c + oC)];
  }

  bool checkGrid9x9(const int *g, int maxVal) {
    int rC[9] = {}, cC[9] = {}, bC[9] = {};
    for (int r = 0; r < 9; r++)
      for (int c = 0; c < 9; c++)
        if (g[r * 9 + c] != 0) {
          rC[r]++;
          cC[c]++;
          bC[(r / 3) * 3 + c / 3]++;
        }
    for (int i = 0; i < 9; i++)
      if (rC[i] > maxVal || cC[i] > maxVal || bC[i] > maxVal)
        return false;
    return true;
  }

  bool checkBothGrids(const PuzzleConfig &config) {
    int g1[81], g2[81];
    extractGrid(g1, 0, 0);
    extractGrid(g2, OO, OO);
    return checkGrid9x9(g1, config.max_per_row) &&
           checkGrid9x9(g2, config.max_per_row);
  }

public:
  SamuraiSudoku() : rng(random_device{}()), fEvalCount(0) { initBoard(); }

  bool generate() {
    const PuzzleConfig &config = TWODOKU_9_CONFIG;
    bool generated = false;

    while (!generated) {
      initBoard();
      if (!solveGen(0))
        continue;

      // Extract each 9x9 grid
      int g1[81], g2[81];
      extractGrid(g1, 0, 0);
      extractGrid(g2, OO, OO);

      // Per-grid target: 81*81/(2*81-9) ≈ 43, use 44 to bias total >= target
      PuzzleConfig gCfg = {44, 5, 5, 5, 9, false, false, 9, false, false};

      PuzzleGenerator<9, 3, 3> gen1;
      if (!gen1.generate(g1, gCfg))
        continue;

      PuzzleGenerator<9, 3, 3> gen2;
      if (!gen2.generate(g2, gCfg))
        continue;

      // Merge: G1 first, then G2 overwrites overlap
      for (int i = 0; i < TS * TS; i++)
        board[i] = 0;
      for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++)
          board[r * TS + c] = g1[r * 9 + c];
      for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++)
          board[(r + OO) * TS + (c + OO)] = g2[r * 9 + c];

      // Re-check G1 constraints (overlap cells changed by G2)
      int fg1[81];
      extractGrid(fg1, 0, 0);
      if (!checkGrid9x9(fg1, config.max_per_row))
        continue;

      // Count total clues
      int totalClues = 0;
      for (int r = 0; r < TS; r++)
        for (int c = 0; c < TS; c++)
          if (isActive(r, c) && board[r * TS + c] != 0)
            totalClues++;

      // If total < target, retry
      if (totalClues < config.target_clues)
        continue;

      // Remove extras to reach target
      if (totalClues > config.target_clues) {
        vector<int> extras;
        for (int r = 0; r < TS; r++)
          for (int c = 0; c < TS; c++)
            if (isActive(r, c) && board[r * TS + c] != 0)
              extras.push_back(r * TS + c);
        shuffle(extras.begin(), extras.end(), rng);

        for (int pos : extras) {
          if (totalClues <= config.target_clues)
            break;
          int bk = board[pos];
          board[pos] = 0;
          if (checkBothGrids(config) && countSolutions() == 1)
            totalClues--;
          else
            board[pos] = bk;
        }
      }

      if (totalClues == config.target_clues && countSolutions() == 1)
        generated = true;
    }

    return generated;
  }

  void printJSON() {
    cout << "{\"type\":\"twodoku\",\"totalRows\":15,\"totalCols\":15,";
    cout << "\"grids\":[{\"r\":0,\"c\":0,\"size\":9,\"subR\":3,\"subC\":3},{"
            "\"r\":6,\"c\":6,\"size\":9,\"subR\":3,\"subC\":3}],";
    cout << "\"grid\":[";
    for (int r = 0; r < TS; r++) {
      if (r)
        cout << ",";
      cout << "[";
      for (int c = 0; c < TS; c++) {
        if (c)
          cout << ",";
        cout << board[r * TS + c];
      }
      cout << "]";
    }
    cout << "],\"active\":[";
    for (int r = 0; r < TS; r++) {
      if (r)
        cout << ",";
      cout << "[";
      for (int c = 0; c < TS; c++) {
        if (c)
          cout << ",";
        cout << (inG1(r, c) || inG2(r, c) ? "true" : "false");
      }
      cout << "]";
    }
    cout << "],\"overlap\":[";
    for (int r = 0; r < TS; r++) {
      if (r)
        cout << ",";
      cout << "[";
      for (int c = 0; c < TS; c++) {
        if (c)
          cout << ",";
        cout << (inG1(r, c) && inG2(r, c) ? "true" : "false");
      }
      cout << "]";
    }
    cout << "]}" << endl;
  }
};

int main() {
  SamuraiSudoku game;
  if (game.generate()) {
    game.printJSON();
  } else {
    cerr << "{\"error\": \"Failed to generate puzzle.\"}" << endl;
    return 1;
  }
  return 0;
}