CC = gcc
CFLAGS = -Wall -Wextra  -I include
LDFLAGS = 
BIN_DIR = bin
SRC_DIR = src

SERVER_BIN = $(BIN_DIR)/server
CLIENT_BIN = $(BIN_DIR)/client
SERVER_SRC = $(SRC_DIR)/server.c
CLIENT_SRC = $(SRC_DIR)/client.c
PROTOCOL_H = protocol.h

.PHONY: all
all: $(SERVER_BIN) $(CLIENT_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(SERVER_BIN): $(SERVER_SRC) | $(BIN_DIR)
	@echo "Compilando Servidor..."
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_SRC) | $(BIN_DIR)
	@echo "Compilando Cliente..."
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	@echo "Removendo binários e diretório $(BIN_DIR)/..."
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
	rm -rf $(BIN_DIR)

.PHONY: run_server
run_server: $(SERVER_BIN)
	@echo "Iniciando servidor (IPv4 na porta 5000)..."
	$(SERVER_BIN) v4 5000

.PHONY: run_client
run_client: $(CLIENT_BIN)
	@echo "Iniciando cliente (Conectando em 127.0.0.1:5000)..."
	$(CLIENT_BIN) 127.0.0.1 5000
