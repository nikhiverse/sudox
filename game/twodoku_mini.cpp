#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>
#include "../clues/random_puzzle.cpp"
#include "../clues/clues_config.hpp"

using namespace std;

const int BOARD_SIZE = 10;
struct Point { int r, c; };

// Random Clue Puzzle Generator Twodoku Mini Version (Overlapped 6x6 constraints 2x3 and 3x2 inner grids mapped logic variables mapping)
class TwinSudoku6x6 {
private:
  int board[BOARD_SIZE * BOARD_SIZE];
  vector<Point> activeCells;
  mt19937 rng;
  int solCount = 0;

  void initLayout() {
    activeCells.clear();
    for (int r = 0; r < BOARD_SIZE; r++) {
      for (int c = 0; c < BOARD_SIZE; c++) {
        board[r * BOARD_SIZE + c] = 0;
        if ((r < 6 && c < 6) || (r >= 4 && c >= 4)) {
          activeCells.push_back({r, c});
        }
      }
    }
  }

  bool isValid(int r, int c, int num) {
    if (r < 6 && c < 6) {
      for (int i = 0; i < 6; i++)
        if (board[r * BOARD_SIZE + i] == num || board[i * BOARD_SIZE + c] == num)
          return false;
      int startR = (r / 2) * 2, startC = (c / 3) * 3;
      for (int i = 0; i < 2; i++)
        for (int j = 0; j < 3; j++)
          if (board[(startR + i) * BOARD_SIZE + (startC + j)] == num)
            return false;
    }
    if (r >= 4 && c >= 4) {
      for (int i = 4; i < 10; i++)
        if (board[r * BOARD_SIZE + i] == num || board[i * BOARD_SIZE + c] == num)
          return false;
      int localR = r - 4, localC = c - 4;
      int startR = (localR / 3) * 3 + 4, startC = (localC / 2) * 2 + 4; // grid2 uses 3x2 logic sequence mappings pointers states arrays bounds arrays mapping Array evaluation
      for (int i = 0; i < 3; i++)
        for (int j = 0; j < 2; j++)
          if (board[(startR + i) * BOARD_SIZE + (startC + j)] == num)
            return false;
    }
    return true;
  }

  bool solve(int index) {
    if (index == (int)activeCells.size())
      return true;
    int r = activeCells[index].r, c = activeCells[index].c;
    int pos = r * BOARD_SIZE + c;
    vector<int> nums = {1, 2, 3, 4, 5, 6};
    shuffle(nums.begin(), nums.end(), rng);
    for (int num : nums) {
      if (isValid(r, c, num)) {
        board[pos] = num;
        if (solve(index + 1))
          return true;
        board[pos] = 0;
      }
    }
    return false;
  }

  // MRV Checking function for uniqueness
  void countMRV() {
    if (solCount > 1) return;
    int bestPos = -1, minChoices = 10;
    uint16_t bestMask = 0;
    
    for (auto& p : activeCells) {
       int pos = p.r * BOARD_SIZE + p.c;
       if (board[pos] == 0) {
           uint16_t used = 0;
           // 1. Grid 1 check
           if (p.r < 6 && p.c < 6) {
               for (int i = 0; i < 6; i++) {
                 if (board[p.r * BOARD_SIZE + i]) used |= (1 << (board[p.r * BOARD_SIZE + i] - 1));
                 if (board[i * BOARD_SIZE + p.c]) used |= (1 << (board[i * BOARD_SIZE + p.c] - 1));
               }
               int sr = (p.r/2)*2, sc = (p.c/3)*3;
               for (int i=0; i<2; i++) for(int j=0; j<3; j++)
                 if (board[(sr+i)*BOARD_SIZE + (sc+j)]) used |= (1 << (board[(sr+i)*BOARD_SIZE + (sc+j)] - 1));
           }
           // 2. Grid 2 check
           if (p.r >= 4 && p.c >= 4) {
               for (int i = 4; i < 10; i++) {
                 if (board[p.r * BOARD_SIZE + i]) used |= (1 << (board[p.r * BOARD_SIZE + i] - 1));
                 if (board[i * BOARD_SIZE + p.c]) used |= (1 << (board[i * BOARD_SIZE + p.c] - 1));
               }
               int lr = p.r - 4, lc = p.c - 4;
               int sr = (lr/3)*3 + 4, sc = (lc/2)*2 + 4;
               for (int i=0; i<3; i++) for(int j=0; j<2; j++)
                 if (board[(sr+i)*BOARD_SIZE + (sc+j)]) used |= (1 << (board[(sr+i)*BOARD_SIZE + (sc+j)] - 1));
           }
           
           uint16_t mask = (~used) & 0x3F; // 1 to 6
           int choices = __builtin_popcount(mask);
           if (choices == 0) return;
           if (choices < minChoices) { minChoices = choices; bestPos = pos; bestMask = mask; }
           if (minChoices == 1) goto search;
       }
    }
    search:
    if (bestPos == -1) { solCount++; return; }
    
    for (uint16_t m = bestMask; m; ) {
        int val = __builtin_ctz(m);
        m &= m - 1;
        board[bestPos] = val + 1;
        countMRV();
        board[bestPos] = 0;
        if (solCount > 1) return;
    }
  }

