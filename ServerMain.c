/*Echo server using UDP*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "Structure.h"
#include <unistd.h>


#define SERVER_UDP_PORT 5000
#define MAXLEN          4096  /*maximum data length*/

long BlockNum[1] = {1};
int isLastBlock[1] = {0};
int finished[1] = {0};
long MsgSent[1] = {0};
long MAX_LENGTH[1] = {0};
char FileREC[516];

#include "Functions.c"

int main(int argc, char **argv){
  int sd, port, n;
  char buf[MAXLEN];
  int ran = 0;//used to creat new socket port number, it will renew in main loop

  struct sockaddr_storage client;
  socklen_t client_len = sizeof(client);

  struct sockaddr_in server;
  struct  hostent *hret;

  switch(argc){
  case 2:
    port = SERVER_UDP_PORT;
    break;
  case 3:
    port = atoi(argv[2]);
    break;
  default:
    fprintf(stderr, "Usage: %s [port]\n", argv[0]);
    exit(1);
  }

/*Creat a datagram socket*/
  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    perror("Cannot create client socket");
    exit(EXIT_FAILURE);
  }

/*Bind an address to the socket*/
  bzero((char *)&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  hret = gethostbyname(argv[1]);
  memcpy(&server.sin_addr.s_addr, hret->h_addr, hret->h_length);
  if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
    perror("Cannot bind socket!");
    exit(EXIT_FAILURE);
  }

  while(1){
    memset(buf, 0, sizeof(buf));
    //client_len = sizeof(client);
    ran += 10;//renew the ran

/*receive packet from client*/
    if ((n = recvfrom(sd, buf, MAXLEN, 0, (struct sockaddr*)&client, &client_len)) < 0){
      perror("Cannot receive datagram!\n");
      exit(EXIT_FAILURE);
    }

    uint16_t host, network;
    const char *tran = buf;

    memcpy((char *)&network, tran, 2);
    host = ntohs(network);
    tran += 2;

/*if client sends the RRQ packet, creat child process to handle it*/
    if (host == RRQ) {
      pid_t pid;
      pid = fork();//creat child process

      if (pid < 0) {
        perror("fork failed!");
        exit(EXIT_FAILURE);
      }

      if (pid == 0) {
          close(sd);//close the original socket
          int newFD;

	/*setup a new socket*/
          if ((newFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
	    perror("Cannot create client socket in child");
	    exit(EXIT_FAILURE);
	  }
	
	/*Bind an address to the new socket*/
	  bzero((char *)&server, sizeof(server));
	  server.sin_family = AF_INET;
	  server.sin_port = htons(port+ran);
	  hret = gethostbyname(argv[1]);
	  memcpy(&server.sin_addr.s_addr, hret->h_addr, hret->h_length);
	  if (bind(newFD, (struct sockaddr *)&server, sizeof(server)) == -1){
	    perror("Cannot bind socket in child!");
	    exit(EXIT_FAILURE);
	  }
	
	/*initialize the value*/
	  BlockNum[0]=1;
	  isLastBlock[0]=0;
	  MsgSent[0]=0;
	  MAX_LENGTH[0]=0;
	  finished[0]=0;

          Unpack(buf, n, newFD, client, client_len, 0);//handle the packet from client
      }//end of creat child process
    }	//end of "host == RRQ"

    else Unpack(buf, n, sd, client, client_len, 0);//if receive ACK, do not need to creat child process. Just handle packet
  }


  close(sd);
  return 0;
}
