

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


#define FALSE 0
#define TRUE 1
#define BUF_SIZE 256
#define F (0x7E)
#define TRANSMITTER_ADDRESS (0x03)
#define TX_0 (0x00)
#define TX_1 (0x80)
#define ESC (0x7D)


void alarmHandler(int signal);
