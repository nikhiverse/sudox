#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include <string>
#include "../clues/random_puzzle.cpp"

using namespace std;

const int SIZE = 9;

struct Point { int r, c; };

// JigsawX Variant: Extreme complexity variant jo randomize blocks/shapes bhi rakhta hai
// Aur Sudoku X diagonals logic dono constraints combine karta hai ek grid mein.
class JigsawX {
private:
    int groupGrid[SIZE * SIZE];
    int numberGrid[SIZE * SIZE];
    uint16_t rowMask[SIZE], colMask[SIZE], groupMask[SIZE];
    uint16_t diag1Mask, diag2Mask; // \ and / primary secondary masks setup mapping rules
    mt19937 rng;
    long long solveAttempts;

    void reset() {
        diag1Mask = diag2Mask = 0;
        solveAttempts = 0;
        for (int i = 0; i < SIZE; i++) {
            rowMask[i] = colMask[i] = groupMask[i] = 0;
        }
        for (int pos = 0; pos < SIZE * SIZE; pos++) {
            groupGrid[pos] = -1; // -1 value representation islye free structure index checking
            numberGrid[pos] = 0;
        }
    }

    // ALGORITHM: Connected shapes builder
    // Growth loop check boundary fill
    bool growGroup(vector<Point>& currentCells, int groupId) {
        if (currentCells.size() == SIZE) return true; // Poori array list complete track full target
        int dr[] = {-1, 1, 0, 0}, dc[] = {0, 0, -1, 1}; // Vector map direction array limits (directions)
        vector<Point> candidates;
        for (auto& p : currentCells) {
            for (int i = 0; i < 4; i++) {
                int nr = p.r + dr[i], nc = p.c + dc[i];
                // Andar allowed check 
                if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE && groupGrid[nr * SIZE + nc] == -1) {
                    bool exists = false;
                    for(auto& cand : candidates) if(cand.r == nr && cand.c == nc) { exists = true; break; }
                    if(!exists) candidates.push_back({nr, nc}); // Queue enqueue memory pattern fill
                }
            }
        }
        if (candidates.empty()) return false; 
        shuffle(candidates.begin(), candidates.end(), rng);
        for (auto& c : candidates) {
            groupGrid[c.r * SIZE + c.c] = groupId;
            currentCells.push_back(c);
            if (growGroup(currentCells, groupId)) return true; // Next growth iteration path recursive calling limits logic step up map target level
            
            // Revert changes on fail
            currentCells.pop_back();
            groupGrid[c.r * SIZE + c.c] = -1;
        }
        return false;
    }

    bool createShapes(int groupId) {
        if (groupId == SIZE) return true;
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (groupGrid[pos] == -1) {
                    groupGrid[pos] = groupId; // Start seed assign root ID
                    vector<Point> curr = {{r, c}};
                    if (growGroup(curr, groupId)) {
                        if (createShapes(groupId + 1)) return true; // Success logic chain nested loops forward
                    }
                    
                    // Undo logic backward limits logic map validation revert failure parameters cleanup
                    groupGrid[pos] = -1;
                    return false; 
                }
            }
        }
        return true;
    }

    // Calculate mask allowed bits numbers
    uint16_t getAllowed(int r, int c) {
        // GroupMask directly maps over index lookup generated array location
        uint16_t used = rowMask[r] | colMask[c] | groupMask[groupGrid[r * SIZE + c]];
        if (r == c) used |= diag1Mask; // Diagonal constraint bit set parameter
        if (r + c == SIZE - 1) used |= diag2Mask;
        return (~used) & 0x1FF;
    }

    // Number solver with heuristics
    bool solve() {
        if (++solveAttempts > 5000) return false;  // Yaha 5k deep limits isliye, kyunki complexity dono variant combine ho chuki he.
        int bestR = -1, bestC = -1;
        int minChoices = 100;
        uint16_t bestAllowed = 0;

        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (numberGrid[pos] == 0) {
                    uint16_t allowed = getAllowed(r, c);
                    int choices = 0;
                    for(int i=0; i<9; i++) if(allowed & (1 << i)) choices++;
                    if (choices == 0) return false;
                    
                    // Priority heuristic: Diagonals ko intentionally zero weighted score pass ho ki jaldi solve hon MRV queue list me!
                    int weight = (r == c || r + c == SIZE - 1) ? 0 : 1;
                    int effectiveChoices = choices * 2 + weight;

                    if (effectiveChoices < minChoices) {
                        minChoices = effectiveChoices;
                        bestR = r; bestC = c;
                        bestAllowed = allowed;
                    }
                    if (choices == 1) goto found; // Hidden Single 1 bit shortcut trick pattern finding jump limit validation loop bypass array 
                }
            }
        }
    found:
        if (bestR == -1) return true;

        vector<int> vals;
        for (int i = 0; i < 9; i++) if (bestAllowed & (1 << i)) vals.push_back(i + 1);
        shuffle(vals.begin(), vals.end(), rng);

        int pos = bestR * SIZE + bestC;

        for (int v : vals) {
            int bit = 1 << (v - 1); // Left shift validation set limit bit parameters assignment tracking
            numberGrid[pos] = v;
            
            rowMask[bestR] |= bit; colMask[bestC] |= bit; groupMask[groupGrid[pos]] |= bit;
            if (bestR == bestC) diag1Mask |= bit;
            if (bestR + bestC == SIZE - 1) diag2Mask |= bit;

            if (solve()) return true; // Next empty grid slot pe apply attempt tracking validation map recursive function calls stack pointer limit

            // Memory clear validation state undo mapping parameter constraints revert algorithm logic step parameter array function cleanup bounds matrix check pointer reference mask update cleanup tracking reverse order parameters loop step map logic
            if (bestR == bestC) diag1Mask &= ~bit;
            if (bestR + bestC == SIZE - 1) diag2Mask &= ~bit;
            groupMask[groupGrid[pos]] &= ~bit; colMask[bestC] &= ~bit; rowMask[bestR] &= ~bit;
            numberGrid[pos] = 0;
        }
        return false;
    }

    bool isB(int r, int c, int dr, int dc) {
        int nr = r + dr, nc = c + dc; // Check mapping boundary map coordinates layout mapping array reference pointers structure index array
        if (nr < 0 || nr >= SIZE || nc < 0 || nc >= SIZE) return true;
        return groupGrid[r * SIZE + c] != groupGrid[nr * SIZE + nc];
    }

