#ifndef AutoHostCpp
#define AutoHostCpp

AutoHost::AutoHost(){
	AutoHosts.Add(this);
	pthread_create(&tid, NULL, &AutoHost::ThreadWrapper, this);
}

AutoHost::~AutoHost(){
	work = false;
	delete CurrentGame; //This is crazy. It calls the destructor, which makes CR halt, and waits for the thread wich is waiting for CR to end...
		//But I think it will crash.
	pthread_join(tid, NULL); //And my own thread will end when all that is done.
	AutoHosts.Remove(this);
}

void AutoHost::Work(){
	while(work){
		CurrentGame = new Game();
		CurrentGame -> SetScen(Config.GetScen());
		CurrentGame -> Start();
		CurrentGame -> KillOnEnd();
		CurrentGame -> AwaitEnd(); //This is no clean programming. After KillOnEnd, the Pointer may not be used anymore. FIXME
	}
}

Game * AutoHost::GetGame(){
	return CurrentGame;
}

void * AutoHost::ThreadWrapper(void * p){
	static_cast<AutoHost*>(p)->Work();
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