#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/inotify.h>

#define MAX_PATH_LENGTH 1024
#define MAX_LINE_LENGTH 1024
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
#define DEFAULT_SIZE_FP 2097152
#define SHM_KEY 0x1234

char log_file[MAX_PATH_LENGTH] = "logfile.log";

// Define semaphores
sem_t mutex;
sem_t processed_mutex; // Semaphore for controlling access to the processed directory

// Structure to hold thread data
typedef struct {
    int thread_id;
    char path[MAX_PATH_LENGTH];
    char inventory_file[MAX_PATH_LENGTH];
    char log_file[MAX_PATH_LENGTH];
    int simulate_sleep_min;
    int simulate_sleep_max;
    char sucursal_dir[MAX_PATH_LENGTH];
} thread_data;

typedef struct {
    int NUM_PROCESOS;
    char PATH_FILES[MAX_LINE_LENGTH];
    char INVENTORY_FILE[MAX_LINE_LENGTH];
    char LOG_FILE[MAX_LINE_LENGTH];
    int SIMULATE_SLEEP_MAX;
    int SIMULATE_SLEEP_MIN;
    int SIZE_FP;
    char sucursales[10][MAX_LINE_LENGTH];
} Configuration;

// Structure for shared memory
typedef struct {
    size_t size;
    char data[1];
} shared_memory;

int shmid;
shared_memory *shm;

// Function to write to log and print
void log_message(const char *message, ...) {
    // Write to log
    FILE *fp = fopen(log_file, "a");
    if (fp != NULL) {
    	va_list args; // Define a variable argument list
        va_start(args, message); // Initialize the argument list
        // Print to stdout
        vprintf(message, args);
        // Print to log file
        vfprintf(fp, message, args);
        va_end(args);
        fclose(fp);
    } else {
        perror("Error opening log file");
    }
}

// Function for parseing the configuration file
int parse_config(const char *filename, Configuration *config) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        log_message("Error: Could not open configuration file");
        return 0; // Return failure
    }
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp) != NULL) {
        strtok(line, "\n");
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");
        if (strcmp(key, "NUM_PROCESOS") == 0) {
            config->NUM_PROCESOS = atoi(value);
        } else if (strcmp(key, "PATH_FILES") == 0) {
            strcpy(config->PATH_FILES, value);
        } else if (strcmp(key, "INVENTORY_FILE") == 0) {
            strcpy(config->INVENTORY_FILE, value);
        } else if (strcmp(key, "LOG_FILE") == 0) {
            strcpy(config->LOG_FILE, value);
        } else if (strcmp(key, "SIMULATE_SLEEP_MAX") == 0) {
            config->SIMULATE_SLEEP_MAX = atoi(value);
        } else if (strcmp(key, "SIMULATE_SLEEP_MIN") == 0) {
            config->SIMULATE_SLEEP_MIN = atoi(value);
        } else if (strcmp(key, "SIZE_FP") == 0) {
            config->SIZE_FP = atoi(value);
        } else {
            for (int i = 0; i < 10; i++) {
                char suc_key[20];
                sprintf(suc_key, "SUCURSAL%d", i + 1);
                if (strcmp(key, suc_key) == 0) {
                    strcpy(config->sucursales[i], value);
                    break;
                }
            }
        }
    }
    fclose(fp);
    return 1; // Return success
}

// Function to create directories
void create_directories(Configuration *config) {
    struct stat st = {0};

    if (stat(config->PATH_FILES, &st) == -1) {
        mkdir(config->PATH_FILES, 0700);
    }

    for (int i = 0; i < config->NUM_PROCESOS; i++) {
        char dir_path[MAX_PATH_LENGTH];
        snprintf(dir_path, MAX_PATH_LENGTH - strlen(config->PATH_FILES)-1-strlen("/"), "%s/%s", config->PATH_FILES, config->sucursales[i]);
        if (stat(dir_path, &st) == -1) {
            mkdir(dir_path, 0700);
        }
    }
}

// Function to save shared memory in case of keyboard interrupt
void handle_sigint(int sig) {
    log_message("Caught signal");
    log_message("Writing shared memory to file before exiting...");

    FILE *fp = fopen("shared_memory_backup.dat", "w");
    if (fp != NULL) {
        fwrite(shm, shm->size, 1, fp);
        fclose(fp);
    }

    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);

    exit(0);
}

