#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>

#include "utils.h"
#include "units.h"
#include "extra.h"
#include "crypto.h"
#include "debug.h"
#include "cause.h"

/*  source
    ├── archive.c
    ├── archive.h
    ├── cause.c [x]
    ├── cause.h
    ├── compiler.c
    ├── compiler.h
    ├── crypto.c
    ├── crypto.h
    ├── curl.c
    ├── curl.h
    ├── debug.c
    ├── debug.h
    ├── extra.c
    ├── extra.h
    ├── library.c
    ├── library.h
    ├── replicate.c
    ├── replicate.h
    ├── runner.c
    ├── runner.h
    ├── units.c
    ├── units.h
    ├── utils.c
    └── utils.h
*/

static
  int
  warning_count=0,error_count=0,header_size=0,code_size=0,data_size=0,
  stack_size=0,total_size=0;
static
  char
  compiler_line[DOG_MAX_PATH]={0},
  compiler_ver[64]={0};

extern causeExplanation ccs[];

static const char *dog_find_warn_err(const char *line)
{
    if (!line || !*line)
        return NULL;
    
    size_t line_len = strlen(line);
    if (line_len == 0 || line_len > DOG_MAX_PATH) {
        return NULL;
    }
    
    for (int cindex = 0; ccs[cindex].cs_t != NULL; ++cindex) {
        if (ccs[cindex].cs_t == NULL || ccs[cindex].cs_i == NULL) {
            continue;
        }
        
        const char *found = strstr(line, ccs[cindex].cs_t);
        if (found != NULL) {
            size_t pattern_len = strlen(ccs[cindex].cs_t);
            if ((size_t)(found - line) + pattern_len <= line_len) {
                return ccs[cindex].cs_i;
            }
        }
    }

    return NULL;
}

static void compiler_detailed(const char *wgoutput,int debug,
                       int warning_count,int error_count,const char *compiler_ver,
                       int header_size,int code_size,int data_size,
                       int stack_size,int total_size)
{
  if (error_count<1)
  println(stdout,
          "Compile Complete! - success. | " FCOLOUR_CYAN "%d pass (warning) " FCOLOUR_DEFAULT "| " FCOLOUR_BLUE "%d fail (error)",
          warning_count,error_count);
  else
  println(stdout,
          "Compile Complete! - failed. | " FCOLOUR_CYAN "%d pass (warning) " FCOLOUR_DEFAULT "| " FCOLOUR_BLUE "%d fail (error)",
          warning_count,error_count);

  println(stdout,"-----------------------------");

  int amx_access=path_exists(wgoutput);
  if (amx_access&&debug != 0&&error_count<1) {

      unsigned long hash=crypto_djb2_hash_file(wgoutput);

      char outbuf[DOG_MAX_PATH];
      int len;

      len=snprintf(outbuf, sizeof outbuf,
          "Output path: %s\n"
          "Header : %dB  |  Total        : %dB\n"
          "Code (static mem)   : %dB  |  hash (djb2)  : %#lx\n"
          "Data (static mem)   : %dB\n"
          "Stack (dynamic mem)  : %dB\n",
          wgoutput,
          header_size,
          total_size,
          code_size,
          hash,
          data_size,
          stack_size
      );

      if (len > 0)
          fwrite(outbuf, 1, (size_t)len, stdout);

      portable_stat_t st;
      if (portable_stat(wgoutput, &st)==0) {

          len=snprintf(outbuf, sizeof outbuf,
              "ino    : %llu   |  File   : %lluB\n"
              "dev    : %llu\n"
              "read   : %s   |  write  : %s\n"
              "execute: %s   |  mode   : %020o\n"
              "atime  : %llu\n"
              "mtime  : %llu\n"
              "ctime  : %llu\n",
              (unsigned long long)st.st_ino,
              (unsigned long long)st.st_size,
              (unsigned long long)st.st_dev,
              (st.st_mode & S_IRUSR) ? "Y" : "N",
              (st.st_mode & S_IWUSR) ? "Y" : "N",
              (st.st_mode & S_IXUSR) ? "Y" : "N",
              st.st_mode,
              (unsigned long long)st.st_latime,
              (unsigned long long)st.st_lmtime,
              (unsigned long long)st.st_mctime
          );

          if (len > 0)
              fwrite(outbuf, 1, (size_t)len, stdout);
      }
  }

  fwrite("\n", 1, 1, stdout);

  {
      char outbuf[512];
      int len=snprintf(outbuf, sizeof outbuf,
          "* Pawn Compiler %s - Copyright (c) 1997-2006, ITB CompuPhase\n",
          compiler_ver
      );
      if (len > 0)
          fwrite(outbuf, 1, (size_t)len, stdout);
  }
  return;
}

