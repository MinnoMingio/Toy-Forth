#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

// Definiamo i tipi in base al numero da usare dopo nella struct tfObj in "int type"
#define TFOBJ_TYPE_INT    0
#define TFOBJ_TYPE_STR    1
#define TFOBJ_TYPE_BOOL   2
#define TFOBJ_TYPE_LIST   3
#define TFOBJ_TYPE_SYMBOL 4

// Definiamo il prototipo di tfContext qua in quanto dopo in tfObj si creerebbero dei
//conflitti perche tfObj cercherebbe di accedere a tfContext prima che quest'utlima esista.
//Quindi dichiaramo il suo prototipo qui per dire a C: "guarda che tfContext esiste ma viene approfondito piu' avanti nel programma"
struct tfContext;
struct FunctionTableRow;
struct FunctionTable;

typedef struct tfObj
{
    int refcount;
    int col;
    int type; // TFOBJ_TYPE_*

    union 
    {
        int i;
        struct String
        {
            char *ptr;
            size_t len;
        } str;
        struct List
        {
            struct tfObj **ele;
            size_t len;
        } list;
    };
    
} tfObj;

typedef struct FunctionTableRow
{
   tfObj *name; // this has to be string type
   void(*func)(struct tfContext *context, tfObj *symbol);
   tfObj *user_declared_func; // this has to be program type cause we need a program to run
} FunctionTableRow;

typedef struct FunctionTable
{
    struct FunctionTableRow **rows;
    int func_count;
} FunctionTable; 

typedef struct tfParser
{
    char *program_to_pars;  // il programma sui cui dobbiamo fare il parsing 
    char *current_token;    // l'elemento corrente su cui stiamo facendo il parsing
    int col;
} tfParser;

typedef struct tfContext
{
    tfObj *stack; // Lo stack e' semplicemente un tfObj in quanto abbiamo detto che il programma in se e' semplicemente una lista di elementi che viene eseguito uno dopo l'altro
    int i;        // where we are in the program - the index of the current element. // e' una stringa (lista di char)
    FunctionTable available_functions;
} tfContext;

void increment_parser(tfParser *parser)
{
    parser->current_token++;
    parser->col++;
}

tfObj *createObject(int type)
{
    tfObj *obj = malloc(sizeof(tfObj));
    obj->refcount = 1;
    obj->type = type;
        
    return obj;
}

tfObj *createObjectInt(int value)
{
    tfObj *obj = createObject(TFOBJ_TYPE_INT);
    obj->i = value;

    return obj;
}

tfObj *createObjectString(char *value, size_t len)
{
    tfObj *obj = createObject(TFOBJ_TYPE_STR);
    obj->str.ptr = malloc(len+1);
    obj->str.len = len;
    memcpy(obj->str.ptr, value, len);
    obj->str.ptr[len] = 0;

    return obj;
}

tfObj *createObjectSymbol(char *value, size_t len)
{
    tfObj *obj = createObjectString(value, len);
    obj->type = TFOBJ_TYPE_SYMBOL;

    return obj;
}

tfObj *createObjectBool(int value)
{
    tfObj *obj = createObject(TFOBJ_TYPE_BOOL);
    obj->i = value;

    return obj; 
}

tfObj *createObjectList(size_t len)
{
    tfObj *obj = createObject(TFOBJ_TYPE_LIST);
    obj->list.ele = malloc(sizeof(tfObj*) * len);
    obj->list.len = len;

    return obj;
}

void releaseObject(tfObj *o);
void printObject(tfObj *o);

void freeObject(tfObj *o)
{
    switch(o->type)
    {
        case TFOBJ_TYPE_STR:
        case TFOBJ_TYPE_SYMBOL:
            //printf("I'm releasing: ");
            //printObject(o);
            //printf(", ");

            free(o->str.ptr); 
            break;
        case TFOBJ_TYPE_LIST:
            for(int i = 0; i < o->list.len; i++)
            {
                releaseObject(o->list.ele[i]);    
            }
            break;
    }
    //printf("I'm freeing: ");
    //printObject(o);
    //printf(", \n");
    free(o);
}

void retainObject(tfObj *o)
{
    o->refcount++;
}

void releaseObject(tfObj *o)
{
    o->refcount--;
    
    if(o->refcount <= 0)
    {
        freeObject(o);
    }
}

