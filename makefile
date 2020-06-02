.PHONY: build clean help targets

CC = gcc
CFLAGS = -std=gnu90 -pthread

# Colori vari
RED=\033[0;31m
BOLD_RED=\033[1;31m
BOLD_GREEN=\033[1;32m
GREEN=\033[0;32m
PINK=\033[0;35m
BOLD_PINK=\033[1;35m
CYAN=\033[0;36m
BOLD_CYAN=\033[1;36m
NC=\033[0m
YELLOW=\e[93m

default: help

targets: bin/M bin/S bin/L bin/T bin/P bin/Q

bin/M: src/M.c 
	$(CC) $(CFLAGS) -o bin/M src/M.c

bin/S: src/S.c 
	$(CC) $(CFLAGS) -o bin/S src/S.c

bin/P: src/P.c
	$(CC) $(CFLAGS) -o bin/P src/P.c

bin/L: src/dispositivi/L.c 
	$(CC) $(CFLAGS) -o bin/L src/dispositivi/L.c

bin/T: src/dispositivi/T.c 
	$(CC) $(CFLAGS) -o bin/T src/dispositivi/T.c

bin/Q: src/Q.c 
	$(CC) $(CFLAGS) -o bin/Q src/Q.c

clean:
	@rm -f -r /tmp/SO
	@rm -f -r bin
	@rm -f M
	@rm -f Q
	@echo "\n$(BOLD_GREEN)Pulizia completa.$(NC)\n"

build:
	@make clean
	@echo "\n$(YELLOW)Inizio la compilazione...$(NC)\n"
	@mkdir bin
	@mkdir -p /tmp/SO
	@make targets
	@ln -s bin/M M
	@chmod +x M
	@ln -s bin/Q Q
	@chmod +x Q
	@echo "\n$(BOLD_GREEN)Compilazione completata!$(NC)\n"
	@echo "Usa $(RED)./M$(NC) per avviare il programma M."
	@echo "Usa $(RED)./Q$(NC) per avviare il programma Q.\n"

help: 
	@echo "\n$(BOLD_GREEN)------------------------- PROGETTO 1 S.O. 2020 -------------------------$(NC)"
	@echo "$(CYAN)Realizzato da:$(NC)"
	@echo "Mattia Paternoster"
	@echo "Francesco Previdi"
	@echo "Tommaso Manzana"
	@echo "Daniele Soprani"
	@echo "\nPer iniziare, digita $(BOLD_CYAN)make build$(NC) per compilare il progetto!"
	@echo "Successivamente:"
	@echo "  $(BOLD_CYAN)./M$(NC) per aprire il programma M." 
	@echo "  $(BOLD_CYAN)./Q$(NC) per aprire il programma Q." 
	@echo "Utilizza $(BOLD_RED)make clean$(NC) per rimuovere il bin e i file temporanei.\n" 
