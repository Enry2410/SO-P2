#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_TRANSACTIONS 1000
#define MAX_FILENAME_LENGTH 256
#define MAX_LINE_LENGTH 1024
#define MAX_TYPES 20
#define MAX_USERS 1000

// Structure to store transaction data
typedef struct {
    char idOperacion[100];
    char fechaInicio[100];
    char fechaFin[100];
    char idUsuario[100];
    char idTipoOperacion[100];
    char noOperacion[100];
    float importe;
    char estado[100];
} Transaction;


void readAndApplyPatterns(int pipe_fd);
void detectPattern1(Transaction transactions[], int numTransactions);
void detectPattern2(Transaction transactions[], int numTransactions);
void detectPattern3(Transaction transactions[], int numTransactions);
void detectPattern4(Transaction transactions[], int numTransactions);
void detectPattern5(Transaction transactions[], int numTransactions);

int main() {
    Transaction transactions[MAX_USERS];

    // Read transactions from stdin (piped from parent process)
    fread(transactions, sizeof(Transaction), MAX_USERS, stdin);

    

    detectPattern1(transactions, MAX_USERS);
    detectPattern2(transactions, MAX_USERS);
    detectPattern3(transactions, MAX_USERS);
    detectPattern4(transactions, MAX_USERS);
    detectPattern5(transactions, MAX_USERS);


    return 0;
}


// Function to detect Pattern 1
void detectPattern1(Transaction transactions[], int numTransactions) {
    int i, j;

    // Iterate over all transactions
    for (i = 0; i < numTransactions; i++) {
        int count = 0;

        // Count how many transactions occur in the same hour for the same user
        for (j = 0; j < numTransactions; j++) {
            // Check if the transactions occur in the same hour for the same user
            if (i != j && strcmp(transactions[i].idUsuario, transactions[j].idUsuario) == 0 && strcmp(transactions[i].fechaInicio, transactions[j].fechaInicio) == 0) {
                count++;
            }
        }

        // If there are more than 5 transactions, print the detected pattern
        if (count > 5) {
            printf("Pattern 1 detected: More than 5 transactions in the same hour for the same user.\n");
            return; // Stop pattern detection after the first pattern is found
        }
    }

    printf("No Pattern 1 detected.\n");
}

// Function to detect Pattern 2
void detectPattern2(Transaction transactions[], int numTransactions) {
    int i, j;

    // Iterate over all transactions
    for (i = 0; i < numTransactions; i++) {
        int count = 0;

        // Check if the transaction is a withdrawal
        if (transactions[i].importe < 0) {
            // Count how many withdrawals occur in the same day for the same user
            for (j = 0; j < numTransactions; j++) {
                // Check if the transactions occur in the same day for the same user
                if (i != j && strcmp(transactions[i].idUsuario, transactions[j].idUsuario) == 0 && strcmp(transactions[i].fechaInicio, transactions[j].fechaInicio) == 0 && transactions[j].importe < 0) {
                    count++;
                }
            }

            // If there are more than 3 withdrawals, print the detected pattern
            if (count > 3) {
                printf("Pattern 2 detected: More than 3 withdrawals in the same day for the same user.\n");
                return; // Stop pattern detection after the first pattern is found
            }
        }
    }

    printf("No Pattern 2 detected.\n");
}


// Function to detect Pattern 3
void detectPattern3(Transaction transactions[], int numTransactions) {
    int i, j;

    // Iterate over all transactions
    for (i = 0; i < numTransactions; i++) {
        int count = 0;

        // Check if the transaction is an error
        if (strcmp(transactions[i].estado, "Error") == 0) {
            // Count how many errors occur in the same day for the same user
            for (j = 0; j < numTransactions; j++) {
                // Check if the transactions occur in the same day for the same user
                if (i != j && strcmp(transactions[i].idUsuario, transactions[j].idUsuario) == 0 && strcmp(transactions[i].fechaInicio, transactions[j].fechaInicio) == 0 && strcmp(transactions[j].estado, "Error") == 0) {
                    count++;
                }
            }

            // If there are more than 3 errors, print the detected pattern
            if (count > 3) {
                printf("Pattern 3 detected: More than 3 errors in the same day for the same user.\n");
                return; // Stop pattern detection after the first pattern is found
            }
        }
    }

    printf("No Pattern 3 detected.\n");
}

void detectPattern4(Transaction transactions[], int numTransactions) {
    int i, j;
    int numUniqueTypes = 0;
    char uniqueTypes[MAX_TYPES][100];

    // Count the number of unique operation types
    for (i = 0; i < numTransactions; i++) {
        int isNewType = 1;
        for (j = 0; j < numUniqueTypes; j++) {
            if (strcmp(transactions[i].idTipoOperacion, uniqueTypes[j]) == 0) {
                isNewType = 0;
                break;
            }
        }
        if (isNewType) {
            strcpy(uniqueTypes[numUniqueTypes], transactions[i].idTipoOperacion);
            numUniqueTypes++;
        }
    }

    // Iterate over all transactions
    for (i = 0; i < numTransactions; i++) {
        int count = 0;

        // Count how many different operations occur on the same day for the same user
        for (j = 0; j < numTransactions; j++) {
            if (i != j && strcmp(transactions[i].idUsuario, transactions[j].idUsuario) == 0 && strcmp(transactions[i].fechaInicio, transactions[j].fechaInicio) == 0) {
                count++;
            }
        }

        // If the number of different operations is equal to the number of unique types, print the pattern
        if (count == numUniqueTypes) {
            printf("Pattern 4 detected: One operation of each type on the same day for the same user.\n");
            return; // Stop pattern detection after the first pattern is found
        }
    }

    printf("No Pattern 4 detected.\n");
}


void detectPattern5(Transaction transactions[], int numTransactions) {
    int i, j;

    // Iterate over all transactions
    for (i = 0; i < numTransactions; i++) {
        float totalRetiro = 0;
        float totalIngreso = 0;

        // Verify if the current transaction is a withdrawal
        if (transactions[i].importe < 0) {
            // Add the amount of the withdrawal to the total withdrawal
            totalRetiro += transactions[i].importe;
        } else {
            // Add the amount of the deposit to the total deposit
            totalIngreso += transactions[i].importe;
        }

        // Iterate over the other transactions of the same day and user
        for (j = 0; j < numTransactions; j++) {
            if (i != j && strcmp(transactions[i].idUsuario, transactions[j].idUsuario) == 0 && strcmp(transactions[i].fechaInicio, transactions[j].fechaInicio) == 0) {
                // Verify if the transaction is a withdrawal or a deposit and add to the corresponding total
                if (transactions[j].importe < 0) {
                    totalRetiro += transactions[j].importe;
                } else {
                    totalIngreso += transactions[j].importe;
                }
            }
        }

        // If the withdrawn amount is greater than the deposited amount, print the detected pattern
        if (totalRetiro > totalIngreso) {
            printf("Pattern 5 detected: The amount withdrawn in one day is greater than the amount deposited on the same day for the same user.\n");
            return; // Stop pattern detection after the first pattern is found
        }
    }

    printf("No Pattern 5 detected.\n");
}

