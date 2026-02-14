#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define TABLE_SIZE 50

/* ---------------- SYMBOL TABLE -------------------- */
typedef struct node {
    char name[50];
    char type[20];
    char argument[100];
    struct node *next;
} Node;

Node *symbolTable[TABLE_SIZE] = {NULL};

int hashFunction(char *str) {
    int sum = 0;
    for (int i=0; str[i]!='\0'; i++) sum += str[i];
    return sum % TABLE_SIZE;
}

void insertSymbol(char *name, char *type, char *arg){
    int index = hashFunction(name);
    Node *temp = symbolTable[index];
    while(temp){ if(strcmp(temp->name,name)==0) return; temp=temp->next; }
    Node *newNode = (Node*)malloc(sizeof(Node));
    strcpy(newNode->name,name);
    strcpy(newNode->type,type);
    if(arg) strcpy(newNode->argument,arg); else strcpy(newNode->argument,"-");
    newNode->next = symbolTable[index];
    symbolTable[index] = newNode;
}

void printSymbolTable(){
    printf("\n========== SYMBOL TABLE ==========\n");
    printf("Name\tType\tArgument\n");
    for(int i=0;i<TABLE_SIZE;i++){
        Node *temp = symbolTable[i];
        while(temp){ 
            printf("%s\t%s\t%s\n", temp->name,temp->type,temp->argument);
            temp=temp->next;
        }
    }
}

/* ---------------- JAVA KEYWORDS ------------------ */
const char *keywords[] = {
    "int","float","double","char","boolean","void",
    "if","else","for","while","do","return","break","continue",
    "public","private","protected","class","static","final","abstract",
    "interface","extends","implements","try","catch","throw","throws",
    "new","package","import","this","super","switch","case","default",
    "enum","instanceof","synchronized"
};

int isKeyword(char *str){
    int n = sizeof(keywords)/sizeof(keywords[0]);
    for(int i=0;i<n;i++) if(strcmp(str,keywords[i])==0) return 1;
    return 0;
}

/* ---------------- OPERATORS / DELIMITERS --------- */
char single_ops[] = "+-*/%=<>!&|^";
char delimiters[] = "(){}[],;:.";
int isOperator(char c){ return strchr(single_ops,c)!=NULL; }
int isDelimiter(char c){ return strchr(delimiters,c)!=NULL; }

/* ---------------- COMMENTS ------------------------ */
void skipCommentsJava(FILE *fp,int *row,int *col){
    int ch=getc(fp);
    if(ch=='/'){
        int ch2=getc(fp);
        if(ch2=='/'){ // single-line
            while((ch=getc(fp))!='\n' && ch!=EOF);
            if(ch=='\n'){ (*row)++; *col=1; }
        } else if(ch2=='*'){ // multi-line
            int prev=0;
            while((ch=getc(fp))!=EOF){
                if(ch=='\n'){ (*row)++; *col=1; } else (*col)++;
                if(prev=='*' && ch=='/') break;
                prev=ch;
            }
        } else ungetc(ch2,fp);
    } else ungetc(ch,fp);
}

/* ---------------- IDENTIFIERS / FUNCTIONS -------- */
void handleIdentifierJava(FILE *fp,int first,int row,int *col){
    char buffer[100]; int i=0,ch;
    buffer[i++]=first;
    while((ch=getc(fp))!=EOF && (isalnum(ch)||ch=='_')) buffer[i++]=ch;
    buffer[i]='\0';
    if(ch!=EOF) ungetc(ch,fp);

    if(isKeyword(buffer)){
        printf("<KEYWORD,%s,%d,%d>\n",buffer,row,*col);
    } else {
        int next=fgetc(fp);
        if(next=='('){ // method/function
            printf("<FUNC,%s,%d,%d>\n",buffer,row,*col);
            insertSymbol(buffer,"FUNC","-");
        } else { // variable / class
            printf("<IDENTIFIER,%s,%d,%d>\n",buffer,row,*col);
            insertSymbol(buffer,"IDENTIFIER","-");
            if(next!=EOF) ungetc(next,fp);
        }
    }
    *col += strlen(buffer);
}

/* ---------------- NUMBERS ------------------------- */
void handleNumberJava(FILE *fp,int first,int row,int *col){
    char buffer[100]; int i=0,ch;
    buffer[i++]=first;
    while((ch=getc(fp))!=EOF && (isdigit(ch)||ch=='.')) buffer[i++]=ch;
    buffer[i]='\0';
    if(ch!=EOF) ungetc(ch,fp);
    printf("<NUM,%s,%d,%d>\n",buffer,row,*col);
    *col += strlen(buffer);
}

/* ---------------- STRING / CHAR ------------------- */
void handleStringJava(FILE *fp,int row,int *col){
    int ch,start=*col;
    int quote=getc(fp); // ' or "
    printf("<STRING,%c",quote); (*col)++;
    while((ch=getc(fp))!=EOF && ch!=quote){ putchar(ch); (*col)++; }
    printf(",%d,%d>\n",row,start); (*col)++;
}

/* ---------------- OPERATOR ------------------------ */
void handleOperatorJava(FILE *fp,char ch,int row,int *col){
    int next=getc(fp);
    if(next=='=' || (ch=='<' && next=='=') || (ch=='>' && next=='=') || 
       (ch=='&' && next=='&') || (ch=='|' && next=='|') ||
       (ch=='+' && next=='+') || (ch=='-' && next=='-')){
        printf("<OP,%c%c,%d,%d>\n",ch,next,row,*col);
        (*col)+=2;
    } else { if(next!=EOF) ungetc(next,fp);
        printf("<OP,%c,%d,%d>\n",ch,row,*col); (*col)++;
    }
}

/* ---------------- DELIMITER ----------------------- */
void handleDelimiterJava(char c,int row,int *col){
    printf("<DELIM,%c,%d,%d>\n",c,row,*col); (*col)++;
}

/* ---------------- MAIN LEXER ---------------------- */
int main(){
    FILE *fp=fopen("input.java","r");
    if(!fp){ printf("Cannot open input.java\n"); return 1; }

    int c,row=1,col=1;
    while((c=getc(fp))!=EOF){
        if(c=='\n'){ row++; col=1; }
        else if(isspace(c)){ col++; }
        else if(c=='/'){ skipCommentsJava(fp,&row,&col); }
        else if(isalpha(c)||c=='_'){ handleIdentifierJava(fp,c,row,&col); }
        else if(isdigit(c)){ handleNumberJava(fp,c,row,&col); }
        else if(c=='"'||c=='\''){ handleStringJava(fp,row,&col); }
        else if(isOperator(c)){ handleOperatorJava(fp,c,row,&col); }
        else if(isDelimiter(c)){ handleDelimiterJava(c,row,&col); }
        else { printf("Invalid token at %d %d\n",row,col); col++; }
    }

    fclose(fp);
    printSymbolTable();
    return 0;
}
