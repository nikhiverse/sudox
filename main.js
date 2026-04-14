// ── Date ──
document.getElementById("todayDate").textContent =
  new Date().toLocaleDateString(undefined, {
    year: "numeric",
    month: "long",
    day: "numeric",
  });

// ── DOM refs ──
const menuState = document.getElementById("menuState");
const puzzleView = document.getElementById("puzzleView");
const puzzleTitle = document.getElementById("puzzleTitle");
const puzzleGrid = document.getElementById("puzzleGrid");
const puzzleIdEl = document.getElementById("puzzleId");
const spinner = document.getElementById("spinner");
const numPanel = document.getElementById("numPanel");
const timerDisplay = document.getElementById("timerDisplay");

// ── Timer State ──
let timerSeconds = 0;
let timerInterval = null;

function startTimer() {
  clearInterval(timerInterval);
  timerSeconds = 0;
  updateTimerDisplay();
  timerInterval = setInterval(() => {
    timerSeconds++;
    updateTimerDisplay();
  }, 1000);
}

function stopTimer() {
  clearInterval(timerInterval);
  timerInterval = null;
}

function updateTimerDisplay() {
  const m = Math.floor(timerSeconds / 60);
  const s = timerSeconds % 60;
  timerDisplay.textContent = `${m}:${s.toString().padStart(2, "0")}`;
  // Update clock emoji based on minute hand position
  const clocks = [
    "🕛",
    "🕐",
    "🕑",
    "🕒",
    "🕓",
    "🕔",
    "🕕",
    "🕖",
    "🕗",
    "🕘",
    "🕙",
    "🕚",
  ];
  const idx = Math.floor(timerSeconds / 5) % 12;
  document.querySelector(".timer-icon").textContent = clocks[idx];
}

// ── State ──
let currentGame = null;
let puzzleData = null; // raw API response
let gridSize = 0;
let totalRows = 0;
let totalCols = 0;
let cells = []; // flat array of cell DOM nodes (null for inactive)
let cellValues = []; // 2-D: user-entered values (0 = empty)
let cellCorrect = []; // 2-D: true once confirmed correct
let solutionGrid = null; // 2-D solution (if API provides it)
let cursorR = -1;
let cursorC = -1;
let selectedNum = null; // which num-btn is "armed" (null = eraser / none)

const gameNames = {
  sudoku_mini: "Sudoku Mini",
  sudoku_easy: "Sudoku Eazy",
  sudoku9: "Sudoku 9",
  sudoku_a: "Sudoku A",
  sudokuX: "Sudoku X",
  sudoku16: "Sudoku 16",
  dozaku: "Dozaku",
  windoku: "Windoku",
  windokuX: "Windoku X",
  windoku_jigsaw: "Windoku Jigsaw",
  jigsaw8: "Jigsaw 8",
  jigsaw9: "Jigsaw 9",
  jigsawX: "Jigsaw X",
  twodoku_mini: "Twodoku Mini",
  twodoku8a: "Twodoku 8A",
  twodoku8b: "Twodoku 8B",
  twodoku9: "Twodoku 9",
  sudoku12: "Sudoku 12",
  twodoku8: "Twodoku 8",
};

// Number labels for sizes > 9
function displayVal(v) {
  if (v === 0) return "";
  if (v <= 9) return String(v);
  return String.fromCharCode(65 + v - 10); // 10→A, 11→B, 12→C …
}

// Parse display char back to int
function parseVal(ch) {
  if (!ch) return 0;
  const n = parseInt(ch, 10);
  if (!isNaN(n)) return n;
  return ch.toUpperCase().charCodeAt(0) - 65 + 10; // A→10
}

// ══════════════════════════════════════════════════════
// CURSOR & CROSSHAIR
// ══════════════════════════════════════════════════════
function cellAt(r, c) {
  if (r < 0 || c < 0 || r >= totalRows || c >= totalCols) return null;
  return cells[r * totalCols + c];
}