void cause_compiler_expl(const char *log_file,const char *wgoutput,int debug)
{
  __create_logging();

  FILE *_log_file=fopen(log_file,"r");
  if(!_log_file)
    return;

  warning_count=0,error_count=0,
  header_size=0,code_size=0,data_size=0,
  stack_size=0,total_size=0;

  memset(compiler_line, 0, sizeof(compiler_line));
  memset(compiler_ver, 0, sizeof(compiler_ver));

  while(fgets(compiler_line,sizeof(compiler_line),_log_file)) {
    if(dog_strcase(compiler_line,"Warnings.") ||
       dog_strcase(compiler_line,"Warning.") ||
       dog_strcase(compiler_line,"Errors.") ||
       dog_strcase(compiler_line,"Error."))
      continue;

    if(dog_strcase(compiler_line,"Header size:")) {
      header_size=strtol(strchr(compiler_line,':')+1,NULL,10);
      continue;
    } else if(dog_strcase(compiler_line,"Code size:")) {
      code_size=strtol(strchr(compiler_line,':')+1,NULL,10);
      continue;
    } else if(dog_strcase(compiler_line,"Data size:")) {
      data_size=strtol(strchr(compiler_line,':')+1,NULL,10);
      continue;
    } else if(dog_strcase(compiler_line,"Stack/heap size:")) {
      stack_size=strtol(strchr(compiler_line,':')+1,NULL,10);
      continue;
    } else if(dog_strcase(compiler_line,"Total requirements:")) {
      total_size=strtol(strchr(compiler_line,':')+1,NULL,10);
      continue;
    } else if(dog_strcase(compiler_line,"Pawn Compiler ")) {
      const char *p=strstr(compiler_line,"Pawn Compiler ");
      if(p) sscanf(p,"Pawn Compiler %63s",compiler_ver);
      continue;
    }

    fwrite(compiler_line,1,strlen(compiler_line),stdout);

    if(dog_strcase(compiler_line,"warning") != false)
      ++warning_count;
    if(dog_strcase(compiler_line,"error") != false)
      ++error_count;

    const char *description=dog_find_warn_err(compiler_line);
    if(description) {
      const char *found=NULL;
      int column=0;
      for(int i=0;ccs[i].cs_t;++i) {
        if((found=strstr(compiler_line,ccs[i].cs_t))) {
          column=found-compiler_line;
          break;
        }
      }
      for(int i=0;i<column;i++)
        putchar(' ');
      pr_color(stdout,FCOLOUR_CYAN,"^ %s \n",description);
    }
  }

  fclose(_log_file);

  compiler_detailed(wgoutput,debug,warning_count,error_count,compiler_ver,header_size,code_size,data_size,stack_size,total_size);
}

