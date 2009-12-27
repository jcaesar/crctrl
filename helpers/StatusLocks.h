#ifndef StatusLocksHpp
#define StatusLocksHpp

#include <pthread.h>

class StatusLocks {
	private:
		pthread_mutex_t statuslock;
		pthread_mutex_t statuslocklock;
		unsigned int lockcount; //positive for read locks, not used with write locks
	protected:
		inline void StatusUnstable() {
			pthread_mutex_lock(&statuslock);
		}
		inline void StatusStable()  {
			pthread_mutex_unlock(&statuslock);
		}
	public:
		inline void LockStatus() {
			pthread_mutex_lock(&statuslocklock);
			if(!lockcount) pthread_mutex_lock(&statuslock);
			lockcount++;
			pthread_mutex_unlock(&statuslocklock);
		}
		inline void UnlockStatus() {
			pthread_mutex_lock(&statuslocklock);
			if(!--lockcount) pthread_mutex_unlock(&statuslock);
			pthread_mutex_unlock(&statuslocklock);
		}
		StatusLocks();
		~StatusLocks();
};

#endif