function refreshHighlights() {
  // 1. Clear all dynamic classes
  for (let r = 0; r < totalRows; r++) {
    for (let c = 0; c < totalCols; c++) {
      const el = cellAt(r, c);
      if (!el || el.classList.contains("inactive-cell")) continue;
      el.classList.remove("cursor", "crosshair");
    }
  }
  if (cursorR < 0 || cursorC < 0) return;

  const cur = cellAt(cursorR, cursorC);
  if (!cur || cur.classList.contains("inactive-cell")) return;

  // 2. Check the value of the currently selected cell
  const currentVal = parseVal(cur.textContent);

  if (currentVal === 0) {
    // EMPTY CELL: Paint crosshair (same row + col)
    for (let r = 0; r < totalRows; r++) {
      for (let c = 0; c < totalCols; c++) {
        if (r === cursorR && c === cursorC) continue;
        const el = cellAt(r, c);
        if (!el || el.classList.contains("inactive-cell")) continue;
        if (r === cursorR || c === cursorC) el.classList.add("crosshair");
      }
    }
  } else {
    // NUMBERED CELL: Paint all cells matching the same number
    for (let r = 0; r < totalRows; r++) {
      for (let c = 0; c < totalCols; c++) {
        if (r === cursorR && c === cursorC) continue;
        const el = cellAt(r, c);
        if (!el || el.classList.contains("inactive-cell")) continue;

        // If the cell's value matches our cursor's value, highlight it
        if (parseVal(el.textContent) === currentVal) {
          el.classList.add("crosshair");
        }
      }
    }
  }

  // 3. Paint the active cursor
  cur.classList.add("cursor");
}

function moveCursor(r, c) {
  cursorR = r;
  cursorC = c;
  refreshHighlights();
}

// ══════════════════════════════════════════════════════
// INPUT — write a value into a cell
// ══════════════════════════════════════════════════════
function writeValue(r, c, val) {
  const el = cellAt(r, c);
  if (!el) return;
  if (el.classList.contains("clue")) return; // clue cells are immutable
  if (cellCorrect[r][c]) return; // already confirmed correct → read-only

  cellValues[r][c] = val;
  el.textContent = displayVal(val);
  el.classList.remove("correct", "wrong");

  if (val === 0) {
    // Erased
  } else if (solutionGrid) {
    // Verify against solution
    if (solutionGrid[r][c] === val) {
      el.classList.add("correct");
      cellCorrect[r][c] = true;
    } else {
      el.classList.add("wrong");
    }
  }
  refreshNumPanel();
}

function eraseValue(r, c) {
  writeValue(r, c, 0);
}

// ══════════════════════════════════════════════════════
// NUMBER PANEL
// ══════════════════════════════════════════════════════

// How many numbers this puzzle uses
function puzzleMaxNum() {
  if (!puzzleData) return 9;
  // twodoku / standard: check size field or totalCols
  if (puzzleData.size) return puzzleData.size;
  if (puzzleData.grids && puzzleData.grids.length > 0)
    return puzzleData.grids[0].size;
  return gridSize || 9;
}

// Count how many times val appears as a correct/clue placement
function countPlaced(val) {
  let n = 0;
  for (let r = 0; r < totalRows; r++) {
    for (let c = 0; c < totalCols; c++) {
      const el = cellAt(r, c);
      if (!el || el.classList.contains("inactive-cell")) continue;
      // Clue cells count too
      if (el.classList.contains("clue") && parseVal(el.textContent) === val) {
        n++;
        continue;
      }
      if (cellCorrect[r][c] && cellValues[r][c] === val) n++;
    }
  }
  return n;
}

function buildNumPanel() {
  numPanel.innerHTML = "";
  const max = puzzleMaxNum();

  for (let v = 1; v <= max; v++) {
    const btn = document.createElement("button");
    btn.className = "num-btn";
    btn.textContent = displayVal(v);
    btn.dataset.val = v;
    btn.addEventListener("click", () => onNumBtn(v, btn));
    numPanel.appendChild(btn);
  }

  // Divider
  const div = document.createElement("span");
  div.className = "num-divider";
  numPanel.appendChild(div);

  // Eraser
  const era = document.createElement("button");
  era.className = "num-btn num-btn--erase";
  era.title = "Erase cell";
  era.innerHTML = `<svg width="18" height="18" viewBox="0 0 24 24" fill="none"
    stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M20 20H7L3 16l10-10 7 7-3.5 3.5"/>
    <path d="M6.5 17.5l4-4"/>
  </svg>`;
  era.addEventListener("click", onErase);
  numPanel.appendChild(era);

  refreshNumPanel();
}

