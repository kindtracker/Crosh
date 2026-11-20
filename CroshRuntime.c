#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>
#include <luajit-2.1/lualib.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char **Argv = NULL;

static int RunCallback(lua_State *Lua, int Index) {
  lua_pushvalue(Lua, Index);

  if (lua_pcall(Lua, 0, 0, 0) != 0) {
    const char *Error = lua_tostring(Lua, -1);
    fprintf(stderr, "Crosh: %s\n", Error ? Error : "Lua callback failed");
    return 1;
  }

  return 0;
}

static int WaitFor(pid_t Pid, int *Status) {
  while (waitpid(Pid, Status, 0) == -1) {
    if (errno == EINTR)
      continue;

    return -1;
  }

  return 0;
}

static int ExitCode(int Status) {
  if (WIFEXITED(Status))
    return WEXITSTATUS(Status);

  if (WIFSIGNALED(Status))
    return 128 + WTERMSIG(Status);

  return 1;
}

static int Load(lua_State *Lua) {
  luaL_checktype(Lua, 1, LUA_TTABLE);

  int Argc = lua_objlen(Lua, 1);
  Argv = malloc(sizeof(char *) * (Argc + 1));

  for (int i = 0; i < Argc; i++) {
    lua_rawgeti(Lua, 1, i + 1);

    const char *Value = luaL_checkstring(Lua, -1);
    Argv[i] = strdup(Value);
    lua_pop(Lua, 1);
  }

  Argv[Argc] = NULL;
  return 0;
}

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
    fprintf(stderr, "%s: %s\n", Argv[1], strerror(errno));
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

static int Pipe(lua_State *Lua) {
  luaL_checktype(Lua, 1, LUA_TFUNCTION);
  luaL_checktype(Lua, 2, LUA_TFUNCTION);

  int PipeBoth = lua_toboolean(Lua, 3);

  int PipeFD[2];

  if (pipe(PipeFD) < 0)
    return luaL_error(Lua, "pipe failed");

  fflush(NULL);

  pid_t LeftPid = fork();

  if (LeftPid < 0) {
    close(PipeFD[0]);
    close(PipeFD[1]);
    return luaL_error(Lua, "failed to fork left process");
  }

  if (LeftPid == 0) {
    close(PipeFD[0]);

    if (dup2(PipeFD[1], STDOUT_FILENO) < 0)
      _exit(1);

    if (PipeBoth && dup2(PipeFD[1], STDERR_FILENO) < 0)
      _exit(1);

    close(PipeFD[1]);

    int Result = RunCallback(Lua, 1);

    fflush(NULL);
    _exit(Result);
  }

  pid_t RightPid = fork();

  if (RightPid < 0) {
    close(PipeFD[0]);
    close(PipeFD[1]);

    kill(LeftPid, SIGTERM);

    int Status;
    WaitFor(LeftPid, &Status);

    return luaL_error(Lua, "failed to fork right process");
  }

  if (RightPid == 0) {
    close(PipeFD[1]);

    if (dup2(PipeFD[0], STDIN_FILENO) < 0)
      _exit(1);

    close(PipeFD[0]);

    int Result = RunCallback(Lua, 2);

    fflush(NULL);
    _exit(Result);
  }

  close(PipeFD[0]);
  close(PipeFD[1]);

  int LeftStatus;
  int RightStatus;

  if (WaitFor(LeftPid, &LeftStatus) < 0)
    return luaL_error(Lua, "failed to wait for left process");

  if (WaitFor(RightPid, &RightStatus) < 0)
    return luaL_error(Lua, "failed to wait for right process");

  lua_pushinteger(Lua, ExitCode(RightStatus));

  return 1;
}

static int SubShell(lua_State *Lua) {
  luaL_checktype(Lua, 1, LUA_TFUNCTION);

  fflush(NULL);

  pid_t Pid = fork();

  if (Pid < 0)
    return luaL_error(Lua, "fork failed");

  if (Pid == 0) {
    int Result = RunCallback(Lua, 1);
    fflush(NULL);
    _exit(Result);
  }

  int Status;

  if (WaitFor(Pid, &Status) < 0)
    return luaL_error(Lua, "waitpid failed");

  lua_pushinteger(Lua, ExitCode(Status));
  return 1;
}

int luaopen_CroshRuntime(lua_State *Lua) {
  lua_newtable(Lua);

  static const luaL_Reg Functions[] = {
      {"Run", Run}, {"SubShell", SubShell}, {"Pipe", Pipe}, {NULL, NULL}};
  luaL_register(Lua, "CroshRuntime", Functions);

  return 1;
}
