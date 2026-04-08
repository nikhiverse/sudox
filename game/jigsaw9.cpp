#include "../clues/random_puzzle.cpp"

using namespace std;

const int SIZE = 9;

struct Point { int r, c; };

// JigsawSudoku: Inme traditional 3x3 blocks nahi hote, 
// balki randomized connected shapes (polyominoes) hote hain.
class JigsawSudoku {
private:
    int groupGrid[SIZE * SIZE]; // Grid jo shapes store karegi
    int board[SIZE * SIZE]; // Grid jo numbers store karegi
    uint16_t rowMask[SIZE], colMask[SIZE], groupMask[SIZE];
    mt19937 rng;
    int solveCounter;

    void reset() {
        solveCounter = 0;
        for (int i = 0; i < SIZE; i++) {
            rowMask[i] = colMask[i] = groupMask[i] = 0;
        }
        for (int pos = 0; pos < SIZE * SIZE; pos++) {
            groupGrid[pos] = -1; // -1 means khali (empty)
            board[pos] = 0;
        }
    }

    // ALGORITHM: Random Jigsaw Shapes Generator
    // Ek specific size (currentCells.size()) ki boundary form kar raha hai
    bool growGroup(vector<Point>& currentCells, int groupId) {
        if (currentCells.size() == SIZE) return true; // Shape size reaches 9, growth complete
        
        // 4 directions (Up, Down, Left, Right)
        int dr[] = {-1, 1, 0, 0}, dc[] = {0, 0, -1, 1};
        vector<Point> candidates;
        
        // Har existing cell ki borders measure karo
        for (auto& p : currentCells) {
            for (int i = 0; i < 4; i++) {
                int nr = p.r + dr[i], nc = p.c + dc[i];
                // Check if neighbor cell is within boundaries aur abhi tak khali hai
                if (nr >= 0 && nr < SIZE && nc >= 0 && nc < SIZE && groupGrid[nr * SIZE + nc] == -1) {
                    bool exists = false;
                    for(auto& cand : candidates) if(cand.r == nr && cand.c == nc) exists = true;
                    if(!exists) candidates.push_back({nr, nc}); // Naya neighbor candidate list me add kiya
                }
            }
        }
        if (candidates.empty()) return false; // Dead end (blocks get stuck), try a different growth pattern
        
        shuffle(candidates.begin(), candidates.end(), rng);
        
        for (auto& c : candidates) {
            groupGrid[c.r * SIZE + c.c] = groupId;
            currentCells.push_back(c);
            
            // Recursively further grow karte hain
            if (growGroup(currentCells, groupId)) return true;
            
            // Failed growth backtrack
            currentCells.pop_back();
            groupGrid[c.r * SIZE + c.c] = -1;
        }
        return false;
    }

    // Shapes creation iterator (har shape sequence form karta hai 0 se 8 tak)
    bool createShapes(int groupId) {
        if (groupId == SIZE) return true; // Saare 9 groups bann gaye
        
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (groupGrid[pos] == -1) {
                    // Seed (Start point) shape ki foundation nikal rahe hain
                    groupGrid[pos] = groupId;
                    vector<Point> curr = {{r, c}};
                    
                    if (growGroup(curr, groupId)) {
                        // Agar present seed complete shape me grow hogi, tab naya seed generate karo
                        if (createShapes(groupId + 1)) return true;
                    }
                    
                    // Root point back track fail safe
                    groupGrid[pos] = -1;
                    return false;
                }
            }
        }
        return true;
    }

    // Number constraint mask
    uint16_t getPossible(int r, int c) {
        // Blocks fixed index based nahi hai, custom groupedGrid mask pe filter hote hain.
        return ~(rowMask[r] | colMask[c] | groupMask[groupGrid[r * SIZE + c]]) & 0x1FF;
    }

    // ALGORITHM: Jigsaw Number Solver (using Exact Cover / MRV method)
    bool solve() {
        if (++solveCounter > 5000) return false;

        int bestR = -1, bestC = -1, minChoices = 10;
        uint16_t bestMask = 0;

        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                int pos = r * SIZE + c;
                if (board[pos] == 0) {
                    uint16_t mask = getPossible(r, c);
                    int choices = 0;
                    for (int i = 0; i < 9; i++) if (mask & (1 << i)) choices++;
                    
                    if (choices == 0) return false;
                    
                    // MRV heuristics: low choice path select
                    if (choices < minChoices) {
                        minChoices = choices; bestR = r; bestC = c; bestMask = mask;
                    }
                    if (minChoices == 1) goto found;
                }
            }
        }

    found:
        if (bestR == -1) return true;

        vector<int> nums;
        for (int i = 0; i < 9; i++) if (bestMask & (1 << i)) nums.push_back(i + 1);
        shuffle(nums.begin(), nums.end(), rng);

        int pos = bestR * SIZE + bestC;

        for (int n : nums) {
            int bit = 1 << (n - 1);
            board[pos] = n; // value push
            // dynamic custom group mask bit status updated
            rowMask[bestR] |= bit; colMask[bestC] |= bit; groupMask[groupGrid[pos]] |= bit;
            
            if (solve()) return true;
            
            // Undo memory changes
            rowMask[bestR] &= ~bit; colMask[bestC] &= ~bit; groupMask[groupGrid[pos]] &= ~bit;
            board[pos] = 0;
        }
        return false;
    }

public:
    JigsawSudoku() : rng(random_device{}()) {}

    void generate() {
        PuzzleGenerator<SIZE, 3, 3> clueGen;
        bool success = false;
        while (!success) {
            reset();
            // Jigsaw puzzles mein steps series hoti hai: Pehle shapes banti hain, 
            // phir numbers place hote hain solver run hooke, finally clues hataye jaate hain
            if (createShapes(0)) {
                if (solve()) {
                    clueGen.gridMap = groupGrid; // Reference supply mapping
                    // Clues logic custom grid structure utilize karti hai layout follow karne
                    success = clueGen.generate(board, JIGSAW_9_CONFIG);
                }
            }
        }
    }

    void printJSON() {
        // Output me custom group mapping object list (groups array matrix) bhi append kia jata hai
        cout << "{\"type\":\"jigsaw\",\"size\":9,\"grid\":[";
        for (int r = 0; r < SIZE; r++) {
            if (r) cout << ",";
            cout << "[";
            for (int c = 0; c < SIZE; c++) {
                if (c) cout << ",";
                cout << board[r * SIZE + c];
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
    JigsawSudoku game;
    game.generate();
    game.printJSON();
    return 0;
}