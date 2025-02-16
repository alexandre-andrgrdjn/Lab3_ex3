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

// Fonction pour convertir une chaîne en numéro
int get_choice_number(const char *choice) {
    if (strcmp(choice, "rock") == 0) return 1;
    if (strcmp(choice, "spock") == 0) return 2;
    if (strcmp(choice, "paper") == 0) return 3;
    if (strcmp(choice, "lizard") == 0) return 4;
    if (strcmp(choice, "scissors") == 0) return 5;
    return 0;
}

// Fonction de jeu
int jeu(int number1, int number2) {
    if ((number1 == 1 && (number2 == 4 || number2 == 5)) ||
        (number1 == 2 && (number2 == 5 || number2 == 1)) ||
        (number1 == 3 && (number2 == 1 || number2 == 4)) ||
        (number1 == 4 && (number2 == 3 || number2 == 2)) ||
        (number1 == 5 && (number2 == 3 || number2 == 4))) {
        return 1;
    } else if (number1 != number2) {
        return 2;
    }
    return 0;
}

void handle_client(int client_socket1, int client_socket2) {
    char buffer1[BUFFER_SIZE] = {0}, buffer2[BUFFER_SIZE] = {0};

    printf("Attente des choix des joueurs...\n");
    if (read(client_socket1, buffer1, BUFFER_SIZE) <= 0 || read(client_socket2, buffer2, BUFFER_SIZE) <= 0) {
        perror("Erreur de lecture des choix");
        close(client_socket1);
        close(client_socket2);
        return;
    }
     pid_t pid = getpid();
    char moves1[3][10], moves2[3][10]; 
    int numbers1[3], numbers2[3];
    sscanf(buffer1, "%s %s %s", moves1[0], moves1[1], moves1[2]);
    sscanf(buffer2, "%s %s %s", moves2[0], moves2[1], moves2[2]);

    for (int i = 0; i < 3; i++) {
        numbers1[i] = get_choice_number(moves1[i]);
        numbers2[i] = get_choice_number(moves2[i]);
    }

    int score1 = 0, score2 = 0;
    char result[BUFFER_SIZE];
    snprintf(result, BUFFER_SIZE, "\nEnter your three moves (rock, paper, scissors, lizard, spock): %s\n", buffer1);
    for (int i = 0; i < 3; i++) {
        int winner = jeu(numbers1[i], numbers2[i]);
        if (winner == 1) score1++;
        else if (winner == 2) score2++;
        
        snprintf(result + strlen(result), BUFFER_SIZE - strlen(result),
                 "[Game %d] Round %d: P1(%s) vs P2(%s)  ->  Score: %d - %d\n",
                pid, i + 1, moves1[i], moves2[i], score1, score2);
    }
    
    snprintf(result + strlen(result), BUFFER_SIZE - strlen(result),
             "[Game %d] Game Over! Winner: %s\n",pid,
             (score1 > score2) ? "Player 1" : (score2 > score1) ? "Player 2" : "Draw");
    
    send(client_socket1, result, strlen(result), 0);
    send(client_socket2, result, strlen(result), 0);
    printf("Partie terminée.\n");
    close(client_socket1);
    close(client_socket2);
}

int main() {
    int server_fd, new_socket1, new_socket2;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Échec de la création du socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Échec de setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Échec du bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 2) < 0) {
        perror("Échec du listen");
        exit(EXIT_FAILURE);
    }

    printf("Serveur en attente sur le port %d...\n", PORT);

    while (1) {
        new_socket1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket1 < 0) { perror("Erreur accept joueur 1"); continue; }

        printf("Joueur 1 connecté. Attente joueur 2...\n");
        
        new_socket2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket2 < 0) { perror("Erreur accept joueur 2"); close(new_socket1); continue; }
        
        printf("Joueur 2 connecté. Début de la partie...\n");

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_client(new_socket1, new_socket2);
            exit(0);
        } else if (pid < 0) {
            perror("Échec du fork");
            close(new_socket1);
            close(new_socket2);
        }
        close(new_socket1);
        close(new_socket2);
    }

    close(server_fd);
    return 0;
}