#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Fonction pour convertir un choix en numéro
int get_choice_number(const char *choice) {
    if (strcmp(choice, "rock") == 0) return 1;
    if (strcmp(choice, "paper") == 0) return 2;
    if (strcmp(choice, "scissors") == 0) return 3;
    if (strcmp(choice, "lizard") == 0) return 4;
    if (strcmp(choice, "spock") == 0) return 5;
    return 0; // Entrée invalide
}

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    // Création du socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Échec de création du socket");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Conversion et connexion au serveur
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Adresse invalide");
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connexion échouée");
        return -1;
    }

    char move1[10], move2[10], move3[10];
    int valid = 0;

    while (!valid) {
        printf("Entrez 3 choix séparés par des espaces (rock, spock, paper, lizard, scissors) : ");
        
        if (scanf("%9s %9s %9s", move1, move2, move3) == 3) {
            printf("DEBUG: Saisie brute: '%s' '%s' '%s'\n", move1, move2, move3);

            int num1 = get_choice_number(move1);
            int num2 = get_choice_number(move2);
            int num3 = get_choice_number(move3);

            printf("DEBUG: Conversion en nombres: %d %d %d\n", num1, num2, num3);

            if (num1 > 0 && num2 > 0 && num3 > 0) {
                valid = 1;
            } else {
                printf("Résultat : Entrée invalide. Veuillez entrer rock, spock, paper, lizard ou scissors.\n");
            }
        } else {
            printf("Erreur de lecture. Veuillez réessayer.\n");
        }
        
        while (getchar() != '\n'); // Nettoyage du buffer d'entrée
    }

    // Envoi au serveur
    snprintf(buffer, BUFFER_SIZE, "%s %s %s", move1, move2, move3);
    printf("DEBUG: Données envoyées au serveur: '%s'\n", buffer);
    send(sock, buffer, strlen(buffer), 0);

    // Réception de la réponse
    memset(buffer, 0, BUFFER_SIZE);
    read(sock, buffer, BUFFER_SIZE);
    printf("Résultat : %s\n", buffer);

    close(sock);
    return 0;
}