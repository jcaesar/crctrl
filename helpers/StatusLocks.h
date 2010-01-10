#ifndef StatusLocksHpp
#define StatusLocksHpp

#include <pthread.h>

//Note: Ihateyouihateyouihateyou. 
//I just can't figure out how to build it that it will allow what I want it to do, without allocating memory in LockStatus-Calls
//I should better implement the fast version. Collisions won't appear for my code anyway, so this is a lot slower than it could be.

class StatusLocks {
	private:
		pthread_mutex_t statuslock;
		pthread_cond_t statuscond;
		struct ReaderInfo {
			pthread_t thread;
			unsigned int count;
			ReaderInfo * next;
		} readerinfo;
		unsigned int writercount;
		pthread_t writer;
	protected:
		void StatusUnstable();
		void StatusStable();
	public:
		void LockStatus();
		void UnlockStatus();
		StatusLocks();
		~StatusLocks();
};

#endif