#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

#include "slice.h"
#include "hashmap.h"
#include "function.h"

struct Interpreter
{
    char const * program;
    char const * current;
};

// Optional int struct for null return
struct optional_int
{
    bool present;
    uint64_t value;
};

// Optional slice struct for null return
struct optional_slice
{
    bool present;
    struct Slice value;
};

uint64_t whileCounter = 5; // Count number of while loops in fun file
uint64_t ifCounter = 5; // Count number of if statements in fun file

void expression(bool effects, struct Interpreter *_interpreter, struct HashMap map);

void statements(bool effects, struct Interpreter *_interpreter, struct HashMap map);

void printFunction(struct Interpreter *_interpreter);

char *clearUntilClosingParen(struct Interpreter *_interpreter, size_t count);

void runFunction(bool effects, struct Interpreter *_interpreter, const char *name, struct HashMap map);

// Terminate program
noreturn void fail(struct Interpreter *_interpreter)
{
    char const *current = _interpreter->current;
    char const *program = _interpreter->program;

    printf("failed at offset %ld\n", (size_t)(current - program));
    printf("%s\n", current);
    exit(1);
}

void end_or_fail(struct Interpreter *_interpreter)
{
    char const *current = _interpreter->current;

    while (isspace(*current))
    {
        current += 1;
    }

    if (*current != 0)
    {
        fail(_interpreter);
    }
}

// Constructor for interpreter
struct Interpreter *constructor1(char const *prog)
{
    struct Interpreter *_interpreter = (struct Interpreter *) malloc(sizeof(struct Interpreter));
    _interpreter->program = prog;
    _interpreter->current = prog;
    return _interpreter;
}

// Remove whitespace in String
void skip(struct Interpreter *_interpreter)
{
    char const *current = _interpreter->current;

    while (isspace(*current))
    {
        current += 1;
    }

    // Update value of current
    _interpreter->current = current;
}

// Parse the current line as a String
bool consume(const char *str, struct Interpreter *_interpreter)
{
    skip(_interpreter);

    char const *current = _interpreter->current;

    size_t i = 0;
    while (true)
    {
        char const expected = str[i];
        char const found = current[i];

        if (expected == 0)
        {
            /* survived to the end of the expected string */
            current += i;
            _interpreter->current = current;
            return true;
        }
        if (expected != found)
        {
            return false;
        }
        // assertion: found != 0
        i += 1;
    }
}


// Return the integer value of a variable (literal) as a pointer
struct optional_int consume_literal(struct Interpreter *_interpreter)
{
    skip(_interpreter);

    struct optional_int ans;
    ans.value = 0;
    ans.present = false;

    char const *current = _interpreter->current;

    if (isdigit(*current))
    {
        uint64_t v = 0; // Value of literal
        do
        {
            v = 10 * v + ((*current) - '0');
            current += 1;
        } while (isdigit(*current));

        // Update value of current
        _interpreter->current = current;
        ans.present = true;
        ans.value = v;
    }
    return ans;
}

// Return the name of the variable (identifier) as a struct
struct optional_slice consume_identifier(struct Interpreter *_interpreter)
{
    skip(_interpreter);

    char const *current = _interpreter->current;

    struct optional_slice v;
    v.present = false;

    if (isalpha(*current)) // Check if current character is alphabetic
    {
        char const *start = current;
        do
        {
            current += 1;
        } while (isalnum(*current)); // Check if current character is alphanumeric

        struct Slice _slice = new_slice1(start, (size_t)(current - start));
        v.present = true;
        v.value = _slice;
        // Update value of current
        _interpreter->current = current;
    }
    return v;
}

