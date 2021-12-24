 /* 
    * Creates multiples clients 
*/

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
#include <time.h>
#include <stdlib.h>

#define PORT "9038" // a porta onde o cliente conectará
#define MAXDATASIZE 64 // número máximo de bytes a obter por vez

// obtém sockaddr, IPv4 ou IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr); 
}

int send_random_number(int sockfd,int client_id) {
  /*gera numero aleatorio de 0 a 9 e envia*/
  int num = rand() % 10;
  printf("Client %d : sending random number %d\n", client_id, num);
  char cnum = (char) num;
  sprintf(&cnum, "%d\n", num);
  if (send(sockfd, &cnum, 1, 0) ==-1){
    perror("send\n");
  }
  return num;
}

int create_client(int client_id, char * hostname) {
  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];
  char all_concat[1000];
  int selected_num;
  srand(getpid() ^ (time(NULL)<<8));
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "Client %d - getaddrinfo: %d\n", client_id, gai_strerror(rv));
    return 1;
  }
  // percorrer todos os resultados e
  // conectar-se ao primeiro que pudermos
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("cliente: socket");
      continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen)== -1) {
      close(sockfd);
      perror("cliente: connect");
      continue;
    }
    break;
  }
  if (p == NULL) {
    fprintf(stderr, "Client %d - falha em connect\n", client_id);
    return 2;
  }
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
  printf("Client %d - cliente: conectando a %s\n", client_id, s);
  freeaddrinfo(servinfo); // tudo feito com essa estrutura

  selected_num = send_random_number(sockfd,client_id);
  /* Recebe os chunks */
  int status = 0;   // 0 = Not Ready, 1 = Ready
  while ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0) {
    buf[numbytes] = '\0';
    if(status == 0) {
      if (strcmp("ready", buf) == 0){
        printf("Client %d - Received ready.\n", client_id);
        status = 1;
      }
      else {
        printf("CLIENT %d - Not ready %s\n", client_id, buf);
        close(sockfd);
        exit(1);
        return 0;
      }
    }
    else {
      const char * next_chunk = "next chunk\0";
     // printf("CLIENT %d -- Received data: %s\n----\n", client_id, buf);
      if(strcmp("", buf) == 0) {
        //printf("RECEIVED NOTHING ---- END \n");
        close(sockfd);
        break;
      }
      strcat(all_concat, buf);
      if (send(sockfd, next_chunk, strlen(next_chunk), 0) ==-1){
        perror("send\n");
      }
      sleep(1);
    }
  }
  printf("--- CLIENT %d: Finished reading file contents: %s\n----------------", client_id, all_concat);
  close(sockfd);
  return 0;
}

int main(int argc, char *argv[]){
  int i;
  char hostname[] = "localhost";
  for(i=0; i<5; i++){
    if(fork() == 0){
      // On child create client and exit loop
      printf("Creating client %d\n", i);
      create_client(i, hostname);
      printf("Exited client %d\n", i);
      break;
    }
    // else: On parent, continue loop to create more clients
  }
  return 0;
}