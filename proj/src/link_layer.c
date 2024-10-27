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

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if (fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate) < 0)
    {
        return -1;
    }
    (void) signal(SIGALRM, alarmHandler);
    alarmCnt = 0;
    enum State state = START; 
    unsigned char bf[BUF_SIZE + 1] = {0};
    Frame frame;

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
                    frame.Flag = F;
                break;

            case FLAG:
                if (bf[0] == TRANSMITTER_ADRESS)
                {
                    state = A;
                    frame.Adress_transmiter = TRANSMITTER_ADRESS; 
                }
                else
                    state = START;
                    memset(&frame, 0, sizeof(Frame));
                break;

            case A:
                if (bf[0] == control)
                    state = C;
                    frame.Control_Field = control;
                else if (bf[0] == FLAG)
                    state = FLAG;
                else
                    state = START;
                    memset(&frame, 0, sizeof(Frame));

                break;

            case C:
                if (bf[0] == (0x03 ^ control))
                    state = BCC1;
                    frame.BCC = bf[0];
                else if (bf[0] == FLAG)
                    state = FLAG;
                else
                    state = START;
                    memset(&frame, 0, sizeof(Frame));
                break;

            case BCC1:
                if (bf[0] == F)
                {
                    frame.Flag_end = F;
                    state = STOP;
                    alarm(0);
                }
                else
                    state = START;
                    memset(&frame, 0, sizeof(Frame));
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
                if (bf[0] == TRANSMITTER_ADRESS)
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
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

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
    
    int clstat = closeSerialPort();
    return clstat;
}
