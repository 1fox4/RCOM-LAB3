/*Non-Canonical Input Processing*/

#include "stdlib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "string.h"
#include "signal.h"
#include "math.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;
int fd, c, res,s = 0;
struct termios oldtio, newtio;
unsigned char buf[255], buffer[255], aux[255];
int sum = 0, speed = 0;
unsigned char recv[255], recv_init[5];
unsigned char SET[5] = {0x5c, 0x01, 0x03, 0x02, 0x5c};
unsigned char dados[10]={0x03,0x0a,0x5d,0x60,0xab,0x5c,0x05,0x50,0x20,0x4a};
unsigned char UA[5] = {0x5c, 0x01, 0x07, 0x00, 0x5c};
unsigned char DISC[5] = {0x5c, 0x01, 0x0b, 0x0a, 0x5c};

    
void leitor(int fd, unsigned char *helper)
{
       STOP = FALSE;
    int i = 0;
    while (STOP == FALSE){      /* loop for input */
        res = read(fd, buf, 1); /* read byte a byte */
        recv[i] = buf[0];
        i++;
        if (recv[i - 1] == '\0'){
            STOP = TRUE;
        }
    }
    helper = &recv;
    int x=0;
    while(x < (i - 1)){
        printf(":0x%.2x:%d\n", helper[x], i);
        x++;
    }
}

void escritor(int fd,unsigned char *sender,int size){
    int i=0;
    while(i < size){
        printf(" 0x%.2x \n", sender[i]);
        i++;
    }
    sender[size] = 0x00;
    res = write(fd, sender, size + 1);
    printf("%d bytes written\n", res);

}

void escritor_DISC(){
    int i=0;
    DISC[3] = DISC[1] ^ DISC[2];
    while (i < 5){
        printf("0x%.2x \n",DISC[i]);
        i++;
    }
    res = write(fd, DISC, 7);
    printf("%d bytes written\n", res);
    alarm(3);
}
void escritor_UA(){
    UA[3] = UA[1] ^ UA[2];
    res = write(fd, UA, 6);
    printf("%d bytes written\n", res);
    alarm(3);
}

void enviar_inicio(){
    SET[3] = SET[1] ^ SET[2];
    res = write(fd, SET, 6);
    printf("%d bytes written\n", res);
    alarm(3);
}


int llopen(int argc, char **argv){

    // comentado para poder o usar o cabo virtual
    /* if ( (argc < 2) ||
          ((strcmp("/dev/ttyS0", argv[1])!=0) &&
           (strcmp("/dev/ttyS1", argv[1])!=0) )) {
         printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
         exit(1);
     }*/

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */

    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(argv[1]);
        exit(-1);
    }

    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 1;  /* blocking read until 5 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */

    tcflush(fd, TCIOFLUSH);
    signal(SIGALRM, enviar_inicio);
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    enviar_inicio();

    leitor(fd, recv_init);
    typedef enum
    {
        start,
        flag_rcv,
        a_rcv,
        c_rcv,
        bcc_ok,
        stop,
        error,
    } recetor_init;

    recetor_init currentstate = start;

    int pos = 0;
    int flag_1 = 0;
    while ((flag_1 == 0) && (pos <= strlen(recv)) && (currentstate!=error)){
        switch (currentstate){
            case start:
                if (recv[pos] == 0x5c){
                    pos++;
                    currentstate = flag_rcv;
                }else{               
                    break;
                }
            break;
            case flag_rcv:
                if (recv[pos] == 0x5c){
                    break;
                } else if (recv[pos] == 0x01){
                    pos++;
                    currentstate = a_rcv;
                }else{
                    currentstate = error;
                }
            break;
            case a_rcv:
                if (recv[pos] == 0x5c){
                    currentstate = flag_rcv;
                }
                else if (recv[pos] == 0x07){
                    pos++;
                    currentstate = c_rcv;
                }
                else{
                    currentstate = error;
                }
            break;
            case c_rcv:
                if (recv[pos] == 0x5c){
                    currentstate = flag_rcv;
                }
                else if (recv[pos] == (recv[pos - 2] ^ recv[pos - 1])){
                    pos++;
                    currentstate = bcc_ok;
                }else{
                    currentstate = error;
                }
            break;
            case bcc_ok:
                if (recv[pos] == 0x5c){
                    currentstate = stop;
                }else{
                    currentstate = error;
                }
            break;
            case stop:{
                printf("OK------------\n");
                alarm(0);
                flag_1 = 1;
            }
        }
    }
    sleep(1);
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }
    return 0;
}

