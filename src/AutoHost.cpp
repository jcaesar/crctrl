
#include "AutoHost.h"

#include <errno.h>
#include <pthread.h>
#include "GameControl.h"
#include "Config.h"
#include "Control.h"

static AutoHostList AutoHosts;

AutoHostList * GetAutoHosts(){
	return &AutoHosts;
}

#ifdef _MSC_VER
	#pragma warning (disable: 4355)
#endif
AutoHost::AutoHost() : ID(AutoHosts.Add(this)), Fails(0), work(true) {
	OutPrefix = new char[11];
	sprintf(OutPrefix, "a#%d", ID);
	pthread_mutex_init(&mutex, NULL);
	pthread_create(&tid, NULL, &AutoHost::ThreadWrapper, this);
}

void AutoHost::SoftEnd(bool wait /*= true*/){
	work = false;
	if(wait && !pthread_equal(pthread_self(),tid)) pthread_join(tid, NULL);
}

AutoHost::~AutoHost(){
	AutoHosts.Remove(this);
	work = false;
	pthread_mutex_lock(&mutex); //onkel phlox hat gesagt: try{...}catch(DeadLockException& e){cout << "so einfach ist das" << endl; e.unlock();}
	delete CurrentGame;
	CurrentGame = NULL;
	pthread_mutex_unlock(&mutex);
	if(!pthread_equal(pthread_self(),tid)) pthread_join(tid, NULL); //And my own thread will end when all that is done. 
	pthread_mutex_destroy(&mutex);
	GetOut()->Put(NULL, OutPrefix, " Terminated.", NULL);
	delete OutPrefix;
}

void AutoHost::Work(){
	GetOut()->Put(NULL, OutPrefix, " New AutoHost working!", NULL);
	const ScenarioSet * scn; const ScenarioSet * lastscn; bool del, delnow;
	pthread_mutex_lock(&mutex);
	while(work){
		lastscn = scn;
		delnow = del;
		if(ScenQueue.empty()) {
			scn = GetConfig()->GetScen();
			del = false;
		} else {
			try {scn = ScenQueue.at(0);} //Vector sux, dunno why.
			catch (...) {continue;}
			ScenQueue.erase(ScenQueue.begin());
		}
		if(Fails && scn == lastscn) continue;
		if(delnow) delete lastscn;
		CurrentGame = new Game(this);
		CurrentGame -> SetScen(scn);
		pthread_mutex_unlock(&mutex);
		CurrentGame -> Start(); //This blocks, until the game is over.´
		if(!work || pthread_mutex_trylock(&mutex) == EBUSY) break;
		if(CurrentGame -> GetStatus() == Failed) {
			Fails++;
			if(Fails >= GetConfig()->MaxExecTrials) {
				GetOut()->Put(this, OutPrefix, " Maximum execution attempts for AutoHost exceeded.", NULL);
				delete this;
				break;
			}
		} else Fails = 0;
		delete CurrentGame;
	}
	pthread_mutex_unlock(&mutex);  //"[EPERM] - The current thread does not hold a lock on mutex." (No need to handle.)
}

Game * AutoHost::GetGame(){
	return CurrentGame;
}

bool AutoHost::Enqueue(const ScenarioSet * scn){
	pthread_mutex_lock(&mutex);
	if(ScenQueue.size() >= GetConfig()->MaxQueueSize){
		pthread_mutex_unlock(&mutex);
		return false; //What about some kind of errno?
	}
	ScenQueue.push_back(scn);
	pthread_mutex_unlock(&mutex);
	return true;
}

void * AutoHost::ThreadWrapper(void * p){
	static_cast<AutoHost*>(p)->AutoHost::Work();
	return NULL;
}

AutoHostList::AutoHostList(){
	Index = 0;
}

AutoHostList::~AutoHostList(){
	DelAll();
}

void AutoHostList::DelAll(){
	int i = Instances.size();
	while(i--) delete Instances[i];
	Instances.clear();
}

int AutoHostList::Add(AutoHost * add){
	Instances.push_back(add);
	return ++Index;
}

bool AutoHostList::Remove(AutoHost * rem){
	int i=Instances.size();
	while(i--){
		if(Instances[i]==rem) {Instances.erase(Instances.begin()+i); return true;}
	}
	return false;
}

AutoHost * AutoHostList::FindByGame(Game * find){
	int i=Instances.size();
	while(i--){
		if(Instances[i]->GetGame()==find) return Instances[i];
	}
	return NULL;
}

AutoHost * AutoHostList::Find(int findid){
	int i=Instances.size();
	while(i--){
		if(Instances[i]->GetID()==findid) return Instances[i];
	}
	return NULL;
}

bool AutoHostList::GameExists(Game * find){
	int i=Instances.size();
	while(i--){
		if(Instances[i]->GetGame()==find) return true;
	}
	return false;
}

bool AutoHostList::Exists(AutoHost * find){
	if(!find) return false;
	int i=Instances.size();
	while(i--){
		if(Instances[i]==find) return true;
	}
	return false;
}


AutoHost * AutoHostList::Get(int i){
	try {return Instances.at(i);} //Vector sux, dunno why.
	catch (...) {return NULL;}
}
