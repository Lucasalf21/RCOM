#ifndef _UTILS_H_
#define _UTILS_H_

#include "link_layer.h"
#include <signal.h>
#include "serial_port.h"



unsigned char read_p(int information_frame, unsigned char *packet);
Packet write_control(unsigned char control, const char *filename, long filesize);
DataPacket write_data(unsigned char *buf, int bufSize);
unsigned char getControlInfo();
unsigned write_RR(unsigned char information_frame);
unsigned write_REJ(int information_frame);
unsigned write_SUD(unsigned char control);


#endif