// Function used for edge cases w/ calling functions, checks if paren is placed after "fun"
bool consumeFunction(const char *str, struct Interpreter *_interpreter)
{
    skip(_interpreter);

    char const *current = _interpreter->current;

    size_t i = 0;
    while (true)
    {
        char const expected = str[i];
        char const found = current[i];

        if (expected == 0)
        {
            /* survived to the end of the expected string */
            current += i;
            _interpreter->current = current;

                // If we have found the paren, exit
            if (consume("(", _interpreter))
            {
                return true;
            }
            else
            {
                current -= i;
                _interpreter->current = current;
                return false;
            }
        }
        if (expected != found)
        {
            return false;
        }

        i += 1;
    }
}

void e1(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    // Get the identifier of the variable
    struct optional_slice testid = consume_identifier(_interpreter);

    if (testid.present) // Check if slice is returned
    {
        struct Slice id = testid.value;
        char *char_id = malloc(id.len + 1);
        strncpy(char_id, id.start, id.len);
        char_id[id.len] = '\0';

            // Check for function
        if (consume("(", _interpreter))
        {
            // We have found a function, run it and return the result
            runFunction(effects, _interpreter, char_id, map);
            puts("push %rax");

            // Reset rax to 0
            puts("mov $0, %rax");
            return;
        }

        // We have found a variable instead
        int displacement = get_value(char_id, map);

        // Get its displacement relative to the frame pointer
        printf("%s%d%s\n", "mov ", displacement, "(%rbp), %r8");
        puts("push %r8");
        return;
    }

    // Get value of variable
    struct optional_int literal_ = consume_literal(_interpreter);

    // Get the value of the variable that is not already stored in the map
    if (literal_.present)
    {
        // Push the literal value of the variable on the stack
        printf("%s%lu%s\n", "mov $", literal_.value, ", %r9");
        puts("push %r9");
        return;
    }
    else if (consume("(", _interpreter)) // Check for parantheses
    {
        expression(effects, _interpreter, map);
        consume(")", _interpreter);
        return;
    }
    else
    {
        fail(_interpreter);
    }
}

// ++ -- unary+ unary- ... (Right)
void e2(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    size_t count = 0;
    skip(_interpreter);
    char const *current = _interpreter->current;

    if (*current == '!')
    {
        // Count number of exclamation marks
        do
        {  
            current += 1;
            count++;
        } while (*current == '!');
        // Update value of current
        _interpreter->current = current;
    }

    e1(effects, _interpreter, map);

    // Check for divisibility of number of exclamation marks

    if (count > 0)
    {
        count %= 2;
        if (count) {
            // Perform xor operation on register once for odd number of exclamations

            puts("pop %rbx");
            puts("xor %rax, %rax");
            puts("test %rbx, %rbx");
            puts("setz \%al");
            puts("push %rax");
        } else {
            // Perform xor operation on register once for odd number of exclamations

            puts("pop %rbx");
            puts("xor %rax, %rax");
            puts("test %rbx, %rbx");
            puts("setz \%al");
            puts("push %rax");

            puts("pop %rbx");
            puts("xor %rax, %rax");
            puts("test %rbx, %rbx");
            puts("setz \%al");
            puts("push %rax");
        }
    }

    return;
}

// * / % (Left)
void e3(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e2(effects, _interpreter, map);

    while (true)
    {
            // Check for multiplication
        if (consume("*", _interpreter))
        {
            e2(effects, _interpreter, map);
            puts("pop %r11");
            puts("pop %r12");
            puts("imul %r11, %r12");
            puts("push %r12");
        }

            // Check for division
        else if (consume("/", _interpreter))
        {
            e2(effects, _interpreter, map);
            puts("pop %r11");
            puts("pop %r12");
            puts("mov $0, %rdx");
            puts("mov %r12, %rax");           
            puts("div %r11"); 
            puts("push %rax"); // Quotient is stored in rax, push back onto stack
        }

            // Check for modulus
        else if (consume("%", _interpreter))
        {
            e2(effects, _interpreter, map);
            puts("pop %r11");
            puts("pop %r12");
            puts("mov $0, %rdx");
            puts("mov %r12, %rax");
            puts("div %r11");
            puts("push %rdx"); // Remainder is stored in rdx, push back onto stack
        }
        else
        {
            return;
        }
    }
}

