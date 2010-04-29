class AutoHost;
#ifndef AutoHostH
#define AutoHostH

#include <pthread.h>
#include <vector>
#include "helpers/StatusLocks.h"
class Game; //#include "GameControl.h"
class ScenarioSet; //#include "Config.h"
class UserControl;


class AutoHost : public StatusLocks {
	private:
		const int ID;
		char * OutPrefix;
		pthread_t tid;
		Game * CurrentGame;
		static void * ThreadWrapper(void *);
		void Work();
		unsigned int Fails;
		bool work;
		std::vector <const ScenarioSet*> ScenQueue;
	public:
		explicit AutoHost(const UserControl * CreatedBy = NULL);
		~AutoHost();
		Game * GetGame();
		bool Enqueue(const ScenarioSet *);
		int GetID(){return ID;}
		const char * GetPrefix(){return OutPrefix;}
		void SoftEnd(bool = true);
		void LockStatus();
		void UnlockStatus();
};

class AutoHostList{
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
		bool Exists(void *);
		bool CatchAndLock(void *);
};

AutoHostList * GetAutoHosts();

#endif