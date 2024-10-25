

enum State
{
    START = 0,
    FLAG,
    A,
    C,
    BCC1,
    D,
    BCC2,
    STOP
};


typedef struct
{
    unsigned char Flag;
    unsigned char A;
    unsigned char C;
    unsigned char BCC;
    unsigned char END_Flag;
    unsigned char data[5];

} Frame;



#define FALSE 0
#define TRUE 1
#define BUF_SIZE 256
#define F (0x7E)
#define TRANSMITTER_ADDRESS (0x03)
#define TX_0 (0x00)
#define TX_1 (0x80)
#define ESC (0x7D)  
#define I0 (0x00)
#define I1 (0x80)
#define RR0 (0x05)
#define RR1 (0x85)
#define REJ0 (0x01)
#defin  
#define STUFFED_ESC (0x5d)


void alarmHandler(int signal);
