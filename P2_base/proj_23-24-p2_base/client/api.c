#include "api.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#define MAX_PIPE_NAME_SIZE 255
#define MAX_REQUEST_SIZE 255

int session_id = -1;  //Global variable to store the session ID (Inicializado como -1 para indicar que não há sessão ativa)
char req_pipe[MAX_PIPE_NAME_SIZE];
char resp_pipe[MAX_PIPE_NAME_SIZE];

// Defina a estrutura fora de qualquer função para torná-la acessível globalmente
struct CreateEventData {
  unsigned int event_id;
  size_t num_rows;
  size_t num_cols;
};

int send_message(int req_fd, char op_code, const void* data, size_t size) {
// Verificar se o descritor de arquivo é válido
  if (req_fd == -1) {
    perror("Invalid request file descriptor");
    return 1;
  }

  // Montar a mensagem (op_code + data)
  char message[MAX_REQUEST_SIZE];
  message[0] = op_code;
  memcpy(&message[1], data, size);

  // Escrever a mensagem no named pipe de solicitações
  ssize_t bytes_written = write(req_fd, message, MAX_REQUEST_SIZE);

  // Verificar se a escrita foi bem-sucedida
  if (bytes_written == -1) {
    perror("Error writing to request pipe");
    return 1;
  } else if (bytes_written != MAX_REQUEST_SIZE) {
    fprintf(stderr, "Incomplete write to request pipe\n");
    return 1;
  }

  return 0;
}

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  if (mkfifo(req_pipe_path, 0666) == -1) {
    perror("Error creating request pipe");
  }
  return 1;

// Create response pipe
  if (mkfifo(resp_pipe_path, 0666) == -1) {
    perror("Error creating response pipe");
    unlink(req_pipe_path); // Cleanup request pipe
    return 1;
  }

  // Open server pipe
  int server_fd = open(server_pipe_path, O_WRONLY);
  if (server_fd == -1) {
    perror("Error opening server pipe");
    unlink(req_pipe_path); // Cleanup request pipe
    unlink(resp_pipe_path); // Cleanup response pipe
    return 1;
  }

  // Send setup request to the server
  char setup_msg[MAX_REQUEST_SIZE];
  setup_msg[0] = OP_SETUP;
  strcpy(&setup_msg[1], req_pipe_path);
  strcpy(&setup_msg[41], resp_pipe_path);
  write(server_fd, setup_msg, MAX_REQUEST_SIZE);

  // Read session ID from the server
  read(server_fd, &session_id, sizeof(session_id));

  // Close server pipe
  close(server_fd);

  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  // Open request pipe
  int req_fd = open(req_pipe, O_WRONLY);
  if (req_fd == -1) {
    perror("Error opening request pipe");
    return 1;
  }

  // Send quit request to the server
  char quit_msg[MAX_REQUEST_SIZE];
  quit_msg[0] = OP_QUIT;
  write(req_fd, quit_msg, MAX_REQUEST_SIZE);

  // Close request pipe
  close(req_fd);
  //close(resp_fd);
  // Cleanup named pipes
  unlink("req_pipe");
  unlink("resp_pipe");

  return 0;
}


int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  //Implemente o envio de uma mensagem de pedido para criar um evento
    // ...

    // Exemplo: abrir named pipe de solicitações (req_pipe) para escrita
    int req_fd = open(req_pipe, O_WRONLY);
    if (req_fd == -1) {
        perror("Error opening request pipe for writing");
        return 1;
    }

  struct CreateEventData create_data = {event_id, num_rows, num_cols};

    //Envie uma mensagem de pedido para o servidor para criar um evento
    if (send_message(req_fd, OP_CREATE, &create_data, sizeof(create_data)) != 0) {
    // Lidar com erro, se necessário
    close(req_fd);
    return 1;
    }
    //Aguarde a resposta do servidor (se necessário)
    // ...

    close(req_fd);
    return 0;
  }


int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  //Implemente o envio de uma mensagem de pedido para reservar assentos
    // ...

  (void)xs;  // Indica ao compilador que a variável não é utilizada
  (void)ys; // meti isso pq estava a aparecer um warning sobre isso

    // Exemplo: abrir named pipe de solicitações (req_pipe) para escrita
    int req_fd = open(req_pipe , O_WRONLY);
    if (req_fd == -1) {
        perror("Error opening request pipe for writing");
        return 1;
    }

    struct CreateEventData reserve_data = {event_id, num_seats, 0}; // Suponha que o terceiro campo seja reservado para outros dados

    // Envie uma mensagem de pedido para o servidor para reservar assentos
    if (send_message(req_fd, OP_RESERVE, &reserve_data, sizeof(reserve_data)) != 0) {
    // Lidar com erro, se necessário
    close(req_fd);
    return 1;
  }

    //Aguarde a resposta do servidor (se necessário)
    // ...

    return 0;
}

int ems_show(int out_fd, unsigned int event_id) {
  //send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  // Implemente o envio de uma mensagem de pedido para mostrar um evento
  // ...
  (void)out_fd;
  // Exemplo: abrir named pipe de solicitações (req_pipe) para escrita
  int req_fd = open(req_pipe , O_WRONLY);
  if (req_fd == -1) {
      perror("Error opening request pipe for writing");
      return 1;
  }

    struct CreateEventData show_data = {event_id, 0, 0}; // Apenas o event_id é necessário para mostrar

  //Envie uma mensagem de pedido para o servidor para mostrar um evento
  if (send_message(req_fd, OP_SHOW, &show_data, sizeof(show_data)) != 0) {
  // Lidar com erro, se necessário
  close(req_fd);
  return 1;
  }

  //Aguarde a resposta do servidor (se necessário)
  // ...

  return 0;
}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
// TODO: Implemente o envio de uma mensagem de pedido para listar eventos
    // ...
  (void)out_fd;
    // Exemplo: abrir named pipe de solicitações (req_pipe) para escrita
    int req_fd = open(req_pipe, O_WRONLY);
    if (req_fd == -1) {
        perror("Error opening request pipe for writing");
        return 1;
    }

    struct CreateEventData list_data = {0, 0, 0}; // Nenhum dado adicional necessário para listar eventos

    //Envie uma mensagem de pedido para o servidor para listar eventos
    if (send_message(req_fd, OP_LIST_EVENTS, &list_data, sizeof(list_data)) != 0) {
    // Lidar com erro, se necessário
    close(req_fd);
    return 1;
  }

    // TODO: Aguarde a resposta do servidor (se necessário)
    // ...

    return 0;
}