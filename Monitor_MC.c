#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define MAX_PATH_LENGTH 1024
#define SHM_KEY 0x1234

sem_t monitor_sem;

typedef struct {
    char path[MAX_PATH_LENGTH];
    int interval;
} monitor_data;

typedef struct {
    size_t size;
    char data[1];
} shared_memory;

shared_memory *shm;

void *monitor_function(void *arg) {
    monitor_data *data = (monitor_data *)arg;
    int file_counter = 0;

    while (1) {
        sem_wait(&monitor_sem); // Wait for the semaphore to notify new data
        sleep(data->interval);

        // Read data from shared memory
        char shared_data[shm->size + 1];
        strncpy(shared_data, shm->data, shm->size);
        shared_data[shm->size] = '\0';

        // Create a pipe
        int pipe_fd[2];
        if (pipe(pipe_fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // Fork a child process
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) { // Child process
            // Close the write end of the pipe
            close(pipe_fd[1]);

            // Redirect stdin to the read end of the pipe
            dup2(pipe_fd[0], STDIN_FILENO);

            // Execute Patrones.c
            execlp("./Patrones", "./Patrones", NULL);

            // Close the read end of the pipe
            close(pipe_fd[0]);
        } else { // Parent process
            // Close the read end of the pipe
            close(pipe_fd[0]);

            // Write data to the pipe
            write(pipe_fd[1], shared_data, strlen(shared_data));

            // Close the write end of the pipe
            close(pipe_fd[1]);

            sem_post(&monitor_sem); // Release the semaphore after processing
            if (file_counter > 0) {
                printf("Data from shared memory sent to Patrones.\n");
            } else {
                printf("No data available in shared memory.\n");
            }
        }
    }
    return NULL;
}

int main() {
	pthread_t monitor_thread;
	monitor_data data;
	data.interval = 10; // Set the interval for the monitor to check for new data

	// Attach to the shared memory
	int shmid = shmget(SHM_KEY, 0, 0666);
	if (shmid < 0) {
	perror("shmget");
	exit(1);
	}
	shm = (shared_memory *)shmat(shmid, NULL, 0);
	if (shm == (shared_memory *)-1) {
	perror("shmat");
	exit(1);
	}
	printf("Attached to shared memory");
	
	sem_init(&monitor_sem, 0, 0); // Initialize the semaphore
	sem_post(&monitor_sem);
	monitor_function;
	sem_destroy(&monitor_sem); // Destroy the semaphore

	// Detach from shared memory
	shmdt(shm);

	return 0;
}