// (Left) + -
void e4(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e3(effects, _interpreter, map);

    while (true)
    {
            // Check for addition
        if (consume("+", _interpreter))
        {
            e3(effects, _interpreter, map);
            puts("pop %r11");
            puts("pop %r12");
            puts("add %r11, %r12");
            puts("push %r12");
        }

            // Check for subtraction
        else if (consume("-", _interpreter))
        {
            e3(effects, _interpreter, map);
            puts("pop %r11");
            puts("pop %r12");
            puts("sub %r11, %r12");
            puts("push %r12");
        }
        else
        {
            return;
        }
    }
}

// << >>
void e5(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e4(effects, _interpreter, map);
}

// < <= > >=
void e6(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e5(effects, _interpreter, map);

    while (true)
    {
            // Check if v1 < v2
        if (consume("<", _interpreter))
        {
                // Check if v1 <= v2
            if (consume("=", _interpreter))
            {
                e5(effects, _interpreter, map);

                puts("pop %r9");
                puts("pop %r8");
                puts("cmp %r9, %r8");
                puts("setbe %r9b"); // Set byte register of r9 
                puts("movzbl %r9b, %r9"); // Clear all other bytes in r9
                puts("push %r9"); // Push all bytes back to stack
            }
            else
            {
                e5(effects, _interpreter, map);

                puts("pop %r9");
                puts("pop %r8");
                puts("cmp %r9, %r8");
                puts("setb %r9b"); // Set byte register of r9 
                puts("movzbl %r9b, %r9"); // Clear all other bytes in r9
                puts("push %r9"); // Push all bytes back to stack
            }
        }

            // Check if v1 > v2
        else if (consume(">", _interpreter))
        {
                // Check if v1 >= v2
            if (consume("=", _interpreter))
            {
                e5(effects, _interpreter, map);

                puts("pop %r9");
                puts("pop %r8");
                puts("cmp %r9, %r8");
                puts("setae %r9b"); // Set byte register of r9 
                puts("movzbl %r9b, %r9"); // Clear all other bytes in r9
                puts("push %r9"); // Push all bytes back to stack
            }
            else
            {
                e5(effects, _interpreter, map);

                puts("pop %r9");
                puts("pop %r8");
                puts("cmp %r9, %r8");
                puts("seta %r9b"); // Set byte register of r9 
                puts("movzbl %r9b, %r9"); // Clear all other bytes in r9
                puts("push %r9"); // Push all bytes back to stack
            }
        }
        else
        {
            return;
        }
    }
}

// == !=
void e7(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e6(effects, _interpreter, map);

    while (true)
    {
        // Check if v1 == v2
        if (consume("==", _interpreter))
        {
            e6(effects, _interpreter, map);

            puts("pop %r9");
            puts("pop %r8");
            puts("cmp %r9, %r8");
            puts("sete %r9b"); // Set byte register of r9 
            puts("movzbl %r9b, %r9"); // Clear all other bytes in r9
            puts("push %r9"); // Push all bytes back to stack
        }

            // Check if v1 != v2
        else if (consume("!=", _interpreter))
        {
            e6(effects, _interpreter, map);

            puts("pop %r9");
            puts("pop %r8");
            puts("cmp %r9, %r8");
            puts("setne %r9b"); // Set byte register of r9 
            puts("movzbl %r9b, %r9"); // Clear all other bytes in r9
            puts("push %r9"); // Push all bytes back to stack
        }
        else
        {
            return;
        }
    }
}

// (left) &
void e8(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e7(effects, _interpreter, map);
}

// ^
void e9(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e8(effects, _interpreter, map);
}

// |
void e10(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e9(effects, _interpreter, map);
}

