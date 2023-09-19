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
#include <sys/select.h>

#include "../log.h"

#define MAX_INPUT 4096
#define MAX_RESPONSE 4096

int connect_to_host_tcp(char *hostname,char *port);
int configure_tcp_server(char *hostname,char *port);