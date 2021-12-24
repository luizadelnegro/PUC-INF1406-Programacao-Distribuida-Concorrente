#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h> 
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "9038"   // Listening port
#define CHUNK_SIZE 64
#define MAX_ALLOWED_CLIENTS 2  // Max clients concurrently allowed
#define TIME_OUT_SECONDS 3 // Timeout in seconds
#define TIME_OUT_MICRO_SECONDS 0 // Timeout in seconds
#define MAX_AGE 5 // Max seconds without data being transmitted to each client



typedef struct {
  int socketNum; // Socket number
  long readBytes;
  time_t lastAccessed;  // The latest communication time
  FILE * file; // Nome do arquivo
} ControlClientFile;

char* which_file(char * buff){
  char prefix[]="file";
  char sufix[]=".dat";

  int length = strlen(buff) + strlen(prefix) + strlen(sufix)+1;
  
  char * new_arr = (char *) malloc(length);  
  strcpy(new_arr, prefix);  
  strcat(new_arr, buff);  
  strcat(new_arr, sufix); 
  //printf("SERVER: SELECTED FILE %s\n", new_arr);
  return new_arr;
}

int get_chunk_of_file(ControlClientFile *c, void * buffer) {
  // TODO: Verificar se precisa passar ponteiro e nao objetos em si pra modificar file e outros atributos
  //    Verificar se abrir e dar seek resolve o problema, fazendo controle externo no readBytes
  long nBytesRead = 0;
  nBytesRead = fread(buffer,sizeof(char), CHUNK_SIZE, c->file);
  c->readBytes += CHUNK_SIZE;
  strcat(buffer, "\0");
  //printf("nbytesRead: %ld   - c.readBytes: %ld \n",nBytesRead,c->readBytes);

  if (nBytesRead < CHUNK_SIZE) {
    return 1;   // EOF
  }
  return 0;
}

// get sockaddr, IPV4 or IPV6
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr); 
}

