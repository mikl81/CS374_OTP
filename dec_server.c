#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define HAND_REQ "DEC_HELLO"
#define HAND_REP "DEC_ACK"
#define TCP_LIMIT 1000

// Error function used for reporting issues
void error(const char *msg)
{
    perror(msg);
    exit(1);
};

char intToChar(int val)
{
    int base = 65;       // Capital A
    int modo = val % 27; // Limit to accepted chars
    int ascii;
    if (modo == 26)
    { // 26 will represent space
        ascii = 32;
    }
    else
    {
        ascii = base + modo; // Otherwise we get the character ascii value
    };

    return (char)ascii; // Cast on return
};

int charToInt(char c)
{
    int base = 65;
    int value;
    if ((int)c == 32)
    {
        value = 26; // 26 represents space
    }
    else
    {
        value = ((int)c) - 65; // subtract base (Capital A) to get val
    }

    return value;
};

char *decoder(const char *toDecode, const char *key)
{
    size_t length = strlen(toDecode);
    char *result = malloc(length + 1);
    result[length] = '\0';
    int i;

    for (i = 0; i < length; i++)
    {
        char decode_char = toDecode[i];
        int decode_val = charToInt(decode_char);
        char keyChar = key[i];
        int key_val = charToInt(keyChar);

        int combined_val = ((decode_val - key_val)+27) % 27;
        char result_char = intToChar(combined_val);

        result[i] = result_char;
    };

    return result;
};

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in *address,
                        int portNumber)
{

    // Clear out the address struct
    memset((char *)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char *argv[])
{
    int connectionSocket, charsRead;
    char buffer[TCP_LIMIT + 1];
    char inputData[256];
    char keyData[256];
    char *encryptedChars;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);
    pid_t pid;

    // Check usage & args
    if (argc < 2)
    {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0)
    {
        error("ERROR opening socket");
    }

    int opt = 1;
    if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        error("ERROR on setsockopt");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket,
             (struct sockaddr *)&serverAddress,
             sizeof(serverAddress)) < 0)
    {
        error("ERROR on binding");
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

    // Accept a connection, blocking if one is not available until one connects
    while (1)
    {
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket,
                                  (struct sockaddr *)&clientAddress,
                                  &sizeOfClientInfo);
        if (connectionSocket < 0)
        {
            error("ERROR on accept");
        }

        pid = fork();
        switch (pid)
        {
        // Failed fork
        case -1:
            error("ERROR creating fork");
            exit(0);
            break;
        // Success
        case 0:
            close(listenSocket);
            // Check handshake
            char handBuffer[32];
            memset(handBuffer, '\0', sizeof(handBuffer));
            recv(connectionSocket, handBuffer, strlen(HAND_REQ), 0);
            if (strncmp(handBuffer, HAND_REQ, strlen(HAND_REQ)) != 0)
            {
                fprintf(stderr, "ERROR: connection from wrong client type\n");
                close(connectionSocket);
                exit(1);
            }
            // Send init ACK
            send(connectionSocket, HAND_REP, strlen(HAND_REP), 0);

            // Receive plaintext
            int plaintextLen;
            recv(connectionSocket, &plaintextLen, sizeof(int), 0);
            memset(inputData, '\0', sizeof(inputData));
            recv(connectionSocket, inputData, plaintextLen, 0);

            // Receive key
            int keyLen;
            recv(connectionSocket, &keyLen, sizeof(int), 0);
            memset(keyData, '\0', sizeof(keyData));
            recv(connectionSocket, keyData, keyLen, 0);

            //Encode data
            char *result = decoder(inputData, keyData);

            // Send result back to client
            int charsWritten = send(connectionSocket, result, strlen(result), 0);
            if (charsWritten < 0)
            {
                error("ERROR writing to socket");
            }
            free(result);

            close(connectionSocket);
            exit(0);
            break;
        default:
            close(connectionSocket);
            break;
        };
    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}
