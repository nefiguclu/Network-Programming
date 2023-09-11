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
#include <stdarg.h>
#include <ctype.h>

#include "../log.h"

#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#ifdef LOG_LEVEL
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

int connect_to_host(char *hostname, char *port);
void send_request(SSL* ssl, char *hostname, char *port, char *path);

int main(int argc, char *argv[]){

    if(argc < 3){
        LOG_ERR("Usage: https_client hostname port");
        return -1;
    }

    LOG_INFO("OpenSSL version %s",OpenSSL_version(SSLEAY_VERSION));

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX *ctx=SSL_CTX_new(TLS_client_method());
    if(!ctx){
        LOG_ERR("SSL_CTX_new failed");
        return -1;
    }

    if(!SSL_CTX_load_verify_locations(ctx,NULL,"/etc/ssl/certs")){
        LOG_ERR("SSL_CTX_load_verify_locations failed");
        return -1;  
    }

    int server = connect_to_host(argv[1],argv[2]);


    SSL *ssl = SSL_new(ctx);
    if(!ssl){
         LOG_ERR("SSL_new failed");
        return -1;         
    }

    //for SNI(Server name identification)
    if(!SSL_set_tlsext_host_name(ssl,argv[1])){
        LOG_ERR("SSL_set_tlsext_host_name failed");
        ERR_print_errors_fp(stderr);
        return -1;
    }

    SSL_set_fd(ssl,server);
    if(SSL_connect(ssl) == -1){
        LOG_ERR("SSL_connect failed");
        ERR_print_errors_fp(stderr);
        return -1;       
    }

    LOG_INFO("SSL/TLS using %s",SSL_get_cipher(ssl));


    X509 *cert = SSL_get_peer_certificate(ssl);
    if(!cert){
        LOG_ERR("SSL_get_peer_certificate failed");
        ERR_print_errors_fp(stderr);      
    }

    char *tmp;
    if(tmp = X509_NAME_oneline(X509_get_subject_name(cert),0,0)){
        LOG_INFO("Subject %s!",tmp);
        OPENSSL_free(tmp);
    }

    if(tmp = X509_NAME_oneline(X509_get_issuer_name(cert),0,0)){
        LOG_INFO("issuer_name %s!",tmp);
        OPENSSL_free(tmp);
    }

    X509_free(cert);


    long verf= SSL_get_verify_result(ssl);
    if(verf == X509_V_OK)
        LOG_INFO("Certificates verified!");
    else{
        LOG_ERR("Certificates NOT verified!");
        return -1;
    }

    send_request(ssl, argv[1],argv[2], "/");

    while(1){

        char buffer[2048];

        int bytes_recv = SSL_read(ssl,buffer,2048);
        if (bytes_recv < 1){
            LOG_ERR("connection closed by peer!");
            break;
        }

        LOG_INFO("recived %d bytes \n %.*s",bytes_recv,bytes_recv,buffer);

    }

    LOG_INFO("Closing socket");

    SSL_shutdown(ssl);
    close(server);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    
    return 0;

}


int connect_to_host(char *hostname, char *port) {


    struct addrinfo hints;
    struct addrinfo *peer_adress;
    
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags= AI_ALL | AI_ADDRCONFIG;

    if(getaddrinfo(hostname,port,&hints,&peer_adress)){
        LOG_ERR("getaddrinfo failed %s",strerror(errno));
        exit(EXIT_FAILURE);
    }

    char host[254];
    char serv[254];

    if(getnameinfo(peer_adress->ai_addr,peer_adress->ai_addrlen,host,sizeof(host),serv,sizeof(serv),NI_NUMERICHOST | NI_NUMERICSERV)){
        LOG_ERR("getaddrinfo failed %s",strerror(errno));
    }

    LOG_INFO("hostname: %s",host);
    LOG_INFO("service: %s",serv);


    int sockfd=socket(peer_adress->ai_family,peer_adress->ai_socktype,peer_adress->ai_protocol);
    if(sockfd<0){
        LOG_ERR("socket failed %s",strerror(errno));
        exit(EXIT_FAILURE);

    }

    if(connect(sockfd,peer_adress->ai_addr,peer_adress->ai_addrlen)){
        LOG_ERR("connect failed %s",strerror(errno));
        exit(EXIT_FAILURE);

    }

    LOG_INFO("Connected...%s",host);


    return sockfd;

}


void send_request(SSL* ssl, char *hostname, char *port, char *path){
    
    char buffer[2048];


    sprintf(buffer,"GET %s HTTP/1.1\r\n",path);
    sprintf(buffer + strlen(buffer),"Host: %s:%s\r\n",hostname,port);
    sprintf(buffer + strlen(buffer),"Connection: close\r\n");
    sprintf(buffer + strlen(buffer),"\r\n");

    SSL_write(ssl,buffer,strlen(buffer));

    LOG_INFO("Send header:\n%s",buffer);


}