function refreshNumPanel() {
  const max = puzzleMaxNum();
  numPanel.querySelectorAll(".num-btn[data-val]").forEach((btn) => {
    const v = parseInt(btn.dataset.val, 10);
    // Total cells that should have this value = number of active cells / max
    // simpler: exhausted when placed count === total occurrences in solution
    const placed = countPlaced(v);
    // If we have solution, compare to expected count
    let expected = 0;
    if (solutionGrid) {
      for (let r = 0; r < totalRows; r++)
        for (let c = 0; c < totalCols; c++)
          if (solutionGrid[r] && solutionGrid[r][c] === v) expected++;
    } else {
      // No solution: never exhaust
      expected = Infinity;
    }
    btn.classList.toggle("num-btn--exhausted", placed >= expected);
  });
}

function onNumBtn(val, btn) {
  if (btn.classList.contains("num-btn--exhausted")) return;
  if (cursorR >= 0 && cursorC >= 0) {
    writeValue(cursorR, cursorC, val);
  }
}

function onErase() {
  if (cursorR >= 0 && cursorC >= 0) {
    eraseValue(cursorR, cursorC);
  }
}

// ══════════════════════════════════════════════════════
// KEYBOARD INPUT
// ══════════════════════════════════════════════════════
document.addEventListener("keydown", (e) => {
  if (puzzleView.classList.contains("hidden")) return;

  // Arrow navigation
  const moves = {
    ArrowUp: [-1, 0],
    ArrowDown: [1, 0],
    ArrowLeft: [0, -1],
    ArrowRight: [0, 1],
  };
  if (moves[e.key]) {
    e.preventDefault();
    let [dr, dc] = moves[e.key];
    let r = cursorR + dr,
      c = cursorC + dc;
    // Skip inactive cells
    for (let i = 0; i < 20; i++) {
      if (r < 0 || r >= totalRows || c < 0 || c >= totalCols) break;
      const el = cellAt(r, c);
      if (el && !el.classList.contains("inactive-cell")) {
        moveCursor(r, c);
        break;
      }
      r += dr;
      c += dc;
    }
    return;
  }

  // Number keys
  const max = puzzleMaxNum();
  if (/^[1-9]$/.test(e.key)) {
    const v = parseInt(e.key, 10);
    if (v <= max) writeValue(cursorR, cursorC, v);
    return;
  }
  // A-C for 12-cell puzzles
  if (/^[a-cA-C]$/.test(e.key)) {
    const v = e.key.toUpperCase().charCodeAt(0) - 65 + 10;
    if (v <= max) writeValue(cursorR, cursorC, v);
    return;
  }
  // Backspace / Delete → erase
  if (e.key === "Backspace" || e.key === "Delete" || e.key === "0") {
    eraseValue(cursorR, cursorC);
  }
});

// ══════════════════════════════════════════════════════
// CELL CLICK → move cursor
// ══════════════════════════════════════════════════════
function attachCellClick(el, r, c) {
  el.addEventListener("click", () => {
    if (el.classList.contains("inactive-cell")) return;
    moveCursor(r, c);
  });
}

// ══════════════════════════════════════════════════════
// RENDERERS (unchanged logic, adds clue class + click)
// ══════════════════════════════════════════════════════

// ── Responsive cell sizing ──
function getResponsiveCellSize(cols, maxCellSize) {
  const container = document.querySelector(".container");
  const style = getComputedStyle(container);
  const paddingLeft = parseFloat(style.paddingLeft) || 24;
  const paddingRight = parseFloat(style.paddingRight) || 24;
  const availableWidth = container.clientWidth - paddingLeft - paddingRight;
  // Account for grid outline (3px each side)
  const gridPadding = 6;
  const computed = Math.floor((availableWidth - gridPadding) / cols);
  return Math.min(computed, maxCellSize);
}

