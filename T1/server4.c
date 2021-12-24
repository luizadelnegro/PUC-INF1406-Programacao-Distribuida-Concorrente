// O serviço a ser desenvolvido deve receber requisições com um número de 0 a 9 e responder uma string (mantida em uma tabela)de 128 bytes.
// Esse servidor deve disparar um novo thread para cada requsição recebida. Cada thread disparado deve atender a uma única requisição, fechar essa conexão com o cliente atendido e terminar execução.
//S4: servidor concorrente sob demanda com threads: A cada requisição, o servidor gera um novo thread para atendimento de uma única requisição do cliente. O novo thread fecha a conexão depois de atender uma requisição e finaliza execução.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#define PORT "3490"  // a porta que os clientes usarão para se conectarem
#define BACKLOG 10   // a quantidade de conexões pendentes a enfileirar
void sigchld_handler(int s) {// waitpid() pode sobrescrever errno, então nós salvamos e restauramos:
  int saved_errno = errno;
  while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// obtém sockaddr, IPv4 ou IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr); 
}

void *create_thread(int new_fd, int sockfd){
  close(sockfd);
  if (send(new_fd, "Hello, world!\n", 13, 0) ==-1){
          perror("serv send\n");
          close(new_fd);
          exit(0); 
        }
        close(new_fd);
        exit(0);
}

int main(void){
  int sockfd, new_fd;  // ouça em sock_fd, nova conexão em new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // informações de endereço do cliente
  socklen_t sin_size; 
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN]; 
  int rv;
  char buff[4];
  pthread_t thid;

  memset(&hints, 0, sizeof hints); hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM; hints.ai_flags = AI_PASSIVE; // use meu IP
  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1; 
  }
// loop através de todos os resultados e
// fazer bind para o primeiro que pudermos 
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) { 
      perror("servidor: socket"); 
      continue;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
      sizeof(int)) == -1) { 
        perror("setsockopt"); exit(1);
    }
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd); perror("servidor: bind"); continue;
    } 
    break;
  }
  freeaddrinfo(servinfo); // tudo feito com essa estrutura
  if (p == NULL) {
    fprintf(stderr, "servidor: falha com bind\n"); exit(1);
  }
  if (listen(sockfd, BACKLOG) == -1) { perror("listen");
    exit(1);
  }
  sa.sa_handler = sigchld_handler; // colher todos os processos mortos
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1); 
  }
  printf("servidor: aguardando por conexões...\n");
  while(1) { // loop principal de accept() sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size); 
    if (new_fd == -1) {
      perror("accept");
      continue; 
    }
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    printf("servidor: tenho uma conexão de %s\n", s);

    while (recv(new_fd, buff, 4, 0)>0){
    
      if (pthread_create(&thid, NULL, create_thread(new_fd,sockfd), "thread 1") != 0) {
        perror("pthread_create() error");
        exit(1);
      }

    }

   // close(new_fd);  // pai não precisa disso
  }
return 0; 
}