// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000
#define MAX_SIZE 64

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

typedef struct 
{
     unsigned char Flag;
    unsigned char Adress_transmiter;
    unsigned char Control_Field;
    unsigned char BCC;
    unsigned char Flag_end;
    unsigned char data[MAX_PAYLOAD_SIZE+5];
    unsigned char size;
    
} Frame;

typedef struct 
{
    unsigned char controlField;
    unsigned char T;
    unsigned char L;
    unsigned char V;
    unsigned char data[MAX_PAYLOAD_SIZE+5];
} Packet;

typedef struct 
{
    unsigned char controlField;
    unsigned char numberSequence;
    unsigned char L2;
    unsigned char L1;
    unsigned char data[MAX_PAYLOAD_SIZE+5];

}DataPacket;




// MISC
#define FALSE 0
#define TRUE 1

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

#endif // _LINK_LAYER_H_
