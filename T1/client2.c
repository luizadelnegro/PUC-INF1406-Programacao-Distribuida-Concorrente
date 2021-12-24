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
/*CLIENTE UNICA REQUISICAO -  conexão aberta para todo o loop de requisições */

#define PORT "3490" // a porta onde o cliente conectará
#define MAXDATASIZE 128 // número máximo de bytes a obter por vez

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
//gcc -Wall -o s1 ./server1.c
//gcc -Wall -o s2 ./server2.c
//gcc -Wall -o cli2 ./client2.c
// ./s1&
// ./s2&
//./cli2 localhost
int main(int argc, char *argv[]){
  struct timeval c2_start;
  struct timeval c2_end;
  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p; int rv;
  char s[INET6_ADDRSTRLEN];
  if (argc != 2) {
    fprintf(stderr,"uso: cliente hostname\n"); 
    exit(1);
  }
  int num_loop =100000;

  memset(&hints, 0, sizeof hints); 
  hints.ai_family = AF_UNSPEC; 
  hints.ai_socktype = SOCK_STREAM;
  

  // aqui obtem infos sobre o servidor, qual o end daquele serv?
  if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n",gai_strerror(rv));
    return 1; 
  }
// percorrer todos os resultados e
// conectar-se ao primeiro que pudermos
  //printf("Cliente se preparando para conectar \n");

  for(p = servinfo; p != NULL; p = p->ai_next) {
      //cria o socket
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) { 
      perror("cliente: socket\n"); 
      continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen)== -1) {  // conecta em um endereco q for retornado la da lista
      close(sockfd); 
      perror("cliente: connect\n"); 
      continue;
    }
    //printf("brk\n");
      break; 
  }
  if (p == NULL) {
    fprintf(stderr, "cliente: falha em connect\n");
    return 2;
  }
  freeaddrinfo(servinfo); 
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);// para onde vou me conectar
  /*LOOP COM CONEXAO ABERTA*/
 gettimeofday(&c2_start, NULL);
  while(num_loop>0){
    //printf("LOOP NUMERO :%d \n",num_loop);
    int num = rand() % 10;
    char snum[4];
    sprintf(snum, "%d", num);
    //printf("Numero gerado: %s\n",snum);
    //printf("cliente 2: conectando a %s\n", s);
    if (send(sockfd, snum, 1, 0) ==-1)//O cliente envia um número de 0 a 9 
        perror("send");
    // se a cresposta for 0 eh pq a conexao fechou entao procura outra
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
      perror("recv");
      exit(1); 
    }
   // printf("cliente 2: recebido '%s'\n",buf);
    num_loop--;
  }
  gettimeofday(&c2_end, NULL);
  printf("Client 2 - time spent: %0.8f sec\n", time_diff(&c2_start, &c2_end));
 // printf("fechando conexao");
  close(sockfd);
return 0; 
}