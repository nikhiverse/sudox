document.addEventListener("DOMContentLoaded", function () {
  const randomBtn = document.getElementById("randomBtn");
  const gridSizeSelect = document.getElementById("gridSize");
  const gridContainer = document.getElementById("gridContainer");
  const downloadBtn = document.getElementById("downloadBtn");

  // Define symbols and block dimensions
  const allSymbols = [
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
  ];

  const symbols = {
    6: allSymbols.slice(0, 6),
    9: allSymbols.slice(0, 9),
    12: allSymbols.slice(0, 12),
    16: allSymbols.slice(0, 16),
  };

  const blockDims = {
    6: { rows: 2, cols: 3 },
    9: { rows: 3, cols: 3 },
    12: { rows: 3, cols: 4 },
    16: { rows: 4, cols: 4 },
  };

  let currentGrid = [];
  let currentSize = parseInt(gridSizeSelect.value);
  let solvedGrid = null;
  let lives = 0;
  window.currentHexId = null; // global tracker for active puzzle

  // ---------- Persistence helpers ----------
  function saveGameState(hexId, currentGrid, solvedGrid, lives) {
    const state = {
      puzzle: currentGrid,
      solution: solvedGrid,
      lives: lives,
      inGame: true,
    };
    localStorage.setItem("sudoku_" + hexId, JSON.stringify(state));
  }
  function loadGameState(hexId) {
    const data = localStorage.getItem("sudoku_" + hexId);
    return data ? JSON.parse(data) : null;
  }
  function clearGameState(hexId) {
    localStorage.removeItem("sudoku_" + hexId);
  }
  // ---------- end persistence helpers ----------

  // Auto-continue saved game on page load
  (function tryAutoContinue() {
    const size = parseInt(gridSizeSelect.value);
    const formatMap = { 6: "mini", 9: "nona", 12: "doza", 16: "hexa" };
    const format = formatMap[size];
    const hexId = getFormatHexId(format);
    const saved = loadGameState(hexId);
    if (saved && saved.inGame) {
      window.currentHexId = hexId;
      currentSize = size;
      currentGrid = saved.puzzle;
      solvedGrid = saved.solution;
      lives = saved.lives;

      document.getElementById("defaultHeading").classList.add("hidden");
      document.getElementById("defaultDate").classList.add("hidden");
      document.getElementById("topControls").classList.add("hidden");
      document.getElementById("puzzleHeading").classList.remove("hidden");
      document.getElementById("puzzleIdText").textContent = hexId;
      document.getElementById("puzzleIdLine").classList.remove("hidden");
      document.getElementById("bottomControls").classList.remove("hidden");
      document.getElementById("gridContainer").classList.remove("hidden");
      document.getElementById("livesCount").textContent = lives;

      renderGrid(currentGrid, currentSize);
    }
  })();

  randomBtn.addEventListener("click", function () {
    currentSize = parseInt(gridSizeSelect.value);

    // Get format name and ID
    const format = {
      6: "mini",
      9: "nona",
      12: "doza",
      16: "hexa",
    }[currentSize];

    const hexId = getFormatHexId(format);
    const seed = parseInt(hexId.slice(1, -2), 16);
    window.currentHexId = hexId;

    const savedState = loadGameState(hexId);

    // Hide default headings
    document.getElementById("defaultHeading").classList.add("hidden");
    document.getElementById("defaultDate").classList.add("hidden");

    // Update and show puzzle heading
    const puzzleName = {
      6: "Mini",
      9: "Nona",
      12: "Doza",
      16: "Hexa",
    }[currentSize];

    document.getElementById("puzzleHeading").innerHTML = `<img
          src="img/sudcon.png"
          alt="Logo"
          style="height: 50px; vertical-align: middle; margin-right: 8px"
        />Sudoku ${puzzleName} Puzzle`;
    document.getElementById("puzzleHeading").classList.remove("hidden");

    // Show puzzle ID in heading
    document.getElementById("puzzleIdText").textContent = hexId;

    //Show lives in heading
    if (puzzleName == "Mini") lives = 1;
    else if (puzzleName == "Nona") lives = 3;
    else if (puzzleName == "Doza") lives = 4;
    else if (puzzleName == "Hexa") lives = 5;

    if (savedState) {
      currentGrid = savedState.puzzle;
      solvedGrid = savedState.solution;
      lives = savedState.lives;
    } else {
      const solution = generateValidGrid(currentSize, seed);
      solvedGrid = solution.map((r) => [...r]);

      if (currentSize === 12 || currentSize === 16) {
        currentGrid = createSymmetricPuzzle(solution, currentSize, seed);
      } else {
        currentGrid = createPuzzleFromGrid(solution, currentSize, seed);
      }

      saveGameState(hexId, currentGrid, solvedGrid, lives);
    }

    document.getElementById("livesCount").textContent = lives;

    document.getElementById("puzzleIdLine").classList.remove("hidden");

    renderGrid(currentGrid, currentSize);

    // Hide top controls (select + generate)
    document.getElementById("topControls").classList.add("hidden");

    // Show puzzle grid
    document.getElementById("gridContainer").classList.remove("hidden");

    // Show download and back buttons
    document.getElementById("bottomControls").classList.remove("hidden");

    // Hide Random Button
    document.getElementById("randomBtn").style.display = "none";

    // Clear any messages if needed
    const messageEl = document.getElementById("message");
    if (messageEl) messageEl.textContent = "";
  });

  //download button
  downloadBtn.addEventListener("click", function () {
    const { jsPDF } = window.jspdf;
    const doc = new jsPDF();

    const linkedinURL = "https://www.linkedin.com/in/rathodnk/";

    const puzzle = currentGrid;
    const size = currentSize;

    // Determine format and hex ID
    const formatMap = { 6: "mini", 9: "nona", 12: "doza", 16: "hexa" };
    const format = formatMap[size];
    const hexId = getFormatHexId(format);

    const titleMap = { 6: "Mini", 9: "Nona", 12: "Doza", 16: "Hexa" };
    const n1 = titleMap[size];

    const cellSize = 10; // mm
    const totalGridSize = size * cellSize;

    // Center grid horizontally
    const pageWidth = doc.internal.pageSize.getWidth();
    const startX = (pageWidth - totalGridSize) / 2;
    const startY = 30;

    // 1. Print ID at top-right corner
    doc.setFontSize(10);
    doc.text(`${hexId}`, pageWidth - 20, 15, { align: "right" });

    // 2. Puzzle title center
    doc.setFontSize(13.5);
    doc.text(`Sudoku ${n1} Puzzle (${size}x${size})`, pageWidth / 2, 20, {
      align: "center",
    });

    // 3. Draw puzzle grid centered
    doc.setFontSize(12);
    for (let row = 0; row < size; row++) {
      for (let col = 0; col < size; col++) {
        const x = startX + col * cellSize;
        const y = startY + row * cellSize;

        doc.rect(x, y, cellSize, cellSize);

        const val = puzzle[row][col];
        if (val !== "") {
          doc.text(val.toString(), x + cellSize / 2, y + cellSize / 2 + 1, {
            align: "center",
            baseline: "middle",
          });
        }
      }
    }

    // 4. Draw block lines thicker
    const block = blockDims[size];
    doc.setLineWidth(1);
    for (let i = 0; i <= size; i++) {
      if (i % block.cols === 0) {
        const x = startX + i * cellSize;
        doc.line(x, startY, x, startY + totalGridSize);
      }
      if (i % block.rows === 0) {
        const y = startY + i * cellSize;
        doc.line(startX, y, startX + totalGridSize, y);
      }
    }

    // 5. Print LinkedIn ID at center bottom
    const pageHeight = doc.internal.pageSize.getHeight();
    doc.setFontSize(10);
    doc.textWithLink(
      "linkedin.com/in/rathodnk", // <--your actual ID
      pageWidth / 2,
      pageHeight - 10,
      {
        url: linkedinURL,
        align: "center",
      }
    );

    // Save PDF
    doc.save(`sudoku_${size}x${size}.pdf`);
  });

  const confirmPopup = document.getElementById("confirmPopup");
  const confirmYes = document.getElementById("confirmYes");
  const confirmNo = document.getElementById("confirmNo");

  document.getElementById("backBtn").addEventListener("click", () => {
    confirmPopup.classList.remove("hidden");
  });

  confirmYes.addEventListener("click", () => {
    // Save progress before going back
    if (window.currentHexId) {
      saveGameState(window.currentHexId, currentGrid, solvedGrid, lives);
    }
    window.currentHexId = null; // prevent instant auto-resume
    savedState = null; // make sure your code doesn't instantly reload state

    // Proceed with back logic
    document.getElementById("topControls").classList.remove("hidden");

    document.getElementById("defaultHeading").classList.remove("hidden");
    document.getElementById("defaultDate").classList.remove("hidden");

    document.getElementById("puzzleHeading").classList.add("hidden");
    document.getElementById("puzzleIdLine").classList.add("hidden");

    document.getElementById("randomBtn").style.display = "inline-block";
    document.getElementById("gridContainer").classList.add("hidden");
    document.getElementById("bottomControls").classList.add("hidden");
    confirmPopup.classList.add("hidden");
  });

  confirmNo.addEventListener("click", () => {
    saveGameState(window.currentHexId, currentGrid, solvedGrid, lives);
    // Just close popup
    confirmPopup.classList.add("hidden");
  });

  function generateValidGrid(size, seed) {
    const dims = blockDims[size];
    let grid;
    let attempts = 0;
    const maxAttempts = 100;
    while (attempts < maxAttempts) {
      grid = Array(size)
        .fill()
        .map(() => Array(size).fill(""));
      const syms = symbols[size];

      for (let br = 0; br < size; br += dims.rows) {
        for (let bc = 0; bc < size; bc += dims.cols) {
          if (br === bc) {
            fillBlock(
              grid,
              br,
              bc,
              dims.rows,
              dims.cols,
              shuffleArraySeeded([...syms], seed + br + bc)
            );
          }
        }
      }

      if (fillRemaining(grid, 0, dims.rows, dims.cols, size, seed)) {
        if (validateGrid(grid, size)) {
          // Store a deep copy before returning
          solvedGrid = grid.map((row) => [...row]);

          return grid;
        }
      }
      attempts++;
    }

    return generateSimpleGrid(size, dims, seed);
  }

  function fillRemaining(grid, index, blockRows, blockCols, size, seed) {
    if (index >= size * size) return true;
    const row = Math.floor(index / size);
    const col = index % size;
    if (grid[row][col] !== "") {
      return fillRemaining(grid, index + 1, blockRows, blockCols, size, seed);
    }
    const syms = shuffleArraySeeded([...symbols[size]], seed + index);
    for (const sym of syms) {
      if (isValidPlacement(grid, row, col, sym, blockRows, blockCols, size)) {
        grid[row][col] = sym;
        if (fillRemaining(grid, index + 1, blockRows, blockCols, size, seed)) {
          return true;
        }
        grid[row][col] = "";
      }
    }
    return false;
  }

  function isValidPlacement(grid, row, col, sym, blockRows, blockCols, size) {
    // Check row
    for (let c = 0; c < size; c++) {
      if (grid[row][c] === sym) return false;
    }

    // Check column
    for (let r = 0; r < size; r++) {
      if (grid[r][col] === sym) return false;
    }

    // Check block
    const blockRow = Math.floor(row / blockRows) * blockRows;
    const blockCol = Math.floor(col / blockCols) * blockCols;

    for (let r = 0; r < blockRows; r++) {
      for (let c = 0; c < blockCols; c++) {
        if (grid[blockRow + r][blockCol + c] === sym) {
          return false;
        }
      }
    }

    return true;
  }

  function generateSimpleGrid(size, dims, seed) {
    const grid = Array(size)
      .fill()
      .map(() => Array(size).fill(""));
    const syms = symbols[size];
    for (let br = 0; br < size; br += dims.rows) {
      for (let bc = 0; bc < size; bc += dims.cols) {
        fillBlock(
          grid,
          br,
          bc,
          dims.rows,
          dims.cols,
          shuffleArraySeeded([...syms], seed + br + bc)
        );
      }
    }
    return grid;
  }

  function validateGrid(grid, size) {
    const dims = blockDims[size];

    // Check all rows
    for (let row = 0; row < size; row++) {
      const rowValues = new Set();
      for (let col = 0; col < size; col++) {
        const val = grid[row][col];
        if (val === "" || rowValues.has(val)) return false;
        rowValues.add(val);
      }
    }

    // Check all columns
    for (let col = 0; col < size; col++) {
      const colValues = new Set();
      for (let row = 0; row < size; row++) {
        const val = grid[row][col];
        if (val === "" || colValues.has(val)) return false;
        colValues.add(val);
      }
    }

    // Check all blocks
    for (let br = 0; br < size; br += dims.rows) {
      for (let bc = 0; bc < size; bc += dims.cols) {
        const blockValues = new Set();
        for (let r = 0; r < dims.rows; r++) {
          for (let c = 0; c < dims.cols; c++) {
            const val = grid[br + r][bc + c];
            if (val === "" || blockValues.has(val)) return false;
            blockValues.add(val);
          }
        }
      }
    }

    return true;
  }

  function fillBlock(grid, startRow, startCol, rows, cols, values) {
    let index = 0;
    for (let r = 0; r < rows; r++) {
      for (let c = 0; c < cols; c++) {
        grid[startRow + r][startCol + c] = values[index++];
      }
    }
  }

  function renderGrid(grid, size) {
    gridContainer.innerHTML = "";
    const dims = blockDims[size];
    gridContainer.style.gridTemplateColumns = `repeat(${size}, 1fr)`;

    const validSymbols = symbols[size];

    for (let row = 0; row < size; row++) {
      for (let col = 0; col < size; col++) {
        const cell = document.createElement("div");
        cell.className = "grid-cell";

        const blockRow = Math.floor(row / dims.rows);
        const blockCol = Math.floor(col / dims.cols);
        cell.classList.add(
          (blockRow + blockCol) % 2 === 0 ? "light-blue" : "white"
        );

        const value = grid[row][col];

        if (value !== "" && solvedGrid[row][col] === value) {
          // Case 1: Clue or correctly solved entry → show fixed
          const span = document.createElement("span");
          span.textContent = value;
          span.classList.add("fixed");
          cell.appendChild(span);
        } else {
          // Case 2 & 3: User input or empty
          const input = document.createElement("input");
          input.type = "text";
          input.maxLength = 1;
          input.classList.add("input-cell");
          input.dataset.row = row;
          input.dataset.col = col;

          if (value !== "") {
            input.value = value;
            if (value === solvedGrid[row][col]) {
              input.style.color = "#1d4ed8"; // correct
              input.readOnly = true;
            } else {
              input.style.color = "red"; // wrong entry
            }
          }

          // Input validation + saving
          input.addEventListener("input", (e) => {
            let val = e.target.value.toUpperCase();

            // This prevents further life deduction or grid interaction
            if (lives <= 0) {
              e.target.value = currentGrid[row][col] || ""; // Revert to last saved value
              return;
            }

            // 1. Clean up and reset styles
            e.target.classList.remove("correct-input");
            e.target.style.color = ""; // Reset inline color (removes the red)

            // 2. Validate symbol and clear if invalid
            const isValidSymbol = validSymbols.includes(val);
            if (!isValidSymbol && val !== "") {
              e.target.value = "";
              currentGrid[row][col] = "";
              saveGameState(
                window.currentHexId,
                currentGrid,
                solvedGrid,
                lives
              );
              return;
            }

            e.target.value = val;
            currentGrid[row][col] = val; // Update progress grid

            // 3. Check for correctness and apply final styling/actions
            if (val === "") {
              // Input is empty, do nothing further
            } else if (solvedGrid && solvedGrid[row][col] == val) {
              // Correct Entry: Lock cell and style
              e.target.classList.add("correct-input"); // Applies the blue style from CSS
              e.target.readOnly = true;
              currentGrid[row][col] = val; // Ensure the correct value is saved
            } else {
              // Incorrect Entry: Apply red color and deduct life
              e.target.style.color = "red";
              lives--;
              document.getElementById("livesCount").textContent = lives;

              if (lives === 0) {
                currentGrid[row][col] = val;
                lockPuzzle(size);
                // Note: The puzzle lock will happen here, no need to set color here as well
              }
            }

            // 4. Save game state and check for win
            if (window.currentHexId)
              saveGameState(
                window.currentHexId,
                currentGrid,
                solvedGrid,
                lives
              );

            checkWin(size);
          });

          // Arrow navigation stays the same
          input.addEventListener("keydown", (e) => {
            const currentRow = parseInt(e.target.dataset.row);
            const currentCol = parseInt(e.target.dataset.col);

            const totalCells = size * size;
            let currentIndex = currentRow * size + currentCol;
            let nextIndex = currentIndex;

            switch (e.key) {
              case "ArrowUp":
                nextIndex = (currentIndex - size + totalCells) % totalCells;
                break;
              case "ArrowDown":
                nextIndex = (currentIndex + size) % totalCells;
                break;
              case "ArrowLeft":
                nextIndex = (currentIndex - 1 + totalCells) % totalCells;
                break;
              case "ArrowRight":
                nextIndex = (currentIndex + 1) % totalCells;
                break;
              default:
                return;
            }

            let searchCount = 0;
            while (searchCount < totalCells) {
              const targetRow = Math.floor(nextIndex / size);
              const targetCol = nextIndex % size;
              const nextInput = document.querySelector(
                `input[data-row="${targetRow}"][data-col="${targetCol}"]`
              );

              if (nextInput) {
                e.preventDefault();
                nextInput.focus();
                return;
              }

              switch (e.key) {
                case "ArrowUp":
                  nextIndex = (nextIndex - size + totalCells) % totalCells;
                  break;
                case "ArrowDown":
                  nextIndex = (nextIndex + size) % totalCells;
                  break;
                case "ArrowLeft":
                  nextIndex = (nextIndex - 1 + totalCells) % totalCells;
                  break;
                case "ArrowRight":
                  nextIndex = (nextIndex + 1) % totalCells;
                  break;
              }
              searchCount++;
            }
          });

          cell.appendChild(input);
        }

        gridContainer.appendChild(cell);
      }
    }
  }

  function seededRandom(seed) {
    let x = Math.sin(seed++) * 10000;
    return x - Math.floor(x);
  }

  function shuffleArraySeeded(array, seed) {
    for (let i = array.length - 1; i > 0; i--) {
      const j = Math.floor(seededRandom(seed + i) * (i + 1));
      [array[i], array[j]] = [array[j], array[i]];
    }
    return array;
  }

  function createPuzzleFromGrid(grid, size, seed) {
    const puzzle = Array(size)
      .fill()
      .map(() => Array(size).fill(""));
    const block = blockDims[size];

    const maxCluesPerUnit = size === 6 ? 3 : 5;
    const totalClueLimit = size === 6 ? 14 : 36;

    const rowClues = Array(size).fill(0);
    const colClues = Array(size).fill(0);
    const blockClues = Array(size).fill(0);

    // Symbol frequency map to ensure each appears at least once and at most 5 times
    const symbolFreq = {};
    const validSymbols = symbols[size];
    for (const sym of validSymbols) {
      symbolFreq[sym] = 0;
    }

    const allPositions = [];
    for (let r = 0; r < size; r++) {
      for (let c = 0; c < size; c++) {
        allPositions.push({ row: r, col: c });
      }
    }

    shuffleArraySeeded(allPositions, seed ^ 0xabcdef);

    let placedClues = 0;
    const rand = mulberry32(seed ^ 0x123456);

    for (const { row, col } of allPositions) {
      if (placedClues >= totalClueLimit) break;

      const blockIndex = getBlockIndex(row, col, block, size);
      const sym = grid[row][col];

      if (
        rowClues[row] < maxCluesPerUnit &&
        colClues[col] < maxCluesPerUnit &&
        blockClues[blockIndex] < maxCluesPerUnit &&
        symbolFreq[sym] < 5 &&
        rand() < 0.7 // 70% chance to accept
      ) {
        puzzle[row][col] = sym;
        rowClues[row]++;
        colClues[col]++;
        blockClues[blockIndex]++;
        symbolFreq[sym]++;
        placedClues++;
      }
    }

    // Ensure each symbol appears at least once (fallback placement if missing)
    for (const sym of validSymbols) {
      if (symbolFreq[sym] === 0) {
        outer: for (let r = 0; r < size; r++) {
          for (let c = 0; c < size; c++) {
            const blockIndex = getBlockIndex(r, c, block, size);
            if (
              puzzle[r][c] === "" &&
              rowClues[r] < maxCluesPerUnit &&
              colClues[c] < maxCluesPerUnit &&
              blockClues[blockIndex] < maxCluesPerUnit
            ) {
              puzzle[r][c] = sym;
              rowClues[r]++;
              colClues[c]++;
              blockClues[blockIndex]++;
              symbolFreq[sym]++;
              placedClues++;
              break outer;
            }
          }
        }
      }
    }

    // Uniqueness check
    if (!hasUniqueSolution(puzzle, size)) {
      return createPuzzleFromGrid(grid, size, seed + 1);
    }

    return puzzle;
  }

  function createSymmetricPuzzle(grid, size, seed) {
    const puzzle = Array(size)
      .fill()
      .map(() => Array(size).fill(""));

    const block = blockDims[size];
    const maxCluesPerUnit = size === 12 ? 7 : 9;
    const totalClueLimit = size === 12 ? 64 : 116;
    const maxSymbolLimit = size === 12 ? 7 : 9;

    const rowClues = Array(size).fill(0);
    const colClues = Array(size).fill(0);
    const blockClues = Array(size).fill(0);

    const symbolFreq = {};
    const validSymbols = symbols[size];
    for (const sym of validSymbols) symbolFreq[sym] = 0;

    const allPositions = [];

    for (let r = 0; r < size; r++) {
      for (let c = 0; c < size; c++) {
        if (r * size + c <= (size * size) / 2) {
          allPositions.push({ row: r, col: c });
        }
      }
    }

    shuffleArraySeeded(allPositions, seed);
    let placedClues = 0;

    for (const { row, col } of allPositions) {
      const symRow = size - 1 - row;
      const symCol = size - 1 - col;
      const isSame = row === symRow && col === symCol;
      const neededClues = isSame ? 1 : 2;

      if (placedClues + neededClues > totalClueLimit) continue;

      const blockIndex1 = getBlockIndex(row, col, block, size);
      const blockIndex2 = getBlockIndex(symRow, symCol, block, size);

      const val1 = grid[row][col];
      const val2 = grid[symRow][symCol];

      // Check frequency limit
      if (symbolFreq[val1] + (isSame ? 1 : 1) > maxSymbolLimit) continue;
      if (!isSame && symbolFreq[val2] + 1 > maxSymbolLimit) continue;

      // Check clue constraints
      if (
        rowClues[row] + 1 > maxCluesPerUnit ||
        colClues[col] + 1 > maxCluesPerUnit ||
        blockClues[blockIndex1] + 1 > maxCluesPerUnit
      )
        continue;

      if (
        !isSame &&
        (rowClues[symRow] + 1 > maxCluesPerUnit ||
          colClues[symCol] + 1 > maxCluesPerUnit ||
          blockClues[blockIndex2] + 1 > maxCluesPerUnit)
      )
        continue;

      // Place first clue
      puzzle[row][col] = val1;
      rowClues[row]++;
      colClues[col]++;
      blockClues[blockIndex1]++;
      symbolFreq[val1]++;
      placedClues++;

      // Place symmetric clue
      if (!isSame) {
        puzzle[symRow][symCol] = val2;
        rowClues[symRow]++;
        colClues[symCol]++;
        blockClues[blockIndex2]++;
        symbolFreq[val2]++;
        placedClues++;
      }
    }

    // Ensure each symbol appears at least once
    for (const sym of validSymbols) {
      if (symbolFreq[sym] === 0) {
        outer: for (let r = 0; r < size; r++) {
          for (let c = 0; c < size; c++) {
            const blockIndex = getBlockIndex(r, c, block, size);
            if (
              puzzle[r][c] === "" &&
              rowClues[r] < maxCluesPerUnit &&
              colClues[c] < maxCluesPerUnit &&
              blockClues[blockIndex] < maxCluesPerUnit
            ) {
              puzzle[r][c] = sym;
              rowClues[r]++;
              colClues[c]++;
              blockClues[blockIndex]++;
              symbolFreq[sym]++;
              placedClues++;
              break outer;
            }
          }
        }
      }
    }

    return puzzle;
  }

  function getBlockIndex(row, col, block, size) {
    const blockRow = Math.floor(row / block.rows);
    const blockCol = Math.floor(col / block.cols);
    const blocksPerRow = size / block.cols;
    return blockRow * blocksPerRow + blockCol;
  }

  function getFormatHexId(format) {
    const d = new Date();
    const dd = String(d.getDate()).padStart(2, "0");
    const mm = String(d.getMonth() + 1).padStart(2, "0");
    const yy = String(d.getFullYear()).slice(-2);
    return `#${
      format === "mini"
        ? "06"
        : format === "nona"
        ? "09"
        : format === "doza"
        ? "12"
        : "16"
    }${dd}${mm}${yy}`.toLowerCase();
  }

  // Backtracking solver that also counts solutions
  function solveSudoku(grid, size, block, limit = 2) {
    let solutions = 0;

    function helper(r, c) {
      if (r === size) {
        solutions++;
        return solutions < limit;
      }
      const nextR = c === size - 1 ? r + 1 : r;
      const nextC = c === size - 1 ? 0 : c + 1;

      if (grid[r][c] !== "") {
        return helper(nextR, nextC);
      }

      for (const sym of symbols[size]) {
        if (isValidPlacement(grid, r, c, sym, block.rows, block.cols, size)) {
          grid[r][c] = sym;
          if (!helper(nextR, nextC)) return false;
          grid[r][c] = "";
        }
      }
      return true;
    }

    helper(0, 0);
    return solutions;
  }

  // Check if puzzle has exactly one solution
  function hasUniqueSolution(puzzle, size) {
    const block = blockDims[size];
    const copy = puzzle.map((row) => [...row]);
    return solveSudoku(copy, size, block) === 1;
  }

  // Linear congruential generator for creating a pseudorandom number stream
  function mulberry32(a) {
    return function () {
      let t = (a += 0x6d2b79f5);
      t = Math.imul(t ^ (t >>> 15), t | 1);
      t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }

  // check Win
  function checkWin(size) {
    const inputs = document.querySelectorAll(".input-cell");
    for (let inp of inputs) {
      const r = parseInt(inp.dataset.row);
      const c = parseInt(inp.dataset.col);
      if (inp.value !== solvedGrid[r][c]) {
        return; // still incomplete
      }
    }

    //All matched → show popup
    showWinPopup(size);
  }

  //check Lose
  function lockPuzzle(size) {
    const inputs = document.querySelectorAll(".input-cell");
    inputs.forEach((inp) => (inp.disabled = true));

    showLosePopup(size);
  }

  function showWinPopup(size) {
    const overlay = document.createElement("div");
    overlay.className = "popup"; // same class

    const popup = document.createElement("div");
    popup.className = "popup-box"; // same class

    popup.innerHTML = `
    <h2>🎉Congratulations!🎊</h2>
    <p>You solved the ${size}x${size} Sudoku puzzle! Keep it up.</p>
    <div class="popup-buttons">
      <button id="mainMenuBtn">Main Menu</button>
      <button id="thanksBtn">Stay</button>
    </div>
  `;

    overlay.appendChild(popup);
    document.body.appendChild(overlay);

    document.getElementById("mainMenuBtn").onclick = () => {
      window.location.href = "index.html"; // go back to menu
    };
    document.getElementById("thanksBtn").onclick = () => {
      document.body.removeChild(overlay); // close popup
    };
  }

  function showLosePopup(size) {
    const overlay = document.createElement("div");
    overlay.className = "popup"; // same class

    const popup = document.createElement("div");
    popup.className = "popup-box"; // same class

    popup.innerHTML = `
    <h2>Game Over!😢</h2>
    <p>You lost all lives for ${size}x${size} Sudoku. Better try next time.</p>
    <div class="popup-buttons">
      <button id="mainMenuBtn">Main Menu</button>
      <button id="thanksBtn">Stay</button>
    </div>
  `;

    overlay.appendChild(popup);
    document.body.appendChild(overlay);

    document.getElementById("mainMenuBtn").onclick = () => {
      window.location.href = "index.html"; // go back to menu
    };
    document.getElementById("thanksBtn").onclick = () => {
      document.body.removeChild(overlay); // close popup
    };
  }
});
