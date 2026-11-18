#include <ctype.h>
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

  return NULL;
}

static char *CroshCompileAstToLua(BashAst *Ast) {
  (void)Ast;
  return "Meow";
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

char *CroshCompileBashToLua(char *Text) {
  BashAst *Ast = CroshCompileBashToAst(Text);
  if (Ast == NULL) {
    return "crosh: failed to compile bash to ast";
  }

  char *LuaCode = CroshCompileAstToLua(Ast);

  CroshFreeBashAst(Ast);
  return LuaCode;
}
