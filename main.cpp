#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define SERIAL_PORT "/dev/ttyACM0"
#define baudRate (B115200)

FILE *fp;
int fd;
char filename[32];

int kbhit(void)
{
    struct termios oldt, newt;
    memset(&oldt, 0, sizeof(oldt));
    memset(&newt, 0, sizeof(newt));
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    //oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
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

void abrt_handler(int sig);

char get_char(){
    char c = 0;
    read(fd, (char *)&c, 1);
    return c;
}

char* read_line(char* buffer){
    int i = 0;
    while(1){
        char c = get_char();
        if(c > 0){
            if(c == '\n'){
                buffer[i] = c;
                buffer[i+1] = '\0';
                i = 0;
                break;
            }else{
                buffer[i] = c;
                i++;
            }
        }
    }
}

int main(int argc, char *argv[]){
    char msg_start[] = "s";
    char msg_end[] = "e";
    char rx_buf[128];
    char w_buf[32];
    struct termios tio;

    memset(&tio, 0, sizeof(tio));
    
    int i;
    int len;
    int ret;
    int size;

    enum phase{first, second, third};
    enum phase phase = first;

    if ( signal(SIGINT, abrt_handler) == SIG_ERR ) {
        exit(1);
    }

    fd = open(SERIAL_PORT, O_RDWR);
    if(fd <= 0){
        printf("SERIAL OPEN ERROR\n");
        return -1;
    }
    printf("Serial opened.\n");
    
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
    printf("\nPlease push any key to start recording.(ESC = Exit)\n");
    while(1){
        switch(phase){
            case first:
                if(kbhit()){
                    char c = getchar();
                    
                    /* ESC keyが押されたら */
                    if(c == 27){
                        close(fd);
                        exit(0);
                    }

                    struct tm* tm;
                    time_t t = time(NULL);
                    tm = localtime(&t);
                    snprintf(filename, 32, "shot-%04d-%02d-%02d-%02d%02d%02d.csv", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
                    fp = fopen(filename,"w");
                    if(fp == NULL){
                        printf("FILE OPEN ERROR\n");
                        return -1;
                    }
                    printf("Now recording...\n");
                    write(fd, msg_start, sizeof(msg_start));
                    phase = second;
                }
                break;
            case second:
                int i = 0;
                read_line(w_buf);
                fprintf(fp, "%s", w_buf);
                if(kbhit()){
                    getchar();
                    write(fd, msg_end, sizeof(msg_end));
                    printf("Recorded at %s\n", filename);
                    fclose(fp);
                    fp = nullptr;
                    printf("Wait... 2 seconds\n");
                    sleep(2);
                    printf("Please push any key to start recording.(ESC = Exit)\n");
                    phase = first;
                }
                break;
        }
    }
    return 0;
}

void abrt_handler(int sig) {
    
    fprintf(stderr, "Abort in recording. close file.\n");
    if(fd > 0){
        close(fd);
    }
    if(fp != nullptr){
        fclose(fp);
    }
    
    exit(1);
}