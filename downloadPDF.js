// ═══════════════════════════════════════════════════════════════
//  downloadPDF.js  —  SudoX puzzle PDF export
//
//  Depends on:
//    • jsPDF  (loaded via CDN in index.html as window.jspdf)
//    • Global state from main.js:
//        puzzleTitle, puzzleIdEl, totalRows, totalCols, cellAt()
//
//  To customise the PDF just edit the sections marked  ── ✏️ ──
// ═══════════════════════════════════════════════════════════════

// ── ✏️  COLOUR PALETTE (light-mode, matches puzz.css tokens) ──
const PDF_PALETTE = {
  warmBg:    [254, 249, 240],   // --surface-bg
  cardBg:    [255, 253, 248],   // --surface-card
  primary:   [180,  83,   9],   // --primary  (amber-brown)
  textMain:  [ 45,  42,  36],   // --text-primary
  textMuted: [156, 144, 132],   // --text-muted
  gridDark:  [ 45,  42,  36],   // --grid-bg  (cell borders / gaps)
  clueText:  [ 45,  42,  36],   // given clue numbers
  altCell:   [254, 243, 199],   // --grid-alt-cell  (checker blocks)
  diagCell:  [180,  83,   9],   // diagonal / window accent bg
  winCell:   [180,  83,   9],
  correctBg: [239, 246, 255],   // --correct-bg
  correctTx: [ 29,  78, 216],   // --correct-text
  wrongBg:   [255, 241, 242],   // --wrong-bg
  wrongTx:   [190,  18,  60],   // --wrong-text
  whitish:   [255, 253, 248],   // normal cell background
};

// ── ✏️  PAGE LAYOUT (all values in mm, A4 portrait) ──
const PDF_LAYOUT = {
  pageW:      210,
  pageH:      297,
  margin:      18,   // left / right margin
  headerH:     36,   // height of the top header band
  footerGap:   22,   // space reserved at the bottom for the footer
  sectionGap:  10,   // gap between header band and "PUZZLE" label + grid
};

// ── ✏️  FOOTER TEXT ──
const PDF_FOOTER_TEXT = "© rathodnk — SudoX Sudoku Variants";

// ── ✏️  BORDER WIDTHS (mm) ──
const PDF_BORDER = {
  thin:  0.6,   // soft inner cell borders
  thick: 1.4,   // box / sub-grid dividers
};

// ───────────────────────────────────────────────────────────────
//  Internal helpers
// ───────────────────────────────────────────────────────────────

/** Draw the header band with logo, brand name, variant, puzzle id, date. */
function _drawHeader(doc, layout, palette, variantName, pidText) {
  const { pageW, margin, headerH } = layout;
  const { cardBg, primary, textMain, textMuted } = palette;

  // Background
  doc.setFillColor(...cardBg);
  doc.rect(0, 0, pageW, headerH, "F");

  // Bottom rule
  doc.setDrawColor(...primary);
  doc.setLineWidth(0.7);
  doc.line(0, headerH, pageW, headerH);

  // Logo badge  "SX"
  doc.setFillColor(...primary);
  doc.roundedRect(margin, 8, 12, 12, 2, 2, "F");
  doc.setFont("Courier", "bold");
  doc.setFontSize(7);
  doc.setTextColor(255, 253, 248);
  doc.text("SX", margin + 6, 16.5, { align: "center" });

  // Brand name
  doc.setFont("Times", "bold");
  doc.setFontSize(15);
  doc.setTextColor(...textMain);
  doc.text("SudoX", margin + 16, 17);

  // Variant name  (right side)
  doc.setFont("Times", "bold");
  doc.setFontSize(13);
  doc.setTextColor(...primary);
  doc.text(variantName, pageW - margin, 14, { align: "right" });

  // Puzzle ID
  doc.setFont("Courier", "normal");
  doc.setFontSize(9);
  doc.setTextColor(...textMuted);
  doc.text(pidText, pageW - margin, 22, { align: "right" });

  // Date
  const dateStr = new Date().toLocaleDateString(undefined, {
    year: "numeric", month: "long", day: "numeric",
  });
  doc.setFont("Helvetica", "normal");
  doc.setFontSize(8);
  doc.setTextColor(...textMuted);
  doc.text(dateStr, pageW - margin, 30, { align: "right" });
}

