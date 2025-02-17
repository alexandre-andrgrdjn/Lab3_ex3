
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to convert a move string to a corresponding number
int get_choice_number(const char *choice) {
    if (strcmp(choice, "rock") == 0) return 1;
    if (strcmp(choice, "paper") == 0) return 2;
    if (strcmp(choice, "scissors") == 0) return 3;
    if (strcmp(choice, "lizard") == 0) return 4;
    if (strcmp(choice, "spock") == 0) return 5;
    return 0; // Invalid choice
}

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert address and connect to the server
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    char move1[10], move2[10], move3[10];

    // Get valid moves from the user
    printf("Enter 3 moves separated by spaces (rock, spock, paper, lizard, scissors): ");
    scanf("%9s %9s %9s", move1, move2, move3);

    // Send moves to the server
    snprintf(buffer, BUFFER_SIZE, "%s %s %s", move1, move2, move3);
    send(sock, buffer, strlen(buffer), 0);

    // Receive and display the result
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    printf("Game Result:\n%s\n", buffer);

    close(sock);
    return 0;
}