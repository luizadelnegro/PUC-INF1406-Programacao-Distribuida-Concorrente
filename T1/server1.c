// O serviço a ser desenvolvido deve receber requisições com um número de 0 a 9 e responder uma string (mantida em uma tabela)de 128 bytes.
// Esse servidor deve disparar um novo processo para cada requsição recebida. Cada processo disparado deve atender a uma única requisição, fechar essa conexão com o cliente atendido e terminar execução.

/*S1 : Um único processo/thread aguarda conexão e atende todas as requisições do cliente*/
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
#include <sys/time.h>
#include <time.h>

#define PORT "3490"  // a porta que os clientes usarão para se conectarem
#define BACKLOG 10000   // a quantidade de conexões pendentes a enfileirar
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

float time_diff(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) + 1e-6*(end->tv_usec - start->tv_usec);
}


int main(void){
  struct timeval s1_start;
  struct timeval s1_end;
  int sockfd, new_fd;  // ouça em sock_fd, nova conexão em new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // informações de endereço do cliente
  socklen_t sin_size; 
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN]; 
  int rv;
  char buff[4]; 

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
  //printf("servidor: aguardando por conexões...\n");
  while(1) { 
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size); 
    if (new_fd == -1) {
      perror("accept");
      continue; 
    }
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    gettimeofday(&s1_start, NULL);

    if (!fork()) { 
      close(sockfd); 
      while (recv(new_fd, buff, 4, 0)>0){
        if (send(new_fd, "Hello, world!\n", 13, 0) ==-1){
          perror("serv send\n");   
        } 
      }
      close(new_fd);
      exit(0); 
      gettimeofday(&s1_end, NULL);
  printf("Service 1 - time spent: %0.8f sec\n", time_diff(&s1_start, &s1_end));
    }

    close(new_fd);  // pai não precisa disso
  }
return 0; 
}