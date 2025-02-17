#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to convert a move string to a corresponding number
int get_choice_number(const char *choice) {
    if (strcmp(choice, "rock") == 0) return 1;
    if (strcmp(choice, "spock") == 0) return 2;
    if (strcmp(choice, "paper") == 0) return 3;
    if (strcmp(choice, "lizard") == 0) return 4;
    if (strcmp(choice, "scissors") == 0) return 5;
    return 0; // Invalid choice
}

// Game function that determines the winner of a round
int jeu(int number1, int number2) {
    if ((number1 == 1 && (number2 == 4 || number2 == 5)) ||
        (number1 == 2 && (number2 == 5 || number2 == 1)) ||
        (number1 == 3 && (number2 == 1 || number2 == 4)) ||
        (number1 == 4 && (number2 == 3 || number2 == 2)) ||
        (number1 == 5 && (number2 == 3 || number2 == 4))) {
        return 1; // Player 1 wins
    } else if (number1 != number2) {
        return 2; // Player 2 wins
    }
    return 0; // Draw
}

// Function to handle communication between two players
void handle_client(int client_socket1, int client_socket2) {
    char buffer1[BUFFER_SIZE] = {0}, buffer2[BUFFER_SIZE] = {0};

    // Read choices from both players
    if (read(client_socket1, buffer1, BUFFER_SIZE) <= 0 || read(client_socket2, buffer2, BUFFER_SIZE) <= 0) {
        perror("Error reading choices");
        close(client_socket1);
        close(client_socket2);
        return;
    }

    pid_t pid = getpid(); // Process ID for logging
    char moves1[3][10], moves2[3][10]; 
    int numbers1[3], numbers2[3];

    // Parse the three moves from each player
    sscanf(buffer1, "%s %s %s", moves1[0], moves1[1], moves1[2]);
    sscanf(buffer2, "%s %s %s", moves2[0], moves2[1], moves2[2]);

    // Convert moves to numbers
    for (int i = 0; i < 3; i++) {
        numbers1[i] = get_choice_number(moves1[i]);
        numbers2[i] = get_choice_number(moves2[i]);
    }

    int score1 = 0, score2 = 0;
    char result[BUFFER_SIZE];

    // Iterate through three rounds
    for (int i = 0; i < 3; i++) {
        int winner = jeu(numbers1[i], numbers2[i]);
        if (winner == 1) score1++;
        else if (winner == 2) score2++;
        
        // Append round result to the result buffer
        snprintf(result + strlen(result), BUFFER_SIZE - strlen(result),
                 "[Game %d] Round %d: P1(%s) vs P2(%s)  ->  Score: %d - %d\n",
                pid, i + 1, moves1[i], moves2[i], score1, score2);
    }
    
    // Append the final game result
    snprintf(result + strlen(result), BUFFER_SIZE - strlen(result),
             "[Game %d] Game Over! Winner: %s\n", pid,
             (score1 > score2) ? "Player 1" : (score2 > score1) ? "Player 2" : "Draw");
    
    // Send results to both players
    send(client_socket1, result, strlen(result), 0);
    send(client_socket2, result, strlen(result), 0);
    
    close(client_socket1);
    close(client_socket2);
}

int main() {
    int server_fd, new_socket1, new_socket2;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 2) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server waiting for connections on port %d...\n", PORT);

    while (1) {
        // Accept first player
        new_socket1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket1 < 0) { perror("Error accepting player 1"); continue; }

        printf("Player 1 connected. Waiting for Player 2...\n");
        
        // Accept second player
        new_socket2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket2 < 0) { perror("Error accepting player 2"); close(new_socket1); continue; }
        
        printf("Player 2 connected. Starting the game...\n");

        // Create a new process to handle the game
        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_client(new_socket1, new_socket2);
            exit(0);
        } else if (pid < 0) {
            perror("Fork failed");
            close(new_socket1);
            close(new_socket2);
        }
        
        // Close sockets in parent process
        close(new_socket1);
        close(new_socket2);
    }

    close(server_fd);
    return 0;
}