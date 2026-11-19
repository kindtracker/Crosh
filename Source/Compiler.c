#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Crosh.h"

typedef enum {
  BASH_AST_PROGRAM,
  BASH_AST_COMMAND,
  BASH_AST_PIPELINE,
  BASH_AST_AND,
  BASH_AST_OR,
  BASH_AST_BACKGROUND,
  BASH_AST_REDIRECTION,
} BashAstKind;

typedef struct BashAst BashAst;

struct BashAst {
  BashAstKind Kind;

  union {
    struct {
      BashAst **Commands;
      unsigned int Count;
    } Program;

    struct {
      char **Arguments;
      unsigned int Count;
    } Command;

    struct {
      BashAst **Commands;
      unsigned int Count;
    } Pipeline;

    struct {
      BashAst *Left;
      BashAst *Right;
    } Binary;

    struct {
      BashAst *Command;
    } Background;

    struct {
      BashAst *Command;
      int FileDescriptor;
      char *Target;
      int Append;
    } Redirection;
  };
};

typedef enum {
  TOKEN_IDENT,
  TOKEN_PIPE,
  TOKEN_PIPE_BOTH,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_EQUAL,
  TOKEN_BACKGROUND,
  TOKEN_SEMICOLON,
  TOKEN_REDIRECT_OUTPUT,
  TOKEN_REDIRECT_APPEND,
  TOKEN_REDIRECT_INPUT,
  TOKEN_REDIRECT_DUPLICATE,
  TOKEN_STRING,
  TOKEN_EOF
} BashTokenKind;

typedef struct BashToken {
  BashTokenKind Kind;
  char *Value;
  struct BashToken *Next;
} BashToken;

static char *CroshBashTokenKindToString(BashTokenKind Kind) {
  static char *Names[] = {"Ident",
                          "Pipe",
                          "PipeBoth",
                          "And",
                          "Or",
                          "Equal",
                          "Background",
                          "Semicolon",
                          "RedirectOutput",
                          "RedirectAppend",
                          "RedirectInput",
                          "RedirectDuplicate",
                          "String",
                          "Eof"};

  return Kind >= TOKEN_IDENT && Kind <= TOKEN_EOF ? Names[Kind] : "Unknown";
}

static char *CroshBashKindToString(BashAstKind Kind) {
  static char *Names[] = {"Program", "Command",    "Pipeline",   "And",
                          "Or",      "Background", "Redirection"};

  return Kind >= BASH_AST_PROGRAM && Kind <= BASH_AST_REDIRECTION ? Names[Kind]
                                                                  : "Unknown";
}

static BashToken *CroshNewToken(BashTokenKind Kind, char *Value) {
  BashToken *Token = calloc(1, sizeof(BashToken));
  if (!Token)
    return NULL;

  Token->Kind = Kind;
  Token->Value = Value ? strdup(Value) : NULL;

  if (Value && !Token->Value) {
    free(Token);
    return NULL;
  }

  return Token;
}

static void CroshFreeTokens(BashToken *Token) {
  while (Token) {
    BashToken *Next = Token->Next;
    free(Token->Value);
    free(Token);
    Token = Next;
  }
}

static int CroshAddToken(BashToken **Head, BashToken **Tail, BashTokenKind Kind,
                         char *Value) {
  BashToken *Token = CroshNewToken(Kind, Value);
  if (!Token)
    return 0;

  if (*Tail)
    (*Tail)->Next = Token;
  else
    *Head = Token;

  *Tail = Token;
  return 1;
}

