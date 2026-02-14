#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>



const char *keywords[] = {
    "int","float","char","double","if","else",
    "while","for","return","void","break","continue"
};



#define TABLE_SIZE 50

#include <stdlib.h>

#define TABLE_SIZE 50

typedef struct node {
    char name[50];
    char type[20];
    char argument[100];
    struct node *next;
} Node;

Node *symbolTable[TABLE_SIZE] = {NULL};

int hashFunction(char *str) {
    int sum = 0;
    for (int i = 0; str[i] != '\0'; i++)
        sum += str[i];
    return sum % TABLE_SIZE;
}

void insertSymbol(char *name, char *type, char *arg) {
    int index = hashFunction(name);

    Node *temp = symbolTable[index];
    while (temp) {
        if (strcmp(temp->name, name) == 0)
            return;
        temp = temp->next;
    }

    Node *newNode = (Node *)malloc(sizeof(Node));
    strcpy(newNode->name, name);
    strcpy(newNode->type, type);

    if (arg)
        strcpy(newNode->argument, arg);
    else
        strcpy(newNode->argument, "-");

    newNode->next = symbolTable[index];
    symbolTable[index] = newNode;
}

void printSymbolTable() {
    printf("\nTOKEN TABLE\n");
    printf("TokenName\tTokenType\tArgument\n");

    for (int i = 0; i < TABLE_SIZE; i++) {
        Node *temp = symbolTable[i];
        while (temp) {
            printf("%s\t\t%s\t\t%s\n",
                   temp->name,
                   temp->type,
                   temp->argument);
            temp = temp->next;
        }
    }
}



int isKeyword(char *str) {
    int total = sizeof(keywords)/sizeof(keywords[0]);
    for (int i = 0; i < total; i++)
        if (strcmp(str, keywords[i]) == 0)
            return 1;
    return 0;
}

int isOperator(char c) {
    return (c=='+'||c=='-'||c=='*'||c=='/'||
            c=='='||c=='<'||c=='>'||c=='!'||
            c=='&'||c=='|'||c=='%');
}

int isDelimiter(char c) {
    return (c=='('||c==')'||c=='{'||c=='}'||
            c=='['||c==']'||c==';'||c==','||c=='.');
}

/* ---------- PREPROCESSOR ---------- */

void hash(FILE *fp) {
    int ch;
    while ((ch = getc(fp)) != '\n' && ch != EOF);
    if (ch == '\n') ungetc(ch, fp);
}

/* ---------- COMMENTS ---------- */

void bar(FILE *fp, int *row, int *col) {
    int ch = getc(fp);

    if (ch == '/') {
        while ((ch = getc(fp)) != '\n' && ch != EOF);
        if (ch == '\n') ungetc(ch, fp);
    }
    else if (ch == '*') {
        int prev = 0;
        while ((ch = getc(fp)) != EOF) {
            if (ch == '\n') {
                (*row)++;
                *col = 1;
            } else (*col)++;

            if (prev == '*' && ch == '/') break;
            prev = ch;
        }
    }
    else {
        printf("<OP, /, %d, %d>\n", *row, *col);
        if (ch != EOF) ungetc(ch, fp);
        (*col)++;
    }
}

/* ---------- IDENTIFIER / KEYWORD ---------- */

void letter(FILE *fp, int first, int *col, int row) {
    char buffer[100];
    int i = 0, ch;
    buffer[i++] = first;

    while ((ch = getc(fp)) != EOF && (isalnum(ch) || ch == '_'))
        buffer[i++] = ch;

    buffer[i] = '\0';

    if (ch != EOF) ungetc(ch, fp);

    if (isKeyword(buffer)) {
        printf("<KEYWORD, %s, %d, %d>\n", buffer, row, *col);
    }
    else {
        int next = getc(fp);

        if (next == '(') {
            printf("<FUNC, %s, %d, %d>\n", buffer, row, *col);
            insertSymbol(buffer, "FUNC", "-");
            ungetc(next, fp);
        }
        else {
            printf("<IDENTIFIER, %s, %d, %d>\n", buffer, row, *col);
            insertSymbol(buffer, "Identifier", "-");
            if (next != EOF) ungetc(next, fp);
        }
    }

    *col += strlen(buffer);
}


/* ---------- NUMBER ---------- */

void number(FILE *fp, int first, int *col, int row) {
    char buffer[100];
    int i = 0, ch;
    buffer[i++] = first;

    while ((ch = getc(fp)) != EOF && isdigit(ch))
        buffer[i++] = ch;

    buffer[i] = '\0';
    if (ch != EOF) ungetc(ch, fp);

    printf("<NUMBER, %s, %d, %d>\n", buffer, row, *col);
    *col += strlen(buffer);
}

/* ---------- STRING ---------- */

void stringLiteral(FILE *fp, int row, int *col) {
    int ch;
    int startCol = *col;
    printf("<STRING, \"");
    (*col)++;

    while ((ch = getc(fp)) != EOF && ch != '"') {
        putchar(ch);
        (*col)++;
    }

    printf("\", %d, %d>\n", row, startCol);
    (*col)++;
}

/* ---------- CHAR ---------- */

void charLiteral(FILE *fp, int row, int *col) {
    int ch;
    int startCol = *col;
    printf("<CHAR, '");
    (*col)++;

    while ((ch = getc(fp)) != EOF && ch != '\'') {
        putchar(ch);
        (*col)++;
    }

    printf("', %d, %d>\n", row, startCol);
    (*col)++;
}

/* ---------- OPERATOR ---------- */

void OperatorHandler(FILE *fp, char ch, int row, int *col) {
    int next = getc(fp);

    if (next == '=' ||
        (ch == '+' && next == '+') ||
        (ch == '-' && next == '-') ||
        (ch == '&' && next == '&') ||
        (ch == '|' && next == '|')) {

        printf("<OP, %c%c, %d, %d>\n", ch, next, row, *col);
        (*col) += 2;
    }
    else {
        if (next != EOF) ungetc(next, fp);
        printf("<OP, %c, %d, %d>\n", ch, row, *col);
        (*col) += 1;
    }
}

/* ---------- DELIMITER ---------- */

void delimiter(char c, int row, int *col) {
    printf("<DELIM, %c, %d, %d>\n", c, row, *col);
    (*col)++;
}

/* ---------- MAIN ---------- */

int main() {
    FILE *fp;
    int c;
    int row = 1, col = 1;

    fp = fopen("input.c", "r");

    if (!fp) {
        printf("File not found\n");
        return 1;
    }

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') {
            row++;
            col = 1;
        }
        else if (isspace(c)) {
            col++;
        }
        else if (c == '#') {
            printf("<PREPROC, #, %d, %d>\n", row, col);
            hash(fp);
            col = 1;
        }
        else if (c == '/') {
            bar(fp, &row, &col);
        }
        else if (isalpha(c) || c == '_') {
            letter(fp, c, &col, row);
        }
        else if (isdigit(c)) {
            number(fp, c, &col, row);
        }
        else if (c == '"') {
            stringLiteral(fp, row, &col);
        }
        else if (c == '\'') {
            charLiteral(fp, row, &col);
        }
        else if (isOperator(c)) {
            OperatorHandler(fp, c, row, &col);
        }
        else if (isDelimiter(c)) {
            delimiter(c, row, &col);
        }
        else {
            printf("Invalid token at %d %d\n", row, col);
            col++;
        }
    }

    fclose(fp);

    printSymbolTable();   // print symbol table

    return 0;
}