void printObject(tfObj *o)
{
    switch(o->type)
    {
        case TFOBJ_TYPE_INT:
            printf("int: %d", o->i);
            break;
        case TFOBJ_TYPE_STR:
            printf("str: %s",o->str.ptr);
            break;
        case TFOBJ_TYPE_BOOL:
            printf("bool: %s", o->i ? "true" : "false");
            break;
        case TFOBJ_TYPE_SYMBOL:
            printf("sym: %s", o->str.ptr);
            break;
        case TFOBJ_TYPE_LIST:
            printf("list: %d = ", o->list.len);
            printf("[");
            for(int i = 0; i < o->list.len; i++)
            {
                printObject(o->list.ele[i]);

                if(i + 1 < o->list.len)
                {
                    printf(", ");
                }
            }
            printf("]");
            break;
    }
}

void pushToList(tfObj *listObj, tfObj *ele)
{
    if(listObj == NULL) return;
    if(ele == NULL) return;

    int len = listObj->list.len;
    listObj->list.ele = realloc(listObj->list.ele, sizeof(tfObj*)*(len+1));
    listObj->list.ele[len] = ele;
    listObj->list.len++; 
}

tfObj *popFromList(tfObj *listObj)
{
    if(listObj == NULL) return NULL;
    if(listObj->list.len < 1) return NULL;
     
    int len = listObj->list.len;
    tfObj *o = listObj->list.ele[len-1]; 
    listObj->list.ele = realloc(listObj->list.ele, sizeof(tfObj*)*(len-1));
    listObj->list.len--;
    
    if(o != NULL) return o;
    else return NULL;
}

void printFunction(tfContext *context, tfObj *symbol)
{
    size_t len = context->stack->list.len;
    if(len < 1) return;

    tfObj *o = context->stack->list.ele[len-1];
    if(o == NULL) return;
    printObject(o);
    printf("\n");
}

tfObj* cloneObject(tfObj *src)
{
   if(src == NULL) return NULL;
   
   tfObj *dest = NULL; 

   switch (src->type)
   {
        case TFOBJ_TYPE_INT: 
            dest = createObjectInt(src->i);
            break;
        case TFOBJ_TYPE_BOOL:
            dest = createObjectBool(src->i);
            break;
        case TFOBJ_TYPE_STR:
        case TFOBJ_TYPE_SYMBOL:
            dest = createObjectString(src->str.ptr, src->str.len);
            if(src->type == TFOBJ_TYPE_SYMBOL) dest->type = TFOBJ_TYPE_SYMBOL; 
            break;
        case TFOBJ_TYPE_LIST:
            dest = createObjectList(src->list.len);

            for(int i = 0; i < src->list.len; i++)
            {
                dest->list.ele[i] = cloneObject(src->list.ele[i]);
            }
            break;
        default:
            break;
   }
   return dest;
}

void duplicate(tfContext *context, tfObj *symbol)
{
    size_t len = context->stack->list.len;
    if(len < 1) return;

    tfObj *o = context->stack->list.ele[len-1];
    if(o == NULL) return;
    tfObj *dup = cloneObject(o);
    pushToList(context->stack, dup);
}

void mathFunctions(tfContext *context, tfObj *symbol) 
{
    size_t len = context->stack->list.len;

    if(len < 2) return;
    
    tfObj *b = popFromList(context->stack);
    tfObj *a = popFromList(context->stack);
    
    tfObj *res = NULL;
   
    if(a == NULL || b == NULL) return; 
    if(a->type != TFOBJ_TYPE_INT || b->type != TFOBJ_TYPE_INT) return;
    
    switch (symbol->str.ptr[0])
    {
        case '+':
            res = createObjectInt(a->i + b->i);
            break;
        case '-':
            res = createObjectInt(a->i - b->i);
            break;
        case '*':
            res = createObjectInt(a->i * b->i);
            break;
        case '/':
            if(b->i != 0)
            res = createObjectInt(a->i / b->i);
            break;
        case '%':
            res = createObjectInt(a->i % b->i);
            break;
        case '>':
            res = createObjectBool(a->i > b->i ? 1 : 0); 
            break;
        case '<':
            res = createObjectBool(a->i < b->i ? 1 : 0); 
            break;
        case '=':
            res = createObjectBool(a->i == b->i ? 1 : 0);
            break;
        default:
            break;
    }

    releaseObject(a);
    releaseObject(b);
    //context->stack->list.len -= 2;

    if(res == NULL) return; 

    pushToList(context->stack, res);
}

int isDigitUnsigned(char c)
{
    return (c >= '0' && c <= '9');
}

