#include "mpi.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include "own_queue.h"

#define MSG_HELLO 100
#define MSG_SIZE 2

#define MAX_NUMBER_OF_SHIPS 20
#define MAX_TRANSPORT_TIME 200
#define MAX_SNOW_PER_SHIP 100
#define MAX_UNLOAD_TIME 100
#define SIZE_OF_UNIT_TIME 10

#define MSG_TAG_QUEUE 1
#define MSG_TAG_SHIPS 2
#define MSG_TAG_PRINTF 3
#define MSG_TAG_REQUEST 4
#define MSG_TAG_CONFIRMATION 5
#define MSG_TAG_RELEASE 6
