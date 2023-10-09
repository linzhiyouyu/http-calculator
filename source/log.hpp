#ifndef __LOG_HPP__
#define __LOG_HPP__

#include<ctime>
#include<iostream>

#define INFO 0
#define DEBUG 1
#define ERROR 2
#define DEFAULT_LOG_LEVEL DEBUG

#define LOG(level, format, ...) do{\
        if (level < DEFAULT_LOG_LEVEL) break;\
        time_t t = time(NULL);\
        struct tm *ltm = localtime(&t);\
        char tmp[32] = {0};\
        strftime(tmp, 31, "%H:%M:%S", ltm);\
        fprintf(stdout, "[%p %s %s:%d] " format "\n", (void*)pthread_self(), tmp, __FILE__, __LINE__, ##__VA_ARGS__);\
    }while(0)

#define INFO_LOG(format, ...) LOG(INFO, format, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) LOG(DEBUG, format, ##__VA_ARGS__)
#define ERROR_LOG(format, ...) LOG(ERROR, format, ##__VA_ARGS__)

#endif // __LOG_HPP__