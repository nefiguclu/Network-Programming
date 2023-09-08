#define _GNU_SOURCE 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



int main(int argc,char *argv[]){

    struct ifaddrs *ifaddr;
    int family;
    char buf[NI_MAXHOST];


    if(getifaddrs(&ifaddr)){
        printf("getifaddrs failed! \n");
        return -1;
    }

    struct ifaddrs *addrs  = ifaddr ;



    while(addrs){

        if (addrs->ifa_addr == NULL)
            continue;

        family = addrs->ifa_addr->sa_family;
        if( family == AF_INET || family == AF_INET6 ){
            printf("%s \t",addrs->ifa_name);
            printf("%s \t",family == AF_INET6 ? "IPv6" :"IPv4");

            getnameinfo(addrs->ifa_addr,
                        (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), buf, NI_MAXHOST, 0, 0, NI_NUMERICHOST);

            printf("%s \t \n",buf);
        }

        addrs = addrs->ifa_next;

    }

    freeifaddrs(ifaddr);

    return 0;
}