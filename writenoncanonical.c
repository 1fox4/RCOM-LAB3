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
    unsigned int buf;
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

    unsigned int flag = 0x5C;
    printf("flag: %x\n", flag);
    unsigned int address = 0x01;
    printf("address: %x\n", address);
    unsigned int control = 0x03;
    printf("control: %x\n", control);
    unsigned int bcc = address ^ control;
    printf("bcc: %x\n", bcc);
    unsigned int set;
    set = (((((((flag << 8) | address) << 8) | control) << 8) | bcc) << 8) | flag;
    printf("Enviado: *%x*\n", set);
    

    /*testing*/

    buf = set;
    res = write(fd,buf,sizeof(buf));
    printf("***%d bytes written***\n", res);

    printf("\n---New different termios structure set---\n\n");
    char aux[255];
    i=0;
/*
    while (STOP==FALSE) {       /* loop for input /
        int res1 = read(fd,buf,1);   /* returns after 5 chars have been input /
        buf[res1]=0;               /* so we can printf... /
        aux[i]=buf[0];
        i++;
        if (buf[0]=='\0'){
            printf("    Recebido: %s :: %d\n\n", aux, strlen(aux));
            STOP=TRUE;
        }
    }  */

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
