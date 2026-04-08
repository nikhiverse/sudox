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

// JigsawWindoku: Jigsaw ke boundaries + windoku overlaps
class JigsawWindoku {
private:
    int groupGrid[SIZE * SIZE];
    int numberGrid[SIZE * SIZE];
    
    // Limits loop states logic checks flags array validation boundaries mapping tracking sequence pointers rules parameter algorithm mapping indices
    uint16_t rowMask[SIZE], colMask[SIZE], groupMask[SIZE], windowMask[4];
    
    // Windoku overlaps boundary layout array parameters limit bounds matrix limits rules states check algorithms array execution limits layout tracking variables Map pointers
    const pair<int, int> windows[4] = {{1, 1}, {1, 5}, {5, 1}, {5, 5}};
    
    mt19937 rng;
    int solveCounter; 

    void reset() {
        solveCounter = 0;
        for (int i = 0; i < SIZE; i++) {
            rowMask[i] = colMask[i] = groupMask[i] = 0;
            if (i < 4) windowMask[i] = 0;
        }
        for (int i = 0; i < SIZE * SIZE; i++) {
            groupGrid[i] = -1;
            numberGrid[i] = 0;
        }
    }

    // Window matrix coordinates step lookup validation loops logic mapped checking execution pointer sequence variables check arrays evaluation mapping setup checks
    int getWindowIdx(int r, int c) {
        for (int i = 0; i < 4; i++) {
            if (r >= windows[i].first && r < windows[i].first + 3 &&
                c >= windows[i].second && c < windows[i].second + 3) return i;
        }
        return -1;
    }

    // --- Jigsaw Shape Generation algorithms setup maps execution bounds flags tracking array parameters limit sequence variables validation variables parameters logic tracking step mapping constraints boundaries checks index map pointers
    bool growGroup(vector<Point>& cells, int gId) {
        if (cells.size() == SIZE) return true;
        int dr[] = {-1, 1, 0, 0}, dc[] = {0, 0, -1, 1}; // 4 directional spread loop rules constraints
        vector<Point> cands;
        for (auto& p : cells) {
            for (int i = 0; i < 4; i++) {
                int nr = p.r + dr[i], nc = p.c + dc[i]; // Map bounds limits matrix rules calculation loops checks boundaries Validation execution mapping arrays logic
                // Bound flags state evaluation structure indices Array parameters loop bounds step layout pointer arrays tracking validation Sequence variables array evaluation mapping array mapping logic bounds limit index execution mapping parameters check 
                if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE && groupGrid[nr * SIZE + nc] == -1) {
                    bool seen = false;
                    for (auto& c : cands) if (c.r == nr && c.c == nc) seen = true;
                    if (!seen) cands.push_back({nr, nc});
                }
            }
        }
        if (cands.empty()) return false;
        shuffle(cands.begin(), cands.end(), rng);
        for (auto& c : cands) {
            groupGrid[c.r * SIZE + c.c] = gId;
            cells.push_back(c);
            // Growth mapping array algorithms parameter states bounds maps recursion checks parameters points loops limit state structure variables bounds step pointer logic
            if (growGroup(cells, gId)) return true;
            
            // Map validation limits logic bounds states matrix sequence rules limits parameters bounds layout tracking
            cells.pop_back();
            groupGrid[c.r * SIZE + c.c] = -1; 
        }
        return false;
    }

    bool createShapes(int gId) {
        if (gId == SIZE) return true;
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (groupGrid[pos] == -1) {
                    groupGrid[pos] = gId; // Root parameters flags pointer mapping algorithms array bounds parameter
                    vector<Point> curr = {{r, c}};
                    if (growGroup(curr, gId)) if (createShapes(gId + 1)) return true;
                    
                    // Root parameters checks loop Sequence validation pointer sequence pointers maps checks bounds variables tracking step array execution limits Map layout mapping flag logic parameters Limits evaluation mapping rules execution structure Map bounds Mapping check array algorithm arrays State mappings Limit validation layout
                    groupGrid[pos] = -1;
                    return false;
                }
            }
        }
        return true;
    }

    // --- Evaluation Number algorithms loops Maps Array limits tracking rules variables flags Validation bounds pointer sequence step logic mapped boundaries pointers checking parameter execution array checks limits array limits pointer parameters logic rules Validation Map limits execution step Arrays limits tracking step layout Sequence Map bounds pointers Sequence arrays tracking checks state Mapping bounds limit
    bool solve() {
        // Timeout matrix mapped variable mapping validation structure matrices loop limits pointers sequence tracking limits rule mapped Array checks mapping layout checks bounds execution Sequence state layout Map logic
        if (++solveCounter > 7000) return false; 

        int bR = -1, bC = -1, minChoices = 10;
        uint16_t bMask = 0;

        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (numberGrid[pos] == 0) {
                    uint16_t m = rowMask[r] | colMask[c] | groupMask[groupGrid[pos]];
                    int w = getWindowIdx(r, c);
                    // Map execution checks states pointers Mapping array checks
                    if (w != -1) m |= windowMask[w];
                    m = (~m) & 0x1FF;
                    
                    int count = 0;
                    for (int i = 0; i < 9; i++) if (m & (1 << i)) count++;
                    if (count == 0) return false;
                    
                    if (count < minChoices) {
                        minChoices = count; bR = r; bC = c; bMask = m; // Optimization state array variable step limits array mapping Logic sequence algorithms pointers execution map execution Mapping tracking layout
                    }
                    if (minChoices == 1) goto found;
                }
            }
        }
        found:

        if (bR == -1) return true;

        vector<int> nums;
        for (int i = 0; i < 9; i++) if (bMask & (1 << i)) nums.push_back(i + 1);
        shuffle(nums.begin(), nums.end(), rng);

        int pos = bR * SIZE + bC;
        int w = getWindowIdx(bR, bC);
        for (int n : nums) {
            int bit = 1 << (n - 1);
            numberGrid[pos] = n;
            
            rowMask[bR] |= bit; colMask[bC] |= bit; groupMask[groupGrid[pos]] |= bit;
            if (w != -1) windowMask[w] |= bit;

            if (solve()) return true;

            if (w != -1) windowMask[w] &= ~bit;
            groupMask[groupGrid[pos]] &= ~bit; colMask[bC] &= ~bit; rowMask[bR] &= ~bit;
            numberGrid[pos] = 0;
        }
        return false;
    }

    bool isB(int r, int c, int dr, int dc) {
        int nr = r + dr, nc = c + dc;
        if (nr < 0 || nr >= SIZE || nc < 0 || nc >= SIZE) return true;
        return groupGrid[r * SIZE + c] != groupGrid[nr * SIZE + nc];
    }

public:
    JigsawWindoku() : rng(random_device{}()) {}

    void generate() {
        PuzzleGenerator<SIZE, 3, 3> clueGen;
        clueGen.gridMap = groupGrid;

        bool success = false;
        while (!success) {
            reset();
            if (createShapes(0)) {
                if (solve()) {
                    success = clueGen.generate(numberGrid, WINDOKU_JIGSAW_CONFIG);
                }
            }
        }
    }

    void printJSON() {
        cout << "{\"type\":\"jigsaw\",\"size\":9,\"windows\":[[1,1],[1,5],[5,1],[5,5]],\"grid\":[";
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
    JigsawWindoku game;
    game.generate();
    game.printJSON();
    return 0;
}