static BashToken *CroshTokenizeBash(char *Text) {
  BashToken *Head = NULL, *Tail = NULL;

  while (*Text) {
    if (isspace((unsigned char)*Text)) {
      Text++;
      continue;
    }

    static struct {
      char *Text;
      BashTokenKind Kind;
    } Operators[] = {{"|&", TOKEN_PIPE_BOTH},
                     {"&&", TOKEN_AND},
                     {"||", TOKEN_OR},
                     {">>", TOKEN_REDIRECT_APPEND},
                     {">&", TOKEN_REDIRECT_DUPLICATE},
                     {"|", TOKEN_PIPE},
                     {"&", TOKEN_BACKGROUND},
                     {";", TOKEN_SEMICOLON},
                     {">", TOKEN_REDIRECT_OUTPUT},
                     {"<", TOKEN_REDIRECT_INPUT},
                     {"=", TOKEN_EQUAL}};

    int Matched = 0;

    for (size_t i = 0; i < sizeof(Operators) / sizeof(*Operators); i++) {
      size_t Length = strlen(Operators[i].Text);

      if (strncmp(Text, Operators[i].Text, Length) == 0) {
        if (!CroshAddToken(&Head, &Tail, Operators[i].Kind, NULL))
          goto Error;

        Text += Length;
        Matched = 1;
        break;
      }
    }

    if (Matched)
      continue;

    char *Start = Text;
    char Quote = '\0';

    while (*Text) {
      if (Quote) {
        if (*Text++ == Quote)
          break;
        continue;
      }

      if (*Text == '\'' || *Text == '"') {
        Quote = *Text++;
        continue;
      }

      if (isspace((unsigned char)*Text) || strchr("|&;><=", *Text))
        break;

      Text++;
    }

    if (Quote && Text[-1] != Quote)
      goto Error;

    size_t Length = Text - Start;
    if (Quote) {
      Start++;
      Length -= 2;
    }

    char *Value = malloc(Length + 1);
    if (!Value)
      goto Error;

    memcpy(Value, Start, Length);
    Value[Length] = '\0';

    int Success =
        CroshAddToken(&Head, &Tail, Quote ? TOKEN_STRING : TOKEN_IDENT, Value);

    free(Value);

    if (!Success)
      goto Error;
  }

  if (!CroshAddToken(&Head, &Tail, TOKEN_EOF, NULL))
    goto Error;

  return Head;

Error:
  CroshFreeTokens(Head);
  return NULL;
}

void CroshFreeBashAst(BashAst *Ast) {
  if (Ast == NULL)
    return;

  switch (Ast->Kind) {
  case BASH_AST_PROGRAM:
    for (unsigned int i = 0; i < Ast->Program.Count; i++)
      CroshFreeBashAst(Ast->Program.Commands[i]);

    free(Ast->Program.Commands);
    break;

  case BASH_AST_COMMAND:
    for (unsigned int i = 0; i < Ast->Command.Count; i++)
      free(Ast->Command.Arguments[i]);

    free(Ast->Command.Arguments);
    break;

  case BASH_AST_AND:
  case BASH_AST_OR:
    CroshFreeBashAst(Ast->Binary.Left);
    CroshFreeBashAst(Ast->Binary.Right);
    break;

  case BASH_AST_PIPELINE:
    for (unsigned int i = 0; i < Ast->Pipeline.Count; i++)
      CroshFreeBashAst(Ast->Pipeline.Commands[i]);

    free(Ast->Pipeline.Commands);
    break;

  case BASH_AST_BACKGROUND:
    CroshFreeBashAst(Ast->Background.Command);
    break;

  case BASH_AST_REDIRECTION:
    CroshFreeBashAst(Ast->Redirection.Command);
    free(Ast->Redirection.Target);
    break;
  }

  free(Ast);
}

static int CroshIsWord(BashToken *T) {
  return T && (T->Kind == TOKEN_IDENT || T->Kind == TOKEN_STRING);
}

static int CroshIsRedirection(BashToken *T) {
  return T && (T->Kind >= TOKEN_REDIRECT_OUTPUT &&
               T->Kind <= TOKEN_REDIRECT_DUPLICATE);
}

static int CroshAppendAst(BashAst ***List, unsigned int *Count, BashAst *Ast) {
  BashAst **New = realloc(*List, (*Count + 1) * sizeof(*New));
  if (!New)
    return 0;

  *List = New;
  (*List)[(*Count)++] = Ast;
  return 1;
}

static int CroshAppendArgument(BashAst *Ast, char *Value) {
  char **New =
      realloc(Ast->Command.Arguments, (Ast->Command.Count + 1) * sizeof(*New));
  if (!New)
    return 0;

  Ast->Command.Arguments = New;
  New[Ast->Command.Count++] = strdup(Value);
  return New[Ast->Command.Count - 1] != NULL;
}

static BashAst *CroshParseCommand(BashToken **T) {
  BashAst *Command = calloc(1, sizeof(*Command));
  if (!Command)
    return NULL;

  Command->Kind = BASH_AST_COMMAND;
  BashAst *Root = Command;
  int Found = 0;

  while (CroshIsWord(*T) || CroshIsRedirection(*T)) {
    if (CroshIsWord(*T)) {
      if (!CroshAppendArgument(Command, (*T)->Value))
        goto Error;
      *T = (*T)->Next;
    } else {
      BashTokenKind Kind = (*T)->Kind;
      *T = (*T)->Next;

      if (!CroshIsWord(*T))
        goto Error;

      BashAst *Redir = calloc(1, sizeof(*Redir));
      if (!Redir)
        goto Error;

      Redir->Kind = BASH_AST_REDIRECTION;
      Redir->Redirection.Command = Root;
      Redir->Redirection.FileDescriptor = Kind == TOKEN_REDIRECT_INPUT ? 0 : 1;
      Redir->Redirection.Target = strdup((*T)->Value);
      Redir->Redirection.Append = Kind == TOKEN_REDIRECT_APPEND;

      if (!Redir->Redirection.Target) {
        free(Redir);
        goto Error;
      }

      Root = Redir;
      *T = (*T)->Next;
    }

    Found = 1;
  }

  if (!Found)
    goto Error;
  return Root;

Error:
  CroshFreeBashAst(Root);
  return NULL;
}

