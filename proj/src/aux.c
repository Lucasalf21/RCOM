#include "header.h"
#include "aux.h"

int alarmEnabled = FALSE;
int alarmCnt = 0;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCnt++;
    printf("Alarm #%d\n", alarmCnt);
}