// Function to process the file
void *process_file(void *arg) {
    thread_data *data = (thread_data *)arg;
    struct dirent *dir;
    char filename[1024];
    char old_path[MAX_PATH_LENGTH];
    char *IdOperacion, *FECHA_INICIO, *FECHA_FIN, *IdUsuario, *IdTipoOperacion, *NoOperacion, *Importe, *Estado;
    while (1) {
        DIR *d = opendir(data->sucursal_dir);        
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (fnmatch("OPE*.data", dir->d_name, 0) == 0) {
                    snprintf(old_path, MAX_PATH_LENGTH - strlen(data->sucursal_dir) - 1 - strlen("/"), "%s/%s", data->sucursal_dir, dir->d_name);
                    FILE *fp_data = fopen(old_path, "r");
                    if (fp_data == NULL) {
                        printf("Error: file %s is empty or cannot be opened\n", dir->d_name);
                        continue; // Continue with the next file
                    }
                    sem_wait(&mutex);
                    char csv_path[MAX_PATH_LENGTH];
                    snprintf(csv_path, MAX_PATH_LENGTH - strlen(data->sucursal_dir) - 1 - strlen("/"), "%s", data->inventory_file);
                    FILE *fp_csv = fopen(csv_path, "a");
                    if (fp_csv == NULL) {
                        log_message("Error: file %s cannot be opened\n", data->inventory_file);
                        sem_post(&mutex);
                        fclose(fp_data);
                        continue; // Continue with the next file
                    }
                    char buffer[1024];
                    while (fgets(buffer, sizeof(buffer), fp_data)) {
                        IdOperacion = strtok(buffer, ";");
                        FECHA_INICIO = strtok(NULL, ";");
                        IdUsuario = strtok(NULL, ";");
                        IdTipoOperacion = strtok(NULL, ";");
                        NoOperacion = strtok(NULL, ";");
                        Importe = strtok(NULL, ";");
                        Estado = strtok(NULL, ";");
                        fprintf(fp_csv, "%s,%s,%s,%s,%s,%s,%s,%s\n", IdOperacion, FECHA_INICIO, FECHA_FIN, IdUsuario, IdTipoOperacion, NoOperacion, Importe, Estado);
                        // Update shared memory
                        snprintf(shm->data + strlen(shm->data), shm->size - strlen(shm->data), "%s,%s,%s,%s,%s,%s,%s,%s\n", IdOperacion, FECHA_INICIO, FECHA_FIN, IdUsuario, IdTipoOperacion, NoOperacion, Importe, Estado);
                    }
                    fclose(fp_data);
                    fclose(fp_csv);
                    sem_post(&mutex);
                    remove(old_path);
                }
            }
            closedir(d);
        }
        sleep(rand() % (data->simulate_sleep_max - data->simulate_sleep_min + 1) + data->simulate_sleep_min);
    }
}


void *watch_directory(void *arg) {
    thread_data *data = (thread_data *)arg;
    int length, i = 0;
    int fd;
    int wd;
    char buffer[BUF_LEN];
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
    }
    wd = inotify_add_watch(fd, data->sucursal_dir, IN_CREATE | IN_MODIFY);
    while (1) {
        length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
        }
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len) {
                if (event->mask & IN_CREATE) {
                    if (!(event->mask & IN_ISDIR)) {
                        log_message("New file %s created in %s.\n", event->name, data->sucursal_dir);
                    }
                }
                if (event->mask & IN_MODIFY) {
                    if (!(event->mask & IN_ISDIR)) {
                        log_message("File %s modified in %s.\n", event->name, data->sucursal_dir);
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }
        i = 0;
    }
    inotify_rm_watch(fd, wd);
    close(fd);
}

int main(int argc, char *argv[]) {
    // Check if initial setup is needed
    if (argc > 1 && strcmp(argv[1], "--setup") == 0) {
        log_message("Setting up initial configuration...\n");
        Configuration config;
        if (!parse_config("fp.conf", &config)) {
            return 1;
        }
        create_directories(&config);
        log_message("Initial configuration completed.\n");
        return 0;
    }

    // Load configuration
    Configuration config;
    if (!parse_config("fp.conf", &config)) {
        return 1;
    }
    strcpy(log_file, config.LOG_FILE);
    log_message("NUM_PROCESOS: %d\n", config.NUM_PROCESOS);
    log_message("SIMULATE_SLEEP_MAX: %d\n", config.SIMULATE_SLEEP_MAX);
    log_message("SIMULATE_SLEEP_MIN: %d\n", config.SIMULATE_SLEEP_MIN);

    // Initialize shared memory
    shmid = shmget(SHM_KEY, config.SIZE_FP, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }
    shm = (shared_memory *)shmat(shmid, NULL, 0);
    if (shm == (shared_memory *)-1) {
        perror("shmat");
        exit(1);
    }
    shm->size = config.SIZE_FP;
    // Restore from backup if it exists
    FILE *fp = fopen("shared_memory_backup.dat", "r");
    if (fp != NULL) {
        fread(shm, config.SIZE_FP, 1, fp);
        fclose(fp);
    }
    signal(SIGINT, handle_sigint);
    // Initialize semaphores
    sem_init(&mutex, 0, 0);
    sem_init(&processed_mutex, 0, 1);

    sem_post(&mutex);

    pthread_t threads[config.NUM_PROCESOS];
    pthread_t watch_thread[config.NUM_PROCESOS];
    thread_data data[config.NUM_PROCESOS];
    
    for (int i = 0; i < config.NUM_PROCESOS; i++) {
        data[i].thread_id = i + 1;
        strcpy(data[i].path, config.PATH_FILES);
        strcpy(data[i].inventory_file, config.INVENTORY_FILE);
        strcpy(data[i].log_file, config.LOG_FILE);
        data[i].simulate_sleep_min = config.SIMULATE_SLEEP_MIN;
        data[i].simulate_sleep_max = config.SIMULATE_SLEEP_MAX;
        snprintf(data[i].sucursal_dir, MAX_PATH_LENGTH - strlen(config.PATH_FILES) - 1 - strlen("/"), "%s/%s", config.PATH_FILES, config.sucursales[i]);
        pthread_create(&threads[i], NULL, process_file, &data[i]);
        pthread_create(&watch_thread[i], NULL, watch_directory, &data[i]);
    }

    for (int i = 0; i < config.NUM_PROCESOS; i++) {
        pthread_join(threads[i], NULL);
        pthread_join(watch_thread[i], NULL);
    }

    // Destroy semaphores
    sem_destroy(&processed_mutex);
    // Detach and remove shared memory
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}
