#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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

    const char *Home = User ? User->pw_dir : "";
    if (User && strncmp(Pwd, Home, strlen(Home)) == 0 &&
        (Pwd[strlen(Home)] == '/' || Pwd[strlen(Home)] == '\0')) {
      printf("[%s@%s ~%s]$ ", UserName, HostName, Pwd + strlen(Home));
    } else {
      printf("[%s@%s %s]$ ", UserName, HostName, Pwd);
    }

    fgets(Buffer, sizeof(Buffer), stdin);

    int Count = 0;
    char *Token = strtok(Buffer, " \t\n");

    while (Token != NULL && Count < MAX_ARGUMENTS - 1) {
      Arguments[Count++] = Token;
      Token = strtok(NULL, " \t\n");
    }

    Arguments[Count] = NULL;

    if (Count == 0) {
      continue;
    }

    pid_t Process = fork();
    if (Process == 0) {
      execvp(Arguments[0], Arguments);
      fprintf(stderr, "crosh: %s: %s\n", Arguments[0], strerror(errno));
      _exit(127);
    } else if (Process > 0) {
      waitpid(Process, NULL, 0);
    } else {
      perror("fork");
    }
  }

  return 0;
}
