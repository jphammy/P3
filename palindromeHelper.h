#ifndef IPCHELPER_H_
#define IPCHELPER_H_
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/stat.h>
#define ASS3_ID 456234
#define SEMAPHORE_ID 234456
#define LINE_NUM 256
#define PERM (S_IRUSR | S_IWUSR)
#define KEYPATH "./spring.4760"
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
    long mType;
    char mText[LINE_NUM];
  } mymsg_t;

char * cutString(const char * string);
int mysemList(char * filename, int * lineNum, char ** * myList);
int sigHandlers();
int mySemaphores(int semID, int lineNum);
int signalHandler1();
int palindromeChecker(const char * string);
int inElement(int semID, int semaphoreNumber, int semaphoreValue);
int getmessageID(key_t mkey);
int removemessageQueue(int messageID);
int setArrayFromFile(const char * filename, char ** list);
int countLines(const char * filename);
int writeToFile(const char * filename, long pid, int index, const char *text);
int deletesharedMem(int messageID, int semaphoreID);
void catchCTRLC(int signo);
void timerHandler(int signo);
void catchCTRLC(int signo);
void timerHandler(int signo);
void handleChild(int signo);
void setsembuf(struct sembuf * s, int n, int op, int flg);
void setmsgid(int msgid);

#endif