int llwrite(unsigned char *data,int bufSize){
    int conta5c = 0,conta5d = 0, i = 0;
    while (i < bufSize){
        printf("***  0x%.2x  ***\n",data[i]);
        if (data[i] == 0x5c){
            conta5c ++;
        }
        if (data[i] == 0x5d){
            conta5d ++;
        }
        i++;
    }
    i = 0;
    unsigned char envio[bufSize + conta5c + conta5d];
    int j=0;
    while( i < (bufSize + conta5c + conta5d)){
        if (data[i] == 0x5c){
            envio[i] = 0x5d;
            printf("---  0x%.2x  ---\n",envio[i]);
            i++;
            envio[i] = 0x7c; 
        }
        else if (data[i] == 0x5d){
            printf("---  0x%.2x  ---\n",envio[i]);
            envio[i]=data[i];
            i++;
            envio[i] = 0x7d; 
        }else{
            envio[i]=data[j];
        }
        printf("---  0x%.2x  ---\n",envio[i]);
        j++;
        i++;
    }
    i = 0;
    unsigned char trama[7 + sizeof(envio)];
    trama[0] = 0x5c;
    trama[1] = 0x01;
    if (s == 0){
        trama[2] = 0x00;
    }
    if (s == 1){
        trama[2] = 0x02;
    }
    trama[3] = trama[1]^trama[2];
    i = 4;
    while (i < sizeof(envio) + 4){
    trama[i] = envio[i-4];
    printf("^^^  0x%.2x ^^^ 0x%.2x  ^^^\n",trama[i],envio[i-4]);
    i++;
    }
    unsigned char bcc2;
    bcc2=envio[0]^envio[1];
    int x = 2;
    while(x < sizeof(envio)){
        bcc2=bcc2^envio[x];
        x++;
    }
    trama[i] = bcc2;
    i++;
    trama[i]=0x5c;
    x = 0;
    while(x < sizeof(trama)){
        printf("0x%.2x \n",trama[x]);
        x++;
    }
    escritor(fd,trama,s);
}

int llclose(int argc, char **argv){
    DISC[3] = DISC[1] ^ DISC[2];
    escritor(fd,DISC,sizeof(DISC));
    int i = 0;
    leitor(fd, recv_init);
    typedef enum{
        start,
        flag_rcv,
        a_rcv,
        c_rcv,
        bcc_ok,
        stop,
        error,
    } recetor_init;
    recetor_init currentstate = start;
    int pos = 0;
    int flag_1 = 0;
    while ((flag_1 == 0) && (pos <= strlen(recv)) && (currentstate!=error)){
        switch (currentstate){
            case start:
                if (recv[pos] == 0x5c){
                    pos++;
                    currentstate = flag_rcv;
                }else{
                    break;
                }
            break;
            case flag_rcv:
                if (recv[pos] == 0x5c)
                {
                    break;
                }
                else if (recv[pos] == 0x01){
                    pos++;
                    currentstate = a_rcv;
                }else{
                    currentstate = error;
                }
            break;
            case a_rcv:
                if (recv[pos] == 0x5c){
                    currentstate = flag_rcv;
                }
                else if (recv[pos] == 0x0B){
                    pos++;
                    currentstate = c_rcv;
                }
                else{
                    currentstate = error;
                }
            break;
            case c_rcv:
                if (recv[pos] == 0x5c){
                    currentstate = flag_rcv;
                }
                else if (recv[pos] == (recv[pos - 2] ^ recv[pos - 1])){
                    pos++;
                    currentstate = bcc_ok;
                }else{
                    currentstate = error;
                }
            break;
            case bcc_ok:
                if (recv[pos] == 0x5c){
                    currentstate = stop;
                }else{
                    currentstate = error;
                }
            break;
            case stop:{
                printf("OK------------\n");
                alarm(0);
                flag_1 = 1;
            }
        }
    }
    escritor_UA();
    sleep(1);
    close(fd);
    return 0;
}

int main(int argc, char **argv){
    llopen(argc,&*argv);
    llwrite(dados,sizeof(dados)+1);
    llclose(argc,&*argv);
}


