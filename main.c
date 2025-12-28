#include "asm_common.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    char input_file[256] = "input.asm";  // Default input file
    char base_name[256];
    char s_file[260], o_file[260], t_file[260];
    
    // Get input file from command line argument if provided
    if (argc > 1) {
        strncpy(input_file, argv[1], 255);
        input_file[255] = '\0';
    }
    
    // Create base name by removing .asm extension
    strncpy(base_name, input_file, 255);
    base_name[255] = '\0';
    char *dot = strrchr(base_name, '.');
    if (dot && strcmp(dot, ".asm") == 0) {
        *dot = '\0';
    }
    
    // Create output filenames
    snprintf(s_file, sizeof(s_file), "%s.s", base_name);
    snprintf(o_file, sizeof(o_file), "%s.o", base_name);
    snprintf(t_file, sizeof(t_file), "%s.t", base_name);
    
    printf("SMPL Assembler\n");
    printf("==============\n");
    printf("Input file: %s\n", input_file);
    
    FILE *in = fopen(input_file, "r");
    FILE *sout = fopen(s_file, "w");

    if (!in) {
        fprintf(stderr, "ERROR: Cannot open input file '%s'\n", input_file);
        return 1;
    }
    if (!sout) {
        fprintf(stderr, "ERROR: Cannot create intermediate file '%s'\n", s_file);
        fclose(in);
        return 1;
    }

    ParsedLine pl;

    printf("\n--- PASS 1 ---\n");
    printf("Parsing and generating partial code...\n\n");
    
    reset_parser();
    init_pass1();

    while (get_next_parsed_line(in, &pl)) {
        display_parsed_line(&pl);  // Display parsed fields (per project spec)
        process_parsed_line_pass1(&pl, sout);
    }

    finalize_pass1(sout);
    
    // Display Symbol Table
    printf("\nSymbol Table (ST):\n");
    for (int i = 0; i < 10; i++) {
        if (ST[i].symbol[0] != '\0') {
            printf("  %s = %04X\n", ST[i].symbol, ST[i].address);
        }
    }
    
    // Display Forward Reference Table
    printf("\nForward Reference Table (FRT):\n");
    int frt_count = 0;
    for (int i = 0; i < 20; i++) {
        if (FRT[i].symbol[0] != '\0') {
            printf("  %s at %04X\n", FRT[i].symbol, FRT[i].address);
            frt_count++;
        }
    }
    if (frt_count == 0) printf("  (empty)\n");
    
    // Close .s output to flush
    fclose(sout);
    fclose(in);
    
    printf("Intermediate file: %s\n", s_file);
    
    // Re-open .s for reading
    sout = fopen(s_file, "r");
    
    FILE *fobj = fopen(o_file, "w");
    FILE *ftab = fopen(t_file, "w");
    
    if (!sout || !fobj || !ftab) {
        fprintf(stderr, "ERROR: Cannot open files for Pass 2\n");
        return 1;
    }
    
    printf("\n--- PASS 2 ---\n");
    run_pass2(sout, fobj, ftab);
    
    if (sout) fclose(sout);
    if (fobj) fclose(fobj);
    if (ftab) fclose(ftab);

    printf("Object file: %s\n", o_file);
    printf("Table file: %s\n", t_file);
    printf("\nAssembly complete.\n");

    return 0;
}