function makeCell(r, c, val, cellSize) {
  const el = document.createElement("div");
  el.className = "grid-cell";
  el.style.width = cellSize + "px";
  el.style.height = cellSize + "px";
  el.style.fontSize = cellSize * 0.48 + "px";

  if (val !== 0) {
    el.textContent = displayVal(val);
    el.classList.add("clue");
  }

  cells[r * totalCols + c] = el;
  attachCellClick(el, r, c);
  return el;
}

function renderStandard(data) {
  const {
    size,
    subRows,
    subCols,
    grid,
    diagonals,
    windows,
    altSubRows,
    altSubCols,
  } = data;
  gridSize = size;
  totalRows = size;
  totalCols = size;
  cells = new Array(size * size).fill(null);
  cellValues = Array.from({ length: size }, () => new Array(size).fill(0));
  cellCorrect = Array.from({ length: size }, () => new Array(size).fill(false));

  const maxCell = size <= 9 ? 44 : size <= 12 ? 36 : 30;
  const cellSize = getResponsiveCellSize(size, maxCell);

  const table = document.createElement("div");
  table.className = "sudoku-grid";
  table.style.gridTemplateColumns = `repeat(${size}, ${cellSize}px)`;
  table.style.gridTemplateRows = `repeat(${size}, ${cellSize}px)`;

  const isWindow = {};
  if (windows) {
    for (const [wr, wc] of windows) {
      for (let r = wr; r < wr + 3; r++)
        for (let c = wc; c < wc + 3; c++) isWindow[`${r},${c}`] = true;
    }
  }

  for (let r = 0; r < size; r++) {
    for (let c = 0; c < size; c++) {
      const el = makeCell(r, c, grid[r][c], cellSize);

      const bw = "2.5px solid var(--grid-bg)";
      if (r % subRows === 0) el.style.borderTop = bw;
      if ((r + 1) % subRows === 0 || r === size - 1) el.style.borderBottom = bw;
      if (c % subCols === 0) el.style.borderLeft = bw;
      if ((c + 1) % subCols === 0 || c === size - 1) el.style.borderRight = bw;

      if (altSubRows && altSubCols) {
        if ((Math.floor(r / altSubRows) + Math.floor(c / altSubCols)) % 2 === 0)
          el.classList.add("alt-block");
      } else if (!diagonals && !windows) {
        if ((Math.floor(r / subRows) + Math.floor(c / subCols)) % 2 === 1)
          el.classList.add("alt-block");
      }

      const isDiag = diagonals && (r === c || r + c === size - 1);
      const isWin = !!isWindow[`${r},${c}`];
      if (isDiag && isWin) el.classList.add("both-cell");
      else if (isDiag) el.classList.add("diagonal-cell");
      else if (isWin) el.classList.add("window-cell");

      table.appendChild(el);
    }
  }
  return table;
}

function renderJigsaw(data) {
  const { size, grid, groups, diagonals, windows } = data;
  gridSize = size;
  totalRows = size;
  totalCols = size;
  cells = new Array(size * size).fill(null);
  cellValues = Array.from({ length: size }, () => new Array(size).fill(0));
  cellCorrect = Array.from({ length: size }, () => new Array(size).fill(false));

  const maxCell = size <= 8 ? 46 : 44;
  const cellSize = getResponsiveCellSize(size, maxCell);

  const table = document.createElement("div");
  table.className = "sudoku-grid";
  table.style.gridTemplateColumns = `repeat(${size}, ${cellSize}px)`;
  table.style.gridTemplateRows = `repeat(${size}, ${cellSize}px)`;

  const isWindow = {};
  if (windows) {
    for (const [wr, wc] of windows) {
      for (let r = wr; r < wr + 3; r++)
        for (let c = wc; c < wc + 3; c++) isWindow[`${r},${c}`] = true;
    }
  }

  for (let r = 0; r < size; r++) {
    for (let c = 0; c < size; c++) {
      const el = makeCell(r, c, grid[r][c], cellSize);

      const gid = groups[r][c];
      const bw = "2.5px solid var(--grid-bg)";
      const bt = "1px solid var(--border-soft)";
      el.style.borderTop = r === 0 || groups[r - 1]?.[c] !== gid ? bw : bt;
      el.style.borderBottom =
        r === size - 1 || groups[r + 1]?.[c] !== gid ? bw : bt;
      el.style.borderLeft = c === 0 || groups[r][c - 1] !== gid ? bw : bt;
      el.style.borderRight =
        c === size - 1 || groups[r][c + 1] !== gid ? bw : bt;

      if (diagonals && (r === c || r + c === size - 1))
        el.classList.add("diagonal-cell");
      if (isWindow[`${r},${c}`]) el.classList.add("window-cell");

      table.appendChild(el);
    }
  }
  return table;
}

