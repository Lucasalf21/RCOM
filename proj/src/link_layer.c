// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "header.h"
#include "aux.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source


extern unsigned char alarmEnabled = FALSE;
extern unsigned char alarmCnt = 0;
unsigned char fd;
unsigned char nRetransmissions = 0;
unsigned char timeout = 0;
unsigned char information_frame = I0;
unsigned char frameCnt = 0;
unsigned char totalRejCount = 0;


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

    int currentTransmission = 0;
    int accepted = 0;

    while (currentTransmission < nRetransmissions){
        alarmEnabled = FALSE;
        alarm(timeout);
        while (alarmEnabled == FALSE && !accepted){
            write(fd, infoFrame, frameSize);
            unsigned char res = getControlInfo();

            if (!res){
                continue;
            } else if (res == RR0 || res == RR1){
                accepted = 1;
                Tx = (Tx + 1) % 2;
                break;
            }
        }
        if (accepted){
            break;
        }
        currentTransmission++;
    }

    free(infoFrame);
    
    if (!accepted){
        llclose(fd);
        return -1;
    }

    return frameSize;
}

unsigned char getControlInfo(){
    unsigned char byte, c;
    enum State state = START;

    while (state != STOP){
        if (read(fd, &byte, 1) > 0){
            switch (state){
                case START:
                    if (byte == F){
                        state = FLAG;
                    }
                    break;
                

                case FLAG:
                    if (byte == RECEIVER_ADDRESS){
                        state = A;
                    } else if (byte != F){
                        state = START;
                    }
                    break;

                case A:
                    if (byte == RR0 || byte == RR1 || byte == REJ0 || byte == REJ1 || byte == DISC){
                        state = C;
                        c = byte;
                    } else if (byte == F){
                        state = FLAG;
                    } else{
                        state = START;
                    }
                    break;

                case C:
                    if (byte == (c ^ RECEIVER_ADDRESS)){
                        state = BCC1;
                    } else if (byte == F){
                        state = FLAG;
                    } else{
                        state = START;
                    }
                    break;

                case BCC1:
                    if (byte == F){
                        state = STOP;
                    } else{
                        state = START;
                    }
                    break;

                default:
                    break;
            }
        } else{
            return 0;
        }
    }

    return c;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    int sz = read_p(fd, information_frame, packet);

    frameCnt++;

    unsigned char BCC2 = 0;
    sz = sz - 1;


    if (sz == -1)
    {

        printf("Repeated information! Resending response!\n");
        if (information_frame == I0)
            write_RR(fd, I0);
        else
            write_RR(fd, I1);
            totalRejCount++;
            return -1;
    }
    
    for(unsigned i = 0; i <= sz; i++)
    {
        BCC2 ^= packet[i];
    }
    
    if(BCC2 == packet[sz])
    {

        write_RR(fd, information_frame);

        if (information_frame == I0)
            information_frame = I1;
        else
            information_frame = I0;
        return sz;

    }
    else
    {
        printf("Deu porcaria!!!\n");
        write_REJ(fd, information_frame);
        totalRejCount++;
        return -1;
    }


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
