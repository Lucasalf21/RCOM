#ifndef _UTILS_H_
#define _UTILS_H_

#include "link_layer.h"
#include <signal.h>
#include "serial_port.h"

void printFrame(Frame frame);
unsigned char read_p(int information_frame, unsigned char *packet);
Packet write_control(unsigned char control, const char *filename, long filesize);
DataPacket write_data(unsigned char *buf, int bufSize);
unsigned char getControlInfo();
unsigned write_RR(unsigned char information_frame);
unsigned write_REJ(int information_frame);
unsigned write_SUD(unsigned char control);
int write_inf_frame(const unsigned char *bf, int bufSize, unsigned char information_frame);
void frame_stuff(Frame *frame1);
int read_frame_resp(int information_frame, unsigned *alarmEnabled);


#endif
