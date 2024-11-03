

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
#define RECEIVER_ADDRESS (0X01)
#define TX_0 (0x00)
#define TX_1 (0x80)
#define ESC (0x7D)  
#define I0 (0x00)
#define I1 (0x80)
#define CONTROL_UA (0x07)
#define CONTROL_SET (0x03)
#define CONTROL_FIELD_PACKET_START (0x02)
#define CONTROL_DATA (0x01)
#define RR0 (0x05)
#define RR1 (0x85)
#define REJ0 (0x01)
#define REJ1 (0x81)
#define DISC (0x0B)
#define T_SIZE (0x00)
#define T_NAME (0x01)
#define CONTROL_FIELD_PACKET_END (0x03)
#define STUFFED_ESC (0x5d)
#define SHOW_STATISTICS_ON 1
#define CONTROL_DISC (0x0B)



void alarmHandler(int signal);
