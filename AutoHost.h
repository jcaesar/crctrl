class AutoHost{
	private:
		pthread_t tid;
		Game * CurrentGame;
		static void * ThreadWrapper(void *);
		void Work();
		bool work;
	public:
		AutoHost();
		~AutoHost();
		Game * GetGame();
};

static class AutoHostList{
	private:
		std::vector <AutoHost *> Instances;
	public:
		~AutoHostList();
		void Add(AutoHost *);
		bool Remove(AutoHost *);
		AutoHost * FindByGame(Game *);
} AutoHosts;

#include "AutoHost.cpp" //This is wierd! Strange! Absolutely! Why have I got to do this? FIXME!!1