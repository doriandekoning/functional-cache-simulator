#ifndef __INPUT_READER_H
#define __INPUT_READER_H

#include <stdbool.h>


#define BATCH_SIZE 2048

int run_input_reader(char* input_path, char* cr3_path, bool input_is_pipe);

#endif /* __INPUT_READER_H */