  int countSolutions(int* stateBoard) {
      solCount = 0;
      countMRV();
      return solCount;
  }

public:
  TwinSudoku6x6() : rng(random_device{}()) {}

  void extractGrid(int* g, int oR, int oC) {
      for (int r = 0; r < 6; r++) for (int c = 0; c < 6; c++)
          g[r * 6 + c] = board[(r + oR) * BOARD_SIZE + (c + oC)];
  }

  void generate() {
    bool generated = false;
    while (!generated) {
      initLayout();
      if (solve(0)) {
        int g1[36], g2[36];
        extractGrid(g1, 0, 0);
        extractGrid(g2, 4, 4);

        PuzzleConfig baseMini = SUDOKU_MINI_CONFIG;
        baseMini.target_clues = 16;

        PuzzleGenerator<6, 2, 3> gen1;
        if (!gen1.generate(g1, baseMini)) continue;

        PuzzleGenerator<6, 3, 2> gen2;
        if (!gen2.generate(g2, baseMini)) continue;

        for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++) board[i] = 0;
        for (int r = 0; r < 6; r++) for (int c = 0; c < 6; c++) board[r * BOARD_SIZE + c] = g1[r * 6 + c];
        for (int r = 0; r < 6; r++) for (int c = 0; c < 6; c++) board[(r + 4) * BOARD_SIZE + (c + 4)] = g2[r * 6 + c];

        int currentClues = 0;
        vector<int> candidates;
        for (auto& p : activeCells) {
            int pos = p.r * BOARD_SIZE + p.c;
            if (board[pos] != 0) {
               currentClues++;
               candidates.push_back(pos);
            }
        }

        if (countSolutions(board) != 1) continue;

        int targetClues = TWODOKU_MINI_CONFIG.target_clues;
        if (currentClues < targetClues) continue;

        int stuckCount = 0;
        while (currentClues > targetClues && stuckCount < 5) {
            shuffle(candidates.begin(), candidates.end(), rng);
            bool removedAny = false;
            
            for (int pos : candidates) {
                if (board[pos] != 0) {
                    int backup = board[pos];
                    board[pos] = 0;
                    if (countSolutions(board) == 1) {
                        currentClues--;
                        removedAny = true;
                        break;
                    } else board[pos] = backup;
                }
            }
            if (!removedAny) stuckCount++;
        }
        
        if (currentClues == targetClues && countSolutions(board) == 1) {
            generated = true;
        }
      }
    }
  }

  void printJSON() {
    cout << "{\"type\":\"twodoku\",\"totalRows\":10,\"totalCols\":10,";
    cout << "\"grids\":[{\"r\":0,\"c\":0,\"size\":6,\"subR\":2,\"subC\":3},{\"r\":4,\"c\":4,\"size\":6,\"subR\":3,\"subC\":2}],";
    cout << "\"grid\":[";
    for (int r = 0; r < BOARD_SIZE; r++) {
      if (r) cout << ",";
      cout << "[";
      for (int c = 0; c < BOARD_SIZE; c++) {
        if (c) cout << ",";
        cout << board[r * BOARD_SIZE + c];
      }
      cout << "]";
    }
    cout << "],\"active\":[";
    for (int r = 0; r < BOARD_SIZE; r++) {
      if (r) cout << ",";
      cout << "[";
      for (int c = 0; c < BOARD_SIZE; c++) {
        if (c) cout << ",";
        bool active = (r < 6 && c < 6) || (r >= 4 && c >= 4);
        cout << (active ? "true" : "false");
      }
      cout << "]";
    }
    cout << "],\"overlap\":[";
    for (int r = 0; r < BOARD_SIZE; r++) {
      if (r) cout << ",";
      cout << "[";
      for (int c = 0; c < BOARD_SIZE; c++) {
        if (c) cout << ",";
        bool overlap = (r >= 4 && r < 6 && c >= 4 && c < 6);
        cout << (overlap ? "true" : "false");
      }
      cout << "]";
    }
    cout << "]}" << endl;
  }
};

int main() {
  TwinSudoku6x6 puzzle;
  puzzle.generate();
  puzzle.printJSON();
  return 0;
}