/*Non-Canonical Input Processing*/
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE;
int res, fd;
unsigned char buf[255], buffer[255];
unsigned char receive[255], receive_init[5];
unsigned char UA[5] = {0x5c, 0x01, 0x07, 0x06, 0x5c};

void llread(int fd, unsigned char *aux) {
    STOP = FALSE;
    int i = 0;
    while (STOP == FALSE) {
        /* loop for input */
        res = read(fd, buf, 1); /* read bite a bite */
        receive[i] = buf[0];
        i++;
        if (receive[i - 1] == '\0') {
            STOP = TRUE;
        }
    }
    aux = &receive;
    for (int x = 0; x < (i - 1); x++) {
        printf("0x%.2x\n", aux[x]);
    }
}

void llwrite(int fd, unsigned char *envio) {
    res = write(fd, envio, strlen(envio) + 1);
    printf("%d bytes written\n", res);
}


int llopen(int argc, char **argv) {
    UA[3] = UA[1] ^ UA[2];
    int c;
    struct termios oldtio, newtio;

    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror(argv[1]);
        exit(-1);
    }

    if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 1;  /* blocking read until 5 chars received */


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    typedef enum {
        start,
        flag_rcv,
        a_rcv,
        c_rcv,
        bcc_ok,
        stop,
    } recetor_init;

    recetor_init currentstate = start;

    int pos = 0;
    int flag_1 = 0;
    while ((flag_1 == 0) && (pos <= strlen(receive))) {
        switch (currentstate) {
            case start:
                llread(fd, receive_init);
                if (receive[pos] == 0x5c) {
                    pos++;
                    currentstate = flag_rcv;
                } else {
                    break;
                }
                break;

            case flag_rcv:
                if (receive[pos] == 0x5c) {
                    break;
                } else if (receive[pos] == 0x01) {
                    pos++;
                    currentstate = a_rcv;
                } else {
                    currentstate = start;
                }

                break;

            case a_rcv:
                if (receive[pos] == 0x5c) {
                    currentstate = flag_rcv;
                } else if (receive[pos] == 0x03) {
                    pos++;
                    currentstate = c_rcv;
                } else {
                    currentstate = start;
                }
                break;

            case c_rcv:

                if (receive[pos] == 0x5c) {
                    currentstate = flag_rcv;
                } else if (receive[pos] == (receive[pos - 2] ^ receive[pos - 1])) {
                    pos++;
                    currentstate = bcc_ok;
                } else {
                    currentstate = start;
                }
                break;

            case bcc_ok:
                if (receive[pos] == 0x5c) {
                    currentstate = stop;
                } else {
                    currentstate = start;
                }
                break;
            case stop: {
                llwrite(fd, UA);
                flag_1 = 1;
            }
        }
    }

    llread(fd, receive);
    return 0;
}


int llclose(int argc, char **argv) {


    llread(fd, receive_init);
    typedef enum {
        start,
        flag_rcv,
        a_rcv,
        c_rcv,
        bcc_ok,
        stop,
        error, // perguntar ao stor ja que o meu nao repete o ua, o que deve fazer se estiver errado
    } recetor_init;

    recetor_init currentstate = start;

    int pos = 0;
    int flag_1 = 0;
    while ((flag_1 == 0) && (pos <= strlen(receive)) && (currentstate != error)) {
        switch (currentstate) {
            case start:
                if (receive[pos] == 0x5c) {
                    pos++;
                    currentstate = flag_rcv;
                } else {
                    break;
                }
                break;

            case flag_rcv:
                if (receive[pos] == 0x5c) {
                    break;
                } else if (receive[pos] == 0x01) {
                    pos++;
                    currentstate = a_rcv;
                } else {
                    currentstate = error;
                }

                break;

            case a_rcv:
                if (receive[pos] == 0x5c) {
                    currentstate = flag_rcv;
                } else if (receive[pos] == 0x0B) {
                    pos++;
                    currentstate = c_rcv;
                } else {
                    currentstate = error;
                }
                break;

            case c_rcv:

                if (receive[pos] == 0x5c) {
                    currentstate = flag_rcv;
                } else if (receive[pos] == (receive[pos - 2] ^ receive[pos - 1])) {
                    pos++;
                    currentstate = bcc_ok;
                } else {
                    currentstate = error;
                }
                break;

            case bcc_ok:
                if (receive[pos] == 0x5c) {
                    currentstate = stop;
                } else {
                    currentstate = error;
                }
                break;
            case stop: {
                printf("OK------------\n");
                flag_1 = 1;
            }
        }
    }

    if (flag_1 == 0) {
        printf("UA errado\n");
    }

    unsigned char DISC[5] = {0x5c, 0x01, 0x0B, 0x00, 0x5c};
    DISC[3] = DISC[1] ^ DISC[2];
    llwrite(fd, DISC);
    printf("receive:  %x\n", receive);
    sleep(1);
    close(fd);
    return 0;
}


int main(int argc, char **argv) {

    llopen(argc, &*argv);
    llclose(argc, &*argv);
    return 0;
}

