// Jonathan Pham
// // CS 4760 Operating Systems
// // Assignment 3: Semaphores & OS Simulator
// // Due: 03/18/2019

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
  int semaphoreValue;
  int childPID;
  int msgid;
  int semID;
  char **myList;
  char palindromeID[16];   
  char palindromeIndex[16];

  // Initialize myList, return error if issue with file
  if (mysemList (INFILE, &lineNum, &myList) == -1) {
    perror("Failed to read file; please try again.\n");
    return ( 1 );
  }

  // Obtain key from shared file with corresponding semaphore ID, KEYPATH and ASS3_ID
  // all of which are defined in palindromeHelper.h
  if (((sharedKey = ftok(KEYPATH, SEMAPHORE_ID)) == -1) || ((memoryKey = ftok(KEYPATH, ASS3_ID)) == -1)) {
    perror("Failed to get keys.\n");
    return ( 1 );
  }

  alarm(100); // Set alarm to 100 seconds per assignment specs.

  // Initialize signal handler
  if ( sigHandlers() == -1) {
    perror("Failed to set up signal handlers; please try again.\n");
    return ( 1 );
  }

  // Initialize & setup semaphore with error checking.
  if ((semID = getsemaphoreID (sharedKey, 3)) == -1) {
    perror("Failed to create semaphore\n.");
    return ( 1 );
  }
  // 3 Semaphores:
  // 2 = limits 20 children max at one time
  // 1 = main will know when to terminate
  // 0 = file input/output locks
  struct sembuf waitfordone[1];
  struct sembuf birthcontrol[1];
  setsembuf(waitfordone, 1, 0, 0);
  setsembuf(birthcontrol, 2, -1, 0);
  if (mySemaphores(semID, lineNum) == -1) {
    perror("Failed to get in semaphores.\n");
    return ( 1 );
  }

  // Set up Message Queue
  if ((msgid = getmessageID(memoryKey)) == -1) {
    perror("Failed to create message queue.\n");
    return ( 1 );
  }

  if (sendMessages(msgid, myList, lineNum) == -1) {
    perror("Failed to send messages to queue.\n");
    return ( 1 );
  }
  free(myList);

  // Send corresponding ID and Index(s) to forked processes.
  // 20 max processes, wait for one to finish before forking
  // more live child processes
  int x = 0;
  while (x < lineNum) {
    if (semop(semID, birthcontrol, 1) == -1) {
      perror("Error with semaphore creation.\n");
      return ( 1 );
    }
    if ((semaphoreValue = semctl(semID, 2, GETVAL)) == -1) {
      perror("Error with semaphore creation.\n.");
      return ( 1 );
    }
    if (semaphoreValue < 2) wait(NULL);
    if ((childPID = fork()) <= 0) {
      sprintf(palindromeID, "%d", x+1%20); 
      sprintf(palindromeIndex, "%d", x);    
      break;
    }
    fprintf(stderr, "... child with corresponding index: %d has spawned!\n", x);
    if (childPID != -1)
      x++;
  }

  // Forking process begins with error handling.
  if (childPID == -1) {
    perror("Failed to create child.\n");
    if (deletesharedMem(msgid, semID) == -1) {
      perror("Failed to remove shared memory segment.\n");
      return ( 1 );
    }
  }

  // If fork is successful, exec palin executable
  // otherwise return error.
  if (childPID == 0) {
    execl("./palin", "palin", palindromeID, palindromeIndex, (char*)NULL);
    perror("Failure in exec.\n");
    return ( 1 );
  }

  // Parent Process
  if (childPID > 0) {
    fprintf(stderr, "Spawning complete, waiting....\n");
    if (semop(semID, waitfordone, 1) == -1) {
      perror("Failed to wait for children.\n");
      return ( 1 );
    }
    // Return error for shared memory error.
    if (deletesharedMem(msgid, semID) == -1) {
      perror("Failed to remove shared memory.\n");
      return ( 1 );
    }
    fprintf(stderr, "Task complete %ld\n", (long) getpgid(getpid()));
  }
  return ( 0 );

}
