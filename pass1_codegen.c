#include "asm_common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

char module_name[10];
int prog_start = 0;
int prog_len = 0;

int LC = 0;

struct SymbolTable     ST[10];
struct ForwardRefTable FRT[20];
struct DirectAdrTable  DAT[30];
struct HDRMTable       HDRMT[20];
struct Memory          M[500];

// Opcode Table from Project Spec
// "In order to simplify type conversion ... you may use integers for addresses and 2-character strings for opcodes."
OpcodeEntry OPTAB[] = {
    {"ADD", "A1", 3}, // Direct
    {"BEQ", "B1", 3}, // Relative
    {"BGT", "B2", 3}, // Relative
    {"BLT", "B3", 3}, // Relative
    {"CLL", "C1", 3}, // Direct
    {"DEC", "D1", 1}, // Implied
    {"INC", "D2", 1}, // Implied
    {"JMP", "B4", 3}, // Direct
    {"LDA", "E1", 3}, // Direct
    {"RET", "C2", 1}, // Implied
    {"STA", "F1", 3}, // Direct
    {"SUB", "A3", 3}, // Direct
    {"HLT", "FE", 1}  // Implied
};
// Notes: 
// ADD immediate -> A2 (2 bytes)
// LDA immediate -> E2 (2 bytes)
// SUB immediate -> A4 (2 bytes)
// These are special cased in logic since tables link 1 mnemonic to 1 opcode usually.

int OPTAB_SIZE = sizeof(OPTAB) / sizeof(OPTAB[0]);

const OpcodeEntry* find_opcode_entry(const char *mnemonic) {
    for (int i = 0; i < OPTAB_SIZE; i++) {
        if (strcmp(OPTAB[i].mnemonic, mnemonic) == 0)
            return &OPTAB[i];
    }
    return NULL;
}

const char* get_opcode_hex(const char* mnemonic, AddrMode mode) {
    // Handle Immediate variations defined in text
    if (mode == AM_IMMEDIATE) {
        if (strcmp(mnemonic, "ADD") == 0) return "A2";
        if (strcmp(mnemonic, "LDA") == 0) return "E2";
        if (strcmp(mnemonic, "SUB") == 0) return "A4";
    }
    
    const OpcodeEntry* e = find_opcode_entry(mnemonic);
    if (!e) return "00";
    return e->opcode_str;
}

int get_instr_size(const char* mnemonic, AddrMode mode) {
    if (mode == AM_IMMEDIATE) return 2;
    if (mode == AM_IMPLIED) return 1;
    // Direct or Relative -> 3
    return 3;
}

// --- Tables Helpers ---

int insert_symbol(const char *label, int address) {
    for (int i = 0; i < 10; i++) {
        if (strcmp(ST[i].symbol, label) == 0) {
            // Re-definition check? Code example shows ENTRY defined before label? 
            // The text says "ENTRY Entry points". Usage example "ENTRY AD5... AD5: ADD".
            // So definition comes later.
            // If it exists in ST already with address?
            // "Update ST... when a label is defined".
            // Parsing loop handles label: definitions.
            fprintf(stderr, "ERROR: Duplicate symbol %s\n", label);
            return -1;
        }
    }
    for (int i = 0; i < 10; i++) {
        if (ST[i].symbol[0] == '\0') {
            strcpy(ST[i].symbol, label);
            ST[i].address = address;
            return 0;
        }
    }
    fprintf(stderr, "ERROR: Symbol table full\n");
    return -1;
}

int find_symbol_address(const char *label) {
    for (int i = 0; i < 10; i++) {
        if (strcmp(ST[i].symbol, label) == 0)
            return ST[i].address;
    }
    return -1;
}

int insert_frt(const char *symbol, int address) {
    for (int i = 0; i < 20; i++) {
        if (FRT[i].symbol[0] == '\0') {
            strcpy(FRT[i].symbol, symbol);
            FRT[i].address = address;
            return 0;
        }
    }
    fprintf(stderr, "ERROR: Forward reference table full\n");
    return -1;
}

