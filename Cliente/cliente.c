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

/*
O cliente terá as seguintes funcionalidades/comandos:
-LIST: para listar os ficheiros no servidor
-UPLOAD <nome>: para fazer upload para o servidor do ficheiro <nome>
-DOWNLOAD <nome>: para efectuar download do ficheiro com dado <nome>
-QUIT: para terminar a ligação
*/

/*-------------- DEFINE'S e GLOBAIS -----------------*/

#define MAX 100
#define BUF_SIZE 1024
#define PORTA_CLIENTE_CACHE 9000
#define PORTA_CACHE_SERVIDOR 9002
#define PORTA_CLIENTE_SERVIDOR 9002

int fd;
char nomePasta[BUF_SIZE];

/*-------------- FUNCOES DE SOCKETS --------------*/

void escreveFicheiro(char nomeFicheironomePasta[]) {
  int fd_file, nreadsock;
  char buffer[BUF_SIZE];
  char nomeFicheiro[BUF_SIZE];
  char nomePastaAux[BUF_SIZE];
  off_t file_size, size_received;

  read(fd, &file_size, sizeof(off_t));
  printf("Tamanho ficheiro: %d\n", (int)file_size);

  strcpy(nomePastaAux, nomePasta);

  strcat(nomePastaAux, nomeFicheironomePasta);
  sprintf(nomeFicheiro, "%s", nomePastaAux);  // para ele guardar na pasta que foi criada para o cliente

  if (file_size > 0) {
    printf("Started receiving file.\n");
    if ((fd_file = open(nomeFicheiro, O_CREAT | O_WRONLY, 0600)) != -1) {  // write the file requested
      size_received = 0;
      while (size_received < file_size) {
        nreadsock = read(fd, buffer, BUF_SIZE);
        size_received += nreadsock;
        write(fd_file, buffer, nreadsock);
        // printf("Percentagem recebida: %f \r", round((int)size_received/(int)file_size)*100);
      }
      close(fd_file);
      printf("\nFicheiro recebido!\n");
    } else {
      printf("Erro ao abrir para escrever!\n");
    }
  } else {
    printf("O ficheiro nao existe no servidor!\n");
  }
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
    write(file_descriptor_client, buffer, nreadfile);
    // printf("Buffer: %s\n", buffer);
  }

  close(file_descriptor_file);
  printf("Ficheiro enviado para o servidor!\n");
}

