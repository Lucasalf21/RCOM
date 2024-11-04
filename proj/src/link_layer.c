// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "header.h"
#include "aux.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source


int alarmEnabled = FALSE;
int alarmCnt = 0;
unsigned char nRetransmissions = 0;
unsigned char timeout = 0;
unsigned char information_frame = I0;
unsigned char frameCnt = 0;
unsigned char totalRejCount = 0;
unsigned sz = 0;
unsigned currentTransmission = 0;
LinkLayer cpy;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCnt++;
    printf("Alarm #%d\n", alarmCnt);
}

int llopen(LinkLayer connectionParameters)
{
    if (openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate) < 0)
    {
        return -1;
    }


    (void) signal(SIGALRM, alarmHandler);
    alarmCnt = 0;
    enum State state = START; 
    unsigned char bf = 0;
    unsigned bytes;
    nRetransmissions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;
    cpy = connectionParameters;
    int control = 0;


    while (state != STOP && alarmCnt < connectionParameters.nRetransmissions )
    {   

        if (alarmEnabled == FALSE && connectionParameters.role == LlTx)
        {
            bytes = write_SUD(CONTROL_SET);
            alarm(connectionParameters.timeout);
            alarmEnabled = TRUE;
        } 
        
        if (connectionParameters.role == LlTx)
        {
            control = CONTROL_UA;
        }
        else
        {
            control = CONTROL_SET;
        }

        if(readByteSerialPort(&bf)  > 0)
        {
        // Check the state machine
        printf("%u\n",bf);
        switch (state)
        {
        // Confirm that the first byte is a FLAG
        case START:
            if (bf == F)
                state = FLAG;

            break;

        // Confirm that the second byte is the adress of the transmitter. If not, go back to the start state
        case FLAG:
            if (bf == TRANSMITTER_ADDRESS)
                state = A;
            else if (bf == F)
                break;
            else
                state = START;
            break;

        // Confirm that the third byte is the control byte that the receiver is expecting. If not, go back to the start state.
        case A:
            if (bf== control)
                state = C;
            else if (bf == F)
                state = FLAG;
            else
                state = START;
            break;

        // Confirm that the fourth byte is the XOR of the adress and the control bytes to confirm that the frame is valid. If not, go back to the first state.
        case C:
            if (bf == (TRANSMITTER_ADDRESS ^ control))
                state = BCC1;
            else if (bf == F)
                state = FLAG;
            else
                state = START;
            break;

        // Confirm that the fifth byte is a flag. If it it, then we set an alarm and we quit from the while. If not, go back to the start state.
        case BCC1:
            if (bf == F)
            {
                state = STOP;
                alarm(0);
                break;
            }
            else
                state = START;
            break;
        default:
            break;
        }
        
        }
    }
     
     
     if (connectionParameters.role == LlRx)
    {
        write_SUD(CONTROL_UA);
    }

    alarmCnt = 0;
 
    // If the alarm tries exceeded the maximum, return 1
    if (alarmCnt >= connectionParameters.nRetransmissions)return 1;
    // Wait until all bytes have been written to the serial port
    sleep(1);

    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////

unsigned char Tx = 0;
int llwrite(const unsigned char *bf, int bufSize)
{

    if (bufSize <= 0)
        return 1;
    alarmEnabled = FALSE;
    alarmCnt = 0;

    int res = 0;

    while (alarmCnt < cpy.timeout)
    {

        if (alarmEnabled == FALSE || res == -1)
        {
            write_inf_frame(bf, bufSize, information_frame);
            frameCnt++;
            alarm(cpy.timeout);
            alarmEnabled = TRUE;
        }

        res = read_frame_resp(information_frame , &alarmEnabled);

        if (res == 0)
        {
            if (information_frame == I0)
                information_frame = I1;
            else
                information_frame = I0;
            return 0;
        }
    }

    return 1;
}
/*
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

    currentTransmission = 0;
    int accepted = 0;

    while (currentTransmission < cpy.nRetransmissions){
        alarmEnabled = FALSE;
        alarm(timeout);
        while (alarmEnabled == FALSE && !accepted){
            writeBytesSerialPort(infoFrame, frameSize);
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
        llclose(1);
        return -1;
    }

    return frameSize;
}*/



////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    sz = read_p(information_frame, packet);

    frameCnt++;

    unsigned char BCC2 = 0;
    sz = sz - 1;


    if (sz == -1)
    {

        printf("Repeated information! Resending response!\n");
        if (information_frame == I0)
            write_RR(I0);
        else
            write_RR(I1);
            totalRejCount++;
            return -1;
    }
    
    for(unsigned i = 0; i <= sz; i++)
    {
        BCC2 ^= packet[i];
    }
    
    if(BCC2 == packet[sz])
    {

        write_RR(information_frame);

        if (information_frame == I0)
            information_frame = I1;
        else
            information_frame = I0;
        return sz;

    }
    else
    {
        printf("Deu porcaria!!!\n");
        write_REJ(information_frame);
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
    
    if (cpy.role == LlTx)
    {
        alarmEnabled = FALSE;
        alarmCnt = 0;


        // Try to close the connection until the alarm tries exceed the maximum
        while (alarmCnt < 4)
        {

            // If the alarm is not enabled yet, send a DISC frame
            if (alarmEnabled == FALSE)
            {
                write_SUD(CONTROL_DISC);
                alarmEnabled = TRUE;
                alarm(4);
            }

            // Wait for a DISC frame to send a UA frame
            if (!read_disc(&alarmEnabled))
            {
                write_SUD(CONTROL_UA);
                printf("Closed successfuly\n");
                break;
            }
        }
        if (alarmCnt >= 4)
        {
            printf("Timed exeded!\n");
            return 1;
        }
    }

    // If the user wants to see the statistics, print them
    if (showStatistics)
    {
        printf("\n\n\nStatistics:\n");
        printf("File size: %d\n", sz);

            printf("Frames sent: %d\n", currentTransmission);
            printf("Total number of alarms: %d\n",alarmCnt);
            printf("Frames read: %d\n", frameCnt);
            printf("Number of rejection/ repeted information %d\n", totalRejCount);
    }

    int clstat = closeSerialPort();
    return clstat;
}
