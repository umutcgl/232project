#include "asm_common.h"
#include <string.h>
#include <stdlib.h>

// Parse a line from .s file
// Expected format: LC(Hex)  Opcode(Hex)  [OperandBytes...]
// Example: "001A  E1  00 00"
// Or "001A  E1"
// Or "001A  E1  A2"
// I need to reconstruct the line, potentially patching the last 2 bytes.

void run_pass2(FILE *sin, FILE *fobj, FILE *ftab) {
    char line[256];
    
    // Write DAT to .t
    fprintf(ftab, "DAT\n");
    for (int i = 0; i < 30; i++) {
        if (DAT[i].address != -1) {
            fprintf(ftab, "%X\n", DAT[i].address);
        }
    }

    // Write HDRM to .t
    fprintf(ftab, "HDRM\n");
    // H record
    fprintf(ftab, "H %s %X %X\n", module_name, prog_start, prog_len);
    // D, R, M records
    for (int i = 0; i < 20; i++) {
        if (HDRMT[i].code == 'D') {
            fprintf(ftab, "D %s %X\n", HDRMT[i].symbol, HDRMT[i].address);
        } else if (HDRMT[i].code == 'R') {
            fprintf(ftab, "R %s\n", HDRMT[i].symbol);
        } else if (HDRMT[i].code == 'M') {
            fprintf(ftab, "M %s %X\n", HDRMT[i].symbol, HDRMT[i].address);
        }
    }

    // Process .s for .o
    rewind(sin);
    while (fgets(line, sizeof(line), sin)) {
        if (strlen(line) < 4) continue;
        
        // Parse LC
        int line_lc;
        if (sscanf(line, "%x", &line_lc) != 1) {
            // Just copy if can't parse (should not happen for valid .s)
            fprintf(fobj, "%s", line);
            continue;
        }

        // Check FRT
        int patched = 0;
        char patch_symbol[10] = {0};
        
        for (int i = 0; i < 20; i++) {
            if (FRT[i].symbol[0] != '\0' && FRT[i].address == line_lc) {
                // Found in FRT
                strcpy(patch_symbol, FRT[i].symbol);
                patched = 1;
                break;
            }
        }

        if (!patched) {
            fprintf(fobj, "%s", line);
        } else {
            // Resolve
            // Look up ST
            int addr = -1;
            for (int i = 0; i < 10; i++) {
                if (strcmp(ST[i].symbol, patch_symbol) == 0) {
                    addr = ST[i].address;
                    break;
                }
            }

            if (addr == -1) {
                // Not in ST. Should be External?
                // If External, Pass 1 should have handled it via M record and NOT added to FRT (per my Pass 1 logic).
                // So if it's in FRT, and not in ST, it's an ERROR (Undefined Symbol).
                fprintf(stderr, "ERROR: Undefined symbol %s at %X\n", patch_symbol, line_lc);
                // Write unpatched line
                fprintf(fobj, "%s", line); 
            } else {
                // Found. Patch.
                // Parse the opcode from the line and reconstruct with resolved address
                char opcode[10]; 
                // We construct the new line
                // But we need the Opcode string.
                // Let's use sscanf to get opcode.
                sscanf(line, "%*x %s", opcode);
                
                fprintf(fobj, "%04X  %s  %02X %02X\n", 
                        line_lc, opcode, (addr >> 8) & 0xFF, addr & 0xFF);
            }
        }
    }
}
