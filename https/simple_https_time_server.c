#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#include "../log.h"

#ifdef LOG_LEVEL
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

int main(){



    LOG_INFO("OpenSSL version %s",OpenSSL_version(SSLEAY_VERSION));

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX *ctx=SSL_CTX_new(TLS_server_method());
    if(!ctx){
        LOG_ERR("SSL_CTX_new failed");
        return -1;
    }

    if(!SSL_CTX_use_certificate_file(ctx,"certs/cert.pem",SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(ctx,"certs/key.pem",SSL_FILETYPE_PEM)){
            LOG_ERR("SSL_CTX_use_certificate_file || SSL_CTX_use_PrivateKey_file failed");
            ERR_print_errors_fp(stderr);
            return -1;
        }


    printf("Configuring local adress...\n");
    struct addrinfo hints;
    memset((void *) &hints,0,sizeof(hints));
    hints.ai_family=PF_INET6;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_PASSIVE;

    printf("\n\nStarted server at 8080\n\n");
    struct addrinfo *bind_address;
    getaddrinfo(NULL,"8080",&hints,&bind_address);

    printf("Creating socket...\n");
    int listen_socket=socket(bind_address->ai_family,bind_address->ai_socktype,bind_address->ai_protocol);
    if(listen_socket==-1){
        fprintf(stderr,"socket failed with error %d\n",errno);
        return 1;
    }

    //dual stack 
    //clear IPV6_V6ONLY opt
    int option=0;
    if(setsockopt(listen_socket,IPPROTO_IPV6,IPV6_V6ONLY,(void *)&option,sizeof(option))){
        fprintf(stderr,"setsockopt failed with error %d\n",errno);
        return 1;      
    }

    printf("Binding socket to local adress... \n");
    if(bind(listen_socket,bind_address->ai_addr,bind_address->ai_addrlen)){
        fprintf(stderr,"bind failed with error %d\n",errno);
        return 1;
    }

    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if(listen(listen_socket,10)){
        fprintf(stderr,"listen failed with error %d\n",errno);
        return 1;           
    }

    while(1){


        printf("Waiting for a connection...\n");

        struct sockaddr_storage client_address;
        socklen_t address_len=sizeof(client_address);
        int client_socket;
        client_socket=accept(listen_socket,(struct sockaddr *)&client_address,&address_len);

        if(client_socket==-1){
            fprintf(stderr,"socket failed with error %d\n",errno);
            return 1;
        }

        printf("Client is connected...");
        char buffer[NI_MAXHOST];
        if(getnameinfo((struct sockaddr *)&client_address,address_len,buffer,NI_MAXHOST,0,0,NI_NUMERICHOST)){
            fprintf(stderr,"getnameinfo failed with error %d\n",errno);
            return 1;        
        }
        printf("%s\n",buffer);



        SSL *ssl = SSL_new(ctx);
        if(!ssl){
             LOG_ERR("SSL_new failed");
            return -1;         
        }

        SSL_set_fd(ssl,client_socket);
        if(SSL_accept(ssl) == -1){
            LOG_ERR("SSL_accept failed");
            ERR_print_errors_fp(stderr);
            close(client_socket);
            SSL_shutdown(ssl);
            SSL_free(ssl);
            continue;
        }

        LOG_INFO("SSL/TLS using %s",SSL_get_cipher(ssl));



        printf("Reading request...\n");
        char request[1024];
        int bytes_received=SSL_read(ssl,request,1024);
        printf("Received %d bytes....\n ",bytes_received);
        printf("%.*s",bytes_received,request);

        printf("Sending response...\n");

        const char *response=
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Local time is: ";

        int bytes_send=SSL_write(ssl,response,strlen(response));

        time_t timer;
        time(&timer);
        char *time_msg=ctime(&timer);
        SSL_write(ssl,time_msg,strlen(time_msg));

        printf("Closing connection...\n");
        close(client_socket);

        SSL_shutdown(ssl);
        SSL_free(ssl);

    }

    printf("Closing listening socket...\n");
    close(listen_socket);


    SSL_CTX_free(ctx);

    printf("finished\n");
    return 0;
}