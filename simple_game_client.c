#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

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

    int number;
    printf("Entrez un nombre entre 1 et 5 : ");
    while (scanf("%d", &number) != 1 || number < 1 || number > 5) {
        printf("Nombre invalide, essayez encore : ");
        while (getchar() != '\n');
    }

    snprintf(buffer, BUFFER_SIZE, "%d", number);
    send(sock, buffer, strlen(buffer), 0);
    read(sock, buffer, BUFFER_SIZE);
    printf("Résultat : %s\n", buffer);

    close(sock);
    return 0;
}
