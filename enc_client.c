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

#define HAND_REQ "ENC_HELLO"
#define HAND_REP "ENC_ACK"
#define TCP_LIMIT 1000

/**
 * Client code
 * 1. Create a socket and connect to the server specified in the command arugments.
 * 2. Prompt the user for input and send that input as a message to the server.
 * 3. Print the message received from the server and exit the program.
 */

// Error function used for reporting issues
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in *address,
                        int portNumber,
                        char *hostname)
{

    // Clear out the address struct
    memset((char *)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent *hostInfo = gethostbyname(hostname);
    if (hostInfo == NULL)
    {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char *)&address->sin_addr.s_addr,
           hostInfo->h_addr_list[0],
           hostInfo->h_length);
}

int main(int argc, char *argv[])
{
    int socketFD, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    //char buffer[256];
    // Check usage & args
    if (argc < 4)
    {
        fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
        exit(1);
    }

    //open plaintext file
    FILE *plaintextFile = fopen(argv[1], "r");
    if (plaintextFile == NULL)
    {
        fprintf(stderr, "ERROR opening plaintext file\n");
        exit(1);
    };
    //read to end to get len then rewind
    fseek(plaintextFile, 0, SEEK_END);
    long plaintextSize = ftell(plaintextFile);
    rewind(plaintextFile);


    char *plaintext = malloc(plaintextSize+1);
    memset(plaintext, '\0', plaintextSize+1);
    fread(plaintext, 1, plaintextSize, plaintextFile);
    fclose(plaintextFile);
    plaintext[strcspn(plaintext, "\n")] = '\0'; // strip newline

    // Read key file
    FILE *keyFile = fopen(argv[2], "r");
    if (keyFile == NULL)
    {
        fprintf(stderr, "enc_client: ERROR opening key file\n");
        exit(1);
    }

    //read to end to get len then rewind
    fseek(keyFile, 0, SEEK_END);
    long keySize = ftell(keyFile);
    rewind(keyFile);

    char* key = malloc(keySize+1);
    memset(key, '\0', keySize+1);
    fread(key, 1, keySize, keyFile);
    fclose(keyFile);
    key[strcspn(key, "\n")] = '\0'; // strip newline

    // Validate plaintext characters
    for (int i = 0; plaintext[i] != '\0'; i++)
    {
        if (plaintext[i] != ' ' && (plaintext[i] < 'A' || plaintext[i] > 'Z'))
        {
            fprintf(stderr, "enc_client: ERROR bad character in plaintext\n");
            exit(1);
        }
    }

    // Validate key length
    if (strlen(key) < strlen(plaintext))
    {
        fprintf(stderr, "ERROR key is too short\n");
        exit(1);
    }

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0)
    {
        error("CLIENT: ERROR opening socket");
    }

    // Set up the server address struct
    setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

    // Connect to server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        error("CLIENT: ERROR connecting");
        exit(2);
    }

    // Send hello to server
    charsWritten = send(socketFD, HAND_REQ, strlen(HAND_REQ), 0);
    if (charsWritten < 0)
    {
        error("CLIENT: ERROR sending handshake");
    }

    // Wait for acknowledgment
    char handBuffer[32];
    memset(handBuffer, '\0', sizeof(handBuffer));
    charsRead = recv(socketFD, handBuffer, strlen(HAND_REP), 0);
    if (charsRead < 0)
    {
        error("ERROR reading handshake");
    }

    // Verify the response is from enc_server
    if (strncmp(handBuffer, HAND_REP, strlen(HAND_REP)) != 0)
    {
        fprintf(stderr, "ERROR connected to wrong server on port %s\n", argv[3]);
        close(socketFD);
        exit(2);
    }
    // Send plaintext and key to server using length-prefix protocol
    int plaintextLen = strlen(plaintext);
    send(socketFD, &plaintextLen, sizeof(int), 0);
    charsWritten = send(socketFD, plaintext, strlen(plaintext), 0);
    if (charsWritten < 0)
    {
        error("ERROR writing plaintext to socket");
    }

    int keyLen = strlen(key);
    send(socketFD, &keyLen, sizeof(int), 0);
    charsWritten = send(socketFD, key, strlen(key), 0);
    if (charsWritten < 0)
    {
        error("ERROR writing key to socket");
    }
    // Receive result length
    int resultLen;
    recv(socketFD, &resultLen, sizeof(int), 0);

    // Receive encrypted result
    char *result = malloc(resultLen + 1);
    memset(result, '\0', resultLen + 1);

    int received = 0;
    while (received < resultLen)
    {
        charsRead = recv(socketFD, result + received, resultLen - received, 0);
        received += charsRead;
    }

    if (charsRead < 0)
    {
        error("ERROR reading from socket");
    }

    // Output ciphertext to stdout (can be redirected to file)
    printf("%s\n", result);

    free(result);

    close(socketFD);
    return 0;
}