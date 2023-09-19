#include <sys/stat.h>
#include <fcntl.h>

#include "util.h"


#define MAX_REQUEST 5096

struct client_info{
    int sockfd;
    struct sockaddr_storage addr;
    socklen_t addr_len;
    char data[MAX_REQUEST + 1];
    int received;
    struct client_info *next;
};

struct client_info *clients=NULL;

char *content_type(char *path);


struct client_info * wait_on_clients(int server_sockfd);
struct client_info *get_client(int client_sockfd);
void drop_client(struct client_info *client);

void send_400(struct client_info *client);
void send_404(struct client_info *client);
void serve_content(struct client_info *client,char *path);
//char *get_client_address(struct client_info * ci);


int main(int argc, char *argv[]){

    struct client_info *c;


    int server_sock=configure_tcp_server(NULL,"8088");


    printf("\n\nStarted server at 8088\n\n");

    while(1){

start:

        c=wait_on_clients(server_sock); 
        if(c){ 
            //there is a message from client
            //get and parse
            while(!strstr(c->data,"\r\n\r\n")){
                if(c->received>=MAX_REQUEST){
                    send_404(c);
                    goto start;
                }

                ssize_t bytes_recived = recv(c->sockfd, c->data, MAX_REQUEST - c->received, 0);
                if(bytes_recived<1){
                    fprintf(stderr,"connection closed by peer\n");
                    drop_client(c);
                }
                c->received+=bytes_recived;
        }

        c->data[c->received]='\0';


        if(strncmp("GET /",c->data,5)){
            send_400(c);
        }
        else{
            char *path=c->data + 4;
            char *end_path=strstr(path," ");
            if(!end_path){
                send_400(c);
            }
            else{
                *end_path='\0';
                serve_content(c,path);
            }
        }
       }

}

    close(server_sock);
    return 0;
}


struct client_info *wait_on_clients(int server_sockfd){
    
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server_sockfd,&reads);

    int max=server_sockfd;

    struct client_info *p = clients;
    
    while(p){
        FD_SET(p->sockfd,&reads);
        if(p->sockfd > max)
            max = p->sockfd;
        p = p->next;
    }

    printf("Wait on clients...\n");
    if(select(max + 1, &reads, 0, 0, 0)==-1){
        fprintf(stderr,"Select Failed %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }


    if(FD_ISSET(server_sockfd,&reads)){
        printf("New connection...\n");
        struct client_info *client=get_client(-1);
        client->sockfd=accept(server_sockfd, (struct sockaddr *)&client->addr, &client->addr_len);
        return NULL;
    }

    p=clients;
    while(p){
        if(FD_ISSET(p->sockfd,&reads)){
            return p;
        }
        p=p->next;
    }

    return NULL;
}


struct client_info *get_client(int client_sockfd){

    struct client_info *p=clients;

    while(p){
        if(p->sockfd==client_sockfd)
            return p;
        p=p->next;
    }

    struct client_info *q=(struct client_info *)calloc(1,sizeof(struct client_info));
    if(!q){
        fprintf(stderr,"calloc Failed\n");
        exit(EXIT_FAILURE); 
    }

    q->addr_len=sizeof(q->addr);
    q->next=clients;
    clients=q;

    return q;
}


void drop_client(struct client_info *client){

    struct client_info *p=clients;

    close(client->sockfd);

    printf("clients before drop\n");
    while (p)
    {
        printf("sockfd %d\n",(p)->sockfd);
        p=(p)->next;
    }


    struct client_info **pp=&clients;
    while(*pp){
        if( *pp == client){

            *pp=client->next;
            free(client);

            p=clients;
            printf("clients after drop\n");
            while (p)
            {
                printf("sockfd %d\n",(p)->sockfd);
                p=(p)->next;
            }

            return;

        }

        pp=&((*pp)->next);
    }
    
}


void serve_content(struct client_info *client,char *path){

    char fullpath[128];
    char readbuf[1024];
    char *type;
    unsigned long content_size;
    int bytes_read;
    
    if(strlen(path)>100){
        send_400(client);
        return;
    }

    if(strstr(path,"..")){
        send_404(client);
        return;
    }

    if(!strcmp(path,"/")){
        path="/index.html";
    }

    sprintf(fullpath,"public%s",path);

    LOG_ERR("FULLPATH %s",fullpath);
    
    int fd=open(fullpath,O_RDONLY);
    if(fd<0){
        fprintf(stderr,"open Failed\n");
        send_404(client);
        return;
    }

    type=content_type(fullpath);
    content_size=lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);


    sprintf(readbuf,"HTTP/1.1 200 OK\r\n"
            "Connection: close\r\n");
    send(client->sockfd,readbuf,strlen(readbuf),0);
    sprintf(readbuf,"Content-Length: %lu\r\n",content_size);
    send(client->sockfd,readbuf,strlen(readbuf),0);
    sprintf(readbuf,"Content-Type: %s\r\n",type);
    send(client->sockfd,readbuf,strlen(readbuf),0);
    sprintf(readbuf,"\r\n");
    send(client->sockfd,readbuf,strlen(readbuf),0);




    while((bytes_read=read(fd,readbuf,sizeof(readbuf)))>0){
        send(client->sockfd,readbuf,bytes_read,0);
    }

    close(fd);
    drop_client(client);

}


void send_400(struct client_info *client){

    char *c400="HTTP/1.1 400 Bad Request\r\n"
            "Connection: close\r\n"
            "Content-Length: 11\r\n\r\nBad Request";
    
    send(client->sockfd,c400,strlen(c400),0);
    drop_client(client);
}


void send_404(struct client_info *client){


    fprintf(stderr,"404\n");


    char *c404="HTTP/1.1 404 Bad Request\r\n"
            "Connection: close\r\n"
            "Content-Length: 9\r\n\r\nNot Found";
    
    send(client->sockfd,c404,strlen(c404),0);
    drop_client(client);

}

char *content_type(char *path){

    static char command[1024];

    sprintf(command, "file --mime-type ");
    sprintf(command + strlen(command), "%s", path);

    LOG_ERR("COMMAND %s",command);

    FILE *fp=popen(command,"r");

    if (fp == NULL) {
        fprintf(stderr,"popen Failed to run command\n");
        exit(EXIT_FAILURE);
    }

    fgets(command, sizeof(command), fp);

    pclose(fp);
    return strchr(command,' ');

}
