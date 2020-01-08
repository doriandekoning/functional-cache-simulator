#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
   #include <unistd.h>

int main(int argc, char** argv) {

	uint64_t bytes_read = 0;
	if(argc < 2 ){
		printf("At least one argument should be provided!\n");
		return 1;
	}
	uint8_t buf[4000];
	FILE* pipe = fopen(argv[1], "r");
	//int pipe = open(argv[1], O_RDONLY);
	int size = 1;
	while(1) {
		if(fread(buf, 1, size, pipe) != size) {
			printf("Not read!\n");
		}else{
			bytes_read+=size;
			if(bytes_read % 10000000 == 0) {
				printf("Read %lu bytes\n", bytes_read);
			}
		}
//		size = size > 8 ? 1 : size*2;
	}

	return 0;
}