// &&
void e11(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e10(effects, _interpreter, map);

    while (true)
    {
            // Check if v1 && v2
        if (consume("&&", _interpreter))
        {
            e10(effects, _interpreter, map);

            puts("pop %r9");
            puts("pop %r8");
            puts("cmpq $0, %r9");
            puts("setne %r10b"); // Set byte value of r10 register into 0 == v1
            puts("cmpq $0, %r8");
            puts("setne %r11b"); // Set byte value of r11 register into 0 == v2
            puts("andb %r11b, %r10b"); // Find byte value of r10 & r11
            puts("movzbl %r10b, %r10"); // Clear all other bytes in register
            puts("push %r10"); // Push final value back to stack
        }
        else
        {
            return;
        }
    }
}

// ||
void e12(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e11(effects, _interpreter, map);

    while (true)
    {
        // Check if v1 || v2
        if (consume("||", _interpreter))
        {
            e11(effects, _interpreter, map);

            puts("pop %r9");
            puts("pop %r8");
            puts("cmpq $0, %r9");
            puts("setne %r10b"); // Set byte value of r10 register into 0 == v1
            puts("cmpq $0, %r8");
            puts("setne %r11b"); // Set byte value of r11 register into 0 == v2
            puts("orb %r11b, %r10b"); // Find byte value of r10 & r11
            puts("movzbl %r10b, %r10"); // Clear all other bytes in register
            puts("push %r10"); // Push final value back to stack
        }
        else
        {
            return;
        }
    }
}

// (right with special treatment for middle expression) ?:
void e13(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e12(effects, _interpreter, map);
}

// = += -= ...
void e14(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e13(effects, _interpreter, map);
}

// ,
void e15(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e14(effects, _interpreter, map);
}

// Parse the arithmetic expression recursively
void expression(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    e15(effects, _interpreter, map);
}

// Add assembly code for printf function
void printFunction(struct Interpreter *_interpreter) {
    puts("mov $0,%rax");
    puts("lea format(%rip),%rdi");
    puts("pop %rsi"); // Pop value to print from stack, store in rsi
    puts(".extern printf");
    puts("call printf");
}

// This method skips through text until a closing bracket is reached, while noting for another parentheses in between
void clearUntilClosingBracket(struct Interpreter *_interpreter)
{
    size_t count = 1;

    char const *current = _interpreter->current;

    while (true)
    {
        // Add to count if we found opening bracket, subtract if we found closing
        if (*current == '{')
        {
            count++;
        }
        else if (*current == '}')
        {
            count--;
        }

        // Check if we reach the end of the file or brackets aren't balanced
        if (*current == '\0' || count < 0)
        {
            fail(_interpreter);
        }

        current += 1;

        // If brackets are even, we can break out of function
        if (count == 0)
        {
            break;
        }
    }

    _interpreter->current = current;
}

// This clears comments
void clearLine(struct Interpreter *_interpreter)
{
    char const *current = _interpreter->current;

    while (true)
    {
        // If the current character if newline or EOF, we have reached end of line
        if (*current == '\n')
        {
            break;
        } else if (*current == EOF) {
            break;
        }

        current += 1;
    }

    _interpreter->current = current;

    if (*current != '\0') {
        _interpreter->current++;
    }
}

// Function used for edge cases w/ if/else and while statements, checks if bracket is placed after if/else and while calls
bool consumeBracket(const char *str, struct Interpreter *_interpreter)
{
    skip(_interpreter);

    char const *current = _interpreter->current;

    size_t i = 0;
    while (true)
    {
        char const expected = str[i];
        char const found = current[i];

        if (expected == 0)
        {
            /* survived to the end of the expected string */
            current += i;
            _interpreter->current = current;

                // If we have reached the bracket, return
            if (consume("{", _interpreter))
            {
                return true;
            }
            else
            {
                current -= i;
                _interpreter->current = current;
                return false;
            }
        }
        if (expected != found)
        {
            return false;
        }

        i += 1;
    }
}

