#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char** argv) {
	printf("Opening pipe!\n");
	int pipe = open("/home/dorian/go/src/github.com/doriandekoning/run-qemu-cache-distributed-sim/pipe", O_RDONLY);
	if(pipe < 0) {
		printf("Failed to open pipe:%s\n", strerror(errno));
		return 1;
	}
	printf("Opened pipe!\n");
	char* buf[10];
	while(1){
		buf[0] = "\0";
		int bytes_read = read(pipe, buf, 8);
		printf("Read: %s\n", buf);
	}
	return 0;
}
