#include "aux.h"
#include "header.h"

#include <stdio.h>
#include <string.h>


Frame frame;

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

void frame_stuff(Frame *frame1)
{

    int i = 4; // packet size

    // Going through the packet
    while (i < sizeof(frame1->data))
    {

        unsigned char c = frame1->data[i];

        // To stuff the flag we need to add 0x7d and 0x5e
        if (c == F)
        {
            frame1->data[i] = 0x7d;
            frame1->data[i+1] = 0x5e;
            i++;
        }
        // To stuff the escape we need to add 0x7d and 0x5d
        else if (c == ESC)
        {
            frame1->data[i] = 0x7d;
            frame1->data[i+1] = 0x5d;
            i++;
        }

        i++;
    }
}

int write_inf_frame(const unsigned char *bf, int bufSize, unsigned char information_frame)
{
    Frame frame1;
    frame1.data[0] = F;
    frame1.data[1] = TRANSMITTER_ADDRESS;

    // Identificando o tipo do quadro (information_frame)
    frame1.data[2] = (information_frame == 0) ? I0 : I1;
    frame1.data[3] = TRANSMITTER_ADDRESS ^ frame1.data[2];

    int packetSize = 4;  // Começamos a partir da posição 4
    unsigned char BCC2 = 0;

    // Processando o buffer bf e calculando BCC2
    while (bufSize > 0 && packetSize < sizeof(frame1.data) - 1)
    {
        BCC2 ^= *bf;
        frame1.data[packetSize] = *bf;
        printf("Byte adicionado ao frame: 0x%x, Novo BCC2: 0x%x\n", *bf, BCC2);
        bf++;
        bufSize--;
        packetSize++;
    }

    if (bufSize > 0)  // Se não conseguimos adicionar todos os dados
    {
        fprintf(stderr, "Error: frame1.data buffer overflow\n");
        return -1;  // Retorno de erro adequado
    }

    // Adicionando BCC2 no frame, verificando se há espaço
    if (packetSize < sizeof(frame1.data) - 1) 
    {
        frame1.data[packetSize] = BCC2;
        packetSize++;
    } 
    else 
    {
        fprintf(stderr, "Error: frame1.data buffer overflow ao adicionar BCC2\n");
        return -1;  // Retorno de erro adequado
    }

    printf("BCC2 final adicionado ao frame: 0x%x\n", BCC2);
    printf("Tamanho do frame antes do stuffing: %u\n", packetSize);

    // Realizando stuffing no frame (exceto FLAG final)
    frame1.size = packetSize;  // Atualiza o tamanho antes do stuffing
    frame_stuff(&frame1);

    // Adicionando FLAG final no frame, verificando espaço
    if (frame1.size < sizeof(frame1.data)) 
    {
        frame1.data[frame1.size] = F;
        frame1.size++;  // Incrementa o tamanho final do frame
    } 
    else 
    {
        fprintf(stderr, "Error: frame1.data buffer overflow ao adicionar FLAG final\n");
        return -1;  // Retorno de erro adequado
    }

    printf("Frame completo após stuffing (incluindo FLAG final):\n");
    for (int i = 0; i < frame1.size; i++) {
        printf("0x%x ", frame1.data[i]);
    }
    printf("\n");

    int bytes = writeBytesSerialPort(frame1.data, frame1.size);
    printf("%d bytes escritos na porta serial\nTamanho final do frame: %u\n", bytes, frame1.size);

    return 1;
}



