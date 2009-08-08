class AutoHost;
#ifndef AutoHostH
#define AutoHostH

#include "GameControl.h"
#include "Config.h"
#include <pthread.h>

class AutoHost{
	private:
		const int ID;
		char * OutPrefix;
		pthread_t tid;
		Game * CurrentGame;
		static void * ThreadWrapper(void *);
		void Work();
		int Fails;
		bool work;
		pthread_mutex_t mutex;
		std::vector <const ScenarioSet*> ScenQueue;
	public:
		AutoHost();
		~AutoHost();
		Game * GetGame();
		bool Enqueue(const ScenarioSet *);
		int GetID(){return ID;}
		const char * GetPrefix(){return OutPrefix;}
		void SoftEnd(bool = true);
};

static class AutoHostList{
	private:
		std::vector <AutoHost *> Instances;
		int Index;
	public:
		AutoHostList();
		~AutoHostList();
		int Add(AutoHost *);
		bool Remove(AutoHost *);
		void DelAll();
		AutoHost * Get(int);
		AutoHost * FindByGame(Game *);
		AutoHost * Find(int);
		bool GameExists(Game *);
		bool Exists(AutoHost *);
} AutoHosts;

#endif