int isDigitSigned(char c, char n)
{
    if (isDigitUnsigned(c))
    {
        return 1;
    }else if(c == '-' && isDigitUnsigned(n))
    {
        return 1;
    }
    else if(c == '+' && isDigitUnsigned(n))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void skipSpaces(tfParser *parser)
{
    while(isspace(parser->current_token[0])) increment_parser(parser); // Skippiamo al token successivo se incontriamo uno spazio. Se abbiamo (" ciao") diventera' ("ciao") perche' abbiamo skippato di un carattere in avanti.
}

tfObj *parseNumber(tfParser *parser)
{
    char *start = parser->current_token;
    char *end;

    char *str = parser->current_token;
    
    while(isDigitSigned(str[0], str[1])) 
    {
        str++;
    }
    end = str;

    int num_length = end - start;
    
    char dest[num_length+1];
    memcpy(dest, start, num_length); // dest sarebbe la stringa tagliata che presenta solo il/i numero/i ("1", oppure "10")
    dest[num_length] = 0;

    parser->current_token = str;
    tfObj *num = createObjectInt(atoi(dest));

    return num;
}

int isValidChar(tfParser *parser)
{
    if(parser->current_token[0] != '\"')
    {
        return 0;
    }else
    {
        return 1;
    }
}

tfObj *parseString(tfParser *parser)
{
    increment_parser(parser);
    char *start = parser->current_token;
    char *end;

    while(!isValidChar(parser) && parser->current_token[0] != '\0')
    {
        increment_parser(parser);    
    }

    end = parser->current_token;

    int num_length = end - start;

    //char *dest = malloc(num_length+1);
    //memcpy(dest, start, num_length); // dest sarebbe la stringa tagliata che presenta solo il/i numero/i ("1", oppure "10")
    //dest[num_length] = 0;

    if(parser->current_token[0] == '\"')
    {
        increment_parser(parser);
    }

    tfObj *string = createObjectString(start, num_length);
    return string;
}

int isSymbol(char c)
{
    char valid_symbols[] = "+-*/<>%=";
    return isalpha(c) || strchr(valid_symbols, c);
}

tfObj *parseSymbol(tfParser *parser)
{
    char *start = parser->current_token;
    char *end;

    while(isSymbol(parser->current_token[0]) && parser->current_token[0]) increment_parser(parser);
    
    end = parser->current_token;

    int len = end - start;

    tfObj *symbol = createObjectSymbol(start, len);
    return symbol;
}

void addUSymbol(tfContext *context, tfObj *symbol, tfObj *user_function);
tfObj *compile(tfContext *context, char *program_txt);

tfObj *parseList(tfContext *context, tfParser *parser)
{
    increment_parser(parser); 
    char *start = parser->current_token;
    char *end;
    int balance = 1;     
    while(balance > 0)
    {
        if(parser->current_token[0] == '[')
        {
            balance++;
        }else if(parser->current_token[0] == ']')
        {
            balance--;
        }
    
        if(parser->current_token[0] == 0)
        {
            printf("List Error!\n");
            return NULL;
        } 
        if (balance > 0)
        {
            increment_parser(parser);
        }
    }

    end = parser->current_token;
    int len = end - start;

    char *program_txt = malloc(len+1);
    memcpy(program_txt, start, len);
    program_txt[len] = 0;

    tfObj *compiled_program = compile(context, program_txt);
    free(program_txt);
    increment_parser(parser);
    return compiled_program;
}

tfObj *parseProgram(tfContext *context, tfParser *parser)
{
    increment_parser(parser);
    char *start = parser->current_token;
    char *end;
    tfObj *list;
    
    while(isSymbol(parser->current_token[0])) 
    {
        increment_parser(parser);
    }

    end = parser->current_token;

    if(parser->current_token[0] == '[')
    {
        list = parseList(context, parser);
    }

    int len = end - start;

    tfObj *symbol = createObjectSymbol(start, len);
    addUSymbol(context, symbol, list);
    increment_parser(parser);
    return symbol;
}

FunctionTableRow *searchSymbol(tfContext *context, tfObj *o);

void addUSymbol(tfContext *context, tfObj *symbol, tfObj *user_function)
{
    int len = context->available_functions.func_count;
    
    FunctionTableRow *existing_function = searchSymbol(context, symbol); 

    if(existing_function != NULL)
    {
        if(existing_function->user_declared_func != NULL)
        {
            releaseObject(existing_function->user_declared_func);
        }
         
        existing_function->user_declared_func = user_function;
        if(user_function) retainObject(user_function);
        return;
    }
    
    context->available_functions.rows = realloc(context->available_functions.rows, sizeof(FunctionTableRow*) * (len+1));

    context->available_functions.rows[len] = malloc(sizeof(FunctionTableRow));

    context->available_functions.rows[len]->name = symbol;
    retainObject(symbol);

    context->available_functions.rows[len]->user_declared_func = user_function;
    context->available_functions.rows[len]->func = NULL;

    context->available_functions.func_count++;
}

void addCSymbol(tfContext *context, tfObj *symbol, void(*func)(tfContext *context, tfObj *o))
{
    int len = context->available_functions.func_count;
    context->available_functions.rows = realloc(context->available_functions.rows, sizeof(FunctionTableRow*) * (len+1));

    context->available_functions.rows[len] = malloc(sizeof(FunctionTableRow));

    context->available_functions.rows[len]->name = symbol;
    retainObject(symbol);
    
    context->available_functions.rows[len]->user_declared_func = NULL;
    context->available_functions.rows[len]->func = func;

    context->available_functions.func_count++;
}

FunctionTableRow *searchSymbol(tfContext *context, tfObj *o)
{
    for(int i = 0; i < context->available_functions.func_count; i++)
    {
        if(strcmp(o->str.ptr, context->available_functions.rows[i]->name->str.ptr) == 0)
        {
            return context->available_functions.rows[i];
        }
    }    
    return NULL;    
    printf("Function %s not found\n", o->str.ptr);
}

int exec(tfContext *context, tfObj *program);

int callSymbol(tfContext *context, tfObj *o)
{
    FunctionTableRow *row = searchSymbol(context, o); 

    if (row != NULL)
    {
        if (row->func != NULL)
        {
            row->func(context, o);
            return 0;
        }
        else
        {
            if(row->user_declared_func != NULL)
            {
                exec(context, row->user_declared_func);
                return 0;
            }
            else return 1;
        }
    }else
    {
        return 1;
    }
}

void if_keyword(tfContext *context, tfObj *symbol)
{
	int len = context->stack->list.len;
 
    if(len < 3) return;

	tfObj *false_branch = popFromList(context->stack);
	tfObj *true_branch = popFromList(context->stack);
	tfObj *condition = popFromList(context->stack);

    int is_true = 0;
    switch (condition->type)
    {
        case TFOBJ_TYPE_BOOL:
        case TFOBJ_TYPE_INT:
            is_true = condition->i;
            break;
        case TFOBJ_TYPE_STR:
            if(strcmp(condition->str.ptr, "true") == 0) is_true = 1;
            else if(strcmp(condition->str.ptr, "false") == 0)is_true = 0;
            break;
    } 
    
	if(is_true)
	{
        releaseObject(condition);
		releaseObject(false_branch);
        exec(context, true_branch);
	}else
	{
        releaseObject(condition);
		releaseObject(true_branch);
        exec(context, false_branch);
    }
}

int exec(tfContext *context, tfObj *compiled_program)
{
    if(compiled_program == NULL) return 1;
    if(context == NULL) return 1;

    for(int i = 0; i < compiled_program->list.len; i++)
    {
        tfObj *o = compiled_program->list.ele[i];
        if(o == NULL) return 1;
        switch(o->type)
        {
            //case TFOBJ_TYPE_LIST:
            //    exec(context, o);
            //    break;
            case TFOBJ_TYPE_SYMBOL:
                callSymbol(context, o);
                break;
            default:
                pushToList(context->stack, o);
                retainObject(o);
                break;
        }
    }
    return 0;
}

tfObj *compile(tfContext *context, char *program_txt)
{
    tfParser parser;
    parser.program_to_pars = program_txt;
    parser.current_token = program_txt; // e' una stringa (lista di char)
    parser.col = 0;
    tfObj *parsed_obj = createObjectList(0);

    while(parser.current_token) // finche' esiste un token, lo parsiamo
    {
        char *saved_token = parser.current_token;

        skipSpaces(&parser);

        tfObj *o = NULL;

        if(parser.current_token[0] == 0) // Abbiamo trovato la fine della stringa e del programma
        {
            break;
        }
        
        if(parser.current_token == NULL)
        {
            return NULL;
        }

        if(parser.current_token[0] == '\'')
        {
            parseProgram(context, &parser);
        }

        if(isDigitSigned(parser.current_token[0], parser.current_token[1]))
        {
            o = parseNumber(&parser);
        }
        else if(parser.current_token[0] == '[')
        {
            o = parseList(context, &parser);
        }
        else if(isValidChar(&parser))
        {
            o = parseString(&parser);        
        }
        else if(isSymbol(parser.current_token[0]))
        {
            o = parseSymbol(&parser);
        }
        else
        {
            parser.current_token++;
        }

        if(o == NULL)
        {
            char error_char = saved_token[parser.col-1];
            int error_message_length = 20;
            for(int i = 0; i < parser.col - 10; i++)
            {
                saved_token++;
            }
            char error_message[error_message_length+1];
            memcpy(error_message, saved_token, error_message_length);
            error_message[error_message_length] = 0;
            printf("Compile Error at col %d[%c] = ...%s...\n", parser.col, error_char, error_message);
            return NULL;
        }else
        {
            if(o->type == TFOBJ_TYPE_SYMBOL && strcmp(o->str.ptr, "if")==0)
            {

            }
            pushToList(parsed_obj, o);
        } 
    }
    printObject(parsed_obj);
    printf(" -> Compiled\n");
    return parsed_obj;
}

tfContext *createContext()
{
    tfContext *context = malloc(sizeof(tfContext));

    context->stack = createObjectList(0); 
    context->available_functions.rows = NULL; 
    context->available_functions.func_count = 0;

    addCSymbol(context, createObjectSymbol("+", 1), mathFunctions);
    addCSymbol(context, createObjectSymbol("-", 1), mathFunctions);
    addCSymbol(context, createObjectSymbol("*", 1), mathFunctions);
    addCSymbol(context, createObjectSymbol("/", 1), mathFunctions);
    addCSymbol(context, createObjectSymbol("%", 1), mathFunctions);
    addCSymbol(context, createObjectSymbol("=", 1), mathFunctions);

    addCSymbol(context, createObjectSymbol(">", 1), mathFunctions);
    addCSymbol(context, createObjectSymbol("<", 1), mathFunctions);

    addCSymbol(context, createObjectSymbol("if", 2), if_keyword);

    addCSymbol(context, createObjectSymbol("print", 5), printFunction);    
    addCSymbol(context, createObjectSymbol("dup", 3), duplicate);
    
    return context;
}

int main(int argc, char **argv)
{
    printf("-------------------------INIZIO PROGRAMMA----------------------------\n");
    argv[1] = "testgemini.txt";
    argc++;

    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    }

    FILE *fp = fopen(argv[1], "r");     // creaiamo il nostro oggetto (struttura) file passando il nome del file (program.txt)

    if(fp == NULL)
    {
        perror("File does not exist");  // controlliamo se il file esiste
        return 1;
    }

    fseek(fp, 0, SEEK_END);                  // qui diciamo al file di mettere il suo indicatore, di dove e' arrivato a leggere il file, alla fine con offset 0 (quindi rimane sempre alla fine)

    long file_size = ftell(fp);              // Qui invece ci restituisce quanti elementi ha trovato e la posizione attuale dell'indicatore
    //printf("File size is %ld bytes\n", file_size);

    char *program_txt = malloc(file_size+1); // qua allochiamo spazio per il nostro file (mettiamo +1 per il null terminator alla fine \0)
    fseek(fp, 0, SEEK_SET);                  // qua resettiamo l'indicatore all'inzio del file
    fread(program_txt, file_size, 1, fp);    // qua invece copiamo il testo trovato nel file program.txt nel puntatore *char program_txt
    program_txt[file_size] = 0;              // Mettiamo il null terminator alla fine della stringa (file_size e non file_size + 1 perche' stiamo accedendo all'indice che parte da 0)
    
    // printf("File content: %s\n", program_txt);

    fclose(fp); // qua chiudiamo il programma

    //char *first_program = "5 5 +";
    
    tfContext *context = createContext();
    tfObj *compiled_program = compile(context, program_txt);
    printf("\nExec:\n");
    int pass = exec(context, compiled_program);
    printf("Code: %d \n", pass);
    //printObject(context->stack);
    //printf(" -> Executed\n");
    printf("\n");
    releaseObject(compiled_program);

    //context->stack = createObjectList(10);
    //tfObj *tf_first_integer = createObjectInt(5);
    //tfObj *tf_second_integer = createObjectInt(10);
    //tfObj *sum_simbol = createObjectSymbol(sum);

    //context->stack->list.ele[0] = tf_first_integer;
    //context->stack->list.ele[1] = tf_second_integer;

    //printf("%d\n", context->stack->list.ele[0]->i);
    //sum_simbol->symbol.fn(context);

    //printf("%d\n", context->stack->list.ele[0]->i);
    //printf("%d\n", is_digit('a'));

    printf("-------------------------FINE PROGRAMMA----------------------------\n");
    return 0;
}