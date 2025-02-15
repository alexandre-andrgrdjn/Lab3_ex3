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

// Structure pour stocker un client
typedef struct {
    int socket_fd;
    struct sockaddr_in address;
} Client;

Client clients[100];  // On suppose qu'il y a au maximum 100 clients
int client_count = 0; // Compteur pour suivre combien de clients sont connectés

int get_choice_number(const char *choice) {
    if (strcmp(choice, "rock") == 0) return 1;
    if (strcmp(choice, "spock") == 0) return 2;
    if (strcmp(choice, "paper") == 0) return 3;
    if (strcmp(choice, "lizard") == 0) return 4;
    if (strcmp(choice, "scissors") == 0) return 5;
    return 0; // Valeur invalide
}

int jeu(int number1, int number2) {
     if ((number1 == 1 && (number2 == 4 || number2 == 5)) ||
        (number1 == 2 && (number2 == 5 || number2 == 1)) ||
        (number1 == 3 && (number2 == 1 || number2 == 4)) ||
        (number1 == 4 && (number2 == 3 || number2 == 2)) ||
        (number1 == 5 && (number2 == 3 || number2 == 4))) {
        return 1;
    } 
    else if ((number2 == 1 && (number1 == 4 || number1 == 5)) ||
             (number2 == 2 && (number1 == 5 || number1 == 1)) ||
             (number2 == 3 && (number1 == 1 || number1 == 4)) ||
             (number2 == 4 && (number1 == 3 || number1 == 2)) ||
             (number2 == 5 && (number1 == 3 || number1 == 4))) {
        return 2;
    } 
    else {
    return 0;
    }
}

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

    // Mise en écoute du serveur uniquement 2 personnes max en attente
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

    // Lire les choix des deux joueurs (ex : "rock paper spock")
    read(client_socket1, buffer1, BUFFER_SIZE);
    read(client_socket2, buffer2, BUFFER_SIZE);

    char moves1[3][10], moves2[3][10]; 
    int numbers1[3], numbers2[3];

    // Extraire les trois choix de chaque joueur
    sscanf(buffer1, "%s %s %s", moves1[0], moves1[1], moves1[2]);
    sscanf(buffer2, "%s %s %s", moves2[0], moves2[1], moves2[2]);

    // Convertir les choix en nombres (1 à 5)
    for (int i = 0; i < 3; i++) {
        numbers1[i] = get_choice_number(moves1[i]);
        numbers2[i] = get_choice_number(moves2[i]);
    }

    // Vérifier si l'un des joueurs a entré une valeur invalide
    for (int i = 0; i < 3; i++) {
        if (numbers1[i] == 0 || numbers2[i] == 0) {
            char error_msg[] = "Entrée invalide. Veuillez entrer rock, spock, paper, lizard ou scissors.\n";
            send(client_socket1, error_msg, strlen(error_msg), 0);
            send(client_socket2, error_msg, strlen(error_msg), 0);
            close(client_socket1);
            close(client_socket2);
            return;
        }
    }

    // Calcul du score
    int score1 = 0, score2 = 0;
    char result[BUFFER_SIZE];
    strcpy(result, "Résultat des manches :\n");

    for (int i = 0; i < 3; i++) {
        int winner = jeu(numbers1[i], numbers2[i]); // Appel de la fonction jeu()

        if (winner == 1) {
            score1++;
            snprintf(result + strlen(result), BUFFER_SIZE - strlen(result), 
                     "Manche %d : Joueur 1 gagne (%s vs %s)\n", i+1, moves1[i], moves2[i]);
        }
        else if (winner == 2) {
            score2++;
            snprintf(result + strlen(result), BUFFER_SIZE - strlen(result), 
                     "Manche %d : Joueur 2 gagne (%s vs %s)\n", i+1, moves1[i], moves2[i]);
        }
        else {
            snprintf(result + strlen(result), BUFFER_SIZE - strlen(result), 
                     "Manche %d : Égalité (%s vs %s)\n", i+1, moves1[i], moves2[i]);
        }
    }

    // Score final et gagnant de la partie
    snprintf(result + strlen(result), BUFFER_SIZE - strlen(result),
             "\nScore final : Joueur 1 = %d | Joueur 2 = %d\n%s\n",
             score1, score2, (score1 > score2) ? "Joueur 1 gagne la partie !" :
             (score2 > score1) ? "Joueur 2 gagne la partie !" : "Match nul !");

    // Envoyer le résultat final aux joueurs
    send(client_socket1, result, strlen(result), 0);
    send(client_socket2, result, strlen(result), 0);

    // Écrire le résultat dans le fichier de log via le pipe
    write(pipe_fd, result, strlen(result));

    // Fermer les sockets
    close(client_socket1);
    close(client_socket2);
}