int insert_hdrm(char code, const char *symbol, int address) {
    for (int i = 0; i < 20; i++) {
        if (HDRMT[i].code == 0) {
            HDRMT[i].code = code;
            if (symbol) strcpy(HDRMT[i].symbol, symbol);
            else HDRMT[i].symbol[0] = '\0';
            HDRMT[i].address = address;
            return 0;
        }
    }
    fprintf(stderr, "ERROR: HDRM table full\n");
    return -1;
}

int is_external(const char *symbol) {
    for (int i = 0; i < 20; i++) {
        if (HDRMT[i].code == 'R' && strcmp(HDRMT[i].symbol, symbol) == 0)
            return 1;
    }
    return 0;
}

int insert_dat(int address) {
    for (int i = 0; i < 30; i++) {
        if (DAT[i].address == -1) {
            DAT[i].address = address;
            return 0;
        }
    }
    fprintf(stderr, "ERROR: DAT table full\n");
    return -1;
}

void init_pass1(void) {
    LC = 0;
    prog_start = 0;
    prog_len = 0;
    memset(module_name, 0, sizeof(module_name));
    memset(ST, 0, sizeof(ST));
    memset(FRT, 0, sizeof(FRT));
    memset(HDRMT, 0, sizeof(HDRMT));
    memset(M, 0, sizeof(M));
    for(int i=0; i<30; i++) DAT[i].address = -1;
}

// --- Parsing Helpers ---

int parse_immediate_value(const char *operand) {
    if (operand[0] == '#')
        return atoi(operand + 1);
    return 0;
}

int parse_word_value(const char *op) {
    return atoi(op);
}

int hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

int parse_hex_byte(const char *s) {
    return (hex_to_int(s[0]) << 4) | hex_to_int(s[1]);
}

// --- Main Processing ---

