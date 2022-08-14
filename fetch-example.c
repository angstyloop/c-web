////////////////////////////////////////////////////////////////////////////////
/*
COMPILE
clear && gcc fetch-example.c -o fetch-example && ./fetch-example

REFERENCES
https://www.gnu.org/software/libc/manual/html_node/Inet-Example.html
*/

// Surprisingly all you need on Ubuntu 22.04 with the build-essential package
// installed.
#include<stdlib.h>
#include<stdio.h>
#include<netdb.h>
#include<unistd.h>
#include<string.h>

// Create a socket of type SOCK_STREAM, which is conventionally used for TCP
// communication, and get the socket file descriptor that uniquely identifies
// the socket as a file on the unix system.
//
// PF_INET and AF_INET are basically synonyms:
// https://stackoverflow.com/questions/6729366/what-is-the-difference-between-af-inet-and-pf-inet-in-socket-programming
//
int make_tcp_socket() {
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket.");
        exit(EXIT_FAILURE); // Non-zero exit code. 
    }
    return sockfd;
}

// Close the socket when we are done with it.
void close_socket(int sockfd) {
    close(sockfd);
}

// Fill in a sockaddr_in structure, which represents the network address of
// the socket we want to connect to.
void init_socket_address(struct sockaddr_in* addr,
                         const char* hostname,
                         uint16_t port)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    struct hostent* host = gethostbyname(hostname);
    if (!host) {
        perror("Unknown host.");
        exit(EXIT_FAILURE);
    }
    addr->sin_addr = *(struct in_addr *) host->h_addr;
}


// Connect a socket to a server using a sockaddr_in struct.
//
// Note the cryptic casting from sockaddr_in* to sockaddr* that has mystified
// socket programmers for generations.
void connect_socket (struct sockaddr_in* addr, int sockfd) {
    if (connect(sockfd, (struct sockaddr *) addr, sizeof(struct sockaddr)) < 0) {
        perror("Unable to connect.");
        exit(EXIT_FAILURE);
    }
}

// Write a message to a socket.
void write_socket(int sockfd, const char message[1]) {
    // Write the HTTP request to the socket.
    int n=0;
    if ((n = write(sockfd, message, strlen(message))) < 0)
        perror("ERROR writing to socket.");
}

// Read from a socket and write to standard output until a read returns zero bytes.
// Note that read will wait for 0 length datagrams instead of consuming them like
// recv does by default.

// Buffer size for read_socket.
#define BUFSIZE 4096

void read_socket(int sockfd) {
    // The buffer we will use to receive bytes. Copy data into this in a loop
    // until the server closes the socket. We will always leave the last byte
    // as zero so that we can easily print the buffer like any other
    // null-terminated string.
    char buffer[BUFSIZE]={0};

    // For the return value of read.
    int n=0;

    // Loop until reading from the socket gives no bytes, meaning the server
    // has closed the socket.
    for (;;) {
        // Securely zero the buffer.
        explicit_bzero(buffer, BUFSIZE);

        // Read BUFSIZE-1 bytes into the buffer. The last byte needs to remain
        // zero so that the buffer can be treated as a null-terminated string.
        if ((n = read(sockfd, buffer, BUFSIZE-1)) < 0) {
            perror("ERROR reading from socket");
            break;
        }

        // If no bytes were read, the server has closed the socket, so we can
        // end the loop.
        if (!n) break;

        // This is how you would print the length of the string read into the
        // buffer after each read.
        // printf("%d\n", (int)strlen(buffer));

        // Print the buffer contents to standard output (STDOUT).
        printf("%s", buffer);
    }
}

int main() {    
    // Create a hostent struct from the DNS hostname.
    char *hostname = "example.com";

    // Choose Port 80, the standard Port for HTTP traffic.
    int port=80;

    // Create a sockaddr_in struct, which holds the network address of the
    // server we want to connect to.
    struct sockaddr_in addr={0};
    init_socket_address(&addr, hostname, port);

    // Create a new socket for TCP communication.
    int sockfd = make_tcp_socket();

    // Connect the socket to the address.
    connect_socket(&addr, sockfd);

    // Form the actual HTTP request. Look familiar?
    char* message = "GET / HTTP/1.0\r\n"
                    "Host: example.com\r\n"
                    "Connection: close\r\n\r\n";

    // Write the HTTP request to the socket.
    write_socket(sockfd, message);

    // Read the HTTP response from the server to standard output.
    read_socket(sockfd);

    // Close the socket.
    close_socket(sockfd);

    // Better than return 0.
    return EXIT_SUCCESS;
}
////////////////////////////////////////////////////////////////////////////////
