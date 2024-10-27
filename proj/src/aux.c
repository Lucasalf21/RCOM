#include "header.h"
#include "aux.h"

int alarmEnabled = FALSE;
int alarmCnt = 0;
Frame frame;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCnt++;
    printf("Alarm #%d\n", alarmCnt);
}

unsigned char read_p(int fd, int information_frame, unsigned char *packet)
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
        read(fd, buf, 1);

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

unsigned write_RR(unsigned fd, unsigned char information_frame)
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

        int bytes = write(fd, frame.data, 5);

        return 1;
}

unsigned write_REJ(unsigned fd, int information_frame)
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

        int bytes = write(fd, frame.data, 5);

        return 1;

}
unsigned write_SUD()
{
    
}