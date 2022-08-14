/*
COMPILE AND RUN

    clear && gcc -o openssl-fetch-example openssl-fetch-example.c -lssl -lcrypto && ./openssl-fetch-example
or
    clear && gcc -o openssl-fetch-example openssl-fetch-example.c `pkg-config --cflags --libs openssl` && ./openssl-fetch-example


DEPENDENCIES

sudo apt install openssl-dev

REFERENCES

https://fm4dd.com/openssl/sslconnect.shtm
https://github.com/fm4dd
*/

#include<sys/socket.h>
#include<resolv.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>

#include<openssl/bio.h>
#include<openssl/ssl.h>
#include<openssl/err.h>
#include<openssl/pem.h>
#include<openssl/x509.h>
#include<openssl/x509_vfy.h>

// Create a socket and connect it to the server over TCP.
int create_and_connect_socket(char url_str[1], BIO* out) {
    char hostname[256]={0};
    char portnum[6] = "443";
    char proto[6]={0};

    // Remove trailing slash '/' from the URL, if any.
    char* tmp_ptr = url_str + strlen(url_str);
    if (*tmp_ptr == '/') *tmp_ptr=0;

    // Extract the protocol string from the URL - the substring up to the first
    // colon ':'.
    strncpy(proto, url_str, (strchr(url_str, ':') - url_str));

    // Extract the hostname from the URL - the substring after the first "://".
    strncpy(hostname, strstr(url_str, "://") + 3, sizeof(hostname));

    // Extract the port number from the hostname, if any.
    tmp_ptr = strchr(hostname, ':');
    if (tmp_ptr) {
        strncpy(portnum, tmp_ptr + 1, sizeof(portnum));
        *tmp_ptr=0;
    }

    int port = atoi(portnum);

    struct hostent* host = gethostbyname(hostname);
    if (!host) {
        BIO_printf(out, "Error: Cannot resolve hostname %s.\n", hostname);
        exit(EXIT_FAILURE);
    }

    // Create a socket capable of TCP communication.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Create the sockaddr_in which contains the address of the server we want
    // to connect the socket to.
    struct sockaddr_in dest_addr={0};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = *(long*)(host->h_addr);

    // Convert the binary address (in network byte order) stored in the struct
    // sin_addr into numbers-and-dots notation, and store it in tmp_ptr.
    tmp_ptr = inet_ntoa(dest_addr.sin_addr);

    // Connect the socket to the server.
    if (connect(sockfd,
                (struct sockaddr*) &dest_addr,
                sizeof(struct sockaddr)) < 0) {
        BIO_printf(out, "Error: Cannot connect to host %s [%s] on port %d.\n",
                   hostname, tmp_ptr, port);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int main() {
    char* dest_url = "https://www.example.com";

    // Load algorithms and strings needed by OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();

    // Create the Input/Output BIOs
    BIO* certbio = BIO_new(BIO_s_file());
    BIO* outbio = BIO_new_fp(stdout, BIO_NOCLOSE);

    // Initialize OpenSSL.
    if (SSL_library_init() < 0)
        BIO_printf(outbio, "Could not initialize the OpenSSL library.\n");

    // The SSLv23_client_method function indicates that the application is a
    // client and supports Transport Layer Security version 1.0 (TLSv1.0),
    // Transport Layer Security version 1.1 (TLSv1.1), and Transport Layer
    // Security version 1.2 (TLSv1.2).
    const SSL_METHOD* method = SSLv23_client_method();

    // Create an SSL context.
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx)
        BIO_printf(outbio, "Unable to create SSL context.\n");

    // Disabling SSLv2 will leaving v3 and TLSv1 for negotiation.
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

    // Create a new SSL session. This does not connect the socket.
    SSL* ssl = SSL_new(ctx);

    // Create a socket and connect it to the server over TCP.
    int serverfd = create_and_connect_socket(dest_url, outbio);
    if (serverfd)
        BIO_printf(outbio,
                   "Successfully made the TCP connection to: %s.\n",
                   dest_url);

    // Attach the SSL session to the socket file descriptor
    SSL_set_fd(ssl, serverfd);

    // Connect to the server over SSL. 1 means success, while 0 or a negative
    // value means an error occurred.
    if (SSL_connect(ssl) < 1) {
        BIO_printf(outbio,
                   "Error: Could not build an SSL session to: %s.\n",
                   dest_url);
    } else {
        BIO_printf(outbio,
                   "Successfully enabled SSL/TLS session to: %s.\n",
                   dest_url);
    }

    // Save the server's TLS certificate.
    X509* cert = SSL_get_peer_certificate(ssl);

    if (!cert) {
       BIO_printf(outbio,
                  "Error: Could not get a certificate from %s.\n",
                  dest_url); 
    } else {
       BIO_printf(outbio,
                  "Retrieved the server's certificate from: %s.\n",
                  dest_url); 
    }

    // Get certificate name
    X509_NAME* certname = X509_get_subject_name(cert);

    // Print the TLS Certificate's Subject
    BIO_printf(outbio, "Displaying the certificate subject data:\n");
    X509_NAME_print_ex(outbio, certname, 0, 0);
    BIO_printf(outbio, "\n");

    // Now, follow the same pattern you would follow to write an HTTP request
    // to a TCP socket and read the response, but use SSL_read/SSL_write instead
    // of read/write.
    int n=0;
    char* message = "GET / HTTP/1.0\r\n"
                    "Host: example.com\r\n"
                    "Connection:close\r\n\r\n";
    if ((n = SSL_write(ssl, message, strlen(message))) < 0)
        perror("ERROR writing to socket.");
    char buffer[4096]={0};
    for (;;) {
        explicit_bzero(buffer, 4096);
        if ((n = SSL_read(ssl, buffer, 4095)) < 0) {
            perror("ERROR reading from socket.");
            break;
        }
        if (!n) break;
        printf("%s", buffer);
    }

    // Cleanup
    SSL_free(ssl);
    close(serverfd);
    X509_free(cert);
    SSL_CTX_free(ctx);
    BIO_printf(outbio,
               "Finished SSL/TLS connection with server: %s.\n",
               dest_url);

    return EXIT_SUCCESS;
}
////////////////////////////////////////////////////////////////////////////////
