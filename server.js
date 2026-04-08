const express = require("express");
const { execSync, execFileSync } = require("child_process");
const path = require("path");
const fs = require("fs");

const app = express();
const PORT = 3000;
const GAME_DIR = path.join(__dirname, "game");

// Serve static files
app.use(express.static(__dirname));

// Valid game names (must match .cpp filenames without extension)
const VALID_GAMES = [
  "sudoku_mini",
  "sudoku_easy",
  "sudoku9",
  "sudoku_a",
  "sudokuX",
  "dozaku",
  "windoku",
  "windokuX",
  "windoku_jigsaw",
  "jigsaw8",
  "jigsaw9",
  "jigsawX",
  "twodoku_mini",
  "twodoku8",
  "twodoku9",
  "sudoku12",
];

// Aliases: games that use another game's binary
const GAME_ALIASES = {

};

// Variation codes for ID generation
const GAME_CODES = {
  sudoku_mini: "06",
  sudoku_easy: "90",
  sudoku9: "91",
  sudoku_a: "92",
  sudokuX: "93",
  dozaku: "13",
  windoku: "94",
  windokuX: "95",
  windoku_jigsaw: "11",
  jigsaw8: "08",
  jigsaw9: "09",
  jigsawX: "10",
  twodoku_mini: "07",
  twodoku8: "14",
  twodoku9: "15",
  sudoku12: "12",
};

function getRandomTwoDigit() {
  const num = Math.floor(Math.random() * 99) + 1;
  return num.toString().padStart(2, "0");
}

// Daily puzzle cache: key = "YYYYMMDD:game", value = full puzzle JSON response
const dailyCache = {};

function getTodayKey(game) {
  const now = new Date();
  const ymd = `${now.getFullYear()}${String(now.getMonth() + 1).padStart(2, "0")}${String(now.getDate()).padStart(2, "0")}`;
  return `${ymd}:${game}`;
}

app.get("/api/puzzle/:game", (req, res) => {
  const game = req.params.game;

  if (!VALID_GAMES.includes(game)) {
    return res.status(400).json({ error: "Invalid game name" });
  }

  // Check cache — return cached puzzle if already generated today
  const cacheKey = getTodayKey(game);
  if (dailyCache[cacheKey]) {
    return res.json(dailyCache[cacheKey]);
  }

  // Purge stale cache entries from previous days
  const todayPrefix = cacheKey.split(":")[0];
  for (const key of Object.keys(dailyCache)) {
    if (!key.startsWith(todayPrefix)) {
      delete dailyCache[key];
    }
  }

  const actualGame = GAME_ALIASES[game] || game;
  const srcPath = path.join(GAME_DIR, `${actualGame}.cpp`);

  // NEW: Define the exe directory and the new binary path
  const EXE_DIR = path.join(GAME_DIR, "exe");
  const binPath = path.join(EXE_DIR, `${actualGame}`);

  if (!fs.existsSync(srcPath)) {
    return res.status(404).json({ error: "Source file not found" });
  }

  // Build IDs (fixed for the day)
  const now = new Date();
  const yy = String(now.getFullYear()).slice(-2);
  const mm = String(now.getMonth() + 1).padStart(2, "0");
  const dd = String(now.getDate()).padStart(2, "0");
  const cc = GAME_CODES[game] || "00";
  const rr = getRandomTwoDigit();

  const uniqueId = `${yy}${mm}${dd}${cc}`; // yymmddcc
  const generationId = `${dd}${cc}${rr}`; // ddccrr

  try {
    // Compile if binary doesn't exist or source is newer
    let needsCompile = !fs.existsSync(binPath);
    if (!needsCompile) {
      const srcTime = fs.statSync(srcPath).mtimeMs;
      const binTime = fs.statSync(binPath).mtimeMs;
      needsCompile = srcTime > binTime;
    }

    if (needsCompile) {
      // NEW: Ensure the 'exe' directory exists before attempting to compile
      if (!fs.existsSync(EXE_DIR)) {
        fs.mkdirSync(EXE_DIR, { recursive: true });
      }

      execSync(`g++ -O2 -o "${binPath}" "${srcPath}"`, { timeout: 15000 });
    }

    // Run the binary ONCE — output is cached for the rest of the day
    const output = execFileSync(binPath, [], {
      timeout: 10000,
      encoding: "utf-8",
      env: { ...process.env, TERM: "dumb" },
    });

    // Strip ANSI escape codes
    const cleanOutput = output.replace(/\x1b\[[0-9;]*m/g, "");

    // Find the JSON object in the output
    const jsonStart = cleanOutput.indexOf("{");
    if (jsonStart === -1) {
      return res
        .status(500)
        .json({ error: "No JSON output from puzzle generator" });
    }
    const jsonStr = cleanOutput.substring(jsonStart);
    const puzzleData = JSON.parse(jsonStr);

    // Inject IDs into response
    puzzleData.uniqueId = uniqueId;
    puzzleData.generationId = generationId;

    // Cache the result for the rest of the day
    dailyCache[cacheKey] = puzzleData;

    res.json(puzzleData);
  } catch (err) {
    console.error(`Error for ${game}:`, err.message);
    res
      .status(500)
      .json({ error: "Failed to generate puzzle. Please try again." });
  }
});

app.listen(PORT, () => {
  console.log(`SudoX server running at http://localhost:${PORT}`);
});
