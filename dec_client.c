#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()

#define HAND_REQ "DEC_HELLO"
#define HAND_REP "DEC_ACK"
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
        fprintf(stderr, "USAGE: %s encryptedText key port\n", argv[0]);
        exit(1);
    }

    //open encryptedText file
    FILE *encryptedFile = fopen(argv[1], "r");
    if (encryptedFile == NULL)
    {
        fprintf(stderr, "ERROR opening encryptedText file\n");
        exit(1);
    };
    //read to end to get len then rewind
    fseek(encryptedFile, 0, SEEK_END);
    long encryptedLen = ftell(encryptedFile);
    rewind(encryptedFile);


    char *encryptedText = malloc(encryptedLen+1);
    memset(encryptedText, '\0', encryptedLen+1);
    fread(encryptedText, 1, encryptedLen, encryptedFile);
    fclose(encryptedFile);
    encryptedText[strcspn(encryptedText, "\n")] = '\0'; // strip newline

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

    // Validate encryptedText characters
    for (int i = 0; encryptedText[i] != '\0'; i++)
    {
        if (encryptedText[i] != ' ' && (encryptedText[i] < 'A' || encryptedText[i] > 'Z'))
        {
            fprintf(stderr, "dec_client error: input contains bad characters\n");
            exit(1);
        }
    }

    // Validate key length
    if (strlen(key) < strlen(encryptedText))
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
    // Send encrypted text and key to server using length-prefix protocol
    int encryptedTextLen = strlen(encryptedText);
    send(socketFD, &encryptedTextLen, sizeof(int), 0);
    charsWritten = send(socketFD, encryptedText, strlen(encryptedText), 0);
    if (charsWritten < 0)
    {
        error("ERROR writing encryptedText to socket");
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

    // Receive decrypted result
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

    // Output encryptedText to stdout (can be redirected to file)
    printf("%s\n", result);

    free(result);

    close(socketFD);
    return 0;
}