#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────
# build.sh — Compile all SudoX C++ puzzle generators
# ──────────────────────────────────────────────────────────
# Usage:
#   chmod +x build.sh
#   ./build.sh            # compile all generators
#   ./build.sh sudoku9    # compile a single generator
# ──────────────────────────────────────────────────────────

set -euo pipefail

GAME_DIR="$(cd "$(dirname "$0")/game" && pwd)"
EXE_DIR="${GAME_DIR}/exe"
CLUES_DIR="$(cd "$(dirname "$0")/clues" && pwd)"

# Compiler settings
CXX="${CXX:-g++}"
CXXFLAGS="-O2 -std=c++17"

# Colours (disabled when not in a terminal)
if [ -t 1 ]; then
  GREEN='\033[0;32m'; RED='\033[0;31m'; CYAN='\033[0;36m'; NC='\033[0m'
else
  GREEN=''; RED=''; CYAN=''; NC=''
fi

# Ensure the exe output directory exists
mkdir -p "$EXE_DIR"

# Gather list of source files to compile
if [ $# -gt 0 ]; then
  # Compile specific game(s) passed as arguments
  SOURCES=()
  for name in "$@"; do
    src="${GAME_DIR}/${name}.cpp"
    if [ ! -f "$src" ]; then
      echo -e "${RED}✗ Source not found: ${name}.cpp${NC}"
      exit 1
    fi
    SOURCES+=("$src")
  done
else
  # Compile all .cpp files in game/
  SOURCES=("${GAME_DIR}"/*.cpp)
fi

echo -e "${CYAN}╔══════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║       SudoX — Building Generators        ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════╝${NC}"
echo ""

PASS=0
FAIL=0

for src in "${SOURCES[@]}"; do
  name="$(basename "$src" .cpp)"
  bin="${EXE_DIR}/${name}"

  printf "  %-24s" "${name}"

  # Skip if binary is newer than source
  if [ -f "$bin" ] && [ "$bin" -nt "$src" ]; then
    echo -e "${GREEN}✓ up to date${NC}"
    ((PASS++))
    continue
  fi

  # Find include paths (clues directory has shared headers)
  if $CXX $CXXFLAGS -I"$CLUES_DIR" -o "$bin" "$src" 2>/tmp/sudox_build_err_$$; then
    echo -e "${GREEN}✓ compiled${NC}"
    ((PASS++))
  else
    echo -e "${RED}✗ FAILED${NC}"
    cat /tmp/sudox_build_err_$$ 2>/dev/null
    ((FAIL++))
  fi
  rm -f /tmp/sudox_build_err_$$
done

echo ""
echo -e "${CYAN}──────────────────────────────────────────${NC}"
echo -e "  ${GREEN}Passed: ${PASS}${NC}   ${RED}Failed: ${FAIL}${NC}"
echo -e "${CYAN}──────────────────────────────────────────${NC}"

if [ "$FAIL" -gt 0 ]; then
  exit 1
fi
