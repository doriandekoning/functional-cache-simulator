
#ifndef FILEREADER_H
#define FILEREADER_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "cache/state.h"
#include "traceevents.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

uint64_t file_get_gem_memory_access(FILE* file, cache_access* access);

#endif //FILEREADER_H