int read_frame_resp(int information_frame, unsigned *alarmEnabled)
{
    // response
    unsigned char res = 0;

    // Current state of the machine state
    enum State state = START;
    unsigned char bf = 0;

    // Adicionando printf para acompanhar o estado e valores importantes
    printf("Iniciando read_frame_resp\n");

    while (state != STOP && alarmEnabled)
    {
        // Returns after 30 seconds without input
        readByteSerialPort(bf);

        // state machine
        switch (state)
        {
        case START:
            //printf("Estado START\n");

            // first state
            if (bf == F)
                state = FLAG;

            break;

        case FLAG:
            printf("Estado FLAG\n");

            // state after receiving a flag
            if (bf == TRANSMITTER_ADDRESS)
                state = A;
            else if (bf == F)
                // if a flag is received we should stay in this state
                break;
            else
                // if a invalid byte is read we should go back to the start
                state = START;
            break;

        case A:
            printf("Estado A\n");

            // State after receiving the Address
            if (bf == RR1 || bf == RR0 || bf == REJ0 || bf == REJ1)
            {
                // A valid response was read, we store it and deal with it later
                res = bf;
                state = C;
            }
            else if (bf == F)
            {
                state = FLAG;
            }
            else
            {
                // invalid data we go back to the beginning
                state = START;
            }
            break;

        case C:
            printf("Estado C\n");

            // State after receiving a control (response)
            if (bf == (TRANSMITTER_ADDRESS ^ res))
                state = BCC1;
            else if (bf == F)
                state = FLAG;
            else
                state = START;
            break;

        case BCC1:
            printf("Estado BCC1\n");

            // state after verifying if the BCC1 is OK
            if (bf == F)
            {
                printf("Pacote recebido com FLAG final. Desativando alarme.\n");
                // disable alarm
                alarm(0);

                // A flag signals the end of the package sent

                // the response was a reject, we signal an error to the link layer
                if (res == REJ1 || res == REJ0)
                {
                    printf("Resposta de rejeição recebida. Retornando erro.\n");
                    return -1;
                }
                // the receiver is waiting for the current information frame, an error has occurred
                if ((res == RR0 && information_frame == I0) || (res == RR1 && information_frame == I1))
                {
                    printf("Erro: o quadro de informação atual ainda está pendente. Retornando erro.\n");
                    return -1;
                }

                // Else we send a positive response
                printf("Resposta positiva recebida. Retornando sucesso.\n");
                return 0;
            }
            else
            {
                state = START;
            }
            break;

        default:
            printf("Estado desconhecido.\n");
            break;
        }
    }
    printf("Saindo de read_frame_resp, retornando 1.\n");
    return 1;
}


Packet write_control(unsigned char control, const char *filename, long filesize)
{
    Packet pckt;
    long i = 0;
    // Definindo o campo de controle
    pckt.controlField = control;
    pckt.data[i++] = control;

    // Calculando o número de bytes necessários para representar o tamanho do arquivo
    unsigned char size = 0;
    long aux = filesize;
    while (aux > 0) {
        aux /= 256;
        size++;
    }


    // Configurando TLV para o tamanho do arquivo
    pckt.T = T_SIZE;
    pckt.V = size;

    pckt.data[i++] = T_SIZE;
    pckt.data[i++] = size;

    printf("Tamanho do arquivo (filesize): %ld\n", filesize);
    printf("Tamanho (V) do campo filesize: %d bytes\n", size);

    // Inserindo cada byte do tamanho do arquivo
    while (size > 0) {
        pckt.data[i++] = (unsigned char)filesize % 256;
        printf("Byte de filesize inserido: 0x%x\n", pckt.data[i++]);
        filesize /= 256;
        size--;
    }

    // Inserindo o nome do arquivo, se existir
    if (*filename != '\0') {
        printf("Nome do arquivo: %s\n", filename);

        // Tamanho do nome do arquivo
        int filenameSize = strlen(filename);
        
        // Configurando TLV para o nome do arquivo
        pckt.data[i++] = T_NAME;
        pckt.data[i++] = filenameSize;


        // Inserindo cada caractere do nome do arquivo
        for (int i = 0; i < filenameSize; i++) {
            pckt.data[i++] = filename[i];
            printf("Byte de filename inserido: 0x%x (%c)\n", pckt.data[i++], filename[i]);
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

unsigned int write_RR(unsigned char information_frame)
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

        int bytes = writeBytesSerialPort(frame.data, 5);

        return 1;
}
unsigned int write_REJ(int information_frame)
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

        int bytes = writeBytesSerialPort(frame.data, 5);

        return 1;

}
void printFrame(Frame frame) {
    printf("Frame:\n");
    printf("  Data: ");
    
    for (int i = 0; i < 5; i++) {
        printf("0x%02X ", frame.data[i]);
    }
    
    printf("\n");
}
unsigned int write_SUD(unsigned char control)
{
    Frame frame1;
    frame1.data[0] = F;
    frame1.data[1] = TRANSMITTER_ADDRESS;
    frame1.data[2] = control;
    frame1.data[3] = (frame1.data[1] ^ frame1.data[2]);
    frame1.data[4] = F;

    unsigned bytes = writeBytesSerialPort(frame1.data, 5);

    //printf("Quantidade de Bytes: %i\n", bytes);

    return bytes;

}

