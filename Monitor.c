#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_USERS 1000
#define MAX_LINE_LENGTH 1024

typedef struct {
    char idOperacion[100];
    char fechaInicio[100];
    char fechaFin[100];
    char idUsuario[100];
    char idTipoOperacion[100];
    char noOperacion[100];
    char importe[100];
    char estado[100];
} Transaction;

typedef struct {
    int NUM_PROCESOS;
    char PATH_FILES[MAX_LINE_LENGTH];
    char INVENTORY_FILE[MAX_LINE_LENGTH];
    char LOG_FILE[MAX_LINE_LENGTH];
    int SIMULATE_SLEEP_MAX;
    int SIMULATE_SLEEP_MIN;
} Configuration;

int parse_config(const char *filename, Configuration *config) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error: Could not open configuration file %s\n", filename);
        return 0; // Return failure
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Remove newline character
        strtok(line, "\n");

        // Split the line into key and value
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
        }
    }

    fclose(fp);
    return 1; // Return success
}

// Función para leer las transacciones desde el archivo CSV y escribirlas en la tubería
Transaction* readTransactions(const char *filename) {
    FILE *fp;
    Transaction *transactions = malloc(MAX_USERS * sizeof(Transaction));
    int num_transactions = 0;

    // Open the CSV file
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Error al abrir el archivo CSV");
        exit(EXIT_FAILURE);
    }

    // Read and store each transaction
    while (fscanf(fp, "%s %s %s %s %s %s %s %s", transactions[num_transactions].idOperacion, transactions[num_transactions].fechaInicio, transactions[num_transactions].fechaFin, transactions[num_transactions].idUsuario, transactions[num_transactions].idTipoOperacion, transactions[num_transactions].noOperacion, transactions[num_transactions].importe, transactions[num_transactions].estado) == 8) {
        num_transactions++;
    }

    // Close the CSV file
    fclose(fp);

    // Resize the transactions array to fit the actual number of transactions
    transactions = realloc(transactions, num_transactions * sizeof(Transaction));

    return transactions;
}

int main() {    
    Configuration config;

    if (!parse_config("fp.conf", &config)) {
        return -1; // Exit if parsing fails
    }

    printf("INVENTORY_FILE: %s\n", config.INVENTORY_FILE);

    const char *filename = config.INVENTORY_FILE;
    
    Transaction *transactions = readTransactions(filename);

    // Pipe file descriptors
    int pipe_fd[2];

    // Create the pipe
    if (pipe(pipe_fd) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    // Fork a child process
    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {  // Parent process
        // Close the read end of the pipe
        close(pipe_fd[0]);

        // Write transactions to the pipe
        write(pipe_fd[1], transactions, MAX_USERS * sizeof(Transaction));

        // Close the write end of the pipe
        close(pipe_fd[1]);

        // Clean up memory
        free(transactions);
    } else {  // Child process
        // Close the write end of the pipe
        close(pipe_fd[1]);

        // Redirect stdin to read from the pipe
        dup2(pipe_fd[0], STDIN_FILENO);

        // Execute the child process (patrones)
        execl("./Patrones", "Patrones", (char *)NULL);

        // If execl() fails
        perror("execl failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}
