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
  switch (Kind) {
  case TOKEN_IDENT:
    return "Ident";
  case TOKEN_PIPE:
    return "Pipe";
  case TOKEN_PIPE_BOTH:
    return "PipeBoth";
  case TOKEN_AND:
    return "And";
  case TOKEN_OR:
    return "Or";
  case TOKEN_EQUAL:
    return "Equal";
  case TOKEN_BACKGROUND:
    return "Background";
  case TOKEN_SEMICOLON:
    return "Semicolon";
  case TOKEN_REDIRECT_OUTPUT:
    return "RedirectOutput";
  case TOKEN_REDIRECT_APPEND:
    return "RedirectAppend";
  case TOKEN_REDIRECT_INPUT:
    return "RedirectInput";
  case TOKEN_REDIRECT_DUPLICATE:
    return "RedirectDuplicate";
  case TOKEN_STRING:
    return "String";
  case TOKEN_EOF:
    return "Eof";
  }

  return "UNKNOWN";
}

static BashToken *CroshNewToken(BashTokenKind Kind, char *Value) {
  BashToken *Token = malloc(sizeof(BashToken));

  if (Token == NULL)
    return NULL;

  Token->Kind = Kind;
  Token->Value = NULL;
  Token->Next = NULL;

  if (Value != NULL) {
    Token->Value = strdup(Value);

    if (Token->Value == NULL) {
      free(Token);
      return NULL;
    }
  }

  return Token;
}

static void CroshFreeTokens(BashToken *Token) {
  while (Token != NULL) {
    BashToken *Next = Token->Next;

    free(Token->Value);
    free(Token);

    Token = Next;
  }
}

static int CroshAddToken(BashToken **Head, BashToken **Tail, BashTokenKind Kind,
                         char *Value) {
  BashToken *Token = CroshNewToken(Kind, Value);

  if (Token == NULL)
    return 0;

  if (*Head == NULL)
    *Head = Token;
  else
    (*Tail)->Next = Token;

  *Tail = Token;
  return 1;
}

static BashToken *CroshTokenizeBash(char *Text) {
  BashToken *Head = NULL;
  BashToken *Tail = NULL;

  while (*Text != '\0') {
    if (*Text == ' ' || *Text == '\t') {
      Text++;
      continue;
    }

    if (*Text == '\n') {
      Text++;
      continue;
    }

    if (strncmp(Text, "|&", 2) == 0) {
      if (!CroshAddToken(&Head, &Tail, TOKEN_PIPE_BOTH, NULL))
        goto Error;
      Text += 2;
      continue;
    }

    if (strncmp(Text, "&&", 2) == 0) {
      if (!CroshAddToken(&Head, &Tail, TOKEN_AND, NULL))
        goto Error;
      Text += 2;
      continue;
    }

    if (strncmp(Text, "||", 2) == 0) {
      if (!CroshAddToken(&Head, &Tail, TOKEN_OR, NULL))
        goto Error;
      Text += 2;
      continue;
    }

    if (strncmp(Text, ">>", 2) == 0) {
      if (!CroshAddToken(&Head, &Tail, TOKEN_REDIRECT_APPEND, NULL))
        goto Error;
      Text += 2;
      continue;
    }

    if (strncmp(Text, ">&", 2) == 0) {
      if (!CroshAddToken(&Head, &Tail, TOKEN_REDIRECT_DUPLICATE, NULL))
        goto Error;
      Text += 2;
      continue;
    }

    BashTokenKind Operator;

    switch (*Text) {
    case '|':
      Operator = TOKEN_PIPE;
      break;
    case '&':
      Operator = TOKEN_BACKGROUND;
      break;
    case ';':
      Operator = TOKEN_SEMICOLON;
      break;
    case '>':
      Operator = TOKEN_REDIRECT_OUTPUT;
      break;
    case '<':
      Operator = TOKEN_REDIRECT_INPUT;
      break;
    case '=':
      Operator = TOKEN_EQUAL;
      break;
    default:
      Operator = TOKEN_EOF;
      break;
    }

    if (Operator != TOKEN_EOF) {
      if (!CroshAddToken(&Head, &Tail, Operator, NULL))
        goto Error;
      Text++;
      continue;
    }

    char *Start = Text;
    char Quote = '\0';

    while (*Text != '\0') {
      if (Quote != '\0') {
        if (*Text == Quote) {
          Text++;
          break;
        }

        Text++;
        continue;
      }

      if (*Text == '\'' || *Text == '"') {
        Quote = *Text;
        Text++;
        continue;
      }

      if (isspace((unsigned char)*Text) || strchr("|&;><=", *Text) != NULL)
        break;

      Text++;
    }

    if (Quote != '\0') {
      Start++;
      Text--;
    }

    size_t Length = (size_t)(Text - Start);
    char *Value = malloc(Length + 1);

    if (Quote != '\0') {
      Text++;
    }

    if (Value == NULL)
      goto Error;

    memcpy(Value, Start, Length);
    Value[Length] = '\0';

    int Success = CroshAddToken(
        &Head, &Tail, Quote != '\0' ? TOKEN_STRING : TOKEN_IDENT, Value);

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

static char *CroshCompileAstToLua(BashAst *Ast) {}

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
