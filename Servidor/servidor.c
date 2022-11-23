#include <ctype.h>
#include <dirent.h>
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

/*-------------- DEFINE'S -----------------*/

#define BUF_SIZE 1024
#define MAX 100

#define PORTA_SERVIDOR 9002

int fd;  // SOCKET
char nomePastaServidor[BUF_SIZE];
int flag;  // para verificar se estou em anonimo ou nao (0 se nao, 1 se sim)

/*-------------- ESTRUTURAS ---------------*/

typedef struct fnode *List;
typedef struct fnode {
  char nome[MAX];
  char pass[MAX];
  List next;
} Utilizador;

/*-------------- FUNCOES LISTAS -----------*/

List criaLista() {
  List lista = (List)malloc(sizeof(Utilizador));
  if (lista != NULL) {
    lista->next = NULL;
  }
  return lista;
}

int lista_vazia(List lista) {
  return (lista->next == NULL ? 1 : 0);
}

void destroiLista(List lista) {
  List temp_ptr;
  while (lista_vazia(lista) == 0) {
    temp_ptr = lista;
    lista = lista->next;
    free(temp_ptr);
  }
  free(lista);
}

void imprimeLista(List lista) {
  List aux = lista->next;

  while (aux) {
    printf("\nNome: %s\n", aux->nome);
    printf("Pass: %s\n", aux->pass);
    printf("\n");
    aux = aux->next;
  }
}

int permissaoLogin(List lista, char nome[], char pass[]) {
  List aux = lista;
  while (aux->next != NULL) {
    if ((strncmp(aux->nome, nome, strlen(nome)) == 0) && (strncmp(aux->pass, pass, strlen(pass)) == 0)) {
      return 0;
    }
    aux = aux->next;
  }
  return 1;
}

/*-------------- FUNCOES FICHEIROS --------*/

void send_file(int file_descriptor_client, int file_descriptor_file) {
  struct stat file_status;
  int nreadfile;
  char buffer[BUF_SIZE];

  bzero(buffer, BUF_SIZE);

  fstat(file_descriptor_file, &file_status);
  write(file_descriptor_client, &file_status.st_size, sizeof(off_t));

  while ((nreadfile = read(file_descriptor_file, buffer, BUF_SIZE)) != 0) {
    write(file_descriptor_client, buffer, nreadfile);
  }
  close(file_descriptor_file);
  printf("Ficheiro tansmitido com sucesso do servidor/cache!\n");
}

void transfereFicheiro(int cliente_fd, char nomeFicheiro[]) {
  int fd_file;
  char aux[BUF_SIZE];
  struct stat file_status;
  off_t res;

  bzero(aux, BUF_SIZE);

  strcpy(aux, nomePastaServidor);
  strcat(aux, nomeFicheiro);

  printf("AUX = %s\n", aux);

  if ((fd_file = open(aux, O_RDONLY)) != -1) {
    printf("O ficheiro existe... A começar a transferencia!\n");
    fstat(fd_file, &file_status);  // get file status to know the size
    send_file(cliente_fd, fd_file);
  } else {
    printf("O ficheiro nao existe no servidor! A enviar erro ao cliente.\n");
    res = 0;
    write(cliente_fd, &res, sizeof(off_t));  // sends 0 to client
  }
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
  printf("Nome do ficheiro a receber: %s\n", nome);
  printf("Tamanho do ficheiro a receber: %s\n", size);

  file_size = atoi(size);

  strcpy(nomePastaAux, nomePastaServidor);
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
        // printf("Percentagem recebida: %f \r", round((int)size_received/(int)file_size)*100);
      }
      printf("\nTamanho recebido: %d\n", (int)size_received);
      close(fd_file);
      printf("Ficheiro recebido com sucesso pelo servidor!\n");
    } else {
      printf("Erro ao abrir o ficheiro para escrita no servidor!\n");
    }
  } else
    printf("Erro ao receber o ficheiro!\n");
}

void eliminaFicheiro(int fd_client, char nomeFicheiro[]) {
  char nomePastaAux[BUF_SIZE];
  char buffer[BUF_SIZE];
  int r;

  strcpy(nomePastaAux, nomePastaServidor);
  strcat(nomePastaAux, nomeFicheiro);

  // printf("nomePastaAux = %s\n", nomePastaAux);
  r = remove(nomePastaAux);

  if (r == 0) {
    strcpy(buffer, "0");
    write(fd_client, buffer, strlen(buffer));
    printf("Ficheiro eliminado com sucesso!\n");
  } else {
    strcpy(buffer, "1");
    write(fd_client, buffer, strlen(buffer));
    printf("Erro ao eliminar o ficheiro!\n");
  }
}

/*-------------- FUNCOES ------------------*/

void erro(char *s) {
  perror(s);
  exit(1);
}

void carregaUtilizadores(List lista) {
  char line[MAX];
  char userFP[MAX], passFP[MAX];
  List aux = lista;
  List novo;

  FILE *fp = fopen("utilizadores.txt", "r");

  while (fgets(line, 80, fp) != NULL) {
    novo = (List)malloc(sizeof(Utilizador));

    sscanf(line, "%s %s", userFP, passFP);
    strcpy(novo->nome, userFP);
    strcpy(novo->pass, passFP);

    novo->next = NULL;
    aux->next = novo;
    aux = aux->next;
  }

  fclose(fp);
}

