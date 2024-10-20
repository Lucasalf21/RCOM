

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
#define TRANSMITTER_ADRESS (0x03)


void alarmHandler(int signal);
