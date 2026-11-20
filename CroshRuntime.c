#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>
#include <luajit-2.1/lualib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int Run(lua_State *Lua) {
  const char *Input = luaL_checkstring(Lua, 1);
  char *Command = strdup(Input);

  if (!Command)
    return luaL_error(Lua, "out of memory");

  size_t Capacity = 8;
  size_t Count = 0;

  char **Arguments = malloc(Capacity * sizeof(char *));

  if (!Arguments) {
    free(Command);
    return luaL_error(Lua, "out of memory");
  }

  char *Save;
  char *Token = strtok_r(Command, " \t\n", &Save);

  while (Token) {
    if (Count + 1 >= Capacity) {
      Capacity *= 2;

      char **NewArguments = realloc(Arguments, Capacity * sizeof(char *));

      if (!NewArguments) {
        free(Arguments);
        free(Command);
        return luaL_error(Lua, "out of memory");
      }

      Arguments = NewArguments;
    }

    Arguments[Count++] = Token;
    Token = strtok_r(NULL, " \t\n", &Save);
  }

  Arguments[Count] = NULL;

  if (Count == 0) {
    free(Arguments);
    free(Command);
    return 0;
  }

  pid_t Pid = fork();

  if (Pid == 0) {
    execvp(Arguments[0], Arguments);
    perror("execvp");
    _exit(127);
  }

  if (Pid < 0) {
    perror("fork");
    free(Arguments);
    free(Command);
    return luaL_error(Lua, "failed to fork");
  }

  int Status;
  waitpid(Pid, &Status, 0);

  free(Arguments);
  free(Command);

  if (WIFEXITED(Status))
    lua_pushinteger(Lua, WEXITSTATUS(Status));
  else
    lua_pushinteger(Lua, 128 + WTERMSIG(Status));

  return 1;
}

int luaopen_CroshRuntime(lua_State *Lua) {
  lua_newtable(Lua);

  static const luaL_Reg Functions[] = {{"Run", Run}, {NULL, NULL}};
  luaL_register(Lua, "CroshRuntime", Functions);

  return 1;
}
