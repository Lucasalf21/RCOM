#include "aux.h"
#include "header.h"

#include <stdio.h>
#include <string.h>


Frame frame;

void alarmHandler(int signal, int* alarmCnt, int* alarmEnabled)
{
    alarmEnabled = FALSE;
    alarmCnt++;
    printf("Alarm #%d\n", alarmCnt);
}

unsigned char getControlInfo(){
    unsigned char byte, c;
    enum State state = START;

    while (state != STOP){
        if (readByteSerialPort(&byte) > 0){
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
int read_disc(int *alarmEnabled)
{
    alarmEnabled = TRUE;
    enum State state = START;
    unsigned char buf[BUF_SIZE + 1] = {0};
    while (state != STOP && alarmEnabled)
    {
        // Returns after 5 chars have been input
        readByteSerialPort(buf);
        switch (state)
        {
        case START:
            if (buf[0] == FLAG)
                state = FLAG;

            break;
        case FLAG:

            if (buf[0] == TRANSMITTER_ADDRESS)
                state = A;
            else if (buf[0] == FLAG)
                break;
            else
                state = START;
            break;

        case A:
            if (buf[0] == (CONTROL_DISC))
                state = C;
            else if (buf[0] == F)
                state = FLAG;
            else
                state = START;

            break;
        case C:

            if (buf[0] == (TRANSMITTER_ADDRESS ^ CONTROL_DISC))
                state = BCC1;
            else if (buf[0] == F)
                state = FLAG;
            else
                state = START;
            break;
        case BCC1:

            if (buf[0] == FLAG)
            {
                alarm(0);
                return 0;
            }
            else
            {
                state = START;
            }
            break;
        default:
            break;
        }
    }
    return 1;
}

Packet write_control(unsigned char control, const char *filename, long filesize)
{
    Packet pckt;


    // Push the C related to the control (START or END)
    pckt.controlField = control;

    unsigned char size = 0;
    long aux = filesize;
    while(aux>0){
        aux/=256;
        size++;
    }
    //size++;
    // Push the T related to the filesize
    pckt.tlv[0].T = T_SIZE;
    // Push the size(V) of the filesize
    pckt.tlv[0].V = size;
    // getting the last two bytes of the filesize, and pushing them always after the size
    while (size > 0)
    {
        pckt.tlv[0].L = (unsigned char)filesize % 256;
        filesize /= 256;
        size--;
    }

    // If there is a filename, push it
    if (*filename != '\0')
    {

        // Push the T related to the filename
        pckt.tlv[1].T = T_NAME;
        // Push the size(V) of the filename
        pckt.tlv[1].V = strlen(filename);
        // Push the filename
        for (int i = 0; i < strlen(filename); i++)
        {
            pckt.tlv[1].L = filename[1];
        }
    }

    return pckt;
}

DataPacket write_data(unsigned char *buf, int bufSize)
{
    DataPacket pckt;


    unsigned char l2 = bufSize / 256;
    unsigned char l1 = bufSize % 256;
    
    pckt.controlField = CONTROL_DATA;

    pckt.L2 = l2;

    pckt.L1 = l1;

    for (int i = 0; i < bufSize; i++)
    {
        pckt.data[i] = buf[i];
    }

    return pckt;
}

unsigned char read_p(int information_frame, unsigned char *packet)
{
    // information frame that is received in the package
    unsigned char read_i;

    // size of the package
    int size = 0;

    // flag that signals if a defuf is needed
    int deStuff = FALSE;

    // Current state of the state machine
    enum State state = START;
    unsigned char buf[BUF_SIZE + 1] = {0};
    while (state != STOP)
    {

        // Returns after 30 seconds without input
        readByteSerialPort(buf);

        // Code to defuf a byte
        if (deStuff)
        {
            if (buf[0] == FLAG)
                packet[size] = F;
            else if (buf[0] == STUFFED_ESC){
                packet[size] = ESC;
            }
            else
                continue;
            size++;
            deStuff = FALSE;
        }
        else
        {
            // State machine
            switch (state)
            {
            case START:
                // First state
                if (buf[0] == F)
                    state = FLAG;

                break;
            case FLAG:
                // State after receiving a flag
                if (buf[0] == TRANSMITTER_ADDRESS)
                    state = A;
                else if (buf[0] == FLAG)
                    // if flag is received should not do nothing
                    break;
                else
                    // if value read is neither the Adress nor the flag it should start over again
                    state = START;
                break;

            case A:
                // State after receiving the adress
                if (buf[0] == I0 || buf[0] == I1)
                {
                    state = C;
                    // information frame can be either one, we will store it for now and deal with it later
                    read_i = buf[0];
                }
                else if (buf[0] == FLAG)
                    // if a flag is receive it should go back to the flag state
                    state = FLAG;
                else
                    // Any other value should make the machine go back to the begining
                    state = START;
                break;
            case C:
                // State after receiving the control (information frame)
                if (buf[0] == (TRANSMITTER_ADDRESS ^ read_i))
                    // BCC1 is correct
                    state = BCC1;
                else if (buf[0] == FLAG)
                    // if a flag is receive it should go back to the flag state
                    state = FLAG;
                else
                    // Any other value should make the machine go back to the begining
                    state = START;
                break;
            case BCC1:
                // State after the BCC1, we will start reading the packet now
                if (buf[0] == FLAG)
                {
                    // When receiving the flag the all the information has been sent
                    state = STOP;
                    break;
                }
                else
                {
                    // Reading the packet
                    if (buf[0] == ESC)
                    {
                        // If a escape appears that means a destuff is needed
                        deStuff = TRUE;
                    }
                    else
                    {
                        // Otherwise we can just insert the data in the packet
                        packet[size] = buf[0];
                        size++;
                    }
                }
                break;
            default:
                break;
            }
        }
    }
    // if current information frame is different from the one read that means that the package is repeated
    // we will signal that to the link layer
    if (information_frame != read_i)
        return -1;

    // Return the size of the package
    return size;
}

unsigned write_RR(unsigned char information_frame)
{

        frame.data[0] = F;
        frame.data[1] = TRANSMITTER_ADDRESS;
        if (information_frame == I0)
        {
            frame.data[2] = RR1;
        }
        else
        {
            frame.data[2] = RR1;        
        }
        frame.data[3] = (TRANSMITTER_ADDRESS ^ frame.data[2]);
        frame.data[4] = F;

        int bytes = writeByteSerialPort(frame.data, 5);

        return 1;
}
unsigned write_REJ(int information_frame)
{
        frame.data[0] = F;
        frame.data[1] = TRANSMITTER_ADDRESS;
        if (information_frame == I0)
        {
            frame.data[2] = REJ0;
        }
        else
        {
            frame.data[2] = REJ1;        
        }
        frame.data[3] = (TRANSMITTER_ADDRESS ^ frame.data[2]);
        frame.data[4] = F;

        int bytes = writeByteSerialPort(frame.data, 5);

        return 1;

}
unsigned write_SUD(unsigned char control)
{
    frame.data[0] = F;
    frame.data[1] = TRANSMITTER_ADDRESS;
    frame.data[2] = control;
    frame.data[3] = (TRANSMITTER_ADDRESS ^ frame.data[2]);
    frame.data[4] = F;

    int bytes = writeByteSerialPort(frame.data, 5);

    return 1;

}