void parseWhile(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    char const *current = _interpreter->current;

    skip(_interpreter);

    // Set boolean condition in stack
    expression(effects, _interpreter, map);

    consume(")", _interpreter);
    consume("{", _interpreter);

    // Get new while counter index to label function
    whileCounter++;
    uint64_t originalWhileCounter = whileCounter;

    // Determine boolean condition of while loop
    puts("pop %r12");
    puts("cmpq $0, %r12");
    printf("%s%ld\n", "je ENDWHILE", originalWhileCounter);
    printf("%s%ld%s\n", "WHILE", originalWhileCounter, ":");

    // Get assembly within while loop
    statements(effects, _interpreter, map);
    _interpreter->current = current;   

    // Check for boolean condition again
    expression(effects, _interpreter, map);
    consume(")", _interpreter);
    consume("{", _interpreter);
    puts("pop %r12");
    puts("cmpq $1, %r12");

    // Jump to start again if condition is true
    printf("%s%ld\n", "jge WHILE", originalWhileCounter);
    
    // Jump to end and exit out of while loop
    printf("%s%ld%s\n", "ENDWHILE", originalWhileCounter, ":");
    clearUntilClosingBracket(_interpreter);
}

void parseIfElse(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    skip(_interpreter);

    // Set boolean condition in stack
    expression(effects, _interpreter, map); 

    consume(")", _interpreter);
    consume("{", _interpreter);

    // Get new if counter index to label function
    ifCounter++;
    uint64_t originalIfCounter = ifCounter;

    // Determine if boolean condition is true
    puts("pop %r12");
    puts("cmpq $0, %r12");

    // If not, jump to else statement
    printf("%s%ld\n", "je ELSE", originalIfCounter);
    statements(effects, _interpreter, map);

    // Run statements inside if portion
    printf("%s%ld\n", "jmp ENDIF", originalIfCounter);

    // Check if condition is false and we enter else
    printf("%s%ld%s\n", "ELSE", originalIfCounter, ":");
    if (consumeBracket("else", _interpreter)) {
        // If so, run statements inside else
        statements(effects, _interpreter, map);
    }

    // Exit out of if statement
    printf("%s%ld%s\n", "ENDIF", originalIfCounter, ":");
}

// Check if 2 strings are equal
bool checkEqualString(char const *a, char *b, size_t len)
{
    while (*b != 0 && *a != 0)
    { 
        if (*a != *b) // Current character i is not equal in a and b
        {
            return false;
        }
        a++;
        b++;
    }

    return *b == 0 && *a == 0; // Check if both strings are same length
}


// This method returns text until a closing parenthesis is reached, accounting for other parens in between
char *clearUntilClosingParen(struct Interpreter *_interpreter, size_t count)
{
    skip(_interpreter);
    // Skip through if else
    size_t i = 0;

    char const *current = _interpreter->current;

    // Code within function to be returned
    char *currString = malloc(0);

    size_t maxSize = 0;

    while (true)
    {
        char const found = current[i];

        if (found == '\0') // If we reach end of file, fail
        {
            fail(_interpreter);
        }
        else if (found == '(') // If we reach opening paren, increment count of open parens
        {
            count++;
        } 
        else if (found == ')') // If we reach opening paren, decrement count of open parens
        {
            count--;
        }

            // Have we reached the final closing paren?
        if (count == 0)
        {
            break;
        }

            // If we reach max size, reallocate more memory for the string
        if (i + 1 >= maxSize)
        {
            maxSize = maxSize * 2 + 1;
            currString = realloc(currString, sizeof(char) * maxSize);
        }

        currString[i] = found;

        i++;
    }

    if (currString != NULL)
    {
        currString[i] = '\0';
    }

    current += i;
    _interpreter->current = current;

    return currString;
}

