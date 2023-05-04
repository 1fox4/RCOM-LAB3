/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

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

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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

    printf("New termios structure set\n");
    //unsigned char set[5] cd posiçao uma parte do set, aux_FI[], aux_A[], aux_C[], aux_B[], aux_FF[], set[];
    //char test_BCC[64];
    char aux[64];
    int i=0;
    while (STOP==FALSE) {       /* loop for input */
        res = read(fd,buf,1);   /* returns after 5 chars have been input */
        buf[res]=0;               /* so we can printf... */
        aux[i]=buf[0];
        i++;
        if (buf[0]=='\0'){
            printf("%s\n", aux);
            STOP=TRUE;
        }
    }
    unsigned char aux_FI[8], test_BCC[8], aux_A[8], aux_C[8], aux_B[8], aux_FF[8];
    for(i=0; i < strlen(aux); i++){
        if(i < 2)
            aux_FI[i] = aux[i];
        if(i < 4  && i >= 2)
            aux_A[i-2] = aux[i];
        if(i < 6  && i >= 4)
            aux_C[i-4] = aux[i];
        if(i < 8  && i >= 6)
            aux_B[i-6] = aux[i];
        if(i < 10  && i >= 8)
            aux_FF[i-8] = aux[i];
    }
        printf("aux_FI %s\n", aux_FI);
        printf("aux_A %s\n", aux_A);
        printf("aux_C %s\n", aux_C);
        printf("aux_B %s\n", aux_B);
        printf("aux_FF %s\n", aux_FF);
   
    typedef enum
    {
        START,
        FLAG_RCV,
        A_RCV,
        C_RCV,
        BCC_OK,
        STOP
    }estados;
    
    estados currentstate = START;
    int j=1;
    while(j==1){
    switch(currentstate){
        case START:
                printf("1111\n");
                
                //if(strcmp(aux_FI, "5C"))
                if(aux_FI == "5C")
                    currentstate = FLAG_RCV;
                else 
                    currentstate = START;
            break; 
            
        case FLAG_RCV:
                printf("t2222\n");
                if(aux_A == "01" || aux_A == "03")
                    currentstate = A_RCV;
                if(aux_FI == "5C")
                    currentstate = FLAG_RCV;
                else
                    currentstate = START;
            break; 
            
        case A_RCV:
                printf("tfyedwfuewf\n");
                if(aux_C == "03" || aux_C == "0B" || aux_C == "07")
                    currentstate = C_RCV;
                if(aux_FI == "05")
                    currentstate = FLAG_RCV;
                else
                    currentstate = START;
            break; 
            
        case C_RCV:
                strcpy(test_BCC, (char)aux_A ^ (char)aux_C);
                printf("test %s\n", test_BCC);
                if(test_BCC == aux_B)
                    currentstate = BCC_OK;
                if(aux_FI == "05")
                    currentstate = FLAG_RCV;
                else
                    currentstate = START;
            break; 
            
        case BCC_OK:
                if(aux_FF == aux_FI)
                    currentstate = STOP;
                else
                    currentstate = START;
            break; 
            
        case STOP:
            j=0;
            strcpy(aux, "5C0107025C\0");
            currentstate = START;
            break;    
            
    }
    }
    res = write(fd,aux,strlen(aux)+1);
    printf("%d bytes written\n", res);

    /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião
    */

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
