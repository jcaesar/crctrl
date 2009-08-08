
#include "AutoHost.h"

AutoHost::AutoHost() : work(true), Fails(0), ID(AutoHosts.Add(this)) {
	OutPrefix = new char[11];
	sprintf(OutPrefix, "a#%d", ID);
	pthread_mutex_init(&mutex, NULL);
	pthread_create(&tid, NULL, &AutoHost::ThreadWrapper, this);
}

void AutoHost::SoftEnd(bool wait /*= true*/){
	work = false;
	if(wait && pthread_self() != tid) pthread_join(tid, NULL);
}

AutoHost::~AutoHost(){
	work = false;
	delete CurrentGame;
	CurrentGame = NULL;
	if(pthread_self() != tid) pthread_join(tid, NULL); //And my own thread will end when all that is done. 
	pthread_mutex_destroy(&mutex);
	AutoHosts.Remove(this);
	delete OutPrefix;
}

void AutoHost::Work(){
	Out.Put(NULL, OutPrefix, " New AutoHost working!", NULL);
	while(work){
		CurrentGame = new Game(this);
		pthread_mutex_lock(&mutex);
		if(ScenQueue.empty()) CurrentGame -> SetScen(Config.GetScen());
		else {
			const ScenarioSet * scn;
			try {scn = ScenQueue.at(0);} //Vector sux, dunno why.
			catch (...) {continue;}
			CurrentGame -> SetScen(scn);
			delete scn;
			ScenQueue.erase(ScenQueue.begin());
		}
		pthread_mutex_unlock(&mutex);
		CurrentGame -> Start(); //This blocks, until the game is over.
		if(!work) break;
		if(CurrentGame -> GetStatus() == Failed) {
			Fails++;
			if(Fails >= Config.MaxExecTrials) delete this;
		} else Fails = 0;
		delete CurrentGame;
	}
	Out.Put(NULL, OutPrefix, " Terminated.", NULL);
}

Game * AutoHost::GetGame(){
	return CurrentGame;
}

bool AutoHost::Enqueue(const ScenarioSet * scn){
	pthread_mutex_lock(&mutex);
	if(ScenQueue.size() >= Config.MaxQueueSize){
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
