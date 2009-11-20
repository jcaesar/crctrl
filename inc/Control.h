#ifndef ControlH
#define ControlH

#include <sys/types.h>
#include <vector>
class StreamReader; // #include "helpers/StreamReader.hpp"
class AutoHost; // #include "AutoHost.h"

class StreamControl{
	private:
		void Work();
		static void * ThreadWrapper(void *);
		const int fd_out;
		StreamReader * sr;
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