# SudoX — Sudoku Puzzle Variants

SudoX is a web-based Sudoku application that features a variety of Sudoku variants, ranging from classic mini and 9x9 grids to complex overlapping variants like Twodoku and Windoku.

The puzzles are generated cleanly without generic aesthetics using a highly optimized C++ engine, served via a Node.js backend to a beautifully styled, responsive vanilla HTML/CSS/JavaScript frontend. SudoX offers a premium user experience featuring auto-saving, visual helper modes, responsive layout, dark/light theme support, and PDF download capabilities.

## Features

- **Wide Variety of Puzzle Types**:
  - Classic Sudoku (`mini`, `easy`, `9`, `12` sizes)
  - Variants (`X`, `A`, `dozaku`)
  - Windoku Series (`windoku`, `windokuX`, `windoku_jigsaw`)
  - Jigsaw Series (`jigsaw8`, `jigsaw9`, `jigsawX`)
  - Interlocking Twodokus (`twodoku_mini`, `twodoku8`, `twodoku9`)
- **Fast, Deterministic Generation**: Powered by a unified C++ generation engine.
- **Premium UI**: 
  - Dynamic responsive scaling.
  - Native Light / Dark color themes.
  - Contextual cursor / crosshair highlighting.
- **Save to PDF**: Built-in support to snapshot puzzle grids into PDFs using `jspdf` and `html2canvas`.

## Tech Stack

- **Frontend**: Vanilla HTML5, CSS3, JavaScript.
- **Backend API**: Node.js (Express). 
- **Generator Core**: C++17.

## Running Locally

To run the application locally, you will need **Node.js** and a **C++ Compiler (g++)** installed.

1. **Install Dependencies**:
   ```bash
   npm install
   ```

2. **Pre-build Generators** (Optional but recommended):
   A build script is included that will selectively compile all C++ binaries to a `game/exe` directory:
   ```bash
   chmod +x build.sh
   ./build.sh
   ```

3. **Start the Development Server**:
   ```bash
   npm start
   ```

4. **Play**: Open your browser to `http://localhost:3000`.

## Deployment (Important Note on GitHub Pages)

**GitHub Pages only provides static file hosting.** If you deploy this repository directly to GitHub Pages, the frontend will work, but **puzzle generation will fail**. This is because SudoX relies on a Node.js Express server (`server.js`) to run local C++ executable binaries on the fly and deliver JSON puzzles.

### How to deploy:
To host the fully functional app, including live puzzle generation, you must use a platform capable of running a Node.js server and executing binaries. Recommended free or low-cost options include:

- **Render** (Recommended)
- **Railway**
- **Fly.io**

When deploying to these platforms, ensure you configure the **Build Command** to build the binaries before starting the app:
* **Build Command**: `chmod +x build.sh && ./build.sh && npm install`
* **Start Command**: `npm start`

*(Alternatively, to run entirely on GitHub Pages, you would need to pre-generate a massive cache of JSON puzzles and alter `main.js` to fetch from these static files rather than the live API. The current architecture uses the live Express backend).*
