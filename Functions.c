void SendERR(uint16_t errcode, char *msg, char *buf, int csd, struct sockaddr_storage client, int client_len, int i);
void SendData(int csd, char *buf, struct sockaddr_storage client, int client_len, int i);



void ReadFile(char *packet, int csd, struct sockaddr_storage client, int client_len, int i){//Read the file

  FILE *infile;

  char *tran=&FileREC[0];//record the RRQ packet
  memcpy(tran, packet, strlen((char *)&packet));

  infile = fopen(FileREC+2, "rb");//open the file
  if (infile == NULL){
    char ErrMsg[]="File not found!";
    SendERR(1, ErrMsg, packet, csd, client, client_len, i);
  }
  else {
    fseek(infile, 0, SEEK_END);//seek the end of file
    MAX_LENGTH[i] = ftell(infile);
    rewind(infile);//reset the infile pointer

    int datalength = 0;

    if (MAX_LENGTH[i] < 512) datalength = MAX_LENGTH[i];//calculate the datalength to send
    else datalength = 512;
    
    char buf[datalength];

    if (fread(buf, 1, datalength, infile) != datalength){//read the file
      char ErrMsg[]="Could not copy Msg to Buf!";
      SendERR(0, ErrMsg, packet, csd, client, client_len, i);//to be finished
      fclose(infile);
    }
    else {
      fclose(infile);
      SendData(csd, buf, client, client_len, i);//to be finished
    }
  }

}

void SendOtherData(int csd, char *buf, struct sockaddr_storage client, int client_len, int i){//if file's size is larger than 511 bytes, the second and latter packet is sent by this function
  
  FILE *infile;

  infile = fopen(FileREC+2, "rb");//open the file
  if (infile == NULL){
    char ErrMsg[]="Could not open file!";
    SendERR(1, ErrMsg, FileREC, csd, client, client_len, i);
  }
  else {

    uint16_t host, network;

    char MsgBuf[516];
    char *tran = &MsgBuf[0];

    host = DATA;
    network = htons(host);
    memcpy(tran, (char *)&network, 2);
    tran += 2;

    host = BlockNum[i];
    network = htons(host);
    memcpy(tran, (char *)&network, 2);
    tran += 2;

    int datalength = 0;

    if (MAX_LENGTH[i] - MsgSent[i] < 512){//caculate the datalength of packet to be sent
      datalength = MAX_LENGTH[i] - MsgSent[i];
    }
    else datalength = 512;

    char buf[datalength];   

    fseek(infile, MsgSent[i], SEEK_SET);//move the file pointer. Here, we read the file 512B by 512B

    if (fread(buf, 1, datalength, infile) != datalength){
      char ErrMsg[]="Could not copy Msg to Buf!";
      SendERR(0, ErrMsg, FileREC, csd, client, client_len, i);
      fclose(infile);
    }
    else {
      fclose(infile);
    }

    memcpy(tran, buf, datalength);//copy the data to the payload
    tran += datalength;

    int sending;
    if ((sending = sendto(csd, MsgBuf, datalength + 4, 0, (struct sockaddr*)&client, client_len)) == -1){//send the data
      perror("Cannot sendto() client!\n");
      exit(EXIT_FAILURE);
    }

    if (datalength < 512){
      /*printf("%s\n","The last block is sent!");
      printf("%s %lu\n","MsgSent[0] =",MsgSent[0]);
      printf("%s %lu\n","MAX_LENGTH[0] =",MAX_LENGTH[0]);
      printf("%s %lu\n","BlockNum[0] =",BlockNum[0]);
      printf("datalength = %d \n", datalength);//for debug*/
      printf("%s\n","The last block is sent!");
      isLastBlock[i] = 1;
    }

  }
}




int SendACK(int csd, char *Msgbuf){//this function is used to send ACK packet
  struct ACK_Header ack;
  ack.ocode = ACK;
  ack.BlockNumber = 0;

  char MsgBuf[516];
  char *tran = &MsgBuf[0];
  
  uint16_t host,network;

  memcpy((char *)&host, (char *)&ack, 2);
  network = htons(host);
  memcpy(tran, (const char *)&network, 2);
  tran += 2;

  memcpy((char *)&host, ((char *)(&ack))+2, 2);
  network = htons(host);
  memcpy(tran, (const char *)&network, 2);
  tran += 2;

  return send(csd, MsgBuf, 4, 0);
}  




int Unpack(char *packet, const unsigned int PktLength, int csd, struct sockaddr_storage client, int client_len, int i){//used to classify the packet from client
  uint16_t host, network;
  const char *tran = packet;

  memcpy((char *)&network, tran, 2);
  host = ntohs(network);
  tran += 2;

  if (host == ACK){
    memcpy((char *)&network, tran, 2);
    host = ntohs(network);
    tran += 2;

    if (host != BlockNum[i]){//error exists in transmitting
      printf("ACK error!\n");
      return 0;
    }

    if (isLastBlock[i] == 1){
      finished[i] = 1;
    }
    else {
        MsgSent[i] += 512;
        BlockNum[i]++;
        SendOtherData(csd, packet, client, client_len, i);//received ACK, and send next block
    }
  }
  else if (host == ERR) {//read the error msg from packet
    memcpy((char *)&network, tran, 2);
    host = ntohs(network);
    tran += 2;
    printf("Error Code is %d \n", host);
    printf("Error Message: %s", packet+4);
    finished[i] = 1;
  }
  else if (host == RRQ){//handle the RRQ packet
    ReadFile(packet, csd, client, client_len, i);
  }

  return 0;
}



void SendData(int csd, char *buf, struct sockaddr_storage client, int client_len, int i){//used to send the first DATA Packet
  uint16_t host, network;
  
  char MsgBuf[516];
  char *tran = &MsgBuf[0];

  host = DATA;
  network = htons(host);
  memcpy(tran, (char *)&network, 2);
  tran += 2;

  host = BlockNum[i];
  network = htons(host);
  memcpy(tran, (char *)&network, 2);
  tran += 2;

  int datalength = 0;

  if (MAX_LENGTH[i] - MsgSent[i] < 512){//caculate the datalength of packet to be sent
    datalength = MAX_LENGTH[i] - MsgSent[i];
  }
  else datalength = 512;

  memcpy(tran, buf + MsgSent[i], datalength);//copy the data to the payload
  tran += datalength;

  int sending;
  if ((sending = sendto(csd, MsgBuf, datalength + 4, 0, (struct sockaddr*)&client, client_len)) == -1){//send the data
    perror("Cannot sendto() client!\n");
    exit(EXIT_FAILURE);
  }

  if (datalength < 512){
    printf("%s\n","The last block is sent!");
    isLastBlock[i] = 1;
    
  }

}


void SendERR(uint16_t errcode, char *msg, char *buf, int csd, struct sockaddr_storage client, int client_len, int i){//send the error packet
  uint16_t host,network;
  char *tran = buf;

  host = ERR;
  network = htons(host);
  memcpy(tran, (char *)&network, 2);
  tran += 2;

  host = errcode;
  network = htons(host);
  memcpy(tran, (char *)&network, 2);
  tran += 2;

  memcpy(tran, msg, strlen(msg));
  tran += strlen(msg);

  *tran = '\0';
  tran++;

  int sending;
  if ((sending = sendto(csd, buf, tran - buf, 0, (struct sockaddr*)&client, client_len)) == -1){//send the error packet
    perror("Cannot sendto() client!\n");
    exit(EXIT_FAILURE);
  }

  finished[i] = 1;

}
