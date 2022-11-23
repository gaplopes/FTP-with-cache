#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*-------------- DEFINE'S e GLOBAIS -----------------*/

#define BUF_SIZE 1024
#define PORTA_CLIENTE_CACHE 9000
#define PORTA_CACHE_SERVIDOR 9002
#define PORTA_CLIENTE_SERVIDOR 9002

int fd_para_cliente;
int fd_para_server;
int fd;
char nomePasta[BUF_SIZE];

/*-------------- FUNCOES ---------------------------*/

void send_file2(int file_descriptor_client, int file_descriptor_file) {
  struct stat file_status;
  int nreadfile;
  char buffer[BUF_SIZE];

  fstat(file_descriptor_file, &file_status);
  write(file_descriptor_client, &file_status.st_size, sizeof(off_t));
  printf("Tamanho do ficheiro a enviar: %d\n", (int)file_status.st_size);

  while ((nreadfile = read(file_descriptor_file, buffer, BUF_SIZE)) != 0) {
    write(file_descriptor_client, buffer, nreadfile);
  }

  close(file_descriptor_file);
  printf("Ficheiro enviado para o cliente!\n");
}

void send_file(int file_descriptor_client, int file_descriptor_file, char nomeFicheiro[]) {
  struct stat file_status;
  int nreadfile;
  char buffer[BUF_SIZE];
  char tamanhoAux[BUF_SIZE];

  bzero(buffer, BUF_SIZE);

  fstat(file_descriptor_file, &file_status);

  sprintf(tamanhoAux, "%i", (int)file_status.st_size);

  strcpy(buffer, nomeFicheiro);
  strcat(buffer, " ");
  strcat(buffer, tamanhoAux);
  write(file_descriptor_client, buffer, strlen(buffer));

  bzero(buffer, BUF_SIZE);
  strcpy(buffer, "");

  usleep(1000);

  while ((nreadfile = read(file_descriptor_file, buffer, BUF_SIZE)) != 0) {
    // printf("NREADFILE: %d\n", nreadfile);
    write(file_descriptor_client, buffer, nreadfile);
  }

  close(file_descriptor_file);
  printf("Ficheiro enviado para o servidor!\n");
}

void uploadServer(int fd, char nomeFicheiro[]) {
  int fd_file;
  struct stat file_status;
  char buffer[BUF_SIZE];
  char nomePastaAux[BUF_SIZE];
  off_t res;

  strcpy(nomePastaAux, nomePasta);

  strcat(nomePastaAux, nomeFicheiro);
  sprintf(buffer, "%s", nomePastaAux);

  if ((fd_file = open(buffer, O_RDONLY)) != -1) {
    printf("Ficheiro encontrado... A enviar...\n");
    fstat(fd_file, &file_status);  // get file status to know the size
    send_file(fd, fd_file, nomeFicheiro);
  } else {
    printf("Ficheiro nao encontrado!\n");
    res = 0;
    write(fd, &res, sizeof(off_t));  // sends 0 to client
  }
  sleep(1);
}

void recebeFicheiro(int fd_client) {
  int nreadsock;
  int fd_file;
  char nomePastaAux[BUF_SIZE];
  char buffer[BUF_SIZE];
  char size[BUF_SIZE], nome[BUF_SIZE];
  off_t file_size, size_received;

  bzero(buffer, BUF_SIZE);
  bzero(nomePastaAux, BUF_SIZE);

  nreadsock = read(fd_client, buffer, BUF_SIZE - 1);
  buffer[nreadsock] = '\0';

  sscanf(buffer, "%s %s", nome, size);
  printf("Nome do ficheiro a transferir ao servidor: %s\n", nome);

  // printf("Tamanho do ficheiro a receber: %s\n",size);

  file_size = atoi(size);

  printf("Tamanho ficheiro a transferir: %d\n", (int)file_size);

  strcpy(nomePastaAux, nomePasta);
  strcat(nomePastaAux, nome);

  bzero(buffer, BUF_SIZE);

  if (file_size > 0) {
    if ((fd_file = open(nomePastaAux, O_CREAT | O_WRONLY, 0600)) != -1) {  // write the file requested
      size_received = 0;
      while (size_received < file_size) {
        bzero(buffer, BUF_SIZE);
        nreadsock = read(fd_client, buffer, BUF_SIZE);
        size_received += nreadsock;
        write(fd_file, buffer, nreadsock);
      }
      printf("Tamanho recebido pelo cliente: %d\n", (int)size_received);
      close(fd_file);
      printf("Ficheiro recebido com sucesso pela cache!\n");
      uploadServer(fd_para_server, nome);
    } else {
      printf("Erro ao abrir o ficheiro para escrita na cache!\n");
    }
  } else
    printf("Erro ao receber o ficheiro!\n");
}

