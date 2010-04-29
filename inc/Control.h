#ifndef ControlH
#define ControlH

#include <sys/types.h>
#include <vector>
#include <pthread.h>
class Stream; // #include "helpers/Stream.h"
class AutoHost; // #include "AutoHost.h"

class UserControl{
	private:
		void Work();
		static void * ThreadWrapper(void *);
		Stream * conn;
		AutoHost * sel;
		pthread_t tid;
		void PrintStatus(AutoHost * = NULL);
	public:
		UserControl(Stream *);
		~UserControl();
		bool Write(const void *, const char *);
};

class OutprintControl{
	private:
		std::vector <UserControl *> ctrls;
		pthread_mutex_t mutex;
	public:
		OutprintControl();
		~OutprintControl();
		void Add(UserControl * ctrl);
		bool Remove(UserControl * ctrl);
		void Put(const void *, const char *, ...);
};

OutprintControl * GetOut();

#endif