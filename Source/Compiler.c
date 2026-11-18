#include <stddef.h>
#include <stdlib.h>

#include "Crosh.h"

typedef enum {
  BASH_AST_PROGRAM,
  BASH_AST_COMMAND,
  BASH_AST_PIPELINE,
  BASH_AST_AND,
  BASH_AST_OR,
  BASH_AST_BACKGROUND,
  BASH_AST_REDIRECTION,
} BashAstType;

typedef struct BashAst BashAst;

struct BashAst {
  BashAstType Type;

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
  TOKEN_WORD,
  TOKEN_PIPE,
  TOKEN_PIPE_BOTH,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_BACKGROUND,
  TOKEN_SEMICOLON,
  TOKEN_REDIRECT_OUTPUT,
  TOKEN_REDIRECT_APPEND,
  TOKEN_REDIRECT_INPUT,
  TOKEN_REDIRECT_DUPLICATE,
  TOKEN_NEWLINE,
  TOKEN_EOF
} BashTokenType;

typedef struct {
  BashTokenType Type;
  char *Value;
} BashToken;

#define NewToken()                                                             \
  Token->Next = malloc(sizeof(BashToken));                                     \
  Token = Token->Next

static BashAst *CroshCompileBashToAst(char *Text) {
  BashToken *Token = malloc(sizeof(BashToken));
  BashToken *Tokens = Token;
  char *Character = Text;

  while (*Character) {
    if (*Character == 'a') {

    } else {
      return NULL;
    }
  }
}

static char *CroshCompileAstToLua(BashAst *Ast) {}

void CroshFreeBashAst(BashAst *Ast) {
  if (Ast == NULL)
    return;

  switch (Ast->Type) {
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
    return "Failed to compile Bash to Ast";
  }

  char *LuaCode = CroshCompileAstToLua(Ast);

  CroshFreeBashAst(Ast);
  return LuaCode;
}
