#include <errno.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Crosh.h"

#define INPUT_MAXIMUM_LENGTH 65536
#define MAX_ARGUMENTS 256

char Buffer[INPUT_MAXIMUM_LENGTH] = {0};
char *Arguments[MAX_ARGUMENTS] = {0};
char UserName[256] = {0};
char HostName[256] = {0};
char Pwd[4096] = {0};

int CroshFunction(lua_State *Lua) {
  int Count = lua_gettop(Lua);

  char *Arguments[MAX_ARGUMENTS + 1] = {0};

  if (Count > MAX_ARGUMENTS)
    return luaL_error(Lua, "too many arguments");

  for (int i = 0; i < Count; i++) {
    Arguments[i] = (char *)lua_tostring(Lua, i + 1);

    if (!Arguments[i])
      return luaL_error(Lua, "argument %d must be a string", i + 1);
  }

  Arguments[Count] = NULL;

  pid_t Pid = fork();
  if (Pid < 0)
    return luaL_error(Lua, "fork failed: %s", strerror(errno));

  if (Pid == 0) {
    execvp(Arguments[0], Arguments);
    fprintf(stderr, "crosh: %s: %s\n", Arguments[0], strerror(errno));
    _exit(127);
  }

  int Status;
  waitpid(Pid, &Status, 0);

  lua_pushinteger(Lua, WIFEXITED(Status) ? WEXITSTATUS(Status) : -1);
  return 1;
}

int main(int Argc, char *Argv[]) {
  (void)Argc;
  (void)Argv;

  lua_State *Lua = luaL_newstate();

  lua_pushcfunction(Lua, CroshFunction);
  lua_setglobal(Lua, "Crosh");

  while (true) {
    memset(Buffer, 0, sizeof(Buffer));

    struct passwd *User = getpwuid(getuid());
    if (User != NULL) {
      strncpy(UserName, User->pw_name, sizeof(UserName) - 1);
    }
    gethostname(HostName, sizeof(HostName));
    getcwd(Pwd, sizeof(Pwd));

    char *Home = User ? User->pw_dir : "";
    if (User && strncmp(Pwd, Home, strlen(Home)) == 0 &&
        (Pwd[strlen(Home)] == '/' || Pwd[strlen(Home)] == '\0')) {
      printf("[%s@%s ~%s]$ ", UserName, HostName, Pwd + strlen(Home));
    } else {
      printf("[%s@%s %s]$ ", UserName, HostName, Pwd);
    }

    fgets(Buffer, sizeof(Buffer), stdin);

    const char *LuaCode = CroshCompileBashToLua(Buffer);

    if (luaL_loadstring(Lua, LuaCode) == 0) {
      if (lua_pcall(Lua, 0, LUA_MULTRET, 0) != 0) {
        fprintf(stderr, "crosh: lua: %s\n", lua_tostring(Lua, -1));
        lua_pop(Lua, 1);
      }
    } else {
      fprintf(stderr, "crosh: lua: %s\n", lua_tostring(Lua, -1));
      lua_pop(Lua, 1);
    }
  }

  lua_close(Lua);

  return 0;
}