// Helper function used to generate assembly for a function
void parseFunction(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    skip(_interpreter);

    // Get name of function
    struct optional_slice test_func_name = consume_identifier(_interpreter);

    // If the function does not exist, fail
    if (!test_func_name.present) {
        fail(_interpreter);
    }

    struct Slice id = test_func_name.value;

    // Get function name from map
    char *function_name = (char *) malloc(id.len + 1);
    strncpy(function_name, id.start, id.len);
    function_name[id.len] = '\0';

    consume("(", _interpreter);
    free(clearUntilClosingParen(_interpreter, 1));
    consume(")", _interpreter);
    consume("{", _interpreter);

    if (checkEqualString(function_name, "main", 4)) {
        printf("%s\n", ".global main");
    }

    // Declare function
    printf("%s%s\n", function_name, ":");
    // Update stack and frame pointers 
    puts("push %rbp");
    puts("mov %rsp, %rbp");

    // Allocate space on stack for all variables in functions
    printf("%s%ld%s\n", "sub $", (map.SIZE - get_function(function_name)->numParams) * 8, ", %rsp");

    // Get assembly within function
    statements(true, _interpreter, map);

    // Remove space on stack for all variables in functions
    printf("%s%ld%s\n", "add $", (map.SIZE - get_function(function_name)->numParams) * 8, ", %rsp");

    // Reset stack/frame pointers
    puts("mov %rbp, %rsp");
    puts("pop %rbp");
    puts("mov $0,%rax");
    puts("ret");
}

int countFunctions(struct Interpreter *_interpreter) {
    char const *prev = _interpreter->current;
    int ans = 0;

    while (true) {
        if (consume("fun ", _interpreter)) {
            ans++;

            // Get name of function
            struct optional_slice test_func_name = consume_identifier(_interpreter);

            if (!test_func_name.present) {
                fail(_interpreter);
            }

            struct Slice id = test_func_name.value;

            // Get parameters of function
            char *function_name = (char *)malloc(id.len + 1);
            strncpy(function_name, id.start, id.len);
            function_name[id.len] = '\0';

            consume("(", _interpreter);

            size_t i = 0;

            // Get parameters of function
            char *allParameters = clearUntilClosingParen(_interpreter, 1);
            consume(")", _interpreter);
            consume("{", _interpreter);

            char *buffer = strtok(allParameters, ",");
            char **parameters = malloc(0); // Store all parameters in char of pointers of pointeres

            // Delimit parameters with comma
            while (buffer)
            {
                while (buffer && *buffer == '\040')
                {
                    buffer++;
                }
                if (!buffer) {
                    break;
                }

                // Reallocate space for string, get parameter
                parameters = realloc(parameters, (i + 1) * sizeof(char *));
                parameters[i] = malloc(strlen(buffer));
                strcpy(parameters[i], buffer);

                int count = 0;
                while (isalnum(parameters[i][count])) {
                    count++;
                }
                parameters[i][count] = '\0';

                // Check for ending whitespace
                buffer = strtok(NULL, ",");
                while (buffer && *buffer == '\040')
                {
                    buffer++;
                }
                i++;
            }

            // Create function struct
            struct Function *func = (struct Function *) malloc(sizeof(struct Function));
            func->params = parameters;
            func->numParams = i;

            // Add function to function hashmap
            insert_function(function_name, func);

            clearUntilClosingBracket(_interpreter);
            skip(_interpreter);

            free(allParameters);
        } else if (consume("#", _interpreter)) { // Check for comments
            const char *current = _interpreter->current;

            while (true)
            {
                // If we have reached new line or end of file, break
                if (*current == EOF || *current == '\n')
                {
                    break;
                }

                current += 1;
            }

            // If we have reached the end, break out of the entire loop
            if (*current == EOF) {
                break;
            }

            _interpreter->current = current;
        } else {
            break;
        }
    }
    _interpreter->current = prev;
    return ans;
}