function renderTwodoku(data) {
  const { totalRows: tR, totalCols: tC, grid, active, grids, blocks } = data;
  totalRows = tR;
  totalCols = tC;
  gridSize = grids ? grids[0].size : 9;
  cells = new Array(tR * tC).fill(null);
  cellValues = Array.from({ length: tR }, () => new Array(tC).fill(0));
  cellCorrect = Array.from({ length: tR }, () => new Array(tC).fill(false));

  const maxCell = tC <= 10 ? 42 : tC <= 12 ? 36 : 30;
  const cellSize = getResponsiveCellSize(tC, maxCell);

  const table = document.createElement("div");
  table.className = "sudoku-grid";
  table.style.gridTemplateColumns = `repeat(${tC}, ${cellSize}px)`;
  table.style.gridTemplateRows = `repeat(${tR}, ${cellSize}px)`;

  function isSubBlockBorder(r, c, dr, dc) {
    if (!grids) return false;
    for (const g of grids) {
      if (!g.subR || !g.subC) continue;
      const gr = g.r,
        gc = g.c,
        sz = g.size;
      if (r >= gr && r < gr + sz && c >= gc && c < gc + sz) {
        const nr = r + dr,
          nc = c + dc;
        if (nr < gr || nr >= gr + sz || nc < gc || nc >= gc + sz) continue;
        if (
          dr !== 0 &&
          Math.floor((r - gr) / g.subR) !== Math.floor((nr - gr) / g.subR)
        )
          return true;
        if (
          dc !== 0 &&
          Math.floor((c - gc) / g.subC) !== Math.floor((nc - gc) / g.subC)
        )
          return true;
      }
    }
    return false;
  }

  function getAltBlock(r, c) {
    if (!grids) return false;
    for (let gi = 0; gi < grids.length; gi++) {
      const g = grids[gi];
      if (!g.subR || !g.subC) continue;
      if (r >= g.r && r < g.r + g.size && c >= g.c && c < g.c + g.size) {
        const bR = Math.floor((r - g.r) / g.subR),
          bC = Math.floor((c - g.c) / g.subC);
        const parity =
          currentGame === "twodoku_mini" ? (gi % 2 === 0 ? 1 : 0) : 1;
        return (bR + bC) % 2 === parity;
      }
    }
    return false;
  }

  for (let r = 0; r < tR; r++) {
    for (let c = 0; c < tC; c++) {
      const el = document.createElement("div");
      el.style.width = cellSize + "px";
      el.style.height = cellSize + "px";
      el.style.fontSize = cellSize * 0.45 + "px";

      if (!active[r][c]) {
        el.className = "grid-cell inactive-cell";
        cells[r * tC + c] = null;
      } else {
        el.className = "grid-cell";
        el.style.border = "1px solid var(--border-soft)";
        cells[r * tC + c] = el;
        attachCellClick(el, r, c);

        if (grid[r][c] !== 0) {
          el.textContent = displayVal(grid[r][c]);
          el.classList.add("clue");
        } else {
          cellValues[r][c] = 0;
        }

        if (getAltBlock(r, c)) el.classList.add("alt-block");

        const bw = "2.5px solid var(--grid-bg)";
        if (!active[r - 1]?.[c]) el.style.borderTop = bw;
        if (!active[r + 1]?.[c]) el.style.borderBottom = bw;
        if (!active[r]?.[c - 1]) el.style.borderLeft = bw;
        if (!active[r]?.[c + 1]) el.style.borderRight = bw;

        if (isSubBlockBorder(r, c, -1, 0)) el.style.borderTop = bw;
        if (isSubBlockBorder(r, c, 1, 0)) el.style.borderBottom = bw;
        if (isSubBlockBorder(r, c, 0, -1)) el.style.borderLeft = bw;
        if (isSubBlockBorder(r, c, 0, 1)) el.style.borderRight = bw;

        if (blocks) {
          const bid = blocks[r][c];
          if (bid >= 0) {
            if (
              active[r - 1]?.[c] &&
              blocks[r - 1]?.[c] !== bid &&
              blocks[r - 1]?.[c] >= 0
            )
              el.style.borderTop = bw;
            if (
              active[r + 1]?.[c] &&
              blocks[r + 1]?.[c] !== bid &&
              blocks[r + 1]?.[c] >= 0
            )
              el.style.borderBottom = bw;
            if (
              active[r]?.[c - 1] &&
              blocks[r]?.[c - 1] !== bid &&
              blocks[r]?.[c - 1] >= 0
            )
              el.style.borderLeft = bw;
            if (
              active[r]?.[c + 1] &&
              blocks[r]?.[c + 1] !== bid &&
              blocks[r]?.[c + 1] >= 0
            )
              el.style.borderRight = bw;
          }
        }
      }
      table.appendChild(el);
    }
  }
  return table;
}