void verificaCache(char nomeFicheiro[]) {
  char aux[BUF_SIZE];
  char diretoria[BUF_SIZE];
  char buffer[BUF_SIZE];
  off_t res;
  int fd_file;
  int nreadsock;
  struct stat file_status;
  off_t file_size, size_received;

  bzero(aux, BUF_SIZE);
  bzero(diretoria, BUF_SIZE);

  strcpy(diretoria, nomePasta);
  strcat(diretoria, nomeFicheiro);

  printf("DIRETORIA = %s\n", diretoria);

  if ((fd_file = open(diretoria, O_RDONLY)) != -1) {  // se estiver na cache envia logo
    printf("O ficheiro esta na cache... A iniciar download!\n");
    send_file2(fd_para_cliente, fd_file);
  } else {                          // senao vai verificar ao servidor
    write(fd_para_server, "3", 1);  // para ele inicar o 3 no servidor

    write(fd_para_server, nomeFicheiro, BUF_SIZE);

    read(fd_para_server, &file_size, sizeof(off_t));
    printf("Tamanho ficheiro: %d\n", (int)file_size);

    if (file_size > 0) {
      if ((fd_file = open(diretoria, O_CREAT | O_WRONLY, 0600)) != -1) {  // write the file requested
        size_received = 0;
        while (size_received < file_size) {
          nreadsock = read(fd_para_server, buffer, BUF_SIZE);
          size_received += nreadsock;
          write(fd_file, buffer, nreadsock);  // escreve no ficheiro
        }
        close(fd_file);
        printf("Ficheiro recebido com sucesso pelo server na cache!\n");

        if ((fd_file = open(diretoria, O_RDONLY)) != -1) {
          printf("A começar a transferencia para o cliente!\n");
          fstat(fd_file, &file_status);  // get file status to know the size
          send_file2(fd_para_cliente, fd_file);
        } else {
          printf("Erro ao tentar enviar ao cliente por parte da cache!\n");
          res = 0;
          write(fd_para_cliente, &res, sizeof(off_t));  // sends 0 to client
        }
      } else {
        printf("Erro ao abrir o ficheiro para escrita no servidor!\n");
      }
    } else {
      printf("O ficheiro nao existe no servidor nem na cache! A enviar erro ao cliente.\n");
      res = 0;
      write(fd_para_cliente, &res, sizeof(off_t));  // sends 0 to client
    }
  }
}

void recebeEReenviaListagem() {
  int nread;
  char files[BUF_SIZE];
  nread = read(fd_para_server, files, BUF_SIZE - 1);
  files[nread] = '\0';

  write(fd_para_cliente, files, BUF_SIZE - 1);
  printf("Acabei de reencaminhar a lista de ficheiros para o cliente...\n");
}

