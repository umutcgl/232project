#include "asm_common.h"
#include <string.h>
#include <ctype.h>

static void rtrim(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || isspace((unsigned char)s[n - 1]))) {
        s[n - 1] = '\0';
        n--;
    }
}

static void ltrim(char *s) {
    int i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i > 0) memmove(s, s + i, strlen(s + i) + 1);
}

static void trim(char *s) { rtrim(s); ltrim(s); }

static int is_comment_line(const char *s) {
    return (s[0] == ';' || s[0] == '#'
            || (s[0] == '/' && s[1] == '/'));
}

static int is_pseudo(const char *op) {
    return (strcmp(op, "START")  == 0) ||
           (strcmp(op, "END")    == 0) ||
           (strcmp(op, "BYTE")   == 0) ||
           (strcmp(op, "WORD")   == 0) ||
           (strcmp(op, "PROG")   == 0) ||
           (strcmp(op, "ENTRY")  == 0) ||
           (strcmp(op, "EXTREF") == 0);
}

static int is_branch(const char *op) {
    return (strcmp(op, "BEQ") == 0) ||
           (strcmp(op, "BGT") == 0) ||
           (strcmp(op, "BLT") == 0);
}

static int is_implied_instr(const char *op) {
    return (strcmp(op, "DEC") == 0) ||
           (strcmp(op, "INC") == 0) ||
           (strcmp(op, "RET") == 0) ||
           (strcmp(op, "HLT") == 0);
}

static AddrMode detect_addr_mode(LineKind kind, const char *opcode, const char *operand) {
    if (kind != LINE_INSTR) return AM_NONE;

    if (is_implied_instr(opcode)) return AM_IMPLIED;

    if (operand == NULL || operand[0] == '\0') return AM_NONE;

    if (operand[0] == '#') return AM_IMMEDIATE;

    if (is_branch(opcode)) return AM_RELATIVE;

    return AM_DIRECT;
}

// Module-level line number tracker
static int parser_line_no = 0;

int get_next_parsed_line(FILE *fp, ParsedLine *out_pl) {
    char line[256];

    if (fp == NULL || out_pl == NULL) return 0;

    if (fgets(line, sizeof(line), fp) == NULL) return 0; // EOF

    parser_line_no++;

    memset(out_pl, 0, sizeof(*out_pl));
    out_pl->kind = LINE_EMPTY;
    out_pl->addr_mode = AM_NONE;
    out_pl->line_no = parser_line_no;

    trim(line);

    if (line[0] == '\0') {
        out_pl->kind = LINE_EMPTY;
        return 1;
    }

    if (is_comment_line(line)) {
        out_pl->kind = LINE_COMMENT;
        return 1;
    }

    // Parse: [optional LABEL:] OPCODE [OPERAND...]
    char *work = line;

    // Optional label
    char *colon = strchr(work, ':');
    if (colon != NULL) {
        *colon = '\0';
        trim(work);
        strncpy(out_pl->label, work, 9);
        out_pl->label[9] = '\0';

        work = colon + 1;
        trim(work);
    }

    // If only label existed
    if (work[0] == '\0') {
        out_pl->kind = LINE_EMPTY;
        return 1;
    }

    // Opcode = first token
    char *p = work;
    while (*p && !isspace((unsigned char)*p)) p++;

    char saved = *p;
    *p = '\0';
    strncpy(out_pl->opcode, work, 9);
    out_pl->opcode[9] = '\0';
    *p = saved;

    // Operand = rest
    trim(p);
    if (p[0] != '\0') {
        strncpy(out_pl->operand, p, 31);
        out_pl->operand[31] = '\0';
    }

    // Kind
    if (is_pseudo(out_pl->opcode)) {
        out_pl->kind = (strcmp(out_pl->opcode, "END") == 0) ? LINE_END : LINE_PSEUDO;
    } else {
        out_pl->kind = LINE_INSTR;
    }

    // Addressing mode
    out_pl->addr_mode = detect_addr_mode(out_pl->kind, out_pl->opcode, out_pl->operand);

    return 1;
}

void reset_parser(void) {
    parser_line_no = 0;
}

void display_parsed_line(const ParsedLine *pl) {
    if (pl->kind == LINE_EMPTY || pl->kind == LINE_COMMENT) return;
    
    printf("Line %d: ", pl->line_no);
    if (pl->label[0]) printf("Label=%-8s ", pl->label);
    else printf("Label=%-8s ", "(none)");
    
    printf("Opcode=%-6s ", pl->opcode);
    
    if (pl->operand[0]) printf("Operand=%-10s", pl->operand);
    else printf("Operand=%-10s", "(none)");
    
    printf("\n");
}
