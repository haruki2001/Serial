#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define FileName "data.csv"
#define SERIAL_PORT "/dev/"
#define baudRate 100

int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}



int main(int argc, char *argv[]){
    unsigned char msg_start[] = "start\n";
    unsigned char msg_end[] = "end\n";
    unsigned char buf[255];
    int fd;
    struct termios tio;

    int i;
    int len;
    int ret;
    int size;
    FILE *fp;

    enum phase{first, second, third};
    enum phase phase = first;

    fd = open(SERIAL_PORT, O_RDWR);
    if(fd < 0){
        printf("SERIAL OPEN ERROR\n");
        return -1;
    }
    
    fp = fopen(FileName,"w");
    if(fp == NULL){
        printf("FILE OPEN ERROR\n");
        return -1;
    }
    
/********** Setup **********/
    tio.c_cflag += CREAD;           //受信有効
    tio.c_cflag += CLOCAL;          //ローカルライン
    tio.c_cflag += CS8;             //データビット 8bit
    tio.c_cflag += 0;               //ストップビット 1bit
    tio.c_cflag += 0;               //パリティ None
    cfsetispeed(&tio, baudRate);    //boutRate設定
    cfsetospeed(&tio, baudRate);    //boutRate設定
    tcsetattr(fd, TCSANOW, &tio);   //デバイス設定

    int flag = 0;
    int count = 0;
    char key;
    while(1){
        switch(phase){
            case first:
                printf("Enterで開始\n");
                scanf("%c",&key);
                printf("状態　：　開始\r");
                if(key == '\n'){
                    write(fd, msg_start, sizeof(msg_start));
                    phase = second;
                    flag = 0;
                }
                break;
            
            case second:
                printf("状態　：　読み取り中\r");
                read(fd, buf, sizeof(buf));
                fprintf(fp,"%s\n",buf);
                flag = kbhit();
                if(flag == 1){
                    phase = third;
                }
                break;

            case third:
                write(fd, msg_end, sizeof(msg_end));
                printf("状態 ：　終了\r");
                fclose(fp);
                _exit(0);
                break;
        }
    }

    return 0;
}