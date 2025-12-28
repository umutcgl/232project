# SMPL Assembler - CSE 232 Systems Programming Term Project

## Project Overview

This is a **2-Pass Assembler** for the SMPL (Simple) assembly language, developed as part of the CSE 232 Systems Programming course at Yeditepe University.

The complete project consists of:
- **Assembler** (Pass 1 + Pass 2) - Translates SMPL assembly to relocatable object code
- **Linker** - Resolves external references between modules
- **Loader** - Loads executable into memory at a given load point

This repository contains the **Assembler component** (Pass 1 and Pass 2).

---

## Implementation Status

### ✅ Completed

**Assembler (Pass 1 + Pass 2)** - Fully implemented and tested

| Component | File | Description |
|-----------|------|-------------|
| Parser | `parser.c` | Separates label, opcode, operand fields |
| Pass 1 | `pass1_codegen.c` | Builds ST, FRT, DAT, HDRM tables; generates `.s` file |
| Pass 2 | `pass2.c` | Resolves forward references; generates `.o` and `.t` files |

**Pass 2 Details:**
1. Reads the `.s` file (intermediate code from Pass 1) line by line
2. For each line, checks if the Location Counter (LC) is in the Forward Reference Table (FRT)
3. If found in FRT → looks up the symbol address from Symbol Table (ST) and patches the object code
4. If not in FRT → writes the line unchanged to `.o` file
5. Writes the DAT and HDRM tables to the `.t` file

### 🔲 To Be Implemented

| Component | Description |
|-----------|-------------|
| **Linker** | Resolves external references between modules, generates `.exe` file |
| **Loader** | Loads executable into memory array at given load point |

---

## How to Build

### On Linux (Required for Submission)
```bash
make
```

Or manually:
```bash
gcc -o assembler main.c parser.c pass1_codegen.c pass2.c -Wall -std=c99
```

### On Windows
```batch
gcc -o assembler.exe main.c parser.c pass1_codegen.c pass2.c -Wall
```

---

## How to Run

```bash
./assembler <input_file.asm>
```

Example:
```bash
./assembler main_prog.asm
```

### Input
- `.asm` file containing SMPL assembly code

### Output
- `.s` file - Intermediate code (from Pass 1)
- `.o` file - Final object code (from Pass 2)
- `.t` file - DAT and HDRM tables

---

## Test Files

Three test modules from the project specification are included:

| File | Description |
|------|-------------|
| `main_prog.asm` | Main program with EXTREF |
| `add_module.asm` | Subroutine module with ENTRY |
| `data_module.asm` | Data module with ENTRY |

### Run All Tests
```bash
make test
```

---

## SMPL Instruction Set

| Mnemonic | Opcode | Bytes | Mode | Description |
|----------|--------|-------|------|-------------|
| ADD | A1/A2 | 3/2 | Direct/Immediate | AC ← AC + M[]/n |
| SUB | A3/A4 | 3/2 | Direct/Immediate | AC ← AC - M[]/n |
| LDA | E1/E2 | 3/2 | Direct/Immediate | AC ← M[]/n |
| STA | F1 | 3 | Direct | M[] ← AC |
| BEQ | B1 | 3 | Relative | Branch if AC = 0 |
| BGT | B2 | 3 | Relative | Branch if AC > 0 |
| BLT | B3 | 3 | Relative | Branch if AC < 0 |
| JMP | B4 | 3 | Direct | Unconditional jump |
| CLL | C1 | 3 | Direct | Call subroutine |
| RET | C2 | 1 | Implied | Return |
| DEC | D1 | 1 | Implied | AC ← AC - 1 |
| INC | D2 | 1 | Implied | AC ← AC + 1 |
| HLT | FE | 1 | Implied | Halt |

### Pseudo-Operations

| Directive | Description |
|-----------|-------------|
| PROG | Program/module name |
| START | Beginning of program (optional start address) |
| END | End of program |
| BYTE n | Allocate 1 byte with value n |
| WORD n | Allocate 2 bytes with value n |
| ENTRY | Define entry points (exported symbols) |
| EXTREF | Declare external references |

---

## File Structure

```
├── main.c           # Driver program
├── parser.c         # Line parser
├── pass1_codegen.c  # Pass 1: Symbol table, code generation
├── pass2.c          # Pass 2: Forward reference resolution
├── asm_common.h     # Common data structures
├── Makefile         # Build script for Linux
├── main_prog.asm    # Test: Main program
├── add_module.asm   # Test: Add subroutine module
└── data_module.asm  # Test: Data module
```

---

## Data Structures (from Project Spec)

```c
struct SymbolTable {
    char symbol[10];
    int address;
};

struct ForwardRefTable {
    int address;
    char symbol[10];
};

struct DirectAdrTable {
    int address;
};

struct HDRMTable {
    char code;       // 'H', 'D', 'R', or 'M'
    char symbol[10];
    int address;
};
```

---

## Example Output

### Input: main_prog.asm
```asm
PROG MAIN
EXTREF AD5,XX,ZZ
START
LOOP: LDA XX
CLL AD5
ADD ZZ
CLL AD5
STA 70
LDA ZZ
SUB #1
BLT EX
JMP LOOP
EX: HLT
END
```

### Output: main_prog.o (Object Code)
```
0000  E1  00 00    ; LDA XX (external)
0003  C1  00 00    ; CLL AD5 (external)
0006  A1  00 00    ; ADD ZZ (external)
0009  C1  00 00    ; CLL AD5 (external)
000C  F1  00 46    ; STA 70 (70 decimal = 0x46)
000F  E1  00 00    ; LDA ZZ (external)
0012  A4  01       ; SUB #1
0014  B3  00 1A    ; BLT EX (EX at 0x1A, forward ref RESOLVED)
0017  B4  00 00    ; JMP LOOP (LOOP at 0x00)
001A  FE           ; HLT
```

### Output: main_prog.t (Tables)
```
DAT
1
4
7
A
10
18
HDRM
H MAIN 0 1B
R AD5
R XX
R ZZ
M XX 1
M AD5 4
M ZZ 7
M AD5 A
M ZZ 10
```

---

## Team

CSE 232 Systems Programming - Fall 2025
Yeditepe University, Department of Computer Engineering

---

## License

Academic project - for educational purposes only.