int main(void) {
  fd_set master;  // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select
  int fdmax;  // Maximum file descriptor number
  struct timeval global_timeout;
  global_timeout.tv_sec = TIME_OUT_SECONDS;
  global_timeout.tv_usec = TIME_OUT_MICRO_SECONDS;

  int listener;  // Listening socket descriptor
  int newfd;  // Newly accepted socket descriptor1

  struct sockaddr_storage remoteaddr; // client address
  socklen_t addrlen;
  int nbytes;

  char remoteIP[INET6_ADDRSTRLEN];

  int yes = 1;   // for setsockopt() SO_REUSEADDR
  int i,rv;

  struct addrinfo hints, *ai, *p;
  ControlClientFile * control_files = (ControlClientFile *) malloc(sizeof(ControlClientFile) * 0); // Controls each client status
  int connectedClients = 0;
  int activeClients = 0;

  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  // get us a socket and bind it
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
    fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
    exit(1);
  }

  for(p=ai; p!=NULL; p=p->ai_next ) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listener < 0) continue;

    // lose the pesky "address already in use" error message
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
        close(listener);
        continue;
    }
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "selectserver: failed to bind\n");
    exit(2);
  }
  freeaddrinfo(ai); // all done with this

  // listen
  if (listen(listener, 10) == -1) {
    perror("listen");
    exit(3);
  }

  // add the listener to the master set
  FD_SET(listener, &master);

  // keep track of the biggest file descriptor
  fdmax = listener; // so far, it's this one

  for(;;) {
    read_fds = master; // copy it
    int selectValue = select(fdmax+1, &read_fds, NULL, NULL, &global_timeout);
    if (selectValue == -1) {
      perror("select");
      exit(4);
    }
    else if(selectValue == 0) {
      printf("SERVER: Timeout!\n");
    }
    else {
      // run through the existing connections looking for data to read
      for(i = 0; i <= fdmax; i++) {
        if (FD_ISSET(i, &read_fds)) { // we got one!!
          if (i == listener) {
            // handle new connections
            addrlen = sizeof remoteaddr;
            newfd = accept(listener,
              (struct sockaddr *)&remoteaddr,
              &addrlen);
            
            if (newfd == -1) {
              perror("accept");
            }
            else {
              // Nova conexao
              //printf("New connection\n");
              if (activeClients < MAX_ALLOWED_CLIENTS) {
                FD_SET(newfd, &master); // ad to master set
                if (newfd > fdmax) { 
                  fdmax = newfd;
                }
                printf("selectserver: new connection from %s on socket %d\n", inet_ntop(remoteaddr.ss_family,             get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN), newfd);

                ControlClientFile ccf;
                ccf.socketNum = newfd;
                ccf.file = NULL;
                time(&ccf.lastAccessed);
                ccf.readBytes = 0;

                int found = 0;
                for(int j=0; j<connectedClients; j++) {
                  if (control_files[j].socketNum == newfd) {
                    // Reuse
                    found = 1;
                    control_files[j] = ccf;
                  }
                }
                if (found==0) {
                  control_files = (ControlClientFile *) realloc(control_files, sizeof(ControlClientFile) * (connectedClients + 1));
                  control_files[connectedClients] = ccf;
                  connectedClients += 1;
                }
                activeClients += 1;
              }
              else {
                printf("SERVER: Reached maximum simultaneous clients connected %d!\n", activeClients);
              }
            }
          }
          else {
            // handle data from a client
            ControlClientFile ccf;
            int found = -1;
            char buffer[CHUNK_SIZE+1] = "";
            char text_buffer[CHUNK_SIZE+1] = "";
            for(int j = 0; j<connectedClients; j++){
              if (control_files[j].socketNum == i){
                found = j;
                ccf = control_files[j];
              }
            }
            if (found == -1) {
              printf("Não foi possivel encontrar Controle para socket %d\n", i);
              continue;
            }
            else if ((nbytes = recv(i, buffer, sizeof buffer, 0)) <= 0) {
              // got error or connection closed by client
              if (nbytes == 0) {
                // connection closed
                printf("selectserver: socket %d hung up\n", i);
              } 
              else {
                perror("recv");
              }
              close(i); // bye!
              FD_CLR(i, &master); // remove from master set
              ccf.socketNum = -1;
            }
            else {
              // Receive data from client
              //printf("SERVER: Received data from client %d: %s\n-------\n", i, buffer);
              if (ccf.file == NULL) {
                // Needs to select file check if file valid
                if (((int) * buffer) - 48 < 0 || ((int) * buffer) - 48 > 9) {
                  // Accepts only file from 0 to 9
                  printf("SERVER: cliente mandou buffer %d de valor errado \n", (int) * buffer);
                  ccf.file=NULL;
                  // Did not open file
                  if (send(i, "not ready\0", CHUNK_SIZE, 0) == -1){
                    perror("send");
                  }//close connection
                  close(i);
                  FD_CLR(i, &master);
                  activeClients -= 1;
                  ccf.socketNum = -1;  
                }//check if number in range          
                else{
                  
                  char* filename=which_file(buffer);
                  printf("SERVER: Abrindo  arquivo %s para o client %d\n",filename,i);
                  FILE * openedFile;
                  openedFile = fopen(filename, "r");
                  if (openedFile == NULL) {
                    printf("SERVER: Nao foi possivel abrir %s\n", filename);
                  }
                  //printf("Opened File. Client %d\n", i);
                  // Sucessfully opened file
                  if (send(i, "ready\0", CHUNK_SIZE, 0) == -1){
                    perror("send");
                  }
                  ccf.file=openedFile;
                }//number in range -opening file
              }//file ==null - new connection
              else  {
                // read/send more data
                int eofb = get_chunk_of_file(&ccf, &text_buffer);
                if (send(i, text_buffer, CHUNK_SIZE, 0) == -1){
                  perror("send");
                }
                if(eofb==1) {
                  printf("FINISHED - EXITING CLIENT %d\n", i);
                  fclose(ccf.file);
                  close(i);
                  FD_CLR(i, &master);
                  activeClients -= 1;
                  ccf.socketNum = -1;
                }
                
              }//else - get chunk
            }//recieve data from client
            control_files[found] = ccf;
          }//handle data from client
        }
      }
    }
    // Reset timer
    global_timeout.tv_sec = TIME_OUT_SECONDS;
    global_timeout.tv_usec = TIME_OUT_MICRO_SECONDS;

    printf("Checking old conn\n");
    // Check for old connections
    for(int j=0; j<connectedClients; j++) {
      if(control_files[j].socketNum != -1) {
        // Active connection
        time_t now;
        time(&now);
        printf("Time %ld  max_age=%d\n", (now - control_files[j].lastAccessed), MAX_AGE);
        if ((now - control_files[j].lastAccessed) > MAX_AGE) {
          // Drop old connection
          printf("Dropping old connection %d\n", control_files[j].socketNum);
          fclose(control_files[j].file);
          FD_CLR(control_files[j].socketNum, &master);
          activeClients -= 1;
          close(control_files[j].socketNum);
          control_files[j].socketNum = -1;
        }
      }
    }
  }
} 