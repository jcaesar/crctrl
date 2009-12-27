
#include "StatusLocks.h"

StatusLocks::StatusLocks() : lockcount(0) {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&statuslock, &attr);
	pthread_mutex_init(&statuslocklock, &attr);
	pthread_mutexattr_destroy(&attr);
}

StatusLocks::~StatusLocks() {
	pthread_mutex_lock(&statuslocklock);
	pthread_mutex_lock(&statuslock);
	pthread_mutex_destroy(&statuslock);
	pthread_mutex_destroy(&statuslocklock);
}