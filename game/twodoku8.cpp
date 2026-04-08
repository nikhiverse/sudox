#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include "../clues/random_puzzle.cpp"

using namespace std;

const int BOARD_SIZE = 12;

struct Rect { int r1, r2, c1, c2; };

// TwinSudoku8x8 Type B: 12x12, overlap at rows 4-7, cols 4-7 (16 cells)
class TwinSudoku8x8 {
private:
    int board[BOARD_SIZE * BOARD_SIZE];
    mt19937 rng;

    Rect g1Bounds[8] = {
        {0, 3, 0, 1}, {0, 3, 2, 3}, {0, 1, 4, 7}, {2, 3, 4, 7},
        {4, 5, 0, 3}, {6, 7, 0, 3}, {4, 7, 4, 5}, {4, 7, 6, 7}
    };
    Rect g2Bounds[8] = {
        {0, 3, 0, 1}, {0, 3, 2, 3}, {0, 1, 4, 7}, {2, 3, 4, 7},
        {4, 5, 0, 3}, {6, 7, 0, 3}, {4, 7, 4, 5}, {4, 7, 6, 7}
    };

    void reset() { for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++) board[i] = 0; }
    bool inG1(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }
    bool inG2(int r, int c) { return r >= 4 && r < 12 && c >= 4 && c < 12; }

    int getG1Block(int r, int c) {
        if (!inG1(r,c)) return -1;
        for (int i = 0; i < 8; i++)
            if (r >= g1Bounds[i].r1 && r <= g1Bounds[i].r2 && c >= g1Bounds[i].c1 && c <= g1Bounds[i].c2) return i;
        return -1;
    }
    int getG2Block(int r, int c) {
        if (!inG2(r,c)) return -1;
        int lr = r - 4, lc = c - 4;
        for (int i = 0; i < 8; i++)
            if (lr >= g2Bounds[i].r1 && lr <= g2Bounds[i].r2 && lc >= g2Bounds[i].c1 && lc <= g2Bounds[i].c2) return i;
        return -1;
    }

    uint8_t getPossibleMask(int r, int c) {
        uint8_t used = 0;
        if (inG1(r, c)) {
            for (int i = 0; i < 8; i++) {
                if (board[r * BOARD_SIZE + i]) used |= (1 << (board[r * BOARD_SIZE + i] - 1));
                if (board[i * BOARD_SIZE + c]) used |= (1 << (board[i * BOARD_SIZE + c] - 1));
            }
            int bIdx = getG1Block(r, c);
            Rect b = g1Bounds[bIdx];
            for (int i = b.r1; i <= b.r2; i++)
                for (int j = b.c1; j <= b.c2; j++)
                    if (board[i * BOARD_SIZE + j]) used |= (1 << (board[i * BOARD_SIZE + j] - 1));
        }
        if (inG2(r, c)) {
            for (int i = 4; i < 12; i++) {
                if (board[r * BOARD_SIZE + i]) used |= (1 << (board[r * BOARD_SIZE + i] - 1));
                if (board[i * BOARD_SIZE + c]) used |= (1 << (board[i * BOARD_SIZE + c] - 1));
            }
            int bIdx = getG2Block(r, c);
            Rect b = g2Bounds[bIdx];
            for (int i = b.r1 + 4; i <= b.r2 + 4; i++)
                for (int j = b.c1 + 4; j <= b.c2 + 4; j++)
                    if (board[i * BOARD_SIZE + j]) used |= (1 << (board[i * BOARD_SIZE + j] - 1));
        }
        return ~used;
    }

    bool solve() {
        int bestR = -1, bestC = -1, minChoices = 9;
        for (int r = 0; r < BOARD_SIZE; r++)
            for (int c = 0; c < BOARD_SIZE; c++) {
                int pos = r * BOARD_SIZE + c;
                if ((inG1(r,c) || inG2(r,c)) && board[pos] == 0) {
                    uint8_t mask = getPossibleMask(r, c);
                    int ch = __builtin_popcount(mask & 0xFF);
                    if (ch == 0) return false;
                    if (ch < minChoices) { minChoices = ch; bestR = r; bestC = c; }
                    if (minChoices == 1) goto found;
                }
            }
        found:
        if (bestR == -1) return true;
        uint8_t mask = getPossibleMask(bestR, bestC);
        vector<int> nums;
        for (int i = 0; i < 8; i++) if (mask & (1 << i)) nums.push_back(i + 1);
        shuffle(nums.begin(), nums.end(), rng);
        int pos = bestR * BOARD_SIZE + bestC;
        for (int n : nums) { board[pos] = n; if (solve()) return true; board[pos] = 0; }
        return false;
    }

    int solCount = 0;
    void countMRV() {
        if (solCount > 1) return;
        int bestR = -1, bestC = -1, minChoices = 9;
        for (int r = 0; r < BOARD_SIZE; r++)
            for (int c = 0; c < BOARD_SIZE; c++) {
                int pos = r * BOARD_SIZE + c;
                if ((inG1(r,c) || inG2(r,c)) && board[pos] == 0) {
                    uint8_t mask = getPossibleMask(r, c);
                    int ch = __builtin_popcount(mask & 0xFF);
                    if (ch == 0) return;
                    if (ch < minChoices) { minChoices = ch; bestR = r; bestC = c; }
                    if (minChoices == 1) goto search;
                }
            }
    search:
        if (bestR == -1) { solCount++; return; }
        uint8_t mask = getPossibleMask(bestR, bestC);
        int pos = bestR * BOARD_SIZE + bestC;
        for (int i = 0; i < 8; i++) {
            if (mask & (1 << i)) {
                board[pos] = i + 1; countMRV(); board[pos] = 0;
                if (solCount > 1) return;
            }
        }
    }
    int countSolutions() { solCount = 0; countMRV(); return solCount; }

    void extractGrid(int* g, int oR, int oC) {
        for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++)
            g[r * 8 + c] = board[(r + oR) * BOARD_SIZE + (c + oC)];
    }
    void buildGridMap(int* gm, const Rect* bounds) {
        for (int i = 0; i < 64; i++) gm[i] = -1;
        for (int i = 0; i < 8; i++)
            for (int r = bounds[i].r1; r <= bounds[i].r2; r++)
                for (int c = bounds[i].c1; c <= bounds[i].c2; c++)
                    gm[r * 8 + c] = i;
    }

    // Check max 5 per block AND exact 8 per 4x4 supergrid
    bool checkConstraints8x8(const int* g, const Rect* bounds) {
        // Max 5 per block
        for (int i = 0; i < 8; i++) {
            int cnt = 0;
            for (int r = bounds[i].r1; r <= bounds[i].r2; r++)
                for (int c = bounds[i].c1; c <= bounds[i].c2; c++)
                    if (g[r*8+c] != 0) cnt++;
            if (cnt > 5) return false;
        }
        // Exact 8 per 4x4 supergrid (4 quadrants)
        for (int qr = 0; qr < 2; qr++)
            for (int qc = 0; qc < 2; qc++) {
                int cnt = 0;
                for (int r = qr*4; r < qr*4+4; r++)
                    for (int c = qc*4; c < qc*4+4; c++)
                        if (g[r*8+c] != 0) cnt++;
                if (cnt != 8) return false;
            }
        return true;
    }

