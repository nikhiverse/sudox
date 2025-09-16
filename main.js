document.addEventListener("DOMContentLoaded", function () {
  const randomBtn = document.getElementById("randomBtn");
  const gridSizeSelect = document.getElementById("gridSize");
  const gridContainer = document.getElementById("gridContainer");
  const downloadBtn = document.getElementById("downloadBtn");

  // Define symbols and block dimensions
  const symbols = {
    6: ["1", "2", "3", "4", "5", "6"],
    9: ["1", "2", "3", "4", "5", "6", "7", "8", "9"],
    12: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C"],
    16: [
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
    ],
  };

  const blockDims = {
    6: { rows: 2, cols: 3 },
    9: { rows: 3, cols: 3 },
    12: { rows: 3, cols: 4 },
    16: { rows: 4, cols: 4 },
  };

  let currentGrid = [];
  let currentSize = parseInt(gridSizeSelect.value);
  let solvedGrid = null; // always holds the current puzzle's full solution
  let lives = 0; // lives remaining

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

    document.getElementById(
      "puzzleHeading"
    ).textContent = `Sudoku ${puzzleName} Puzzle`;
    document.getElementById("puzzleHeading").classList.remove("hidden");

    // Show puzzle ID in heading
    document.getElementById("puzzleIdText").textContent = hexId;

    //Show lives in heading
    if (puzzleName == "Mini") lives = 1;
    else if (puzzleName == "Nona") lives = 3;
    else if (puzzleName == "Doza") lives = 4;
    else if (puzzleName == "Hexa") lives = 5;

    document.getElementById("livesCount").textContent = lives;

    document.getElementById("puzzleIdLine").classList.remove("hidden");

    // Generate grid
    const solution = generateValidGrid(currentSize, seed);

    if (currentSize === 12 || currentSize === 16) {
      currentGrid = createSymmetricPuzzle(solution, currentSize, seed);
    } else {
      // FIX: Pass the seed to the function for 9x9 and 6x6
      currentGrid = createPuzzleFromGrid(solution, currentSize, seed);
    }

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

        if (value !== "") {
          const span = document.createElement("span");
          span.textContent = value;
          span.classList.add("fixed");
          cell.appendChild(span);
        } else {
          const input = document.createElement("input");
          input.setAttribute("type", "text");
          input.setAttribute("maxlength", "1");
          input.classList.add("input-cell");
          input.dataset.row = row;
          input.dataset.col = col;

          // Validate input
          input.addEventListener("input", (e) => {
            let val = e.target.value.toUpperCase();

            //Invalid input → clear
            if (!validSymbols.includes(val)) {
              e.target.value = "";
              return;
            }

            e.target.value = val;

            // Correct input
            if (solvedGrid && solvedGrid[row][col] === val) {
              e.target.style.color = "#1d4ed8"; // correct
              e.target.readOnly = true; // lock cell
            } else {
              // Wrong input
              e.target.style.color = "red";

              lives--;
              document.getElementById("livesCount").textContent = lives;

              if (lives === 0) lockPuzzle(size);
            }

            // Always check win after any move
            checkWin(size);
          });

          // FIX: Updated keydown listener to properly skip obstacles
          input.addEventListener("keydown", (e) => {
            const currentRow = parseInt(e.target.dataset.row);
            const currentCol = parseInt(e.target.dataset.col);

            const totalCells = size * size;
            let currentIndex = currentRow * size + currentCol;
            let nextIndex = currentIndex;

            switch (e.key) {
              case "ArrowUp":
                // Move backward in the linear index
                nextIndex = (currentIndex - size + totalCells) % totalCells;
                break;
              case "ArrowDown":
                // Move forward in the linear index
                nextIndex = (currentIndex + size) % totalCells;
                break;
              case "ArrowLeft":
                // Move backward, handle wrap-around for row end to beginning
                nextIndex = (currentIndex - 1 + totalCells) % totalCells;
                break;
              case "ArrowRight":
                // Move forward, handle wrap-around for row beginning to end
                nextIndex = (currentIndex + 1) % totalCells;
                break;
              default:
                return; // Exit if not an arrow key
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

              // Move to the next cell to check, based on key direction
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
        ? 12
        : 16
    }${dd}${mm}${yy}`.toLowerCase();
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
    inputs.forEach((inp) => (inp.readOnly = true));

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
