#include "mpi.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define MSG_HELLO 100
#define MSG_SIZE 2

#define SHIPS 20
#define MAX_TRANSPORT_TIME 20
#define MAX_SNOW_PER_SHIP 10
#define MAX_UNLOAD_TIME 10
#define SIZE_OF_UNIT_TIME 500

#define MSG_TAG_QUEUE 1
#define MSG_TAG_SHIPS 2
#define MSG_TAG_PRINTF 3