public:
    TwinSudoku8x8() : rng(random_device{}()) {}

    void generate() {
        const PuzzleConfig& config = TWODOKU_8_CONFIG;
        bool generated = false;
        while (!generated) {
            reset();
            if (!solve()) continue;
            int original[BOARD_SIZE * BOARD_SIZE];
            for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++) original[i] = board[i];

            // PuzzleGenerator<8,2,4>: NUM_GRIDS=8, CELLS_PER_GRID=8
            int g1[64], g1Map[64];
            extractGrid(g1, 0, 0);
            buildGridMap(g1Map, g1Bounds);
            PuzzleConfig gCfg = {32, 5, 5, 5, 8, false, false, 8, false, false};
            PuzzleGenerator<8, 2, 4> gen1;
            gen1.gridMap = g1Map;
            if (!gen1.generate(g1, gCfg)) continue;
            if (!checkConstraints8x8(g1, g1Bounds)) continue;

            int g2[64], g2Map[64];
            extractGrid(g2, 4, 4);
            buildGridMap(g2Map, g2Bounds);
            PuzzleGenerator<8, 2, 4> gen2;
            gen2.gridMap = g2Map;
            if (!gen2.generate(g2, gCfg)) continue;
            if (!checkConstraints8x8(g2, g2Bounds)) continue;

            // Merge: Grid1 first, then Grid2 overwrites (including overlap)
            for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++) board[i] = 0;
            for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++)
                board[r * BOARD_SIZE + c] = g1[r * 8 + c];
            for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++)
                board[(r+4) * BOARD_SIZE + (c+4)] = g2[r * 8 + c];

            int totalClues = 0;
            for (int r = 0; r < BOARD_SIZE; r++)
                for (int c = 0; c < BOARD_SIZE; c++)
                    if ((inG1(r,c)||inG2(r,c)) && board[r*BOARD_SIZE+c] != 0) totalClues++;

            // Re-check merged Grid1 constraints
            int fg1[64]; extractGrid(fg1, 0, 0);
            if (!checkConstraints8x8(fg1, g1Bounds)) continue;

            // Adjust total clues to target
            if (totalClues > config.target_clues) {
                vector<int> extras;
                for (int r = 0; r < BOARD_SIZE; r++)
                    for (int c = 0; c < BOARD_SIZE; c++)
                        if ((inG1(r,c)||inG2(r,c)) && board[r*BOARD_SIZE+c] != 0)
                            extras.push_back(r*BOARD_SIZE+c);
                shuffle(extras.begin(), extras.end(), rng);
                for (int pos : extras) {
                    if (totalClues <= config.target_clues) break;
                    int bk = board[pos];
                    board[pos] = 0;
                    int fg1t[64], fg2t[64];
                    extractGrid(fg1t, 0, 0); extractGrid(fg2t, 4, 4);
                    if (checkConstraints8x8(fg1t, g1Bounds) && checkConstraints8x8(fg2t, g2Bounds) && countSolutions() == 1)
                        totalClues--;
                    else board[pos] = bk;
                }
            }
            if (totalClues == config.target_clues && countSolutions() == 1) generated = true;
        }
    }

    void printJSON() {
        cout << "{\"type\":\"twodoku\",\"totalRows\":12,\"totalCols\":12,";
        cout << "\"grids\":[{\"r\":0,\"c\":0,\"size\":8,\"subR\":4,\"subC\":4},{\"r\":4,\"c\":4,\"size\":8,\"subR\":4,\"subC\":4}],";
        cout << "\"grid\":[";
        for (int r = 0; r < BOARD_SIZE; r++) {
            if (r) cout << ","; cout << "[";
            for (int c = 0; c < BOARD_SIZE; c++) { if (c) cout << ","; cout << board[r*BOARD_SIZE+c]; }
            cout << "]";
        }
        cout << "],\"blocks\":[";
        for (int r = 0; r < BOARD_SIZE; r++) {
            if (r) cout << ","; cout << "[";
            for (int c = 0; c < BOARD_SIZE; c++) {
                if (c) cout << ",";
                int bid = -1;
                if (inG1(r,c) && inG2(r,c)) bid = getG2Block(r,c)+8;
                else if (inG1(r,c)) bid = getG1Block(r,c);
                else if (inG2(r,c)) bid = getG2Block(r,c)+8;
                cout << bid;
            }
            cout << "]";
        }
        cout << "],\"active\":[";
        for (int r = 0; r < BOARD_SIZE; r++) {
            if (r) cout << ","; cout << "[";
            for (int c = 0; c < BOARD_SIZE; c++) { if (c) cout << ","; cout << (inG1(r,c)||inG2(r,c)?"true":"false"); }
            cout << "]";
        }
        cout << "],\"overlap\":[";
        for (int r = 0; r < BOARD_SIZE; r++) {
            if (r) cout << ","; cout << "[";
            for (int c = 0; c < BOARD_SIZE; c++) { if (c) cout << ","; cout << (inG1(r,c)&&inG2(r,c)?"true":"false"); }
            cout << "]";
        }
        cout << "]}" << endl;
    }
};

int main() {
    TwinSudoku8x8 game;
    game.generate();
    game.printJSON();
    return 0;
}