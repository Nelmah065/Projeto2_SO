#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "api.h"
#include <asm-generic/fcntl.h>

int main(int argc, char* argv[]) {
  /*if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }*/
  if (argc != 2) {
  fprintf(stderr, "Usage: %s <pipe_path>\n", argv[0]);
  return 1;
}

  char pipe_path[MAX_PIPE_NAME_SIZE];
  strcpy(pipe_path, argv[1]);

  // 1. Criação do Named Pipe do Servidor
  if (mkfifo(pipe_path, 0666) == -1) {
    perror("Error creating server pipe");
    return 1;
  }

  // 2. Abertura do Named Pipe do Servidor
  int server_fd = open(pipe_path, O_RDONLY);
  if (server_fd == -1) {
    perror("Error opening server pipe");
    return 1;
  }
  
  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  //TODO: Intialize server, create worker threads
  int session_count = 0;
  while (1) {
    //TODO: Read from pipe
    if (session_count < MAX_SESSIONS) {
      // Ler os pedidos dos clientes e processar
      if (process_request(server_fd) == 0) {
        // Incrementar o número de sessões após processar com sucesso
        session_count++;
      }
    } else {
      // Aguardar que uma sessão termine para criar uma nova
      wait_session_termination();
      session_count--;
    }
  }

  // 5. Encerramento do Servidor
  close(server_fd);
  unlink(pipe_path);

  // 6. Encerramento do EMS
  ems_terminate();

  return 0;
}