void uploadFile(char nomeFicheiro[]) {
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

void recebeListagem() {
  int nread;
  char files[BUF_SIZE], file1[BUF_SIZE];
  char *token;
  nread = read(fd, files, BUF_SIZE - 1);
  files[nread] = '\0';

  strcpy(file1, strtok(files, "-"));

  if (strcmp(file1, "") != 0) {
    printf("\nLista de ficheiros:\n");
    printf("%s\n", file1);
    while ((token = strtok(NULL, "-"))) {
      strcpy(file1, token);
      printf("%s\n", file1);
    }
  }
}

/*-------------- FUNCOES ------------------*/

void erro(char *s) {
  perror(s);
  exit(1);
}

void closeSocket(int signal) {
  close(fd);
  printf("\nReceived Ctrl+C !! SOCKET FECHADA!\n");
  exit(0);
}

void clear_screen() {
#ifdef linux
  system("clear");
#else
  system("cls");
#endif
}

void menu() {
  int escolha;
  char nomeFicheiro[BUF_SIZE];
  char buffer[BUF_SIZE];
  int nread, aux;

  printf("\n\n-----Servidor de ficheiros-----\n");
  printf("1- Listar ficheiros\n");
  printf("2- Upload de um ficheiro\n");
  printf("3- Download de um ficheiro\n");
  printf("4- Apagar um ficheiro na pasta do servidor\n");
  printf("0- Sair\n");
  printf("Opção: ");
  fflush(stdin); /* isto é preciso? */
  scanf("%d", &escolha);
  getchar();

  switch (escolha) {
    case (1):
      // contacto direto com o servidor, vai listar todos os ficheiros na pasta
      write(fd, "1", 1);
      recebeListagem();
      menu();
      break;

    case (2):
      // funcao de upload para a cache
      bzero(nomeFicheiro, BUF_SIZE);
      write(fd, "2", 1);
      printf("Nome do ficheiro a enviar = ");
      scanf("%s", nomeFicheiro);
      getchar();
      // write(fd, nomeFicheiro, BUF_SIZE);
      uploadFile(nomeFicheiro);
      menu();
      break;

    case (3):
      // funcao de download, primeiro verifica se esta na cache e se nao estiver , copia para la e a cache manda pra o cliente
      bzero(nomeFicheiro, BUF_SIZE);
      write(fd, "3", 1);
      printf("Nome do ficheiro a receber = ");
      scanf("%s", nomeFicheiro);
      getchar();
      write(fd, nomeFicheiro, strlen(nomeFicheiro));
      escreveFicheiro(nomeFicheiro);
      menu();
      break;

    case (4):
      bzero(nomeFicheiro, BUF_SIZE);
      write(fd, "4", 1);
      printf("Nome do ficheiro a eliminar = ");
      scanf("%s", nomeFicheiro);
      getchar();
      write(fd, nomeFicheiro, strlen(nomeFicheiro));
      nread = read(fd, buffer, BUF_SIZE);
      buffer[nread] = '\0';
      aux = atoi(buffer);
      if (aux == 0) {
        printf("Ficheiro eliminado com sucesso no servidor!\n");
      } else {
        printf("Erro ao eliminar o ficheiro no servidor!\n");
      }
      menu();
      break;

    case (0):
      // funcao de exit e limpar a memoria
      write(fd, "0", 1);
      printf("\nTudo fechado e limpo! A sair...\n");
      close(fd);
      exit(0);

    default:
      clear_screen();
      printf("Erro! Verifique novamente a opção!\n");
      menu();
  }
}

int main(int argc, char *argv[]) {
  char endServer[100];
  struct sockaddr_in addr;
  struct hostent *hostPtr;

  int nread;
  char buffer[BUF_SIZE];
  struct stat st = {0};         //é preciso para criar as pastas
  signal(SIGINT, closeSocket);  // para apanhar o CTRL+C

  if (argc != 3) {
    if (argc == 2) {
      printf("Entrei no modo anonimo...\n");
    } else {
      printf("cliente <host> <user pass> (3 arguments) ou cliente <host> (2 arguments)\n");
      exit(-1);
    }
  }

  strcpy(endServer, argv[1]);

  if ((hostPtr = gethostbyname(endServer)) == 0)
    erro("Nao consegui obter endereço");

  bzero((void *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;

  if (argc == 3) {
    addr.sin_port = htons(PORTA_CLIENTE_SERVIDOR);
  } else {
    addr.sin_port = htons(PORTA_CLIENTE_CACHE);
  }

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    erro("socket");

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    erro("Connect");

  if (argc == 3) {
    // quando acede com login vai logo diretamente à cache e faz tudo sem passar pela cache e tenta encontrar o ficheiro na pasta geral ou na sua pasta

    write(fd, argv[2], 1 + strlen(argv[2]));  // vai passar o nome e pass ao servidor

    nread = read(fd, buffer, BUF_SIZE - 1);  // vai receber se é valido ou nao
    buffer[nread] = '\0';

    if (strncmp(buffer, "VALIDO", strlen("VALIDO")) == 0) {
      printf("Login aceite... Bem vindo %s!\n", strtok(argv[2], " "));

      sprintf(nomePasta, "%s/", strtok(argv[2], " "));

      if (stat(nomePasta, &st) == -1) {  // verifica se a pasta ja existe, senao cria
        mkdir(nomePasta, 0700);
      }

      menu();
    } else {
      printf("Login recusado... Good bye!\n");
      // vai sair logo em baixo e fechar a socket
    }
  } else if (argc == 2) {
    // vai entrar em contacto com a cache

    sprintf(nomePasta, "%s/", "Anonimos");

    if (stat(nomePasta, &st) == -1) {  // verifica se a pasta ja existe, senao cria
      mkdir(nomePasta, 0700);
    }

    menu();
  }

  return 0;
}