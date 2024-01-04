#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // Para close e unlink
#include <pthread.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
//#include <asm-generic/fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


// Estrutura para armazenar informações sobre um cliente
typedef struct ClientInfo {
    int read_fd;   // Descritor de arquivo para leitura (pipe de pedidos)
    int write_fd;  // Descritor de arquivo para escrita (pipe de respostas)
    // Outras informações relevantes sobre o cliente
} ClientInfo;

void* client_handler(void* arg) {
    ClientInfo* client_info = (ClientInfo*)arg;
    int read_fd = client_info->read_fd;
    int write_fd = client_info->write_fd;

    // Lógica para processar pedidos do cliente
    char request_msg[81];
    ssize_t bytes_read = read(read_fd, request_msg, sizeof(char)*81);

    // Verificar se a leitura foi bem-sucedida
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            // O cliente fechou o pipe, encerrando a sessão
            close(read_fd);
            close(write_fd);
            free(client_info);
            pthread_exit(NULL);
        } else {
            perror("Error reading request");
            // Lidar com erro, se necessário
        }
    } else {
        // Lógica para processar o pedido do cliente
        // ...

        // Exemplo: Enviar uma resposta ao cliente
        char response_msg[82];
        // Preencher response_msg com a resposta apropriada
        ssize_t bytes_written = write(write_fd, response_msg, 82);
        if (bytes_written == -1) {
            perror("Error writing response to client");
            // Lidar com erro, se necessário
        }
    }

    // Fechar os descritores de arquivo do cliente quando terminar
    close(read_fd);
    close(write_fd);

    // Liberar a memória alocada para informações do cliente
    free(client_info);

    pthread_exit(NULL);
}

int process_request(int server_fd) {
    char request_msg[81];
    ssize_t bytes_read = read(server_fd, request_msg, sizeof(char)*81);

    // Verificar se a leitura foi bem-sucedida
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            // O cliente fechou o pipe, encerrando a sessão
            return -1;
        } else {
            perror("Error reading request");
            return 1;  // Tratamento de erro
        }
    }

    // Criar uma estrutura para armazenar informações sobre o cliente
    ClientInfo* client_info = (ClientInfo*)malloc(sizeof(ClientInfo));
    if (client_info == NULL) {
        perror("Error allocating memory for client info");
        return 1; // Tratamento de erro
    }

    // Configurar client_info com as informações apropriadas
    // (por exemplo, descritores de arquivo do cliente para leitura e escrita)

    // Criar uma nova thread para lidar com o cliente
    pthread_t client_thread;
    if (pthread_create(&client_thread, NULL, client_handler, (void*)client_info) != 0) {
        perror("Error creating client thread");
        free(client_info);  // Liberar memória em caso de erro
        return 1; // Tratamento de erro
    }

    // Continuar a processar outros registros ou fazer o que for necessário
    // ...

    return 0;  // Sucesso
}


pthread_t sessions[MAX_SESSIONS];
int active_sessions = 0;

void* session_thread(void* arg) {
    int session_id = active_sessions;  // Identificador único para a sessão
    active_sessions++;

    printf("Session %d started\n", session_id);

    // Lógica da sessão
    // Exemplo: Aguardar por algum tempo simulando o trabalho da sessão
    sleep(1000000);  // Aguardar 1 segundo

    printf("Session %d completed\n", session_id);

    // Terminar a sessão
    pthread_exit(NULL);
    (void)arg;
}

void wait_session_termination(void){
// Aguardar que uma sessão termine para criar uma nova
    while (active_sessions >= MAX_SESSIONS) {
        sleep(1000);  // Aguardar 1ms antes de verificar novamente
    }

    // Criar uma nova sessão
    pthread_create(&sessions[active_sessions], NULL, session_thread, NULL);

}

int main(int argc, char* argv[]) {
  while(1){
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

  wait_session_termination();
}

  return 0;
}
