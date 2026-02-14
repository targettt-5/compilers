#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define TABLE_SIZE 50

/* ------------------- SYMBOL TABLE -------------------- */
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
    printf("\n========== SYMBOL TABLE ==========\n");
    printf("Name\tType\tArgument\n");
    for (int i = 0; i < TABLE_SIZE; i++) {
        Node *temp = symbolTable[i];
        while (temp) {
            printf("%s\t%s\t%s\n", temp->name, temp->type, temp->argument);
            temp = temp->next;
        }
    }
}

/* ------------------- PYTHON KEYWORDS ----------------- */
const char *keywords[] = {
    "def","import","for","in","if","else","elif","while",
    "return","break","continue","class","with","as","pass","global","nonlocal"
};

int isKeyword(char *str) {
    int total = sizeof(keywords)/sizeof(keywords[0]);
    for (int i=0;i<total;i++)
        if (strcmp(str, keywords[i])==0)
            return 1;
    return 0;
}

/* ------------------- OPERATORS / DELIMITERS ----------- */
char single_ops[] = "+-*/%=!<>|&";
char delimiters[] = "():,[]";

int isOperator(char c) { return strchr(single_ops,c)!=NULL; }
int isDelimiter(char c) { return strchr(delimiters,c)!=NULL; }

/* ------------------- COMMENTS ------------------------ */
void skipCommentsPython(FILE *fp, int *row, int *col) {
    int ch = getc(fp);
    if (ch == '#') {
        while((ch=getc(fp)) != '\n' && ch != EOF);
        if(ch=='\n') { (*row)++; *col = 1; }
    }
    else if (ch == '\'' || ch=='"') { // check triple quote
        int quote = ch;
        int count=1, prev;
        prev = getc(fp);
        while(prev == quote) { count++; prev=getc(fp); }
        if(count==3) { // multi-line string as comment
            int consecutive=0;
            while((ch=getc(fp))!=EOF){
                if(ch=='\n'){ (*row)++; *col=1; } else (*col)++;
                if(ch==quote) consecutive++; else consecutive=0;
                if(consecutive==3) break;
            }
        } else ungetc(prev, fp);
    } else ungetc(ch, fp);
}

/* ------------------- IDENTIFIERS / FUNCTIONS ---------- */
void handleIdentifier(FILE *fp, int first, int row, int *col, char *prevKeyword) {
    char buffer[100]; int i=0, ch;
    buffer[i++] = first;
    while((ch=getc(fp))!=EOF && (isalnum(ch)||ch=='_'))
        buffer[i++] = ch;
    buffer[i]='\0';
    if(ch!=EOF) ungetc(ch, fp);

    if(isKeyword(buffer)) {
        printf("<KEYWORD,%s,%d,%d>\n", buffer,row,*col);
        strcpy(prevKeyword, buffer);
    } else {
        int next = fgetc(fp);
        if(strcmp(prevKeyword,"def")==0 && next=='(') {
            printf("<FUNC,%s,%d,%d>\n", buffer,row,*col);
            insertSymbol(buffer,"FUNC","-");
        } else {
            printf("<IDENTIFIER,%s,%d,%d>\n", buffer,row,*col);
            insertSymbol(buffer,"IDENTIFIER","-");
            if(next!=EOF) ungetc(next, fp);
        }
        if(next!=EOF) ungetc(next, fp);
    }
    *col += strlen(buffer);
}

/* ------------------- NUMBERS -------------------------- */
void handleNumber(FILE *fp, int first, int row, int *col) {
    char buffer[100]; int i=0, ch;
    buffer[i++] = first;
    while((ch=getc(fp))!=EOF && isdigit(ch)) buffer[i++] = ch;
    buffer[i]='\0';
    if(ch!=EOF) ungetc(ch, fp);
    printf("<NUM,%s,%d,%d>\n", buffer,row,*col);
    *col += strlen(buffer);
}

/* ------------------- STRINGS -------------------------- */
void handleString(FILE *fp, int row, int *col) {
    int ch, startCol = *col;
    int quote = getc(fp);
    printf("<STRING,%c", quote); (*col)++;
    while((ch=getc(fp))!=EOF && ch!=quote) { putchar(ch); (*col)++; }
    printf(",%d,%d>\n", row, startCol); (*col)++;
}

/* ------------------- OPERATORS ------------------------ */
void handleOperator(FILE *fp, char ch, int row, int *col) {
    int next = getc(fp);
    if(next=='=' || (ch=='+' && next=='+') || (ch=='-' && next=='-')) {
        printf("<OP,%c%c,%d,%d>\n",ch,next,row,*col);
        (*col)+=2;
    } else {
        if(next!=EOF) ungetc(next,fp);
        printf("<OP,%c,%d,%d>\n",ch,row,*col);
        (*col)++;
    }
}

/* ------------------- DELIMITERS ----------------------- */
void handleDelimiter(char c, int row, int *col) {
    printf("<DELIM,%c,%d,%d>\n",c,row,*col);
    (*col)++;
}

/* ------------------- MAIN ---------------------------- */
int main() {
    FILE *fp = fopen("input.py","r");
    if(!fp){ printf("Cannot open file\n"); return 1; }

    int c, row=1, col=1;
    char prevKeyword[20]="";

    while((c=getc(fp))!=EOF){
        if(c=='\n'){ row++; col=1; }
        else if(isspace(c)){ col++; }
        else if(c=='#' || c=='\'' || c=='"') skipCommentsPython(fp,&row,&col);
        else if(isalpha(c)||c=='_') handleIdentifier(fp,c,row,&col,prevKeyword);
        else if(isdigit(c)) handleNumber(fp,c,row,&col);
        else if(c=='"'||c=='\'') handleString(fp,row,&col);
        else if(isOperator(c)) handleOperator(fp,c,row,&col);
        else if(isDelimiter(c)) handleDelimiter(c,row,&col);
        else { printf("Invalid token at %d %d\n", row,col); col++; }
    }

    fclose(fp);
    printSymbolTable();
    return 0;
}