/** Draw the "PUZZLE" section label and return the updated yPos. */
function _drawSectionLabel(doc, layout, palette, yPos) {
  const { margin } = layout;
  const { primary } = palette;

  doc.setFont("Helvetica", "bold");
  doc.setFontSize(8);
  doc.setTextColor(...primary);
  doc.setCharSpace(1.5);
  doc.text("PUZZLE", margin, yPos);
  doc.setCharSpace(0);

  return yPos + 6;
}

/** Draw every cell (background + number) and all borders. */
function _drawGrid(doc, layout, palette, border, yPos) {
  const { pageW, pageH, margin, footerGap } = layout;
  const contentW = pageW - margin * 2;

  const {
    gridDark, clueText, altCell, diagCell, winCell,
    correctBg, correctTx, wrongBg, wrongTx, whitish, primary,
  } = palette;

  // Fit grid into the remaining vertical space
  const maxGridW = contentW;
  const maxGridH = pageH - yPos - footerGap;
  const cellMM = Math.min(
    Math.floor(maxGridW / totalCols),
    Math.floor(maxGridH / totalRows),
  );
  const gridW = cellMM * totalCols;
  const gridH = cellMM * totalRows;
  const gridX = margin + (contentW - gridW) / 2;

  // ── ✏️  Font size: cellMM is in mm, jsPDF fontSize is in pt (1 mm = 2.835 pt) ──
  const fontSize = Math.max(8, cellMM * 2.835 * 0.62);

  // ── Pass 1: backgrounds ──
  for (let r = 0; r < totalRows; r++) {
    for (let c = 0; c < totalCols; c++) {
      const el = cellAt(r, c);
      const x  = gridX + c * cellMM;
      const y  = yPos  + r * cellMM;

      if (!el || el.classList.contains("inactive-cell")) {
        doc.setFillColor(...gridDark);
        doc.rect(x, y, cellMM, cellMM, "F");
        continue;
      }

      // Background colour
      let bg = whitish;
      if      (el.classList.contains("both-cell"))      bg = diagCell;
      else if (el.classList.contains("diagonal-cell"))  bg = diagCell;
      else if (el.classList.contains("window-cell"))    bg = winCell;
      else if (el.classList.contains("correct"))        bg = correctBg;
      else if (el.classList.contains("wrong"))          bg = wrongBg;
      else if (el.classList.contains("alt-block"))      bg = altCell;

      doc.setFillColor(...bg);
      doc.rect(x, y, cellMM, cellMM, "F");

      // Number
      const val = el.textContent.trim();
      if (val) {
        let tx = clueText;
        if (
          el.classList.contains("diagonal-cell") ||
          el.classList.contains("window-cell")   ||
          el.classList.contains("both-cell")
        ) {
          tx = [255, 253, 248];
        } else if (el.classList.contains("correct")) {
          tx = correctTx;
        } else if (el.classList.contains("wrong")) {
          tx = wrongTx;
        } else if (!el.classList.contains("clue")) {
          tx = primary;   // user-entered value
        }
        doc.setFont("Courier", "bold");
        doc.setFontSize(fontSize);
        doc.setTextColor(...tx);
        doc.text(val, x + cellMM / 2, y + cellMM * 0.63, { align: "center" });
      }
    }
  }

  // ── Pass 2: borders ──
  const parseBorder = (bStr) => {
    if (!bStr) return null;
    return (bStr.includes("2.5px") || bStr.includes("2px"))
      ? border.thick
      : border.thin;
  };

  for (let r = 0; r < totalRows; r++) {
    for (let c = 0; c < totalCols; c++) {
      const el = cellAt(r, c);
      if (!el || el.classList.contains("inactive-cell")) continue;

      const x  = gridX + c * cellMM;
      const y  = yPos  + r * cellMM;
      const st = el.style;

      doc.setDrawColor(...gridDark);

      const sides = [
        { w: parseBorder(st.borderTop),    x1: x,           y1: y,           x2: x + cellMM, y2: y           },
        { w: parseBorder(st.borderBottom), x1: x,           y1: y + cellMM,  x2: x + cellMM, y2: y + cellMM  },
        { w: parseBorder(st.borderLeft),   x1: x,           y1: y,           x2: x,          y2: y + cellMM  },
        { w: parseBorder(st.borderRight),  x1: x + cellMM,  y1: y,           x2: x + cellMM, y2: y + cellMM  },
      ];

      for (const s of sides) {
        if (s.w !== null) {
          doc.setLineWidth(s.w);
          doc.line(s.x1, s.y1, s.x2, s.y2);
        }
      }
    }
  }

  // Outer border
  doc.setDrawColor(...gridDark);
  doc.setLineWidth(border.thick);
  doc.rect(gridX, yPos, gridW, gridH, "S");

  return yPos + gridH;
}

