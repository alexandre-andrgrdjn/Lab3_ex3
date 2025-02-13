# Variables
CC = gcc
CFLAGS = -Wall -Wextra
SERVER_SRC = simple_game_server.c
CLIENT_SRC = simple_game_client.c
SERVER_OBJ = simple_game_server.o
CLIENT_OBJ = simple_game_client.o
SERVER_EXEC = simple_game_server
CLIENT_EXEC = simple_game_client

# Cible par défaut
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Compilation du serveur
$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $(SERVER_EXEC) $(SERVER_OBJ)

# Compilation du client
$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $(CLIENT_EXEC) $(CLIENT_OBJ)

# Compilation des fichiers objets du serveur
$(SERVER_OBJ): $(SERVER_SRC)
	$(CC) $(CFLAGS) -c $(SERVER_SRC)

# Compilation des fichiers objets du client
$(CLIENT_OBJ): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC)

# Nettoyage des fichiers générés
clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER_EXEC) $(CLIENT_EXEC)

# Cible pour exécuter le serveur et le client
run: $(SERVER_EXEC) $(CLIENT_EXEC)
	./$(SERVER_EXEC) & ./$(CLIENT_EXEC) && wait

