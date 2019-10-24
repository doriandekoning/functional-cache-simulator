#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char** argv) {
        printf("Opening pipe!\n");
        int pipe = open("pipe", O_WRONLY);
        if(pipe < 0) {
                printf("Failed to open pipe:%s\n", strerror(errno));
                return 1;
        }
        printf("Opened pipe!\n");
        char* str = "ietsiets";
        int bytes_written = write(pipe, str, 4);
        printf("Written: %d\n", bytes_written);
	sleep(10);
        bytes_written = write(pipe, str, 8);
        printf("Written: %d\n", bytes_written);
        return 0;
}
