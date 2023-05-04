/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//gcc -o wnc writenoncanonical.c

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("\n---New termios structure set---\n\n");

    /*unsigned char flag[] = "0x5C";
    printf("flag: %s\n", flag);
    unsigned char address[] = "0x01";
    printf("address: %s\n", address);
    unsigned char control[] = "0x03";
    printf("control: %s\n", control);
    unsigned char bcc[8] = ;
    strcpy(bcc, (char)address ^ (char)control);
    printf("bcc: %s\n", bcc);
    unsigned char set[100];
    //sprintf(set, ("0x%s%s%s%s%s", flag, address, control, bcc, flag));
/*
    for (i = 0; i < strlen(set) ; i++) {
        strcpy(buf[i], set[i]);
    }*/
    strcpy(buf, "0x5C0x010x030x020x5C\0");
    printf("    Enviado: %s :: %d\n",buf, strlen(buf));



    /*testing*/

    res = write(fd,buf,255);
    printf("***%d bytes written***\n", res);

    printf("\n---New different termios structure set---\n\n");
    char aux[255];
    i=0;

    while (STOP==FALSE) {       /* loop for input */
        int res1 = read(fd,buf,1);   /* returns after 5 chars have been input */
        //printf("***%d bytes lidos***\n", res1);
        buf[res1]=0;               /* so we can printf... */
        aux[i]=buf[0];
        i++;
        if (buf[0]=='\0'){
            printf("    Recebido: %s :: %d\n\n", aux, strlen(aux));
            STOP=TRUE;
        }
    }  

    /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
    */


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    return 0;
}