// ── Dispatch ──
function renderPuzzle(data) {
  puzzleGrid.innerHTML = "";
  cursorR = -1;
  cursorC = -1;
  let el;
  if (data.type === "standard") el = renderStandard(data);
  else if (data.type === "jigsaw") el = renderJigsaw(data);
  else if (data.type === "twodoku") el = renderTwodoku(data);
  if (el) puzzleGrid.appendChild(el);

  // Store solution if available
  solutionGrid = data.solution || null;

  buildNumPanel();

  // Auto-place cursor on first empty cell
  outer: for (let r = 0; r < totalRows; r++) {
    for (let c = 0; c < totalCols; c++) {
      const el2 = cellAt(r, c);
      if (
        el2 &&
        !el2.classList.contains("inactive-cell") &&
        !el2.classList.contains("clue")
      ) {
        moveCursor(r, c);
        break outer;
      }
    }
  }
}

// ══════════════════════════════════════════════════════
// FETCH
// ══════════════════════════════════════════════════════
async function generatePuzzle(game) {
  currentGame = game;
  puzzleTitle.textContent = gameNames[game] || game;
  puzzleGrid.innerHTML = "";
  puzzleIdEl.textContent = "";
  numPanel.innerHTML = "";

  spinner.classList.remove("hidden");
  menuState.classList.add("hidden");
  puzzleView.classList.remove("hidden");
  stopTimer();
  timerSeconds = 0;
  updateTimerDisplay();

  try {
    const res = await fetch(`/api/puzzle/${game}`);
    const data = await res.json();
    puzzleData = data;
    if (data.error) {
      puzzleGrid.innerHTML = `<p style="color:var(--primary);font-family:'DM Sans',sans-serif">${data.error}</p>`;
    } else {
      if (data.uniqueId) puzzleIdEl.textContent = `#${data.uniqueId}`;
      renderPuzzle(data);
      startTimer(); // Timer starts only after puzzle is rendered
    }
  } catch {
    puzzleGrid.innerHTML = `<p style="color:var(--primary);font-family:'DM Sans',sans-serif">Failed to connect — is the server running?</p>`;
  } finally {
    spinner.classList.add("hidden");
  }
}

function showMenu() {
  puzzleView.classList.add("hidden");
  menuState.classList.remove("hidden");
  stopTimer();
}

// ── Events ──
document
  .querySelectorAll(".diff-btn[data-game]")
  .forEach((btn) =>
    btn.addEventListener("click", () => generatePuzzle(btn.dataset.game)),
  );
document.getElementById("puzzleBackBtn").addEventListener("click", showMenu);

// ══════════════════════════════════════════════════════
// HAMBURGER MENU
// ══════════════════════════════════════════════════════
const hamburgerBtn = document.getElementById("hamburgerBtn");
const menuOverlay = document.getElementById("menuOverlay");
const menuDrawer = document.getElementById("menuDrawer");
const drawerCloseBtn = document.getElementById("drawerCloseBtn");
const drawerList = document.getElementById("drawerList");

