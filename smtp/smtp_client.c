#include "util.h"

#define MAX_INPUT 512
#define MAX_RESPONSE 1024

#ifdef LOG_LEVEL
#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif


void get_input(char *promt, char *buffer);
void send_format(int sockfd, char *text,...);
int parse_response(char *response);
void wait_response(int sockfd,int expecting);


int main(){

    LOG_INFO("usage: cat input | ./smtp_client");


    char hostname[MAX_INPUT];
    get_input("Mail Server: ",hostname);

    LOG_INFO("connecting to mail server %s",hostname);
    int serverfd=connect_to_host_tcp(hostname, "25");

    wait_response(serverfd,220);

    send_format(serverfd, "HELO TESTAGENT\r\n");
    wait_response(serverfd,250);

    char send_addr[MAX_INPUT];
    get_input("From: ",send_addr);
    send_format(serverfd, "MAIL FROM:<%s>\r\n",send_addr);
    wait_response(serverfd,250);


    char recp_addr[MAX_INPUT];
    get_input("To: ",recp_addr);
    send_format(serverfd, "RCPT TO:<%s>\r\n",recp_addr);
    wait_response(serverfd,250);

    send_format(serverfd, "DATA\r\n");
    wait_response(serverfd,354);

    char subject[MAX_INPUT];
    get_input("Subject: ",subject);
    send_format(serverfd, "From:<%s>\r\n",send_addr);
    send_format(serverfd, "To:<%s>\r\n",recp_addr);
    send_format(serverfd, "Subject:<%s>\r\n",subject);


    send_format(serverfd, "\r\n");


    LOG_INFO("--Enter E-mail body end with single \".\" line");

    while(1){
        char body[MAX_INPUT];
        get_input("> ",body);
        send_format(serverfd, "%s\r\n",body);
        if(!strcmp(body,".")){
            break;
        }
    }

    wait_response(serverfd,250);

    send_format(serverfd, "QUIT \r\n");
    wait_response(serverfd,221);

    LOG_INFO("Closing Socket .");
    close(serverfd);

    return 0;
}


void get_input(char *promt, char *buffer){

    LOG_DBG("%s", promt);
    fgets(buffer,MAX_INPUT,stdin);
    int read=strlen(buffer);
    if(read > 0){
        buffer[read-1]='\0'; //remove new line character
    }
}


void send_format(int sockfd, char *text,...){

    char buf[MAX_INPUT];
    va_list arg;
    va_start(arg, text);

    vsprintf(buf,text,arg);
    va_end(arg);

    send(sockfd,buf,strlen(buf),0);

    LOG_INFO("Client Req: %s",buf);

}


void wait_response(int sockfd,int expecting){

    char response_buf[MAX_RESPONSE+1];
    int code=0;

    char *p=response_buf;
    char *end=response_buf + MAX_RESPONSE;

    do{
        if(p==end){
            LOG_ERR("buffer overflow");
            exit(EXIT_FAILURE);
        }

        int bytes_received = recv(sockfd,response_buf,end-p,0);
        if(bytes_received<1){
            LOG_ERR("connection closed by server");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        p+=bytes_received;
        *p= '\0';

        code=parse_response(response_buf);

    }while(code == 0);


    if(code != expecting){
        LOG_ERR("Error wrong code");
        LOG_ERR("Response %s",response_buf);
        exit(EXIT_FAILURE);
    }

    LOG_INFO("Server Response: %s",response_buf);

}

int parse_response(char *response){

    char *p = response;

    if( !p[0] || !p[1] || !p[2] )
        return 0;

    for( ; p[3]; p++){
        if( p==response || p[-1]=='\n'){
            if(isdigit(p[0]) && isdigit(p[1]) && isdigit(p[2])){
                if(p[3] != '-'){
                    if(strstr(p,"\r\n")){
                        return strtol(p,NULL,10);
                    }
                }
            }
        }
    }

    return 0;
}