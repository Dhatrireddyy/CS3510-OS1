#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_N 100000

typedef struct {
    int result[MAX_N];
    int count;
    int check[MAX_N];
} SharedData;

void tetrahedral(int n, int *result, int *count) {
    *count = 0;
    for (int i = 1; i <= n; i++) {
        int num = (i*(i+1)*(i+2))/6;
        if (num <= n) {
            result[(*count)++] = num;
        }
        else{
            break;
        }
    }
}

void writeOutputToFile(SharedData *sharedData, int K) {
    FILE *output = fopen("OutMain.txt", "w");
    for (int i = 1; i <= K; i++) {
        fprintf(output, "P%d:", i);
        for (int j = 0; j < sharedData->count; j++) {
            if (sharedData->check[sharedData->result[j]] == i) {
                fprintf(output, " %d", sharedData->result[j]);
            }
        }
        fprintf(output, "\n");
    }
    fclose(output);
}

void childP(int start, int end, SharedData *sharedData, int process_N, int N) {

    int pResult[MAX_N];
    int pCount = 0;

    // Create a log file for each child process
    char log_filename[20];
    sprintf(log_filename, "OutFile%d.txt", process_N);
    FILE *logFile = fopen(log_filename, "w");

    fprintf(logFile, "Process %d: From %d to %d:\n", process_N, start, end);

    for (int i = start; i <= end; i++) {

        int isTetrahedral = 0;
        tetrahedral(i, pResult, &pCount);
        if (pResult[pCount - 1] == i && pCount > 0) {
            isTetrahedral = 1;
        }
        fprintf(logFile, "%d: ", i);

        if (!isTetrahedral) {
            fprintf(logFile, "Not a tetrahedral number\n");
        } else {
            fprintf(logFile, "a tetrahedral number\n");
            sharedData->result[sharedData->count++] = i;
            sharedData->check[i] = process_N;
        }       
    }
    fclose(logFile);
}

int main() {

    int N, K;
    
    FILE *input = fopen("input.txt", "r");
    fscanf(input, "%d %d", &N, &K);
    fclose(input);

    clock_t startTime = clock();

    // Create shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT|0666);
    SharedData *sharedData = (SharedData *)shmat(shmid, NULL, 0);

    for (int i = 0; i <= N; i++) {
        sharedData->check[i] = 0;
    }

    pid_t P_id;

    for (int i = 0; i < K; i++) {
        P_id = fork();

        if (P_id == 0) {
            int start = (i*(N/K)) + 1;
            int end = ((i+1)*(N/K))+(i == K-1 ? N%K : 0);
            childP(start, end, sharedData, i + 1, N);
            exit(0);
        }
    }

    for (int i = 0; i < K; i++) {
        wait(NULL);
    }

    writeOutputToFile(sharedData, K);

    clock_t endTime = clock();

    double time = ((double)(endTime - startTime))/CLOCKS_PER_SEC;
    printf("Time: %f seconds\n", time);

    // Detach and remove shared memory
    shmdt(sharedData);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}

