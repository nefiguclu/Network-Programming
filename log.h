#include <stdio.h>


#ifndef LOG_H
#define LOG_H

#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_DBG     3

#define LOG_LEVEL LOG_LEVEL_DBG

FILE *stream;

#define LOG(level, log_str , msg, ...)\
        do {                            \
            if( level <= LOG_LEVEL ) {   \
                if( level == LOG_LEVEL_ERROR)\
                    stream = stderr;\
                else\
                    stream = stdout; \
                fprintf(stream,"[%s]:%d:  "msg" \n",\
                log_str,__LINE__, ##__VA_ARGS__);\
            }\
        }while(0)\


#define LOG_ERR(...)    LOG(LOG_LEVEL_ERROR,"ERROR", __VA_ARGS__)
#define LOG_INFO(...)    LOG(LOG_LEVEL_INFO,"INFO", __VA_ARGS__)
#define LOG_DBG(...)    LOG(LOG_LEVEL_DBG,"DEBUG", __VA_ARGS__)


#endif