void process_client(int fd_para_cliente) {
  int nread;
  char buffer[BUF_SIZE];

  sprintf(nomePasta, "%s/", "Files");
  // printf("PASTA = %s\n", nomePasta);

  bzero(buffer, BUF_SIZE);

  strcpy(buffer, "anonimo");
  write(fd_para_server, buffer, strlen(buffer) + 1);

  while (1) {
    bzero(buffer, BUF_SIZE);

    nread = read(fd_para_cliente, buffer, BUF_SIZE - 1);
    buffer[nread] = '\0';

    if (strncmp(buffer, "1", 1) == 0) {
      bzero(buffer, BUF_SIZE);
      write(fd_para_server, "1", 1);
      // envia um ao servidor, recebe a listagem e reenvia para o cliente
      recebeEReenviaListagem();

    } else if (strncmp(buffer, "2", 1) == 0) {
      bzero(buffer, BUF_SIZE);
      write(fd_para_server, "2", 1);
      recebeFicheiro(fd_para_cliente);
    } else if (strncmp(buffer, "3", 1) == 0) {
      bzero(buffer, BUF_SIZE);
      read(fd_para_cliente, buffer, BUF_SIZE - 1);
      printf("Nome do ficheiro para download na cache = %s\n", buffer);
      // recebe o nome do ficheiro e procura na cache se existe, se nao existir vai procurar no servidor!
      //  se estiver no servidor, transfere para a cache e depois envia ao cliente
      verificaCache(buffer);
    } else if (strncmp(buffer, "4", 1) == 0) {
      bzero(buffer, BUF_SIZE);
      write(fd_para_server, "4", 1);
      read(fd_para_cliente, buffer, BUF_SIZE - 1);
      printf("Nome do ficheiro que vou tentar eliminar no servidor = %s\n", buffer);
      write(fd_para_server, buffer, strlen(buffer));
      bzero(buffer, BUF_SIZE);
      read(fd_para_server, buffer, BUF_SIZE - 1);
      write(fd_para_cliente, buffer, strlen(buffer));
    } else {
      break;
    }
  }
}

void closeSocket(int signal) {
  close(fd_para_cliente);
  close(fd_para_server);
  close(fd);
  printf("\nRecebi Ctrl+C...!\n");
  exit(0);
}

void erro(char *s) {
  perror(s);
  exit(1);
}

int main(int argc, char *argv[]) {
  char endServer[100];
  struct sockaddr_in addr_client, addr_server;
  struct hostent *hostPtr;
  int client_addr_size;

  signal(SIGINT, closeSocket);  // para apanhar o CTRL+C

  if (argc != 2) {
    printf("cache <host>\n");
    exit(-1);
  }

  // Parte da ligaçao ao servidor

  strcpy(endServer, argv[1]);

  if ((hostPtr = gethostbyname(endServer)) == 0)
    erro("Nao consegui obter endereço");

  bzero((void *)&addr_server, sizeof(addr_server));
  addr_server.sin_family = AF_INET;
  addr_server.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr_server.sin_port = htons(PORTA_CACHE_SERVIDOR);

  if ((fd_para_server = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    erro("socket");

  if (connect(fd_para_server, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0)
    erro("Connect");

  // Parte da receçao por parte do cliente

  bzero((void *)&addr_client, sizeof(addr_client));
  addr_client.sin_family = AF_INET;
  addr_client.sin_addr.s_addr = htonl(INADDR_ANY);
  addr_client.sin_port = htons(PORTA_CLIENTE_CACHE);

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    erro("na funcao socket");

  if (bind(fd, (struct sockaddr *)&addr_client, sizeof(addr_client)) < 0)
    erro("na funcao bind");

  if (listen(fd, 5) < 0)
    erro("na funcao listen");

  while (1) {
    client_addr_size = sizeof(addr_client);
    fd_para_cliente = accept(fd, (struct sockaddr *)&addr_client, (socklen_t *)&client_addr_size);
    if (fd_para_cliente > 0) {
      if (fork() == 0) {
        printf("Novo cliente anonimo em cache...\n");
        close(fd);
        process_client(fd_para_cliente);
        exit(0);
      }
      close(fd_para_cliente);
    }
  }

  exit(0);
}