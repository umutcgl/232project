#ifndef ASM_COMMON_H
#define ASM_COMMON_H

#include <stdio.h>



typedef enum {
    LINE_EMPTY = 0,
    LINE_COMMENT,
    LINE_INSTR,
    LINE_PSEUDO,
    LINE_END
} LineKind;

typedef enum {
    AM_NONE = 0,
    AM_IMPLIED,
    AM_IMMEDIATE,
    AM_DIRECT,
    AM_RELATIVE
} AddrMode;

typedef struct {
    LineKind kind;
    char     label[10];
    char     opcode[10];
    char     operand[32];
    AddrMode addr_mode;
    int      line_no;
} ParsedLine;



struct SymbolTable {
    char symbol[10];
    int  address;
};

struct ForwardRefTable {
    int  address;
    char symbol[10];
};

struct DirectAdrTable {
    int address;
};

struct HDRMTable {
    char code;
    char symbol[10];
    int  address;
};

struct Memory {
    int  address;
    char symbol[2];
};

extern struct SymbolTable     ST[10];
extern struct ForwardRefTable FRT[20];
extern struct DirectAdrTable  DAT[30];
extern struct HDRMTable       HDRMT[20];
extern struct Memory          M[500];

extern int LC;
extern char module_name[10];
extern int prog_start;
extern int prog_len;

int get_next_parsed_line(FILE *fp, ParsedLine *out_pl);
void reset_parser(void);
void display_parsed_line(const ParsedLine *pl);

void init_pass1(void);
void process_parsed_line_pass1(const ParsedLine *pl, FILE *sout);
void finalize_pass1(FILE *sout);

void run_pass2(FILE *sin, FILE *fobj, FILE *ftab);



typedef struct {
    char mnemonic[8];
    char opcode_str[3];
    int  nbytes;
} OpcodeEntry;

extern OpcodeEntry OPTAB[];
extern int OPTAB_SIZE;

#endif