int listaFicheiros(int cliente_fd) {
  DIR *dir;
  char diretoria[BUF_SIZE], files[BUF_SIZE];
  ;
  struct dirent *lsdir;
  strcpy(diretoria, "/home/user/Desktop/MiniProjeto2/Servidor/");
  strcat(diretoria, nomePastaServidor);  // para ele ler os ficheiros apenas do cliente na diretoria

  dir = opendir(diretoria);
  /* print all the files and directories within directory */
  strcpy(files, "");
  while ((lsdir = readdir(dir)) != NULL) {
    strcat(files, lsdir->d_name);
    strcat(files, "-");
  }
  write(cliente_fd, files, strlen(files));
  printf("Lista de ficheiros enviada com sucesso!\n");

  closedir(dir);
  return 0;
}

void process_client(int client_fd) {
  int nread = 0;
  char buffer[BUF_SIZE], bufferAux[BUF_SIZE];
  char nome[MAX], pass[MAX];
  struct stat st = {0};  //é preciso para criar as pastas
  List lista = criaLista();

  carregaUtilizadores(lista);  // vai carregar o ficheiro para lista

  // imprimeLista(lista);

  nread = read(client_fd, buffer, BUF_SIZE - 1);  // recebe o nome do utilizador
  buffer[nread] = '\0';

  if (strncmp(buffer, "anonimo", strlen("anonimo")) == 0) {  // no caso de utilizador sem conta

    flag = 0;  // porque estou em anonimo

    strcpy(nomePastaServidor, "Gerais");
    strcat(nomePastaServidor, "/");  // vai guardar para ficarmos com o nome da pasta geral acedida pelos anonimos

    if (stat(nomePastaServidor, &st) == -1) {  // verifica se a pasta ja existe, senao cria
      mkdir(nomePastaServidor, 0700);
    }

    while (1) {
      nread = read(client_fd, buffer, BUF_SIZE - 1);
      buffer[nread] = '\0';

      if (strncmp(buffer, "1", 1) == 0) {
        listaFicheiros(client_fd);
      } else if (strncmp(buffer, "2", 1) == 0) {
        bzero(buffer, BUF_SIZE);
        // nread = read(client_fd, buffer, BUF_SIZE-1);
        // buffer[nread] = '\0';
        recebeFicheiro(client_fd);
      } else if (strncmp(buffer, "3", 1) == 0) {
        bzero(buffer, BUF_SIZE);
        read(client_fd, buffer, BUF_SIZE - 1);
        printf("Nome do ficheiro a procurar = %s\n", buffer);
        transfereFicheiro(client_fd, buffer);
      } else if (strncmp(buffer, "4", 1) == 0) {
        bzero(buffer, BUF_SIZE);
        read(client_fd, buffer, BUF_SIZE - 1);
        printf("Nome do ficheiro a eliminar = %s\n", buffer);
        eliminaFicheiro(client_fd, buffer);
      } else {
        break;
      }
    }
  }

  else {  // no caso de login

    flag = 1;  // flag a 1 porque estou com login

    sscanf(buffer, "%s %s", nome, pass);

    if (permissaoLogin(lista, nome, pass) == 0) {
      strcpy(nomePastaServidor, nome);
      strcat(nomePastaServidor, "/");  // vai guardar o nome do utilizador para so ir à pasta dele buscar os ficheiros dele

      if (stat(nomePastaServidor, &st) == -1) {  // verifica se a pasta ja existe, se nao cria
        mkdir(nomePastaServidor, 0700);
      }

      strcpy(bufferAux, "VALIDO");
      write(client_fd, bufferAux, 1 + strlen(bufferAux));

      strcpy(buffer, "");  // vai colocar a null todo o array

      while (1) {
        nread = read(client_fd, buffer, BUF_SIZE - 1);
        buffer[nread] = '\0';

        if (strncmp(buffer, "1", 1) == 0) {
          listaFicheiros(client_fd);
        } else if (strncmp(buffer, "2", 1) == 0) {
          bzero(buffer, BUF_SIZE);
          recebeFicheiro(client_fd);
        } else if (strncmp(buffer, "3", 1) == 0) {
          bzero(buffer, BUF_SIZE);
          read(client_fd, buffer, BUF_SIZE - 1);
          printf("Nome do ficheiro para download = %s\n", buffer);
          transfereFicheiro(client_fd, buffer);
        } else if (strncmp(buffer, "4", 1) == 0) {
          bzero(buffer, BUF_SIZE);
          read(client_fd, buffer, BUF_SIZE - 1);
          printf("Nome do ficheiro a eliminar = %s\n", buffer);
          eliminaFicheiro(client_fd, buffer);
        } else {
          break;
        }
      }
    }

    else {
      strcpy(bufferAux, "INVALIDO");
      write(client_fd, bufferAux, 1 + strlen(bufferAux));
    }
  }
}

void closeSocket(int signal) {
  close(fd);
  printf("\nRecebi Ctrl+C... Socket fechada e lista apagada!\n");
  exit(0);
}

int main(int argc, char *argv[]) {
  int client;
  struct sockaddr_in addr, client_addr;
  int client_addr_size;

  signal(SIGINT, closeSocket);

  bzero((void *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(PORTA_SERVIDOR);

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    erro("na funcao socket");

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    erro("na funcao bind");

  if (listen(fd, 5) < 0)
    erro("na funcao listen");

  while (1) {
    client_addr_size = sizeof(client_addr);
    client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
    if (client > 0) {
      if (fork() == 0) {
        close(fd);
        printf("Novo cliente no servidor!\n");
        process_client(client);
        exit(0);
      }
      close(client);
    }
  }

  close(fd);
  return 0;
}