void process_parsed_line_pass1(const ParsedLine *pl, FILE *sout) {
    if (pl->kind == LINE_EMPTY || pl->kind == LINE_COMMENT)
        return;

    // Pseudo-ops
    if (pl->kind == LINE_PSEUDO || pl->kind == LINE_END) {
        if (strcmp(pl->opcode, "PROG") == 0) {
            if (pl->operand[0]) strncpy(module_name, pl->operand, 9);
            return;
        }
        if (strcmp(pl->opcode, "START") == 0) {
             if (pl->operand[0]) LC = atoi(pl->operand); 
             prog_start = LC;
             return;
        }
        if (strcmp(pl->opcode, "END") == 0) {
             prog_len = LC - prog_start;
             return;
        }
        if (strcmp(pl->opcode, "ENTRY") == 0) {
            char temp[32];
            strncpy(temp, pl->operand, 31);
            temp[31] = '\0';
            char *token = strtok(temp, ", \t");
            while (token) {
                insert_hdrm('D', token, 0); 
                token = strtok(NULL, ", \t");
            }
            return;
        }
        if (strcmp(pl->opcode, "EXTREF") == 0) {
            char temp[32];
            strncpy(temp, pl->operand, 31);
            temp[31] = '\0';
            char *token = strtok(temp, ", \t");
            while (token) {
                insert_hdrm('R', token, 0);
                token = strtok(NULL, ", \t");
            }
            return;
        }
    }

    if (pl->label[0] != '\0') {
        insert_symbol(pl->label, LC);
    }

    if (strcmp(pl->opcode, "WORD") == 0) {
        int value = parse_word_value(pl->operand);
        fprintf(sout, "%04X  %02X %02X\n", LC, (value >> 8) & 0xFF, value & 0xFF);
        LC += 2;
        return;
    }

    if (strcmp(pl->opcode, "BYTE") == 0) {
        if (pl->operand[0] == 'C') {
             const char *p = pl->operand + 2;
             int addr = LC;
             while (*p && *p != '\'') {
                 fprintf(sout, "%04X  %02X\n", addr, (unsigned char)*p);
                 addr++; p++;
             }
             LC = addr;
             return;
        }
        if (pl->operand[0] == 'X') {
             const char *p = pl->operand + 2;
             int addr = LC;
             while (*p && *p != '\'') {
                 int byte = parse_hex_byte(p);
                 fprintf(sout, "%04X  %02X\n", addr, byte);
                 addr++; p+=2;
             }
             LC = addr;
             return;
        }
        int val = atoi(pl->operand);
        fprintf(sout, "%04X  %02X\n", LC, val & 0xFF);
        LC += 1;
        return;
    }

    const char* op_hex = get_opcode_hex(pl->opcode, pl->addr_mode);
    int instr_size = get_instr_size(pl->opcode, pl->addr_mode);

    if (strcmp(op_hex, "00") == 0 && pl->kind == LINE_INSTR) {
        fprintf(stderr, "ERROR: Unknown opcode %s\n", pl->opcode);
        return;
    }

    int oldLC = LC;
    LC += instr_size;

    if (pl->addr_mode == AM_IMPLIED) {
        fprintf(sout, "%04X  %s\n", oldLC, op_hex);
    }
    else if (pl->addr_mode == AM_IMMEDIATE) {
        int val = parse_immediate_value(pl->operand);
        if (instr_size == 2) {
             fprintf(sout, "%04X  %s  %02X\n", oldLC, op_hex, val & 0xFF);
        } else {
             fprintf(sout, "%04X  %s  %02X%02X\n", oldLC, op_hex, (val>>8)&0xFF, val&0xFF);
        }
    }
    else if (pl->addr_mode == AM_DIRECT || pl->addr_mode == AM_RELATIVE) {
        // Check if operand is a numeric literal address (e.g., STA 70)
        int is_numeric = 1;
        for (int i = 0; pl->operand[i]; i++) {
            if (!isdigit((unsigned char)pl->operand[i])) {
                is_numeric = 0;
                break;
            }
        }

        if (is_numeric && pl->operand[0] != '\0') {
            // Pure numeric address - use directly
            int num_addr = atoi(pl->operand);
            if (pl->addr_mode == AM_DIRECT) {
                // For numeric addresses, we still add to DAT for relocation
                // But per spec example "STA 70" -> "F1 00 70", address 70 is absolute
                // We DON'T add numeric literals to DAT (they're absolute, not relocatable)
            }
            fprintf(sout, "%04X  %s  %02X %02X\n", oldLC, op_hex, (num_addr >> 8) & 0xFF, num_addr & 0xFF);
        } else {
            // Symbol operand
            if (pl->addr_mode == AM_DIRECT) {
                insert_dat(oldLC + 1);
            }

            int addr = find_symbol_address(pl->operand);
            
            if (addr >= 0) {
                // Symbol found - use absolute address
                // Note: Per project spec, both direct and relative use absolute addresses in object code
                // The "relative" mode just means branch instructions, but the output still shows target address
                fprintf(sout, "%04X  %s  %02X %02X\n", oldLC, op_hex, (addr >> 8) & 0xFF, addr & 0xFF);
            } else {
                if (is_external(pl->operand)) {
                    insert_hdrm('M', pl->operand, oldLC + 1);
                    fprintf(sout, "%04X  %s  00 00\n", oldLC, op_hex);
                } else {
                    // Forward reference - add to FRT for Pass 2 resolution
                    insert_frt(pl->operand, oldLC); 
                    fprintf(sout, "%04X  %s  00 00\n", oldLC, op_hex);
                }
            }
        }
    }
}

void finalize_pass1(FILE *sout) {
    for (int i = 0; i < 20; i++) {
        if (HDRMT[i].code == 'D') {
            int addr = find_symbol_address(HDRMT[i].symbol);
            if (addr >= 0) {
                HDRMT[i].address = addr;
            } else {
                fprintf(stderr, "ERROR: Undefined ENTRY symbol %s\n", HDRMT[i].symbol);
            }
        }
    }
}