static BashAst *CroshParsePipeline(BashToken **T) {
  BashAst *First = CroshParseCommand(T);
  if (!First)
    return NULL;

  if ((*T)->Kind != TOKEN_PIPE && (*T)->Kind != TOKEN_PIPE_BOTH)
    return First;

  BashAst *Pipeline = calloc(1, sizeof(*Pipeline));
  if (!Pipeline) {
    CroshFreeBashAst(First);
    return NULL;
  }

  Pipeline->Kind = BASH_AST_PIPELINE;
  CroshAppendAst(&Pipeline->Pipeline.Commands, &Pipeline->Pipeline.Count,
                 First);

  while ((*T)->Kind == TOKEN_PIPE || (*T)->Kind == TOKEN_PIPE_BOTH) {
    *T = (*T)->Next;

    BashAst *Command = CroshParseCommand(T);
    if (!Command || !CroshAppendAst(&Pipeline->Pipeline.Commands,
                                    &Pipeline->Pipeline.Count, Command)) {
      CroshFreeBashAst(Command);
      CroshFreeBashAst(Pipeline);
      return NULL;
    }
  }

  return Pipeline;
}

static BashAst *CroshParseAndOr(BashToken **T) {
  BashAst *Left = CroshParsePipeline(T);
  if (!Left)
    return NULL;

  while ((*T)->Kind == TOKEN_AND || (*T)->Kind == TOKEN_OR) {
    BashAstKind Kind = (*T)->Kind == TOKEN_AND ? BASH_AST_AND : BASH_AST_OR;
    *T = (*T)->Next;

    BashAst *Right = CroshParsePipeline(T);
    if (!Right) {
      CroshFreeBashAst(Left);
      return NULL;
    }

    BashAst *Binary = calloc(1, sizeof(*Binary));
    if (!Binary) {
      CroshFreeBashAst(Left);
      CroshFreeBashAst(Right);
      return NULL;
    }

    Binary->Kind = Kind;
    Binary->Binary.Left = Left;
    Binary->Binary.Right = Right;
    Left = Binary;
  }

  return Left;
}

static BashAst *CroshParseBash(BashToken **T) {
  BashAst *Program = calloc(1, sizeof(*Program));
  if (!Program)
    return NULL;

  Program->Kind = BASH_AST_PROGRAM;

  while (*T && (*T)->Kind != TOKEN_EOF) {
    if ((*T)->Kind == TOKEN_SEMICOLON) {
      *T = (*T)->Next;
      continue;
    }

    BashAst *Command = CroshParseAndOr(T);
    if (!Command)
      goto Error;

    int Background = (*T)->Kind == TOKEN_BACKGROUND;

    if (Background) {
      *T = (*T)->Next;

      BashAst *Node = calloc(1, sizeof(*Node));
      if (!Node) {
        CroshFreeBashAst(Command);
        goto Error;
      }

      Node->Kind = BASH_AST_BACKGROUND;
      Node->Background.Command = Command;
      Command = Node;
    }

    if (!CroshAppendAst(&Program->Program.Commands, &Program->Program.Count,
                        Command)) {
      CroshFreeBashAst(Command);
      goto Error;
    }

    if ((*T)->Kind == TOKEN_SEMICOLON)
      *T = (*T)->Next;
    else if ((*T)->Kind != TOKEN_EOF && !Background)
      goto Error;
  }

  return Program;

Error:
  CroshFreeBashAst(Program);
  return NULL;
}

static void CroshPrintAst(BashAst *Ast, int Depth) {
  if (!Ast)
    return;

  for (int i = 0; i < Depth; i++)
    printf("  ");

  switch (Ast->Kind) {
  case BASH_AST_PROGRAM:
    puts("Program");
    for (unsigned int i = 0; i < Ast->Program.Count; i++)
      CroshPrintAst(Ast->Program.Commands[i], Depth + 1);
    break;

  case BASH_AST_COMMAND:
    printf("Command");
    for (unsigned int i = 0; i < Ast->Command.Count; i++)
      printf(" %s", Ast->Command.Arguments[i]);
    putchar('\n');
    break;

  case BASH_AST_PIPELINE:
    puts("Pipeline");
    for (unsigned int i = 0; i < Ast->Pipeline.Count; i++)
      CroshPrintAst(Ast->Pipeline.Commands[i], Depth + 1);
    break;

  case BASH_AST_AND:
  case BASH_AST_OR:
    puts(Ast->Kind == BASH_AST_AND ? "And" : "Or");
    CroshPrintAst(Ast->Binary.Left, Depth + 1);
    CroshPrintAst(Ast->Binary.Right, Depth + 1);
    break;

  case BASH_AST_BACKGROUND:
    puts("Background");
    CroshPrintAst(Ast->Background.Command, Depth + 1);
    break;

  case BASH_AST_REDIRECTION:
    printf("Redirection (fd=%d, target=%s, append=%d)\n",
           Ast->Redirection.FileDescriptor, Ast->Redirection.Target,
           Ast->Redirection.Append);
    CroshPrintAst(Ast->Redirection.Command, Depth + 1);
    break;
  }
}

