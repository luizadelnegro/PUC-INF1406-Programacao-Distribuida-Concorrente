#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

/*CLIENTE MULTIPLAS REQUISICOES - uma única requisição por conexão */
#define PORT "3490" // a porta onde o cliente conectará
#define MAXDATASIZE 128 // número máximo de bytes a obter por vez
// obtém sockaddr, IPv4 ou IPv6:
void *get_in_addr(struct sockaddr *sa) {
if (sa->sa_family == AF_INET) {
return &(((struct sockaddr_in*)sa)->sin_addr);
}
return &(((struct sockaddr_in6*)sa)->sin6_addr); }
float time_diff(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) + 1e-6*(end->tv_usec - start->tv_usec);
}

int main(int argc, char *argv[]){
  struct timeval c1_start;
  struct timeval c1_end;
  int num_loop =100000;
  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p; int rv;
  char s[INET6_ADDRSTRLEN];
  if (argc != 2) {
    fprintf(stderr,"uso: cliente hostname\n"); exit(1);
  }
  memset(&hints, 0, sizeof hints); 
  hints.ai_family = AF_UNSPEC; 
  hints.ai_socktype = SOCK_STREAM;
 gettimeofday(&c1_start, NULL);
  while(num_loop>0){
    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1; 
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1){ 
        perror("cliente: socket"); 
        continue;
      }
      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        close(sockfd); 
        perror("cliente: connect"); 
        continue;
      }
      break; 
    }
    if (p == NULL) {
      fprintf(stderr, "cliente: falha em connect\n"); return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    freeaddrinfo(servinfo); 
    int num = rand() % 10;
    char snum[4];
    sprintf(snum, "%d\n", num);
    if (send(sockfd, snum, 1, 0) ==-1){
      perror("send\n");
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
      perror("recv");
      exit(1); 
    }
    buf[numbytes] = '\0';
    close(sockfd);
    num_loop--;
  }
  gettimeofday(&c1_end, NULL);
  printf("Client 1 - time spent: %0.8f sec\n", time_diff(&c1_start, &c1_end));
return 0; 
}