/** Draw the footer rule and copyright line. */
function _drawFooter(doc, layout, palette, footerText) {
  const { pageW, pageH, margin } = layout;
  const { primary, textMuted } = palette;

  const footerY = pageH - 16;

  doc.setDrawColor(...primary);
  doc.setLineWidth(0.4);
  doc.line(margin, footerY - 6, pageW - margin, footerY - 6);

  doc.setFont("Helvetica", "normal");
  doc.setFontSize(8);
  doc.setTextColor(...textMuted);
  doc.text(footerText, pageW / 2, footerY, { align: "center" });
}

// ───────────────────────────────────────────────────────────────
//  Main export function  —  called by the button in main.js
// ───────────────────────────────────────────────────────────────

/**
 * generatePuzzlePDF()
 * Builds and triggers download of a coloured A4 PDF for the
 * currently loaded puzzle.  Reads state (totalRows, totalCols,
 * cellAt, puzzleTitle, puzzleIdEl) from the global scope set
 * by main.js.
 */
async function generatePuzzlePDF() {
  const { jsPDF } = window.jspdf;
  const doc = new jsPDF({ orientation: "portrait", unit: "mm", format: "a4" });

  const layout  = PDF_LAYOUT;
  const palette = PDF_PALETTE;
  const border  = PDF_BORDER;

  const variantName = puzzleTitle.textContent || "Sudoku";
  const pidText     = puzzleIdEl.textContent  || "";

  // Page background
  doc.setFillColor(...palette.warmBg);
  doc.rect(0, 0, layout.pageW, layout.pageH, "F");

  // Header
  _drawHeader(doc, layout, palette, variantName, pidText);

  // Section label
  let yPos = layout.headerH + layout.sectionGap;
  yPos = _drawSectionLabel(doc, layout, palette, yPos);

  // Grid
  const gridEl = document.querySelector(".sudoku-grid");
  if (gridEl && totalRows > 0 && totalCols > 0) {
    yPos = _drawGrid(doc, layout, palette, border, yPos);
  }

  // Footer
  _drawFooter(doc, layout, palette, PDF_FOOTER_TEXT);

  // Save — filename = puzzleId (digits only) or fallback
  const pid = pidText.replace(/[^a-zA-Z0-9_-]/g, "");
  doc.save(`${pid || "sudox_puzzle"}.pdf`);
}

// ───────────────────────────────────────────────────────────────
//  Download is triggered via the hamburger menu in main.js
//  which calls generatePuzzlePDF() directly.
// ───────────────────────────────────────────────────────────────

