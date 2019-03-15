#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <math.h> 
#include <errno.h> 
#include <unistd.h> 
#include <sys/sem.h> 
#include <sys/ipc.h> 
#include <fcntl.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include "palindromeHelper.h"
#define INFILE "./in.txt"

int main(int argc, char ** argv) {

  key_t memoryKey;
  key_t sharedKey;

  int lineNum;
  char ** myList;
  if (mysemList(INFILE, & lineNum, & myList) == -1) {
    perror("Failed to read file.\n");
    return (1);
  }

  if (((sharedKey = ftok(KEYPATH, SEMAPHORE_ID)) == -1) || ((memoryKey = ftok(KEYPATH, ASS3_ID)) == -1)) {
    perror("Failed to retreive keys.\n");
    return (1);
  }

  alarm(100);

  if (sigHandlers() == -1) {
    perror("Failed to set up signal handlers.\n");
    return (1);
  }

  int semID;
  if ((semID = getsemaphoreID(sharedKey, 3)) == -1) {
    perror("Failed to create semaphore\n.");
    return (1);
  }

  struct sembuf waitfordone[1];
  struct sembuf birthcontrol[1];
  setsembuf(waitfordone, 1, 0, 0);
  setsembuf(birthcontrol, 2, -1, 0);
  if (mySemaphores(semID, lineNum) == -1) {
    perror("Failed to init semaphores.\n");
    return (1);
  }

  int msgid;
  if ((msgid = getmessageID(memoryKey)) == -1) {
    perror("Failed to create message queue.\n");
    return (1);
  }

  if (sendMessages(msgid, myList, lineNum) == -1) {
    perror("Failed to send messages to queue.\n");
    return (1);
  }

  free(myList);

  char palinid[16];
  char palinindex[16];
  int semaphoreValue;
  int childPID;
  int x = 0;
  while (x < lineNum) {
    if (semop(semID, birthcontrol, 1) == -1) {
      perror("Semaphore birth control.");
      return (1);
    }
    if ((semaphoreValue = semctl(semID, 2, GETVAL)) == -1) {
      perror("Semaphore birth control.");
      return (1);
    }
    if (semaphoreValue < 2) wait(NULL);

    if ((childPID = fork()) <= 0) {
      sprintf(palinid, "%d", x + 1 % 20);
      sprintf(palinindex, "%d", x);
      break;
    }
    fprintf(stderr, "... child with corresponding index: %d has spawned!\n", x);
    if (childPID != -1)
      x++;
  }

  if (childPID == -1) {
    perror("Failed to create child.\n");
    if (deletesharedMem(msgid, semID) == -1) {
      perror("Failed to remove shared memory segment.\n");
      return (1);
    }
  }

  if (childPID == 0) {
    execl("./palin", "palin", palinid, palinindex, (char * ) NULL);
    perror("Failure in exec.\n");
    return (1);
  }

  if (childPID > 0) {
    fprintf(stderr, "Spawning complete, waiting....\n");
    if (semop(semID, waitfordone, 1) == -1) {
      perror("Failed to wait for children.\n");
      return (1);
    }
    if (deletesharedMem(msgid, semID) == -1) {
      perror("Failed to remove shared memory.\n");
      return (1);
    }
    fprintf(stderr, "Task complete %ld\n", (long) getpgid(getpid()));
  }
  return (0);

}
