#include <string.h>
#include "palindromeHelper.h"
#include <stdio.h>
#include <stdlib.h>

int semaphoreID;
int messageID;

int inElement(int semID, int semaphoreNumber, int semaphoreValue) {
  union semaphoreNum {
    struct semid_ds * buf;
    unsigned short * array;
    int value;
  }
  arg;
  arg.value = semaphoreValue;
  return semctl(semID, semaphoreNumber, SETVAL, arg);
}

int getsemaphoreID(key_t sharedKey, int nsems) {
  int semID;
  if ((semID = semget(sharedKey, nsems, PERM | IPC_CREAT)) == -1) {
    return (-1);
  }
  semaphoreID = semID;
  return semaphoreID;
}

void setsembuf(struct sembuf * s, int n, int op, int flg) {
  s-> sem_num = (short) n;
  s-> sem_op = (short) op;
  s-> sem_flg = (short) flg;
  return;
}

int getmessageID(key_t mkey) {
  int msgID;
  if ((msgID = msgget(mkey, PERM | IPC_CREAT)) == -1) {
    return (-1);
  }
  messageID = msgID;
  return messageID;
}

int sendMessages(int msgID, char ** myList, int lineNum) {
  int i;
  mymsg_t * mymsg;
  for (i = 0; i < lineNum; i++) {
    if ((mymsg = (mymsg_t * ) malloc(sizeof(mymsg_t))) == NULL) {
      return (-1);
    }
    mymsg -> mType = i + 1;
    memcpy(mymsg -> mText, myList[i], LINE_NUM);
    if (msgsnd(msgID, mymsg, LINE_NUM, 0) == -1) {
      return (-1);
    }
  }
  return (0);
}

int removemessageQueue(int msgID) {
  return msgctl(msgID, IPC_RMID, NULL);
}

int deletesharedMem(int msgID, int semID) {
  if (msgID == -1)
    msgID = messageID;
  if (semaphoreID == -1)
    semID = semaphoreID;
  char * msg = "Killing message queue.\n";
  write(STDERR_FILENO, msg, 18);
  if (removemessageQueue(msgID) == -1) {
    perror("Failed to destroy message queue.\n");
  }

  msg = "Killing semaphore set.\n";
  write(STDERR_FILENO, msg, 23);
  if (semctl(semID, 0, IPC_RMID) == -1) {
    perror("Failed to remove semaphore set.\n");
  }
  if (errno != 0)
    return (-1);
  return (0);
}

int setArrayFromFile(const char * filename, char ** list) {
  FILE * fp;
  fp = fopen(filename, "r");
  if (fp == NULL) {
    return (-1);
  }
  int n = 0;
  char * line = malloc(LINE_NUM * sizeof(char));
  rewind(fp);
  while (fgets(line, LINE_NUM, (FILE * ) fp)) {
    list[n] = malloc(LINE_NUM * sizeof(char));
    memcpy(list[n], line, LINE_NUM);
    list[n][strcspn(list[n], "\n")] = '\0';
    n++;
  }
  free(line);
  fclose(fp);
  if (errno != 0)
    return (-1);
  return (0);
}

int countLines(const char * filename) {
  FILE * fp = fopen(filename, "r");
  if (fp == NULL) {
    return -1;
  }
  int x = 0;
  char * line = malloc(LINE_NUM * sizeof(char));
  rewind(fp);
  while (fgets(line, LINE_NUM, (FILE * ) fp)) {
    x++;
  }
  free(line);
  fclose(fp);
  if (errno != 0)
    return (-1);
  return (x);
}

int writeToFile(const char * filename, long pid, int index,
  const char * text) {
  FILE * fp;
  fp = fopen(filename, "a+");
  if (fp == NULL) {
    return (-1);
  }
  fprintf((FILE * ) fp, "%ld\t%d\t%s\n", pid, index, text);
  fclose(fp);
  return 0;
}

static alarmer = 0;
void catchCTRLC(int signo) {
  alarm(0); // cancel alarm
  if (alarmer == 0) {
    char * msg = "Ctrl^C has been pressed! Killing child processes.\n";
    write(STDERR_FILENO, msg, 36);
  }
  deletesharedMem(-1, -1);
  pid_t pgid = getpgid(getpid());
  while (wait(NULL)) {
    if (errno == ECHILD)
      break;
  }

  kill(pgid, SIGKILL);
}

void timerHandler(int signo) {
  alarmer = 1;
  char * msg = "Alarm occured. Time to kill children.\n";
  write(STDERR_FILENO, msg, 39);
  pid_t pgid = getpgid(getpid());

  kill(pgid, SIGINT);
}

void handleChild(int signo) {
  char msg[] = "Child interrupted. Goodbye.\n";
  write(STDERR_FILENO, msg, 32);
  exit(1);
}

int mysemList(char * filename, int * lineNum, char ** * myList) {
  if (( * lineNum = countLines(filename)) == -1) {
    return (-1);
  }
  * myList = malloc(( * lineNum) * sizeof(char * ));
  if (setArrayFromFile(filename, * myList) == -1) {
    return (-1);
  }
  return (0);
}

int sigHandlers() {
  struct sigaction newact = {
    0
  };
  struct sigaction timer = {
    0
  };
  timer.sa_handler = timerHandler;
  timer.sa_flags = 0;
  newact.sa_handler = catchCTRLC;
  newact.sa_flags = 0;

  if ((sigemptyset( & timer.sa_mask) == -1) ||
    (sigaction(SIGALRM, & timer, NULL) == -1)) {
    return (-1);
  }

  if ((sigemptyset( & newact.sa_mask) == -1) ||
    (sigaction(SIGINT, & newact, NULL) == -1)) {
    return (-1);
  }
  return (0);
}

int mySemaphores(int semID, int lineNum) {
  if (inElement(semID, 0, 1) == -1) {
    if (semctl(semID, 0, IPC_RMID) == -1)
      return (-1);
    return (-1);
  }
  if (inElement(semID, 1, lineNum) == -1) {
    if (semctl(semID, 0, IPC_RMID) == -1)
      return (-1);
    return (-1);
  }
  if (inElement(semID, 2, 19) == -1) {
    if (semctl(semID, 0, IPC_RMID) == -1)
      return (-1);
    return (-1);
  }
  return (0);
}

int palindromeChecker(const char * string) {
  int x = 0;
  char * snippit = cutString(string);
  int len = strlen(snippit);
  int y = len - 1;
  for (x = 0; x < len / 2; x++) {
    if (snippit[x] != snippit[y])
      return (-1);
    y--;
  }
  return (1);
}

char * cutString(const char * string) {
  char * snippit = (char * ) malloc(LINE_NUM * sizeof(char));
  int x, y;
  char t;
  y = 0;
  for (x = 0; x < LINE_NUM; x++) {
    t = string[x];
    if (isalpha(t) || isdigit(t) || (t == '\0')) {
      if (isupper(t)) {
        t = tolower(t);
      }
      snippit[y] = t;
      if (snippit[y] == '\0')
        break;
      y++;
    }
  }
  return snippit;
}

int signalHandler1() {
  struct sigaction act = {
    0
  };
  act.sa_handler = handleChild;
  act.sa_flags = 0;
  if ((sigemptyset( & act.sa_mask) == -1) ||
    (sigaction(SIGINT, & act, NULL) == -1) ||
    (sigaction(SIGALRM, & act, NULL) == -1)) {
    return (-1);
  }
  return (0);
}
