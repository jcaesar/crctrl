#ifndef AutoHostCpp
#define AutoHostCpp

AutoHost::AutoHost() : work(true) {
	AutoHosts.Add(this);
	pthread_mutex_init(&mutex, NULL);
	pthread_create(&tid, NULL, &AutoHost::ThreadWrapper, this);
}

AutoHost::~AutoHost(){
	work = false;
	delete CurrentGame; //This is crazy. It calls the destructor, which makes CR halt, and waits for the thread wich is waiting for CR to end...
		//But I think it will crash.
	pthread_join(tid, NULL); //And my own thread will end when all that is done.
	pthread_mutex_destroy(&mutex);
	AutoHosts.Remove(this);
}

void AutoHost::Work(){
	while(work){
		CurrentGame = new Game();
		pthread_mutex_lock(&mutex);
		if(ScenQueue.empty())CurrentGame -> SetScen(Config.GetScen());
		else {
			CurrentGame -> SetScen(ScenQueue[0]);
			delete [] ScenQueue[0];
			ScenQueue.erase(ScenQueue.begin());
		}
		pthread_mutex_unlock(&mutex);
		CurrentGame -> Start();
		CurrentGame -> KillOnEnd();
		CurrentGame -> AwaitEnd(); //This is no clean programming. After KillOnEnd, the Pointer may not be used anymore. FIXME, or better fix KillOnEnd
	}
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


AutoHostList::~AutoHostList(){
	int i = Instances.size();
	while(i--) delete Instances[i];
	Instances.clear();
}

void AutoHostList::Add(AutoHost * add){
	Instances.push_back(add);
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

#endif