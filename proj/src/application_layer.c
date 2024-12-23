// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "header.h"
#include "aux.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned totalRejCnt = 0;
unsigned int timeReadingFrames = 0;
unsigned startOfProgram;
unsigned endOfProgram;
LinkLayer ll;
unsigned fileSize;
Packet packet;

int pow_int(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}






//xau
int length(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}
int tlv_length(TLV* data[])
{
    int len = 0;
    while (data[len] != '\0') len++;
    return len;
}


void applicationLayer(const char *serialPort, const char *role, int baudRate, int nTries, int timeout, const char *filename)
{
    
    int start;
    int end;
    // Configuring link layer
    ll.baudRate = baudRate;
    ll.nRetransmissions = nTries;
    if (role[0] == 't')
        ll.role = LlTx;
    else
        ll.role = LlRx;
    strcpy(ll.serialPort, serialPort);
    ll.timeout = timeout;

    // Connecting to serial port
    if (llopen(ll))
    {
        printf("Time exeded!\nFailed to connect\n");
        return;
    }
    printf("Connected\n");
    start = time(NULL);

    // Receiving File
    if (ll.role == LlRx)
    {

        // Creating file to write. If it already exists, it will be overwritten.
        

        long bytes_to_read = 0;
        unsigned char packet[MAX_PAYLOAD_SIZE + 5];
        int c = 0;
        // Read until the first start control packet is received
        do
        {
            llread(packet);
        } while (packet[0] != CONTROL_FIELD_PACKET_START);
        
        if (packet[1] == 0x00)
        {
            bytes_to_read = 0;
            unsigned char L1 = packet[2];
            
            for (int i = L1; i > 0; i--)
            {
                bytes_to_read += pow_int(256, i - 1) * (int)packet[c + 3];
                c++;
            }
            fileSize = bytes_to_read;
        }
        else
        {
            printf("Error in the start control packet\n");
            exit(1);
        }
        int lenght = packet[c+4];
        char name[lenght+1];
        if(packet[c+3] ==0x01){
            
            for(int i = 0; i< lenght; i++){
                name[i]= packet[c+5+i];
            }
            name[lenght]='\0';
        }
        else{
            printf("Name not sent\n");
            exit(1);
        }
        FILE *file;
        if(packet[1] == 1) file = fopen(filename, "wb");
        else file = fopen(name, "wb");
        if (file == NULL)
        {
            printf("Error: Unable to create or open the file %s for writing.\n", filename);
            return;
        }

        // Read until the end control packet is received
        int size = -1;

        while (packet[0] != CONTROL_FIELD_PACKET_END)
        {

            // Read until a data packet is received
            while (size == -1)
            {

                size = llread(packet);
            }

            // Check if the packet is the end control packet
            if (packet[0] == CONTROL_FIELD_PACKET_END)
                break;

            // Write the data to the file
            if (size > 0)
            {
                size_t bytesWrittenNow = fwrite(packet + 3, 1, size - 3, file);
                if (bytesWrittenNow == (long)0)
                    break;
                bytes_to_read -= bytesWrittenNow;
            }
            size = -1;
        }

        // Check if all the bytes were read and written
        if (bytes_to_read > (long)0)
        {
            printf("Error: Data was lost! %ld bytes were lost\n", bytes_to_read);
            exit(1);
        }
        printf("All bytes were read and written\n");
        end = time(NULL);
        timeReadingFrames += difftime(end, start);

        fclose(file);
    }

    // Sending File
    else
    {

        // Opening file to read.
        FILE *file = fopen(filename, "rb");
        if (file == NULL)
        {
            printf("Error: Unable to open the file %s for reading.\n", filename);
            return;
        }

        // Get file size
        fseek(file, 0, SEEK_END);
        long fileLength = ftell(file);
        fseek(file, 0, SEEK_SET);

        unsigned char buffer[MAX_PAYLOAD_SIZE];
        size_t bytes_to_Read = MAX_PAYLOAD_SIZE;

        // Send start control packet
        fileSize = fileLength;
        Packet pckt = write_control(CONTROL_FIELD_PACKET_START, filename, fileSize);
        DataPacket dataPckt;
        unsigned max = tlv_length(pckt.tlv);
        unsigned size = 1 + max * 3;
        unsigned char data[size];
        char a = 1;
        for(unsigned i = 0; i <= max; i++){
            if(a == 1){
                a = 0;
                data[i] = pckt.controlField;
                i++;
            }
            data[i++] = pckt.tlv->T;
            data[i++] = pckt.tlv->L;
            data[i++] = pckt.tlv->V;
        }
        if (llwrite(data, size))
        {
            printf("Time exeded\n");
            return;
        }

        // Send data packets until all the bytes are sent
        while (fileLength > 0)
        {

            // Read the file to check if it is the last packet
            int bytesRead = fread(buffer, 1, (fileLength >= bytes_to_Read) ? bytes_to_Read : fileLength, file);
            if (bytesRead == 0)
            {
                printf("Error reading the file.\n");
                break;
            }

            fileLength -= bytesRead;
            dataPckt = write_data(buffer, bytesRead);
            if (llwrite(dataPckt.data, length(dataPckt.data)))
            {
                printf("timed exeded\n");
                return;
            }
        }

        // Send end control packet
        pckt = write_control(CONTROL_FIELD_PACKET_END, filename, fileSize);
        max = tlv_length(pckt.tlv);
        size = 1 + max * 3;
        a = 1;
        for(unsigned i = 0; i <= max; i++){
            if(a == 1){
                a = 0;
                data[i] = pckt.controlField;
                i++;
            }
            data[i++] = pckt.tlv->T;
            data[i++] = pckt.tlv->L;
            data[i++] = pckt.tlv->V;
        }
        llwrite(data, size);

        printf("All bytes were written\n");

        end = time(NULL);
        timeReadingFrames += difftime(end, start);

        fclose(file);
    }
    endOfProgram = time(NULL);

    llclose(SHOW_STATISTICS_ON);
}
