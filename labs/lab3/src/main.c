#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define RING_SIZE 10


static pthread_mutex_t lockSource;
static pthread_mutex_t lockDest;


static sem_t semInsert;
static sem_t semAdder;
static sem_t semReader;
static sem_t semEnd;

static pthread_t t1;
static pthread_t t2;
static pthread_t t3;

typedef struct circularBuffer {
    int head;
    int tail;
    int array[RING_SIZE];
} CIRCULAR_BUFFER;

void enqueue(int el, CIRCULAR_BUFFER* buffer) {
    buffer->array[buffer->head++ % RING_SIZE] = el;
}

int isEmpty(CIRCULAR_BUFFER* buffer) {
    return buffer->tail == buffer->head;
}


void* insertSource(void* source) {
    int element;
    int err = 1;
    CIRCULAR_BUFFER* sourceBuffer = (CIRCULAR_BUFFER*) source;

    while(1) {

    sem_wait(&semInsert);

    do {
            printf("Unesite broj izmedju 0 i 9 ili bilo koji karakter za odustajanje\n"); 
            char input[20];

            scanf("%19s", input);

            if (strcmp(input, "q") == 0) {
            sem_post(&semEnd);
            sem_post(&semEnd);
            sem_post(&semAdder);
            sem_post(&semReader);
            element = -1;
            err = 0;
            break;

            } else {
                element = atoi(input);
                if (element >= 0 && element <= 9) {
                    printf("\n");
                    err = 0;
                } else {
                    printf("Pogresan unos\n");
                    err = 1;
                }
            }
    } while(err == 1);

    pthread_mutex_lock(&lockSource);
    enqueue(element, sourceBuffer);
    pthread_mutex_unlock(&lockSource);

    if(sem_trywait(&semEnd) == 0) {
        pthread_join(t2, NULL);
        pthread_join(t3, NULL);
        break;
    }

    if(sourceBuffer->head - sourceBuffer->tail >= 2) {
        sem_post(&semAdder);
    }

    }

    return NULL;
    
}

//addNums rade tako sto uzmu dva susjedna broja i saberu, a oslobodi se jedno mjesto. Recimo da imamo 1 2 i 3 u baferu
//sabrace 1 i 2 zatim 2 i 3. Da je trebalo da izbaci oba sabirka iz bafera a ne samo prvi, onda bi se stavila jos jedna sem_post(&semInesert) komanda
//i tail bi se incrementovao za 2 mjesta a ne za jedno samo.

void* addNums(void* buffers) {
    CIRCULAR_BUFFER* circularBuffers = (CIRCULAR_BUFFER*) buffers;
    CIRCULAR_BUFFER* sourceBuffer = &circularBuffers[0];
    CIRCULAR_BUFFER* destBuffer = &circularBuffers[1];

    while(1) {
        sem_wait(&semAdder);

                pthread_mutex_lock(&lockSource);
                int a = sourceBuffer->array[sourceBuffer->tail % RING_SIZE]; 
                int b = sourceBuffer->array[(++sourceBuffer->tail) % RING_SIZE];
                int result;
                if(a == -1 || b == -1) {
                    sem_post(&semReader);
                    break;
                } else {
                    result = a+b;
                }

                sem_post(&semInsert);
                pthread_mutex_unlock(&lockSource); 

                pthread_mutex_lock(&lockDest);
                enqueue(result, destBuffer);
                pthread_mutex_unlock(&lockDest);

                sem_post(&semReader);
                
            }

    return NULL;
    }


void* print(void* outputBuffer) {
    CIRCULAR_BUFFER* destBuffer = (CIRCULAR_BUFFER*) outputBuffer;

    while(1) {
        sem_wait(&semReader);

            if(sem_trywait(&semEnd) == 0) {
                break;
            }

            pthread_mutex_lock(&lockDest);
            int result = destBuffer->array[(destBuffer->tail++) % RING_SIZE];
            printf("suma: %d \n", result);
            pthread_mutex_unlock(&lockDest);


        }
    }




int main(int argc, char *argv[])
{
    sem_init(&semEnd, 0, 0);
    sem_init(&semInsert, 0, RING_SIZE);
    sem_init(&semAdder, 0, 0);
    sem_init(&semReader, 0, 0);

    pthread_mutex_init(&lockSource, NULL);
    pthread_mutex_init(&lockDest, NULL);

    CIRCULAR_BUFFER buffers[2] = {0};

    pthread_create(&t1, NULL, insertSource, (void*)(&buffers[0]));
    pthread_create(&t2, NULL, addNums, (void*)buffers);
    pthread_create(&t3, NULL, print, (void*)(&buffers[1]));

    pthread_join(t1, NULL);

    printf("Kraj \n");
}
