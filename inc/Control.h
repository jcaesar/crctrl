#ifndef ControlH
#define ControlH

#include "AutoHost.h"
#include "Config.h"
#include <pthread.h>

class StreamControl{
	private:
		void Work();
		static void * ThreadWrapper(void *);
		StreamReader * sr;
		const int fd_out;
		AutoHost * sel;
		pthread_t tid;
		void PrintStatus(AutoHost * = NULL);
	public:
		StreamControl(int, int);
		bool Write(void *, const char *);
};

class OutprintControl{
	private:
		std::vector <StreamControl *> ctrls;
		pthread_mutex_t mutex;
	public:
		OutprintControl();
		~OutprintControl();
		void Add(StreamControl * ctrl);
		bool Remove(StreamControl * ctrl);
		void Put(void *, const char *, ...);
};

OutprintControl * GetOut();

#endif