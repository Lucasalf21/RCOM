// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "header.h"
#include "aux.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source


extern int alarmEnabled = FALSE;
extern int alarmCnt = 0;
int fd;
int nRetransmissions = 0;
int timeout = 0;


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if (openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate) < 0)
    {
        return -1;
    }
    (void) signal(SIGALRM, alarmHandler);
    alarmCnt = 0;
    enum State state = START;
    unsigned char bf[BUF_SIZE + 1] = {0};
    nRetransmissions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;

    while(state != STOP && alarmCnt < 3)// 3 is number specified to the number of tries
    {

        if(alarmEnabled == FALSE && connectionParameters.role == LlTx)
        {
            alarm(4);
            alarmEnabled = TRUE;
        }

        read(fd, bf, 1);


        if(connectionParameters.role = LlRx)
        {
            int control = 0x03;

            switch (state)
            {
                
            case START:
                if (bf[0] == F)
                    state = FLAG;

                break;

            case FLAG:
                if (bf[0] == TRANSMITTER_ADDRESS)
                    state = A;
                else if (bf[0] == FLAG)
                    break;
                else
                    state = START;
                break;

            case A:
                if (bf[0] == control)
                    state = C;
                else if (bf[0] == FLAG)
                    state = FLAG;
                else
                    state = START;
                break;

            case C:
                if (bf[0] == (0x03 ^ control))
                    state = BCC1;
                else if (bf[0] == FLAG)
                    state = FLAG;
                else
                    state = START;
                break;

            case BCC1:
                if (bf[0] == F)
                {
                    state = STOP;
                    alarm(0);
                }
                else
                    state = START;
                break;
            default:
                break;
            }

            int bytes = write(fd, bf, 5);
        }

        if(connectionParameters.role = LlTx)
        {
            int control = 0x07;

            switch (state)
            {
                
            case START:
                if (bf[0] == F)
                    state = FLAG;

                break;

            case FLAG:
                if (bf[0] == TRANSMITTER_ADDRESS)
                    state = A;
                else if (bf[0] == FLAG)
                    break;
                else
                    state = START;
                break;

            case A:
                if (bf[0] == control)
                    state = C;
                else if (bf[0] == FLAG)
                    state = FLAG;
                else
                    state = START;
                break;

            case C:
                if (bf[0] == (0x03 ^ control))
                    state = BCC1;
                else if (bf[0] == FLAG)
                    state = FLAG;
                else
                    state = START;
                break;

            case BCC1:
                if (bf[0] == F)
                {
                    state = STOP;
                    alarm(0);
                }
                else
                    state = START;
                break;
            default:
                break;
            }
        }
        
    }

    

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////

unsigned char Tx = 0;

int llwrite(const unsigned char *buf, int bufSize)
{
    int frameSize = bufSize + 6;
    unsigned char *infoFrame = (unsigned char*) malloc(frameSize);

    infoFrame[0] = F;
    infoFrame[1] = TRANSMITTER_ADDRESS;
    if (Tx % 2 == 0){
        infoFrame[2] = TX_0;
    } else{
        infoFrame[2] = TX_1;
    }
    infoFrame[3] = infoFrame[1] ^ infoFrame[2];
    
    int j = 4;
    for (int i = 0; i < bufSize; i++){
        if (buf[i] == F || buf[i] == ESC){
            infoFrame = realloc(infoFrame, ++frameSize);
            infoFrame[j++] = ESC;
            infoFrame[j++] = buf[i] ^ 0x20;
        } else{
            infoFrame[j++] = buf[i];
        }
    }

    unsigned char bcc2 = 0;
    for (int i = 0;  i < bufSize; i++){
        bcc2 ^= buf[i];
    }
    infoFrame[j++] = bcc2;
    infoFrame[j] = F;

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}
