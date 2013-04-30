#include <pthread.h>

void threadAboutToExit(pthread_t thread);

extern unsigned int threadCount;
extern pthread_mutex_t threadCountMutex;