// SVG icon helpers
const icons = {
  about: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><path d="M12 16v-4M12 8h.01"/></svg>`,
  theme: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>`,
  login: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M15 3h4a2 2 0 012 2v14a2 2 0 01-2 2h-4M10 17l5-5-5-5M15 12H3"/></svg>`,
  guide: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 3h6a4 4 0 014 4v14a3 3 0 00-3-3H2z"/><path d="M22 3h-6a4 4 0 00-4 4v14a3 3 0 013-3h7z"/></svg>`,
  download: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 3v12M6 11l6 6 6-6"/><path d="M3 18h18"/></svg>`,
};

function isGameActive() {
  return !puzzleView.classList.contains("hidden");
}

function populateDrawer() {
  drawerList.innerHTML = "";
  const items = isGameActive()
    ? [
        { label: "Guide to Play", icon: icons.guide, action: "guide" },
        { label: "Theme", icon: icons.theme, action: "theme" },
        { label: "Download PDF", icon: icons.download, action: "download" },
        { divider: true },
        { label: "Login", icon: icons.login, action: "login" },
      ]
    : [
        { label: "About", icon: icons.about, action: "about" },
        { label: "Theme", icon: icons.theme, action: "theme" },
        { divider: true },
        { label: "Login", icon: icons.login, action: "login" },
      ];

  items.forEach((item) => {
    if (item.divider) {
      const div = document.createElement("li");
      div.className = "drawer-divider";
      drawerList.appendChild(div);
      return;
    }
    const li = document.createElement("li");
    const btn = document.createElement("button");
    btn.className = "drawer-item";
    btn.innerHTML = item.icon + item.label;
    btn.addEventListener("click", () => {
      closeDrawer();
      handleMenuAction(item.action);
    });
    li.appendChild(btn);
    drawerList.appendChild(li);
  });
}

function openDrawer() {
  populateDrawer();
  menuOverlay.classList.remove("hidden");
  menuDrawer.classList.remove("hidden");
  // Trigger reflow for animation
  requestAnimationFrame(() => {
    menuOverlay.classList.add("visible");
    menuDrawer.classList.add("visible");
    hamburgerBtn.classList.add("open");
  });
}

function closeDrawer() {
  menuOverlay.classList.remove("visible");
  menuDrawer.classList.remove("visible");
  hamburgerBtn.classList.remove("open");
  setTimeout(() => {
    menuOverlay.classList.add("hidden");
    menuDrawer.classList.add("hidden");
  }, 300);
}

function handleMenuAction(action) {
  switch (action) {
    case "about":
      alert("SudoX — Daily Sudoku Variant Puzzles\n\nCreated by rathodnk");
      break;
    case "theme":
      document.documentElement.classList.toggle("force-dark");
      document.documentElement.classList.toggle("force-light");
      break;
    case "guide":
      alert(
        "How to Play:\n\n" +
          "1. Click a cell to select it\n" +
          "2. Use the number buttons or keyboard to fill in values\n" +
          "3. Green = correct, Red = wrong\n" +
          "4. Use the eraser (or Backspace) to clear a cell\n" +
          "5. Arrow keys navigate between cells\n" +
          "6. Complete all cells correctly to solve the puzzle!"
      );
      break;
    case "download":
      if (typeof generatePuzzlePDF === "function") {
        generatePuzzlePDF();
      }
      break;
    case "login":
      alert("Login feature coming soon!");
      break;
  }
}

hamburgerBtn.addEventListener("click", () => {
  if (menuDrawer.classList.contains("visible")) {
    closeDrawer();
  } else {
    openDrawer();
  }
});
menuOverlay.addEventListener("click", closeDrawer);
drawerCloseBtn.addEventListener("click", closeDrawer);

// ── Responsive resize: re-render grid on viewport/orientation change ──
let resizeTimeout = null;
window.addEventListener("resize", () => {
  clearTimeout(resizeTimeout);
  resizeTimeout = setTimeout(() => {
    if (puzzleData && !puzzleView.classList.contains("hidden")) {
      renderPuzzle(puzzleData);
    }
  }, 200);
});

// ── Download PDF ──  →  handled entirely in downloadPDF.js
// The button listener (puzzleDownloadBtn) and all PDF logic
// live there. This file only manages game state and rendering.
