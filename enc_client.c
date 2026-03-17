/*
This program connects to enc_server, and asks it to perform a one-time pad style encryption as 
detailed above. By itself, enc_client doesn’t do the encryption - enc_server does. The syntax 
of enc_client is as follows:

enc_client plaintext key port
In this syntax, plaintext is the name of a file in the current directory that contains the 
plaintext you wish to encrypt. Similarly, key contains the encryption key you wish to use to 
encrypt the text. Finally, port is the port that enc_client should attempt to connect to 
enc_server on. When enc_client receives the ciphertext back from enc_server, it should output 
it to stdout. Thus, enc_client can be launched in any of the following methods, and should send
its output appropriately:

$ enc_client myplaintext mykey 57171
$ enc_client myplaintext mykey 57171 > myciphertext
$ enc_client myplaintext mykey 57171 > myciphertext &
If enc_client receives key or plaintext files with ANY bad characters in them, or the key file 
is shorter than the plaintext, then it should terminate, send appropriate error text to stderr, 
and set the exit value to 1.

enc_client should NOT be able to connect to dec_server, even if it tries to connect on the correct 
port - you’ll need to have the programs reject each other. If this happens, enc_client should report
the rejection to stderr and then terminate itself. In more detail: if enc_client cannot connect to 
the enc_server server, for any reason (including that it has accidentally tried to connect to the dec_server 
server), it should report this error to stderr with the attempted port, and set the exit value to 2. 
Otherwise, upon successfully running and terminating, enc_client should set the exit value to 0.

Again, any and all error text must be output to stderr (not into the plaintext or ciphertext files).
*/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

int main(int argc, char *argv[]) {
  int socketFD, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  char buffer[256];
  // Check usage & args
  if (argc < 3) { 
    fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
    exit(0); 
  } 

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[2]), argv[1]);

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }
  // Get input message from user
  printf("CLIENT: Enter text to send to the server, and then hit enter: ");
  // Clear out the buffer array
  memset(buffer, '\0', sizeof(buffer));
  // Get input from the user, trunc to buffer - 1 chars, leaving \0
  fgets(buffer, sizeof(buffer) - 1, stdin);
  // Remove the trailing \n that fgets adds
  buffer[strcspn(buffer, "\n")] = '\0'; 

  // Send message to server
  // Write to the server
  charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
  if (charsWritten < 0){
    error("CLIENT: ERROR writing to socket");
  }
  if (charsWritten < strlen(buffer)){
    printf("CLIENT: WARNING: Not all data written to socket!\n");
  }

  // Get return message from server
  // Clear out the buffer again for reuse
  memset(buffer, '\0', sizeof(buffer));
  // Read data from the socket, leaving \0 at end
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket");
  }
  printf("CLIENT: I received this from the server: \"%s\"\n", buffer);

  // Close the socket
  close(socketFD); 
  return 0;
}