causeExplanation ccs[] =
{
/* 001 */  /* SYNTAX ERROR */
{
    "expected token",
    "A required syntactic element is missing from the parse tree. The parser expected one of: a semicolon ';', comma ',', closing parenthesis ')', bracket ']', or brace '}'. This typically indicates a malformed statement, improper expression termination, or incorrect nesting of control structures. Verify the statement's grammatical completeness according to Pawn's context-free grammar."
},

/* 002 */  /* SYNTAX ERROR */
{
    "only a single statement",
    "The `case` label syntactic production permits exactly one statement as its immediate successor. For multiple statements, you must encapsulate them within a compound statement delimited by braces `{ ... }`. This restriction stems from Pawn's simplified switch statement implementation which avoids implicit block creation."
},

/* 003 */  /* SYNTAX ERROR */
{
    "declaration of a local variable must appear in a compound block",
    "Variable declarations within `case` or `default` labels require explicit scoping via compound blocks due to potential jump-to-label issues in the control flow graph. Without explicit braces, the variable's lifetime and scope cannot be properly determined, violating the language's static single assignment analysis."
},

/* 012 */  /* SYNTAX ERROR */
{
    "invalid function call, not a valid address",
    "The identifier preceding the parenthesized argument list does not resolve to a function symbol in the current scope, or the call expression violates the function call syntax. Possible causes: attempting to call a non-function identifier, using incorrect call syntax for function pointers, or encountering a malformed expression that the parser misinterprets as a function call."
},

/* 014 */  /* SYNTAX ERROR */
{
    "invalid statement; not in switch",
    "The `case` or `default` labeled statement appears outside the lexical scope of any `switch` statement. These labels are context-sensitive productions that are only syntactically valid when nested within a switch statement's body according to Pawn's grammar specification section 6.8.1."
},

/* 015 */  /* SEMANTIC ERROR */
{
    "default case must be the last case",
    "The `default` label within a switch statement's case list must appear after all explicit `case` constant expressions. This ordering constraint is enforced by the Pawn Compiler's semantic analysis phase to ensure predictable control flow and to simplify the generated jump table implementation."
},

/* 016 */  /* SEMANTIC ERROR */
{
    "multiple defaults in switch",
    "A `switch` statement contains more than one `default` label, which creates ambiguous control flow. According to the language specification (ISO/IEC TR 18037:2008), each switch statement may have at most one default case to serve as the catch-all branch in the decision tree."
},

/* 019 */  /* SEMANTIC ERROR */
{
    "not a label",
    "The identifier following the `goto` keyword does not correspond to any labeled statement in the current function's scope. Label resolution occurs during the semantic analysis phase, and the target must be a label defined earlier in the same function body (forward jumps are permitted)."
},

/* 020 */  /* SYNTAX ERROR */
{
    "invalid symbol name",
    "The identifier violates Pawn's lexical conventions for symbol names. Valid identifiers must match the regular expression: `[_@a-zA-Z][_@a-zA-Z0-9]*`. The '@' character has special significance for public/forward declarations and must be used consistently throughout the symbol's lifetime."
},

/* 036 */  /* SYNTAX ERROR */
{
    "empty statement",
    "A standalone semicolon constitutes a null statement, which Pawn's grammar specifically disallows in most contexts to prevent accidental emptiness. If intentional empty statement is needed, use an explicit empty block `{}`. Note that double semicolons `;;` often indicate a missing statement between them."
},

/* 037 */  /* SYNTAX ERROR */
{
    "missing semicolon",
    "A statement terminator (';') is required but absent. In Pawn's LL(1) grammar, semicolons terminate: expression statements, declarations, iteration statements, jump statements, and return statements. The parser's predictive parsing table expected this token to complete the current production."
},

/* 030 */  /* SYNTAX ERROR */
{
    "unexpected end of file",
    "The lexical analyzer reached EOF while the parser was still expecting tokens to complete one or more grammatical constructs. Common causes: unclosed block comment `/*`, string literal without terminating quote, unmatched braces/parentheses/brackets, or incomplete function/control structure."
},

/* 027 */  /* SYNTAX ERROR */
{
    "illegal character",
    "The source character (codepoint) is not valid in the current lexical context. Outside of string literals and comments, Pawn only accepts characters from its valid character set (typically ASCII or the active codepage). Control characters (0x00-0x1F) except whitespace are generally illegal."
},

/* 026 */  /* SYNTAX ERROR */
{
    "missing closing parenthesis",
    "An opening parenthesis '(' lacks its corresponding closing ')', creating an unbalanced delimiter sequence. This affects expression grouping, function call syntax, and condition specifications. The parser's delimiter stack detected this mismatch during syntax tree construction."
},

/* 028 */  /* SYNTAX ERROR */
{
    "missing closing bracket",
    "An opening bracket '[' was not matched with a closing ']'. This affects array subscripting, array declarations, and sizeof expressions. The bracket matching algorithm in the parser's shift-reduce automaton failed to find a closing bracket before the relevant scope ended."
},

/* 054 */  /* SYNTAX ERROR */
{
    "missing closing brace",
    "An opening brace '{' lacks its corresponding closing '}'. This affects compound statements, initializer lists, and function bodies. The brace nesting counter in the lexical analyzer reached EOF without returning to zero, indicating structural incompleteness."
},

/* 004 */  /* SEMANTIC ERROR */
{
    "is not implemented",
    "A function prototype was declared (forward reference) but no corresponding definition appears in the translation unit. This may also indicate that the compiler's symbol table contains an unresolved external reference, possibly due to a previous function's missing closing brace causing the parser to incorrectly associate following code."
},

/* 005 */  /* SEMANTIC ERROR */
{
    "function may not have arguments",
    "The `main()` function, serving as the program entry point, must have signature `main()` with zero parameters. This restriction ensures consistent program initialization across all Pawn implementations and prevents ambiguity in startup argument passing conventions."
},

/* 006 */  /* SEMANTIC ERROR */
{
    "must be assigned to an array",
    "String literals are rvalues of type 'array of char' and can only be assigned to compatible array types. The assignment operator's left operand must be an array lvalue with sufficient capacity (including null terminator). This is enforced during type checking of assignment expressions."
},

/* 007 */  /* SEMANTIC ERROR */
{
    "operator cannot be redefined",
    "Attempt to overload an operator that Pawn does not support for overloading. Only a specific subset of operators (typically arithmetic and comparison operators) can be overloaded via operator functions. Consult the language specification for the exhaustive list of overloadable operators."
},

/* 008 */  /* SEMANTIC ERROR */
{
    "must be a constant expression; assumed zero",
    "The context requires a compile-time evaluable constant expression but received a runtime expression. This affects: array dimension specifiers, case labels, bit-field widths, enumeration values, and preprocessor conditionals. The constant folder attempted evaluation but found variable references or non-constant operations."
},

/* 009 */  /* SEMANTIC ERROR */
{
    "invalid array size",
    "Array dimension specifier evaluates to a non-positive integer. Array sizes must be ≥1. For VLAs (Variable Length Arrays), the size expression must evaluate to positive at the point of declaration. This check occurs during array type construction in the type system."
},

/* 017 */  /* SEMANTIC ERROR */
{
    "undefined symbol",
    "Identifier lookup in the current scope chain failed to find any declaration for this symbol. The compiler traversed: local block scope → function scope → file scope → global scope. Possible causes: typographical error, missing include directive, symbol declared in excluded conditional compilation block, or incorrect namespace/visibility qualifiers."
},

/* 018 */  /* SEMANTIC ERROR */
{
    "initialization data exceeds declared size",
    "The initializer list contains more elements than the array's declared capacity. For aggregate initialization, the number of initializer-clauses must not exceed the array bound. For string literals, the literal length (including null terminator) must not exceed array size."
},

/* 022 */  /* SEMANTIC ERROR */
{
    "must be lvalue",
    "The left operand of an assignment operator (=, +=, etc.) does not designate a modifiable location in storage. Valid lvalues include: variables, array subscript expressions, dereferenced pointers, and structure/union members. Constants, literals, and rvalue expressions cannot appear on the left of assignment."
},

/* 023 */  /* SEMANTIC ERROR */
{
    "array assignment must be simple assignment",
    "Arrays cannot be used with compound assignment operators due to the semantic complexity of element-wise operations. Only simple assignment '=' is permitted for array types, which performs memcpy-like behavior. For element-wise operations, explicit loops or functions must be used."
},

/* 024 */  /* SEMANTIC ERROR */
{
    "break or continue is out of context",
    "A `break` statement appears outside any switch/loop construct, or `continue` appears outside any loop construct. These jump statements are context-sensitive and require specific enclosing syntactic structures. The control flow graph builder validates these constraints."
},

/* 025 */  /* SEMANTIC ERROR */
{
    "function heading differs from prototype",
    "Function definition signature does not match previous declaration in: return type, parameter count, parameter types (including qualifiers), or calling convention. This violates the one-definition rule and causes type incompatibility in the function type consistency check."
},

/* 027 */  /* SEMANTIC ERROR */
{
    "invalid character constant",
    "Character constant syntax error: multiple characters in single quotes, unknown escape sequence, or numeric escape sequence out of valid range (0-255). Valid escape sequences are: \\a, \\b, \\e, \\f, \\n, \\r, \\t, \\v, \\\\, \\', \\\", \\xHH, \\OOO (octal)."
},

/* 029 */  /* SEMANTIC ERROR */
{
    "invalid expression, assumed zero",
    "The expression parser encountered syntactically valid but semantically meaningless construct, such as mismatched operator operands, incorrect operator precedence binding, or type-incompatible operations. The expression evaluator defaults to zero to allow continued parsing for additional error detection."
},

/* 032 */  /* SEMANTIC ERROR */
{
    "array index out of bounds",
    "Subscript expression evaluates to value outside array bounds [0, size-1]. This is a compile-time check for constant indices; runtime bounds checking depends on implementation. For multidimensional arrays, each dimension is checked independently."
},

/* 045 */  /* SEMANTIC ERROR */
{
    "too many function arguments",
    "Function call contains more than 64 actual arguments, exceeding Pawn's implementation limit. This architectural constraint stems from the virtual machine's call frame design and register allocation scheme. Consider refactoring using structures or arrays for parameter groups."
},

/* 203 */  /* WARNING */
{
    "symbol is never used",
    "Variable, constant, or function declared but never referenced in any reachable code path. This may indicate: dead code, incomplete implementation, debugging remnants, or accidental omission. The compiler's data flow analysis determined no read operations on the symbol after its declaration."
},

/* 204 */  /* WARNING */
{
    "symbol is assigned a value that is never used",
    "Variable receives a value (via assignment or initialization) that is subsequently never read. This suggests: unnecessary computation, redundant initialization, or logical error where the variable should be used but isn't. The live variable analysis tracks definitions and uses."
},

/* 205 */  /* WARNING */
{
    "redundant code: constant expression is zero",
    "Conditional expression in if/while/for evaluates to compile-time constant false (0), making the controlled block dead code. This often results from: macro expansion errors, contradictory preprocessor conditions, or logical errors in constant expressions. The constant folder detected this during control flow analysis."
},

/* 209 */  /* SEMANTIC ERROR */
{
    "should return a value",
    "Non-void function reaches end of control flow without returning a value via return statement. All possible execution paths must return a value of compatible type. The control flow graph analyzer found at least one path terminating at function end without return."
},

/* 211 */  /* WARNING */
{
    "possibly unintended assignment",
    "Assignment expression appears in boolean context where equality comparison is typical (e.g., if condition). The expression `if (a=b)` assigns b to a, then tests a's truth value. If comparison was intended, use `if (a == b)`. This heuristic warning triggers on assignment in conditional context."
},

/* 010 */  /* SYNTAX ERROR */
{
    "invalid function or declaration",
    "The parser expected a function declarator or variable declaration but encountered tokens that don't conform to declaration syntax. This can indicate: misplaced storage class specifiers, incorrect type syntax, missing identifier, or malformed parameter list. Verify the declaration follows Pawn's declaration syntax exactly."
},

/* 213 */  /* SEMANTIC ERROR */
{
    "tag mismatch",
    "Type compatibility violation: expression type differs from expected type in assignment, argument passing, or return context. Pawn's tag system enforces type safety for numeric types. The type checker found incompatible tags between source and destination types."
},

/* 215 */  /* WARNING */
{
    "expression has no effect",
    "Expression statement computes a value but doesn't produce side effects or store result. Examples: `a + b;` or `func();` where func returns value ignored. This often indicates: missing assignment, incorrect function call, or leftover debug expression. The side-effect analyzer detected pure expression without observable effect."
},

/* 217 */  /* WARNING */
{
    "loose indentation",
    "Inconsistent whitespace usage (spaces vs tabs, or varying indentation levels) detected. While syntactically irrelevant, inconsistent indentation impairs readability and may indicate structural misunderstandings. The lexer tracks column positions and detects abrupt indentation changes."
},

/* 234 */  /* WARNING */
{
    "Function is deprecated",
    "Function marked with deprecated attribute via `forward deprecated:` or similar. Usage triggers warning but compiles. Deprecation suggests: API evolution, security concerns, performance issues, or planned removal. Consult documentation for replacement API."
},

/* 013 */  /* SEMANTIC ERROR */
{
    "no entry point",
    "Translation unit lacks valid program entry point. Required: `main()` function or designated public function based on target environment. The linker/loader cannot determine startup address. Some environments allow alternative entry points via compiler options or specific pragmas."
},

/* 021 */  /* SEMANTIC ERROR */
{
    "symbol already defined",
    "Redeclaration of identifier in same scope violates one-definition rule. Each identifier in a given namespace must have unique declaration (except for overloading, which Pawn doesn't support). The symbol table insertion failed due to duplicate key in current scope."
},

/* 028 */  /* SEMANTIC ERROR */
{
    "invalid subscript",
    "Bracket operator applied to non-array type, or subscript expression has wrong type. Left operand must have array or pointer type, subscript must be integer expression. The type checker validates subscript expressions during expression evaluation."
},

/* 033 */  /* SEMANTIC ERROR */
{
    "array must be indexed",
    "Array identifier used in value context without subscript. In most expressions, arrays decay to pointer to first element, but certain contexts require explicit element access. This prevents accidental pointer decay when element access was intended."
},

/* 034 */  /* SEMANTIC ERROR */
{
    "argument does not have a default value",
    "Named argument syntax used with parameter that lacks default value specification. When calling with named arguments (`func(.param=value)`), all parameters without defaults must be explicitly provided. The argument binder cannot resolve missing required parameter."
},

/* 035 */  /* SEMANTIC ERROR */
{
    "argument type mismatch",
    "Actual argument type incompatible with formal parameter type. This includes: tag mismatch, array vs non-array, dimension mismatch for arrays, or value range issues. Function call type checking ensures actual arguments can be converted to formal parameter types."
},

/* 037 */  /* SEMANTIC ERROR */
{
    "invalid string",
    "String literal malformed: unterminated (missing closing quote), contains invalid escape sequences, or includes illegal characters for current codepage. The lexer's string literal parsing state machine encountered unexpected input or premature EOF."
},

/* 039 */  /* SEMANTIC ERROR */
{
    "constant symbol has no size",
    "`sizeof` operator applied to symbolic constant (enumeration constant, #define macro). `sizeof` requires type name or object expression with storage. Constants exist only at compile-time and have no runtime representation with measurable size."
},

/* 040 */  /* SEMANTIC ERROR */
{
    "duplicate case label",
    "Multiple `case` labels in same switch statement have identical constant expression values. Each case must be distinct to ensure deterministic control flow. The switch statement semantic analyzer builds case value table and detects collisions."
},

/* 041 */  /* SEMANTIC ERROR */
{
    "invalid ellipsis",
    "Array initializer with `...` (ellipsis) cannot determine appropriate array size. Ellipsis initializer requires either explicit array size or preceding explicit elements from which to extrapolate. The array initializer resolver failed to compute array dimension."
},

/* 042 */  /* SEMANTIC ERROR */
{
    "invalid combination of class specifiers",
    "Storage class specifiers combined illegally (e.g., `public static`, `forward native`). Each storage class has compatibility rules. The declaration specifier parser validates specifier combinations according to language grammar."
},

/* 043 */  /* SEMANTIC ERROR */
{
    "character constant exceeds range",
    "Character constant numeric value outside valid 0-255 range (8-bit character set). Pawn uses unsigned 8-bit characters. Integer character constants, escape sequences, or multibyte characters must fit in 8 bits for portability across all Pawn implementations."
},

/* 044 */  /* SEMANTIC ERROR */
{
    "positional parameters must precede",
    "In mixed argument passing, positional arguments appear after named arguments. Syntax requires all positional arguments first, then named arguments (`func(1, 2, .param=3)`). The parser's argument list processing enforces this ordering for unambiguous binding."
},

/* 046 */  /* SEMANTIC ERROR */
{
    "unknown array size",
    "Array declaration lacks size specifier and initializer, making size indeterminate. Array types must have known size at declaration time (except for extern incomplete arrays). The type constructor cannot create array type with unspecified bound."
},

/* 047 */  /* SEMANTIC ERROR */
{
    "array sizes do not match",
    "Array assignment between arrays of different sizes. For array assignment, source and destination must have identical size (number of elements). The type compatibility checker for assignment verifies array dimension equality."
},

/* 048 */  /* SEMANTIC ERROR */
{
    "array dimensions do not match",
    "Array operation (arithmetic, comparison) between arrays of different dimensions. Element-wise operations require identical shape. The array operation validator checks dimension compatibility before generating element-wise code."
},

/* 049 */  /* SEMANTIC ERROR */
{
    "invalid line continuation",
    "Backslash-newline sequence appears outside valid context (preprocessor directive or string literal). Line continuation only permitted in: #define macros, #include paths, and string literals. The preprocessor's line splicing logic detected illegal continuation."
},

/* 050 */  /* SEMANTIC ERROR */
{
    "invalid range",
    "Range expression (e.g., in enum or array initializer) malformed: `start .. end` where start > end, or values non-integral, or exceeds implementation limits. Range validator ensures start ≤ end and both are integer constant expressions."
},

/* 055 */  /* SEMANTIC ERROR */
{
    "start of function body without function header",
    "Compound statement `{ ... }` appears where function body expected but no preceding function declarator. This often indicates: missing function header, extra brace, or incorrect nesting. The parser's function definition production expects declarator before body."
},

/* 100 */  /* FATAL ERROR */
{
    "cannot read from file",
    "File I/O error opening or reading source file. Possible causes: file doesn't exist, insufficient permissions, path too long, file locked by another process, or disk/media error. The compiler's file layer returns system error which gets mapped to this message."
},

/* 101 */  /* FATAL ERROR */
{
    "cannot write to file",
    "Output file creation/write failure. Check: disk full, write protection, insufficient permissions, output directory doesn't exist, or file in use. The code generator's output routines failed to write compiled output."
},

/* 102 */  /* FATAL ERROR */
{
    "table overflow",
    "Internal compiler data structure capacity exceeded. Limits may include: symbol table entries, hash table chains, parse tree nodes, or string table size. Source code too large/complex for compiler's fixed-size internal tables. Consider modularization or compiler with larger limits."
},

/* 103 */  /* FATAL ERROR */
{
    "insufficient memory",
    "Dynamic memory allocation failed during compilation. The compiler's memory manager cannot satisfy allocation request due to system memory exhaustion or fragmentation. Source code may be too large, or compiler has memory leak."
},

/* 104 */  /* FATAL ERROR */
{
    "invalid assembler instruction",
    "`#emit` directive contains unrecognized or illegal opcode/operand combination. The embedded assembler validates instruction against Pawn VM instruction set. Check opcode spelling, operand count, and operand types against VM specification."
},

/* 105 */  /* FATAL ERROR */
{
    "numeric overflow",
    "Numeric constant exceeds representable range for its type. Integer constants beyond 32-bit signed range, or floating-point beyond implementation limits. The constant parser's range checking detected value outside allowed bounds."
},

/* 107 */  /* FATAL ERROR */
{
    "too many error messages on one line",
    "Error reporting threshold exceeded for single source line. Prevents infinite error cascades from severely malformed code. Compiler stops processing this line after reporting maximum errors (typically 10-20)."
},

/* 108 */  /* FATAL ERROR */
{
    "codepage mapping file not found",
    "Character set conversion mapping file specified via `-C` option missing or unreadable. The compiler needs this file for non-ASCII source character processing. Verify file exists in compiler directory or specified path."
},

/* 109 */  /* FATAL ERROR */
{
    "invalid path",
    "File system path syntax error or inaccessible directory component. Path may contain illegal characters, be too long, or refer to non-existent network resource. The compiler's path normalization routine rejected the path."
},

/* 110 */  /* FATAL ERROR */
{
    "assertion failed",
    "Compile-time assertion via `#assert` directive evaluated false. Assertion condition must be constant expression; if false, compilation aborts. Used for validating compile-time assumptions about types, sizes, or configurations."
},

/* 111 */  /* FATAL ERROR */
{
    "user error",
    "`#error` directive encountered with diagnostic message. Intentionally halts compilation with custom error message. Used for conditional compilation checks, deprecated code paths, or unmet prerequisites."
},

/* 214 */  /* WARNING */
{
    "literal array/string passed to non-const parameter",
    "String literal or array initializer passed to function parameter not declared `const`. While syntactically valid, this risks modification of literal data which may be in read-only memory. Declare parameter as `const` if function doesn't modify data, or copy literal to mutable array first."
},

/* 200 */  /* WARNING */
{
    "is truncated to",
    "Identifier exceeds implementation length limit (typically 31-63 characters). Excess characters ignored for symbol table purposes. This may cause collisions if truncated names become identical. Consider shorter, distinct names."
},

/* 201 */  /* WARNING */
{
    "redefinition of constant",
    "Macro or `const` variable redefined with different value. The new definition overrides previous. This often occurs from conflicting header files or conditional compilation issues. If intentional, use `#undef` first or check inclusion order."
},

/* 202 */  /* WARNING */
{
    "number of arguments does not match",
    "Function call argument count differs from prototype parameter count. For variadic functions, minimum count must be satisfied. The argument count checker compares call site with function signature in symbol table."
},

/* 206 */  /* WARNING */
{
    "redundant test: constant expression is non-zero",
    "Condition always true at compile-time, making conditional redundant. Similar to warning 205 but for always-true conditions. May indicate: over-constrained condition, debug code left enabled, or macro expansion issue."
},

/* 214 */  /* WARNING */
{
    "array argument was intended as const",
    "Non-constant array argument passed to const-qualified parameter triggers this advisory. The function promises not to modify via const, but caller passes non-const. This is safe but suggests interface inconsistency."
},

/* 060 */  /* PREPROCESSOR ERROR */
{
    "too many nested includes",
    "Include file nesting depth exceeds implementation limit (typically 50-100). May indicate: recursive inclusion, deep header hierarchies, or include loop. Use include guards (#ifndef) and forward declarations to reduce nesting."
},

/* 061 */  /* PREPROCESSOR ERROR */
{
    "recursive include",
    "Circular include dependency detected. File A includes B which includes A (directly or indirectly). The include graph cycle detection prevents infinite recursion. Use include guards and forward declarations to break cycles."
},

/* 062 */  /* PREPROCESSOR ERROR */
{
    "macro recursion too deep",
    "Macro expansion recursion depth limit exceeded. Occurs with recursively defined macros without termination condition. The preprocessor's macro expansion stack has safety limit to prevent infinite recursion."
},

/* 068 */  /* PREPROCESSOR ERROR */
{
    "division by zero",
    "Constant expression evaluator encountered division/modulo by zero. Check: array sizes, case labels, enumeration values, or #if expressions. The constant folder performs arithmetic during preprocessing and catches this error."
},

/* 069 */  /* PREPROCESSOR ERROR */
{
    "overflow in constant expression",
    "Arithmetic overflow during constant expression evaluation in preprocessor. Integer operations exceed 32-bit signed range. The preprocessor uses signed 32-bit arithmetic for constant expressions."
},

/* 070 */  /* PREPROCESSOR ERROR */
{
    "undefined macro",
    "Macro identifier used in `#if` or `#elif` not defined. Treated as 0 per C standard. May indicate: missing header, typo, or conditional compilation branch for undefined configuration. Use `#ifdef` or `#if defined()` to test existence."
},

/* 071 */  /* PREPROCESSOR ERROR */
{
    "missing preprocessor argument",
    "Function-like macro invoked with insufficient arguments. Each parameter in macro definition must correspond to actual argument. Check macro invocation against its definition parameter count."
},

/* 072 */  /* PREPROCESSOR ERROR */
{
    "too many macro arguments",
    "Function-like macro invoked with excess arguments beyond parameter list. Extra arguments are ignored but indicate likely error. Verify macro definition matches usage pattern."
},

/* 038 */  /* PREPROCESSOR ERROR */
{
    "extra characters on line",
    "Trailing tokens after preprocessor directive. Preprocessor directives must occupy complete logical line (except line continuation). Common with stray semicolons or comments on `#include` lines."
},

/* Sentinel value to mark the end of the array. */
{NULL, NULL}
};
