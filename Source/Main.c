#include <errno.h>
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
char *Arguments[MAX_ARGUMENTS] = {{0}};
char UserName[256] = {0};
char HostName[256] = {0};
char Pwd[4096] = {0};

int main(int Argc, char *Argv[]) {
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
    puts(LuaCode);
  }

  return 0;
}