struct HashMap countVariables(struct Interpreter *_interpreter) {
    skip(_interpreter);

    // Get name of function
    struct optional_slice test_func_name = consume_identifier(_interpreter);

    if (!test_func_name.present) {
        fail(_interpreter);
    }

    struct Slice id = test_func_name.value;

    // Get name of function
    char *function_name = (char *)malloc(id.len + 1);
    strncpy(function_name, id.start, id.len);
    function_name[id.len] = '\0';

    consume("(", _interpreter);
    free(clearUntilClosingParen(_interpreter, 1));
    consume(")", _interpreter);
    consume("{", _interpreter);

    struct HashMap map = init_variable_table();

    size_t count = 1;

    // Get function parameters
    char** parameters = get_function(function_name)->params;

    int displacement = 16;

    for (int j = get_function(function_name)->numParams - 1; j >= 0; j--) {
        map = add_value(parameters[j], displacement, map);
        displacement += 8;
    }

    displacement = -8;

    // Count number of local variables declared within function
    while (true) {
        if (*_interpreter->current == '{')
        {
            count++;
            _interpreter->current = _interpreter->current+1;
            continue;
        }
        else if (*_interpreter->current == '}')
        {
            count--;
            if (count == 0) {
                break;
            }
            _interpreter->current = _interpreter->current+1;
        }

        struct optional_slice testid = consume_identifier(_interpreter);

        // If we find a local variable, add it to the map and update displacement
        if (testid.present && consume("=", _interpreter)) {
            char *char_id = (char *) malloc(testid.value.len + 1); // Name of variable
            strncpy(char_id, testid.value.start, testid.value.len);
            char_id[testid.value.len] = '\0';
            if (!contains_key(char_id, map)) {
                map = add_value(char_id, displacement, map);
                displacement -= 8;
            }
        }

        char const *current = _interpreter->current;

        // Clear opening/closing brackets of function and all code
        while (true)
        {
            if (*current == '\n')
            {
                break;
            } else if (*current == EOF) {
                break;
            }

            if (*current == '{') {
                count++;
            } else if (*current == '}') {
                count--;
            }

            current += 1;
        }

        _interpreter->current = current;

        // Have we reached the end of the file?
        if (*current != '\0') {
            _interpreter->current++;
        } else {
            break;
        }
        
    }   

    return map;
}

// Generates assembly code to prepare a function to be called
void runFunction(bool effects, struct Interpreter *_interpreter, const char *name, struct HashMap map)
{
    struct Function *func = get_function(name);

    // Check if function exists in map
    if (func != NULL)
    {
        // Allocate space on stack for parameter storage
        printf("%s%lu%s\n", "sub $", (func->numParams * 8), ", %rsp");

            // Get all parameters of function
        char *allParameters = clearUntilClosingParen(_interpreter, 1);
        consume(")", _interpreter);

        size_t i = 0; // Current position in string
        size_t paramNum = 0; // Number of parameters

        while (allParameters[i])
        {
            size_t parens = 0; // Number of parentheses
            char *currString = ""; // Current parameter

                // Delimit each parameter by commas
            while (allParameters[i] && (allParameters[i] != ',' || parens > 0))
            {
                char found = allParameters[i];

                if (found == '(')
                {
                    parens++;
                }
                else if (found == ')')
                {
                    parens--;
                }

                if (parens < 0)
                {
                    fail(_interpreter);
                }

                        // Append 1 character to string
                size_t sz = strlen(currString);
                char *str = malloc(sz + 2);
                strcpy(str, currString);
                str[sz] = allParameters[i];
                str[sz + 1] = '\0';
                currString = str;

                i++;
            }

                // Check for comma
            if (allParameters[i] == ',')
            {
                i++;
            }

                // Check for whitespace
            while (allParameters[i] && allParameters[i] == '\040')
            {
                i++;
            }

            // Get calculated value of each parameter
            struct Interpreter *param_interpreter = constructor1(currString);
                expression(true, param_interpreter, map);

            puts("pop %r8");

            paramNum++;
            
            // Move the parameter literal value onto designated space on stack
            printf("%s%lu%s\n", "mov %r8, ", (func->numParams - paramNum) * 8, "(%rsp)");

            free(currString);
        }
        
        // Call function
        printf("%s%s\n", "call ", name);

        // Remove space on stack for parameter storage
        printf("%s%lu%s\n", "add $", (func->numParams * 8), ", %rsp");

        free(allParameters);
    }
    else
    {
        fail(_interpreter);
    }
}

