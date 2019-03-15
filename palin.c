// Jonathan Pham
// CS 4760 Operating Systems
// Assignment 3: Semaphores & OS Simulator
// Due: 03/18/2019

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "palindromeHelper.h"
#define PALINDROME "./palin.out"
#define NOPALINDROME  "./nopalin.out"

int semaphore_ID;
struct sembuf mutex[2];


int main(int argc, char ** argv) {

//  if (argc < 3) {
//    fprintf(stderr, "Please try again, wrong number of arguments.\n");
//    return (1);
//  }

  int index = atoi(argv[2]);
  int id = atoi(argv[1]);

  int randomNum1, randomNum2;
  struct timespec tyme;
  clock_gettime(CLOCK_MONOTONIC, & tyme);
  srand((unsigned)(tyme.tv_sec ^ tyme.tv_nsec ^ (tyme.tv_nsec >> 31)));
  randomNum1 = rand() % 3;
  randomNum2 = rand() % 3;

  key_t mkey, skey;
  if (((mkey = ftok(KEYPATH, ASS3_ID)) == -1) ||
    ((skey = ftok(KEYPATH, SEMAPHORE_ID)) == -1)) {
    perror("Failed to retreive corresponding keys.\n");
    return (1);
  }

  if (signalHandler1() == -1) {
    perror("Failed to set up signal handlers.\n");
    return (1);
  }

  if ((semaphore_ID = semget(skey, 3, PERM)) == -1) {
    if (errno == EIDRM) {
      perror("Failed to create semaphore.\n");
      return (1);
    }
    perror("Failed to set up semaphore.\n");
    return (1);

  }

  struct sembuf signalDad[2];
  setsembuf(mutex, 0, -1, 0);
  setsembuf(mutex + 1, 0, 1, 0);
  setsembuf(signalDad, 1, -1, 0);
  setsembuf(signalDad + 1, 2, 1, 0);

  int message_ID;
  size_t size;
  mymsg_t mymsg;
  if ((message_ID = msgget(mkey, PERM)) == -1) {
    if (errno == EIDRM) {
      perror("Failed to set up semaphore.\n");
      return (1);
    }
    perror("Failed to create message queue.\n");
    return (1);
  }

  if ((size = msgrcv(message_ID, & mymsg, LINE_NUM, index + 1, 0)) == -1) {
    perror("Failed to receive message.\n");
    return (1);
  }

  if (semop(semaphore_ID, mutex, 1) == -1) {
    if (errno == EIDRM) {
      fprintf(stderr, "(Index Number:%d) interrupted.\n", index);
      return (1);
    }
    perror("Failed to lock semid.");
    return (1);
  }

  sigset_t newmask, oldmask;
  if ((sigfillset( & newmask) == -1) ||
    (sigprocmask(SIG_BLOCK, & newmask, & oldmask) == -1)) {
    perror("Failed setting signal mask.\n");
    return (1);
  }

  long pid = (long) getpid();
  const time_t tma = time(NULL);
  char * tme = ctime( & tma);
  fprintf(stderr, "(Index Number:%d) in critical section: %s", index, tme);
  sleep(randomNum1);
  int p = palindromeChecker(mymsg.messageString);
  char * filename;
  if (p < 0) {
    filename = NOPALINDROME;
  } else {
    filename = PALINDROME;
  }

  if (writeToFile(filename, pid, index, mymsg.messageString) == -1) {
    perror("Failed to open file.\n");

    if (semop(semaphore_ID, mutex + 1, 1) == -1)
      perror("Failed to unlock semid.\n");

    if (semop(semaphore_ID, signalDad, 2) == -1)
      perror("Failed to signal parent.\n");
    return (1);
  }
  sleep(randomNum2);

  if ((sigprocmask(SIG_BLOCK, & newmask, & oldmask) == -1)) {
    perror("Failed setting signal mask.\n");
    return (1);
  }

  if (semop(semaphore_ID, mutex + 1, 1) == -1) {
    if (errno == EINVAL) {
      char * msg = "finished critical section after signal";
      fprintf(stderr, "(Index:=%d) %s\n", index, msg);
      return (1);
    }
    perror("Failed to unlock semid.\n");
    return (1);
  }

  if (semop(semaphore_ID, signalDad, 2) == -1) {
    perror("Failed to signal parent.\n");
    return (1);
  }
  //if (errno != 0) {
  //  perror("palin uncaught error:");
  //  return 1;
  //}
  return (0);
}