public:
    JigsawX() : rng(random_device{}()) {}

    void generate() {
        PuzzleGenerator<SIZE, 3, 3> clueGen;
        clueGen.gridMap = groupGrid;

        bool success = false;
        while (!success) {
            reset();
            // Jigsaw shapes generation level step limits check success constraints parameter checks setup boundaries limits mapping execution level
            if (createShapes(0)) {
                if (solve()) {
                    // Random clue removing algorithm generator pattern execution setup output mapping logic
                    success = clueGen.generate(numberGrid, JIGSAW_X_CONFIG);
                }
            }
        }
    }

    void printJSON() {
        cout << "{\"type\":\"jigsaw\",\"size\":9,\"diagonals\":true,\"grid\":[";
        for (int r = 0; r < SIZE; r++) {
            if (r) cout << ",";
            cout << "[";
            for (int c = 0; c < SIZE; c++) {
                if (c) cout << ",";
                cout << numberGrid[r * SIZE + c];
            }
            cout << "]";
        }
        cout << "],\"groups\":[";
        for (int r = 0; r < SIZE; r++) {
            if (r) cout << ",";
            cout << "[";
            for (int c = 0; c < SIZE; c++) {
                if (c) cout << ",";
                cout << groupGrid[r * SIZE + c];
            }
            cout << "]";
        }
        cout << "]}" << endl;
    }
};

int main() {
    JigsawX puzzle;
    puzzle.generate();
    puzzle.printJSON();
    return 0;
}