static BashAst *CroshCompileBashToAst(char *Text) {
  BashToken *Tokens = CroshTokenizeBash(Text);

  BashToken *Token = Tokens;
  int Idx = 1;
  while (Token) {
    printf("Token %d\n", Idx);
    printf("  Kind='%s'\n", CroshBashTokenKindToString(Token->Kind));
    printf("  Value='%s'\n", Token->Value);
    Idx++;

    Token = Token->Next;
  }

  BashAst *Ast = CroshParseBash(&Tokens);
  if (Ast)
    CroshPrintAst(Ast, 0);

  return Ast;
}

static int CroshAppendString(char **Buffer, char *Text) {
  size_t Old = strlen(*Buffer);
  size_t Len = strlen(Text);

  char *New = realloc(*Buffer, Old + Len + 2);
  if (!New)
    return 0;

  memcpy(New + Old, Text, Len);
  New[Old + Len] = '\0';

  *Buffer = New;
  return 1;
}

static int CroshPutFormatted(char **Buffer, char *Format, ...) {
  va_list Args;
  va_start(Args, Format);

  va_list Copy;
  va_copy(Copy, Args);
  int Length = vsnprintf(NULL, 0, Format, Copy);
  va_end(Copy);

  if (Length < 0) {
    va_end(Args);
    return 0;
  }

  size_t Old = strlen(*Buffer);
  char *New = realloc(*Buffer, Old + Length + 2);

  if (!New) {
    va_end(Args);
    return 0;
  }

  *Buffer = New;
  vsnprintf(New + Old, Length + 1, Format, Args);
  New[Old + Length + 1] = '\0';

  va_end(Args);
  return 1;
}

static char *CroshReturnFormatted(char *Format, ...) {
  va_list Args, Copy;

  va_start(Args, Format);
  va_copy(Copy, Args);

  int Length = vsnprintf(NULL, 0, Format, Copy);
  va_end(Copy);

  if (Length < 0) {
    va_end(Args);
    return NULL;
  }

  char *Result = malloc((size_t)Length + 1);
  if (!Result) {
    va_end(Args);
    return NULL;
  }

  vsnprintf(Result, (size_t)Length + 1, Format, Args);
  va_end(Args);

  return Result;
}

static char *CroshCompileAstToLua(BashAst *Ast, int Depth) {
  if (!Ast)
    return NULL;

  if (Ast->Kind == BASH_AST_PROGRAM) {
    char *Result = strdup("");

    for (unsigned int i = 0; i < Ast->Program.Count; i++) {
      char *Lua = CroshCompileAstToLua(Ast->Program.Commands[i], Depth + 1);

      if (!Lua || !CroshAppendString(&Result, Lua)) {
        free(Lua);
        free(Result);
        return NULL;
      }

      free(Lua);
    }

    return Result;
  }

  if (Ast->Kind == BASH_AST_COMMAND) {
    char *Arguments = strdup("");

    for (unsigned int i = 0; i < Ast->Command.Count; i++) {
      char *Argument = Ast->Command.Arguments[i];
      if (i > 0)
        CroshPutFormatted(&Arguments, ", ");
      CroshPutFormatted(&Arguments, "\"%s\"", Argument);
    }

    char *Result = CroshReturnFormatted("Crosh(%s)\n", Arguments);
    free(Arguments);

    return Result;
  }

  char Buffer[128];
  snprintf(Buffer, sizeof(Buffer), "-- Couldn't find handler for %s",
           CroshBashKindToString(Ast->Kind));

  return strdup(Buffer);
}

char *CroshCompileBashToLua(char *Text) {
  BashAst *Ast = CroshCompileBashToAst(Text);
  if (Ast == NULL) {
    return "crosh: failed to compile bash to ast";
  }

  char *LuaCode = CroshCompileAstToLua(Ast, 0);

  CroshFreeBashAst(Ast);
  return LuaCode;
}
