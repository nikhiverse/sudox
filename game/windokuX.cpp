#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include "../clues/random_puzzle.cpp"

using namespace std;

const int SIZE = 9;

// Sudoku X + Windoku = Extreme variant
// Is variant me 9x9 block, 4 windoku overlapped boxes, AND 2 cross diagonals saare simultaneously apply hote hain limit maps me.
class SudokuXWindoku {
private:
    int board[SIZE * SIZE];
    
    // Bitmasks memory validation structures parameters array (Fast O(1) checking constraint limits matrix checks rules bounds parameter mapping values index lookup)
    uint16_t rowMask[SIZE], colMask[SIZE], boxMask[SIZE];
    uint16_t diag1Mask, diag2Mask; // (\) AND (/) mapping indices pointers logic mapping algorithm setup variables structure tracking map pointers variable array step layout array checks array pattern array reference flags limits
    uint16_t winMask[4];           // 4 Windoku limit parameter check window limits setup pointers algorithm matrix array pointers indexing structure mapping bounds mapping lookup arrays limits limits loop flags layout
    mt19937 rng;

    void reset() {
        diag1Mask = diag2Mask = 0;
        for (int i = 0; i < SIZE; i++) {
            rowMask[i] = colMask[i] = boxMask[i] = 0;
            if (i < 4) winMask[i] = 0;
        }
        for (int i = 0; i < SIZE * SIZE; i++) {
            board[i] = 0; // Empty fill logic parameters map step bounds bounds matrix mapping limits loop mapping layout pointers structure logic limit steps loop execution parameters check array logic parameter validation checks sequence array mapping logic parameter logic
        }
    }

    // Helper method constraints indices extraction logic validation layout logic pattern check bounds checks parameter lookup logic variable structure mapping variables step limit pointers validation boundaries
    int getWinId(int r, int c) {
        if (r >= 1 && r <= 3) {
            if (c >= 1 && c <= 3) return 0;
            if (c >= 5 && c <= 7) return 1;
        } else if (r >= 5 && r <= 7) {
            if (c >= 1 && c <= 3) return 2;
            if (c >= 5 && c <= 7) return 3;
        }
        return -1; // If limits bounds check fails parameter limits pointer boundaries parameter sequence pointers array validation state matrix bounds array execution checks sequence map matrix check sequence
    }

    // Extraction map memory limit extraction mapping loop logic array pointers bounds step limit validation state boundaries
    uint16_t getPossible(int r, int c) {
        // Combination set validation pointers matrix check matrix map limit flags loop array checks checks sequence step execution state limits layout mapping algorithm checks matrices execution validation logic structure parameter arrays map constraint steps pointers rules parameters boundaries step loop checks limit bounds variable bounds check loop array structure sequence Array structure limits map
        uint16_t used = rowMask[r] | colMask[c] | boxMask[(r / 3) * 3 + (c / 3)];
        
        // Custom variant matrix algorithm mapping logic execution algorithm maps parameter execution sequence mapped boundaries flags states parameter step loop arrays map check loop logic check validation pointers check pointers limit check
        if (r == c) used |= diag1Mask; 
        if (r + c == SIZE - 1) used |= diag2Mask; 
        
        int w = getWinId(r, c);
        if (w != -1) used |= winMask[w]; // Optional logic rule sequence evaluation map array layout sequence pointers map boundaries limit step state limit checks constraint step logic
        
        return (~used) & 0x1FF; // Execution limits inverted mask output limit parameter bit indices arrays algorithm points layout list 
    }

    // Evaluation process loops parameters checks loop checks boundaries mapped tracking arrays map limits limit validation logic state structure parameter sequence state bounds index array layout step loop arrays flags algorithm layout array checking pointers pointers sequence map logic pointers algorithm limits sequence limits matrix check map boundaries loop layout index array validation matrix steps Map
    bool solve() {
        int bestR = -1, bestC = -1, minChoices = 10;
        uint16_t bestPossible = 0;

        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (board[pos] == 0) {
                    uint16_t possible = getPossible(r, c);
                    int count = 0;
                    for (int i = 0; i < 9; i++) if (possible & (1 << i)) count++;
                    
                    if (count == 0) return false; // Fail check pattern checks index map limit validation matrix step state map rules index pointers limit setup pointers array array execution pointers sequence algorithm pointer mappings structure pointers limit state parameters loop evaluation bounds checks logic array checks evaluation rules sequence flags array limit
                    if (count < minChoices) {
                        minChoices = count;
                        bestR = r; bestC = c;
                        bestPossible = possible;
                    }
                    if (minChoices == 1) goto found; // Faster bypass algorithm index bounds arrays loop states checks array execution array pointers map validation logic pointer map sequence mapping tracking loops pointers logic flags parameter limit sequence indices logic Map checks layout rules Array structure tracking execution Matrix pointer limits array algorithm boundaries parameter step mapping indices bounds mapping evaluation mapping structure
                }
            }
        }
        found:
        if (bestR == -1) return true; // Checked

        vector<int> nums;
        for (int i = 0; i < 9; i++) if (bestPossible & (1 << i)) nums.push_back(i + 1);
        shuffle(nums.begin(), nums.end(), rng);

        int pos = bestR * SIZE + bestC;
        int bIdx = (bestR / 3) * 3 + (bestC / 3);
        int wIdx = getWinId(bestR, bestC);

        for (int n : nums) {
            uint16_t bit = 1 << (n - 1);
            board[pos] = n; // Commit parameters structure layout memory setup map Array execution Array mapping points
            
            // Multiple mask setup flag rules mapping index limit validation tracking check evaluation execution tracking pointers states pointers sequence parameter rules mapping map map boundaries mapping variables index steps pointer sequence limits states index bounds execution mapping matrix flags State limits layout bounds structure Map logic mapping Map pointers loop checks array arrays layout Map step Sequence parameter logic structure pointers arrays parameters matrix Limit mapping loop
            rowMask[bestR] |= bit; colMask[bestC] |= bit; boxMask[bIdx] |= bit;
            if (bestR == bestC) diag1Mask |= bit;
            if (bestR + bestC == SIZE - 1) diag2Mask |= bit;
            if (wIdx != -1) winMask[wIdx] |= bit;

            if (solve()) return true;

            // Rollback map rules evaluation checks bounds structure points limits layout pointer algorithms variables flags boundary matrix array Validation points mappings execution rules limits limits tracking loop states flags points loop checks states structure bounds pointer parameter algorithm logic loops limits layout index logic index pointers sequence pointers boundaries bounds step validation step step variables loops array limits check
            if (wIdx != -1) winMask[wIdx] &= ~bit;
            if (bestR == bestC) diag1Mask &= ~bit;
            if (bestR + bestC == SIZE - 1) diag2Mask &= ~bit;
            boxMask[bIdx] &= ~bit; colMask[bestC] &= ~bit; rowMask[bestR] &= ~bit;
            board[pos] = 0; // Empty limit layout map Array validation Limit mapping array loop bounds
        }
        return false;
    }

public:
    SudokuXWindoku() : rng(random_device{}()) {}

    void generate() {
        PuzzleGenerator<SIZE, 3, 3> clueGen;
        bool success = false;
        while (!success) {
            reset();
            solve();
            success = clueGen.generate(board, WINDOKU_X_CONFIG);
        }
    }

    void printJSON() {
        cout << "{\"type\":\"standard\",\"size\":9,\"subRows\":3,\"subCols\":3,\"diagonals\":true,\"windows\":[[1,1],[1,5],[5,1],[5,5]],\"grid\":[";
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
    SudokuXWindoku game;
    game.generate();
    game.printJSON();
    return 0;
}