// Method for running expressions in a global scope
bool statement(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    // Check for print statements
    if (consumeFunction("print", _interpreter))
    {
        // print ...
        if (effects)
        {
            // Call print function
            expression(effects, _interpreter, map);
            printFunction(_interpreter);
            consume(")", _interpreter);
        }
        return true;
    } 
    else if (consumeFunction("while", _interpreter))
    {
        // if (...       )

        if (effects)
        {
            parseWhile(effects, _interpreter, map);
        }
        return true;
    }
    else if (consumeFunction("if", _interpreter))
    {
        // if (...       )

        if (effects)
        {
            parseIfElse(effects, _interpreter, map);
        }
        return true;
    } 
    else if (consume("fun ", _interpreter))
    {
            // fun <FUNCTION_NAME>(..., , ) 
        if (effects)
        {
            const char *current = _interpreter->current;
            struct HashMap map = countVariables(_interpreter); // Get variables for function

            _interpreter->current = current;
            parseFunction(effects, _interpreter, map); // Get assembly for function code
        }
        return true;
    }
    else if (consume("return ", _interpreter))
     {
            // return <EXPRESSION>

        if (effects)
        {
            expression(effects, _interpreter, map);
        }

        // Reset stack and frame pointers
        puts("pop %rax");
        puts("mov %rbp, %rsp");
        puts("pop %rbp");
        puts("ret");

        return true;
    } else if (consume("#", _interpreter)) {

        // Check for comments: #
        if (effects)
        {
            clearLine(_interpreter);
        }
        
        return true;
    }
    else if (consume("}", _interpreter))
    {
            // Check for closing bracket
        return false;
    }

    struct optional_slice testid = consume_identifier(_interpreter);

    if (testid.present)
    {
        struct Slice id = testid.value;

        char *char_id = (char *) malloc(id.len + 1);
        strncpy(char_id, id.start, id.len);
        char_id[id.len] = '\0';

        // Is this an L-value variable assignment?
        if (consume("=", _interpreter)) 
        {
            expression(effects, _interpreter, map);
            int displacement = get_value(char_id, map);

            // Assign R-value ot L value, store on respective space on stack
            puts("pop %r8");
            printf("%s%d%s\n", "mov %r8, ", displacement, "(%rbp)");
        }
        else if (consume("(", _interpreter))
        {
            // Check for function
            runFunction(effects, _interpreter, char_id, map);
            free(char_id);
        }
        else
        {
            fail(_interpreter);
        }

        return true;
    }

    return false;
}

void statements(bool effects, struct Interpreter *_interpreter, struct HashMap map)
{
    // Run program line by line
    while (statement(effects, _interpreter, map));
}

int main() {

    // stdin
    char funFile[5000];
    char curr;
    int i = 0;

    while ((curr = getchar()) != EOF) {
        funFile[i++] = curr;
    }

    funFile[i] = EOF;

    // Headings
    puts(".data");
    puts("format:  .byte '%', 'l', 'u', 10, 0");
    puts(".text");

    // Initialize function hashmap
    init_function_table();
    struct Interpreter *x = constructor1(funFile); // Get Interpreter struct

    // Run before to account for functions that have been called before they have been defined
    countFunctions(x);
    statements(true, x, init_variable_table());

    return 0;
}
