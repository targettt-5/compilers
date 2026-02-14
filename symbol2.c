#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define TABLE_SIZE 101

/* ================= SYMBOL TABLE ================= */
typedef struct entry {
    char name[50];       // variable or function name
    char type[20];       // int, float, string, etc.
    char scope[50];      // Global / Local (function name)
    char category[20];   // FUNCTION / VARIABLE / CONSTANT / IDENTIFIER
    char info[100];      // additional info: return type, stack allocated, etc.
    struct entry *next;
} Entry;

Entry *symbolTable[TABLE_SIZE] = {NULL};

int hash(char *str) {
    unsigned long h = 5381;
    int c;
    while ((c = *str++))
        h = ((h << 5) + h) + c;
    return h % TABLE_SIZE;
}

void insertSymbol(char *name, char *type, char *scope, char *category, char *info) {
    int idx = hash(name);
    Entry *temp = symbolTable[idx];
    while (temp) {
        if (strcmp(temp->name, name) == 0 && strcmp(temp->scope, scope) == 0)
            return;
        temp = temp->next;
    }
    Entry *newNode = malloc(sizeof(Entry));
    strcpy(newNode->name, name);
    strcpy(newNode->type, type);
    strcpy(newNode->scope, scope);
    strcpy(newNode->category, category);
    strcpy(newNode->info, info);
    newNode->next = symbolTable[idx];
    symbolTable[idx] = newNode;
}

void printSymbolTable() {
    printf("\n%-15s %-10s %-15s %-12s %-20s\n",
           "Name", "Type", "Scope", "Category", "Additional Info");
    printf("-------------------------------------------------------------------------------\n");
    for (int i = 0; i < TABLE_SIZE; i++) {
        Entry *t = symbolTable[i];
        while (t) {
            printf("%-15s %-10s %-15s %-12s %-20s\n",
                   t->name, t->type, t->scope, t->category, t->info);
            t = t->next;
        }
    }
}

/* ================= LANGUAGE SIGNATURE ================= */
const char *keywords[] = {
    "int","float","char","double","void","if","else","while","for","return","const"
};
int isKeyword(char *str) {
    int n = sizeof(keywords)/sizeof(keywords[0]);
    for (int i = 0; i < n; i++)
        if (strcmp(str, keywords[i]) == 0)
            return 1;
    return 0;
}

const char *multi_ops[] = {
    "==","!=","<=",">=","&&","||","++","--"
};

char single_ops[] = "+-*/%=!<>|&";
char delimiters[] = "(){}[];,";

/* ================= HELPERS ================= */
int isMultiOperator(char *str) {
    int n = sizeof(multi_ops)/sizeof(multi_ops[0]);
    for (int i = 0; i < n; i++)
        if (strcmp(str, multi_ops[i]) == 0)
            return 1;
    return 0;
}

/* ================= PREPROCESSOR / COMMENTS ================= */
void skipComments(FILE *fp, int *row, int *col) {
    int ch = fgetc(fp);
    if (ch == '/') { // single line
        while ((ch = fgetc(fp)) != '\n' && ch != EOF);
        (*row)++; *col = 1;
    } else if (ch == '*') { // multi line
        int prev = 0;
        while ((ch = fgetc(fp)) != EOF) {
            if (ch == '\n') { (*row)++; *col = 1; }
            if (prev == '*' && ch == '/') break;
            prev = ch;
        }
    } else {
        ungetc(ch, fp);
        printf("<OP,/,%d,%d>\n", *row, *col);
        (*col)++;
    }
}

/* ================= TOKEN HANDLERS ================= */
void handleIdentifier(FILE *fp, char first, int row, int *col, char *currentScope) {
    char buffer[100]; int i = 0, ch;
    buffer[i++] = first;
    while ((ch = fgetc(fp)) != EOF && (isalnum(ch) || ch == '_'))
        buffer[i++] = ch;
    buffer[i] = '\0';
    if (ch != EOF) ungetc(ch, fp);

    if (isKeyword(buffer)) {
        printf("<KEYWORD,%s,%d,%d>\n", buffer, row, *col);
    } else {
        int next = fgetc(fp);
        if (next == '(') { // function
            printf("<FUNC,%s,%d,%d>\n", buffer, row, *col);
            insertSymbol(buffer, "Unknown", "Global", "FUNCTION", "Returns Unknown");
            ungetc(next, fp);
            strcpy(currentScope, buffer); // set scope for local vars
        } else { // variable
            printf("<ID,%s,%d,%d>\n", buffer, row, *col);
            insertSymbol(buffer, "Unknown", currentScope, "VARIABLE", "Stack allocated");
            if (next != EOF) ungetc(next, fp);
        }
    }
    *col += strlen(buffer);
}

void handleNumber(FILE *fp, char first, int row, int *col) {
    char buffer[100]; int i = 0, ch;
    buffer[i++] = first;
    while ((ch = fgetc(fp)) != EOF && isdigit(ch))
        buffer[i++] = ch;
    buffer[i] = '\0';
    if (ch != EOF) ungetc(ch, fp);
    printf("<NUM,%s,%d,%d>\n", buffer, row, *col);
    *col += strlen(buffer);
}

void handleOperator(FILE *fp, char ch, int row, int *col) {
    char next = fgetc(fp);
    char buf[3] = {ch, next, '\0'};
    if (isMultiOperator(buf)) {
        printf("<OP,%s,%d,%d>\n", buf, row, *col);
        *col += 2;
    } else {
        if (next != EOF) ungetc(next, fp);
        printf("<OP,%c,%d,%d>\n", ch, row, *col);
        (*col)++;
    }
}

void handleString(FILE *fp, int row, int *col) {
    int ch; int start = *col;
    printf("<STRING,\"");
    while ((ch = fgetc(fp)) != EOF && ch != '"') {
        putchar(ch); (*col)++;
    }
    printf("\",%d,%d>\n", row, start); (*col)++;
}

/* ================= MAIN LEXER ================= */
int main() {
    FILE *fp = fopen("input.c", "r");
    if (!fp) { printf("Cannot open input.c\n"); return 1; }

    int c, row = 1, col = 1;
    char currentScope[50] = "Global";

    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') { row++; col = 1; }
        else if (isspace(c)) col++;
        else if (c == '/') skipComments(fp, &row, &col);
        else if (isalpha(c) || c == '_') handleIdentifier(fp, c, row, &col, currentScope);
        else if (isdigit(c)) handleNumber(fp, c, row, &col);
        else if (c == '"') handleString(fp, row, &col);
        else if (strchr(single_ops, c)) handleOperator(fp, c, row, &col);
        else if (strchr(delimiters, c)) { printf("<SYM,%c,%d,%d>\n", c, row, col); col++; }
    }

    fclose(fp);
    printSymbolTable();
    return 0;
}
