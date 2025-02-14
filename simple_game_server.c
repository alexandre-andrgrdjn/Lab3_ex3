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
#define LOG_FILE "simple_game_log.txt"

void handle_client(int client_socket1, int client_socket2, int pipe_fd);

int main() {
    int server_fd, new_socket1, new_socket2;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Création du socket serveur
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Échec de la création du socket");
        exit(EXIT_FAILURE); 
    }

    // Options pour réutiliser le port immédiatement après fermeture
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Échec de setsockopt");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Liaison du socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Échec du bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Mise en écoute du serveur uniquement 2 personnes max
    if (listen(server_fd, 2) < 0) {
        perror("Échec du listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Serveur en attente de connexions sur le port %d...\n", PORT);

    // Création du pipe pour communication avec le parent
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Échec du pipe");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accepter les connexions de deux clients
        new_socket1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket1 < 0) { perror("Échec de accept"); continue; }

        new_socket2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket2 < 0) { perror("Échec de accept"); close(new_socket1); continue; }

        // Création d'un processus enfant pour gérer la partie
        pid_t pid = fork();
        if (pid == 0) { // Processus enfant
            close(server_fd);
            close(pipe_fd[0]); // Fermer la lecture du pipe
            handle_client(new_socket1, new_socket2, pipe_fd[1]);
            close(pipe_fd[1]);
            exit(0);
        } else if (pid < 0) {
            perror("Échec du fork");
            close(new_socket1);
            close(new_socket2);
        }

        // Processus parent ferme les sockets des clients
        close(new_socket1);
        close(new_socket2);

        // Lire le message du pipe et l'écrire dans le fichier de log
        char log_buffer[BUFFER_SIZE];
        int log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (log_fd < 0) { perror("Échec d'ouverture du fichier de log"); continue; }

        ssize_t bytes_read = read(pipe_fd[0], log_buffer, BUFFER_SIZE - 1);
        if (bytes_read > 0) {
            log_buffer[bytes_read] = '\0';
            write(log_fd, log_buffer, bytes_read);
        }

        close(log_fd);
    }

    close(server_fd);
    return 0;
}

void handle_client(int client_socket1, int client_socket2, int pipe_fd) {
    char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];

    read(client_socket1, buffer1, BUFFER_SIZE);
    read(client_socket2, buffer2, BUFFER_SIZE);

    int number1 = atoi(buffer1);
    int number2 = atoi(buffer2);

    char result[BUFFER_SIZE];
    snprintf(result, BUFFER_SIZE, (number1 > number2) ? "Joueur 1 gagne !\n" :
                                                       (number2 > number1) ? "Joueur 2 gagne !\n" :
                                                                            "Égalité !\n");

    send(client_socket1, result, strlen(result), 0);
    send(client_socket2, result, strlen(result), 0);

    write(pipe_fd, result, strlen(result));
    close(client_socket1);
    close(client_socket2);
}
