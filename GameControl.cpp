#ifndef GameControlCpp
#define GameControlCpp

int GameRegister::Add(Game * g){
	pthread_mutex_lock(&mutex);
	Register.push_back(g);
	Count++;
	Index++;
	pthread_mutex_unlock(&mutex);
	return Index;
}

Game * GameRegister::Find(const char * name){
	pthread_mutex_lock(&mutex);
	const char * gname;
	for(int i=Count;i-->0;){
		gname = (*(Register[i])).GetName();
		if(!strcmp(Register[i]->GetName(),name)){
			Game * tmp = Register[i];
			pthread_mutex_unlock(&mutex);
			return tmp;
		}
	}
	pthread_mutex_unlock(&mutex);
	return NULL;
}

bool GameRegister::Exists(Game * ptr){
	pthread_mutex_lock(&mutex);
	for(int i=Count;i-->0;){
		if(Register[i] == ptr){
			pthread_mutex_unlock(&mutex);
			return true;
		}
	}
	pthread_mutex_unlock(&mutex);
	return NULL;
}

bool GameRegister::Delete(Game * g){
	pthread_mutex_lock(&mutex);
	for(int i=Count;i-->0;){
		if(Register[i] == g){
			Register.erase(Register.begin()+i);
			Count--;
			pthread_mutex_unlock(&mutex);
			return true;
		}
	}
	return false;
	pthread_mutex_unlock(&mutex);
}

void GameRegister::DelAll(){
	pthread_mutex_lock(&mutex);
	while(Count-->0){
		delete Register[Count];
	}
	Register.clear();
	pthread_mutex_unlock(&mutex);
}


Game::Game(){
	Selfkill=false;
	pthread_mutex_init(&msgmutex, NULL); //Deleted in MsgQueue
	pthread_cond_init(&msgcond, NULL);   // -"-
	use_conds = true;
	cleanup=false;
	tid=NULL;
	pid=NULL;
	ExecTrials=0;
	//Standard Settings:
	Settings.Ports.TCP=Config.Ports.TCP;
	Settings.Ports.UDP=Config.Ports.UDP;
	Settings.LobbyTime=Config.LobbyTime;
	Settings.SignOn=Config.SignOn;
	Settings.Record=Config.Record;
	Settings.League = Config.League > static_cast<float>(rand())/RAND_MAX;
	Settings.Scen = new char [1];
	OutPrefix = new char[11];
	sprintf(OutPrefix, "g#%d", Games.Add(this));
	Status=Setting;
}

bool Game::SetScen(const char * scen){
	if(Status==Setting){
		delete [] Settings.Scen;
		Settings.Scen = new char[strlen(scen)+1];
		strcpy(Settings.Scen,scen);
		return true;
	} 
	return false;
}

bool Game::SetScen(const ScenarioSet * scen, bool changeleague){
	if(Status==Setting){
		delete [] Settings.Scen;
		Settings.Scen = new char[strlen(scen->Get()Path)+1];
		strcpy(Settings.Scen,scen->GetPath());
		Settings.LobbyTime=scen->GetTime();
		if(changeleague){
			srand((unsigned)time(NULL)); 
			Settings.League = (scen->GetLeague()) > static_cast<float>(rand())/RAND_MAX;
		}
		return true;
	} 
	return false;
}

void Game::Start(){
	//std::string cmd;
	StringCollector cmd("\"");
	if(Settings.SignOn) cmd += "\"/signup\"";
	else cmd += "\"/nosignup\"";
	if(Settings.League)cmd += " \"/league\"";
	else cmd += " \"/noleague\"";
	if(Settings.Record) cmd += " \"/record\"";
	cmd += " \"/tcpport:";
	cmd	+= Settings.Ports.TCP;
	cmd	+= "\"";
	cmd	+= " \"/udpport:";
	cmd	+= Settings.Ports.UDP;
	cmd += "\"";
	if(Settings.LobbyTime>0){
		cmd += " \"/lobby:";
		cmd += Settings.LobbyTime;
		cmd += "\""; 
	}
	if(strlen(Config.ConfigPath)>0){
		cmd += " /config:\"";
		cmd += Config.ConfigPath;
		cmd += "\"";
	}
	cmd += " \"";
	cmd += Settings.Scen;
	cmd += "\"";
	Start(cmd.GetBlock());
}

int Game::Start(const char * args){
	int fd1[2];
	int fd2[2];
	int fderr[2];
	Out.Put(OutPrefix, " ", "Starting Round: ", args, NULL);
	Status=PreLobby;
	if ( (pipe(fd1) < 0) || (pipe(fd2) < 0) || (pipe(fderr) < 0) ){
		std::cerr << "Error opening Pipes." << std::endl;
		return -2;
	}
	if ( (pid = fork()) < 0 ){
		std::cerr << "Error forking process." << std::endl;
		return -3;
	}
	else  if (pid == 0){     // CHILD PROCESS
		close(fd1[1]);
		close(fd2[0]);
		close(fderr[0]);
		if (fd1[0] != STDIN_FILENO){
			if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO) std::cerr << "Error with stdin" << std::endl;
			close(fd1[0]);
		}
		if (fd2[1] != STDOUT_FILENO){
			if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO) std::cerr << "Error with stdout" << std::endl;
			close(fd2[1]);
		}
		if (fderr[1] != STDERR_FILENO){
			if (dup2(fderr[1], STDERR_FILENO) != STDERR_FILENO) std::cerr << "Error with stderr" << std::endl;
			close(fderr[1]);
		}
		char * fullpath = new char[strlen(Config.Path) + 7];
		strcpy(fullpath, Config.Path);
		strcpy(fullpath+strlen(Config.Path), "/clonk");
		execl(fullpath, Config.Path, args, NULL);
		//When execl does fine, this will never return.
		std::cerr << "Could not Start. Error: " << errno << std::endl; //Give parent process a notice.
		close(fd1[0]);
		close(fd2[1]);
		close(fderr[1]);
		exit(-4);
	}
	else
	{
		close(fd1[0]);
		close(fd2[1]);
		close(fderr[1]);
		sr=new StreamReader(fd2[0]);
		pipe_out=fd1[1];
		pipe_err=fderr[0];
		if(tid==0){
			pthread_create(&tid, NULL, &Game::ControlThreadWrapper, this);
			pthread_create(&msgtid, NULL, &Game::MsgThreadWrapper, this);
		} else {
			Control();
		}
	}
	return 0;
}

void Game::Control(){
	std::string line;
	boost::smatch regex_ret;
	//std::getline(pipe_in,line); //This does wtf not work. 'no matching function' coqsucking
	while(sr->ReadLine(&line)){
		Out.Put(OutPrefix, " ", line.c_str(), NULL);
		//Scan for events
		if(regex_match(line, regex_ret, rx::cm_base)){
			if(!regex_ret[2].compare("hilf")) {
				SendMsg("Liste aller Befehle:\n----%list\n--------Listet alle verfügbaren Szenarien auf\n----%start Szenname -lobby:Sekunden -passwort:\"pw\" -liga\n--------Startet ein Szenario. Alles ab -lobby ist optional.\n----%hilf\n--------Gibt das hier aus.\n");
			} else if(!regex_ret[2].compare("list")) {
				SendMsg("Noch nicht implementiert.\n");
			} else if(!regex_ret[2].compare("start")) {
				SendMsg("Programmierer sind keine Marathonlaeufer. Wart, bis ich es implementiert hab, " + regex_ret[1] + ".\n");
			} else {
				SendMsg("Es gibt kein Kommando: \"" +regex_ret[2]+ "\". Gib %hilf ein, um alle Kommandos anzuzeigen.\n");
			}
		} else if(Status==Lobby){
			if(regex_match(line, rx::gm_gohot)) {
				SendMsg("Jetzt geht es los!\n");
				SendMsg("Viel Glueck und viel Spass!\n");
			} else if(regex_match(line, regex_ret, rx::cl_join)){
				SendMsg("Hi! Viel Spass beim Spielen, " + regex_ret[1] + ".\n");
				SendMsg("Mehr ueber diesen Server erfaehrst du unter cserv.game-host.org\nJeder kann bestimmen, was gehostet wird. Gib %hilf ein!\n", 2);
			} else if(regex_match(line, regex_ret, rx::cl_part)){
				SendMsg("Boeh, " + regex_ret[1] + " ist ein Leaver.\n");
			} else if(regex_match(line, rx::gm_load)){
				Status=Load;
			}
		} else if(Status==Run) {
			if(regex_match(line, regex_ret, rx::pl_die)){
				SendMsg("Wie die Fliegen, wie die Fliegen. Pass das naechste mal besser auf, " + regex_ret[1] + ", du Tropf.\n.");
			} else if(regex_match(line, rx::gm_tick0)){
				SendMsg("/me r0kt!\n");
				SendMsg("/set maxplayer 1\n", 20);
			}
		} else if(Status==Load) {
			if(regex_match(line, rx::gm_go)){
				Status=Run;
			}
		} else if(Status==PreLobby) {
			if(regex_match(line, rx::ctrl_err) || regex_match(line, rx::gm_exit)) {
				Fail();
				
			}
			if(regex_match(line, rx::gm_lobby)){
				Status=Lobby;
				SendMsg("/set maxplayer 1337\n");
			}
		}
		line = "";
	}
	if(Status==PreLobby) Fail();
	sr = NULL;
	Out.Put(OutPrefix, " Clonk Rage terminated.", NULL);
	if(Selfkill) delete this;
}

void Game::Exit(bool wait = true, bool soft = true){
	if(soft)
		if(pipe_out != 0) {
			close(pipe_out);
			pipe_out=0;
			//I just hope, that CR closes by saying this. If it does not, I will have Thread and memory leaks...
		}
	else kill(pid, SIGKILL);
	if(wait) AwaitEnd();
}

Game::~Game(){
	cleanup = true;
	if(use_conds) pthread_cond_signal(&msgcond);
	Exit(true);
	kill(pid, SIGKILL); //Make sure, there do not rest clonk <defunct> in the process table. Don't care about errors.
}

bool Game::Fail(){
	ExecTrials++;
	if(ExecTrials >= Config.MaxExecTrials) {
		Out.Put(OutPrefix, " Maximum execution attempts exceeded.", NULL);
		AutoHost * ah = AutoHosts.FindByGame(this);
		if(ah!=NULL) 
			delete ah; //Will delete this anyway.
		else 
			if(Selfkill) delete this;
		pthread_exit(NULL);
	} else {
		sleep(5);
		Start();
		pthread_exit(NULL);
	}
}

void Game::AwaitEnd(){
	if(pthread_self() != tid) pthread_join(tid, NULL);
}

void * Game::ControlThreadWrapper(void * p) {
	static_cast<Game*>(p)->Control();
	return NULL;
}

void * Game::MsgThreadWrapper(void * p) {
	static_cast<Game*>(p)->MsgTimer();
	return NULL;
}

bool Game::SendMsg(const char * msg){
	int len=strlen(msg);
	if(!pipe_out) return false;
	if(*msg=='/') //Some commands don't have feedback. Just do a notification.
		Out.Put(OutPrefix, " ", msg, NULL);
	return (write(pipe_out, msg, len)==len);
}

void Game::SendMsg(const char * msg, int secs){
	TimedMsg * tmsg = new TimedMsg; //Deleted in Game::MsgTimer
	tmsg->Stamp = time(NULL) + secs;
	tmsg->Msg = new char[strlen(msg)+1]; //Deleted in D'tor of struct
	strcpy(tmsg->Msg,msg);
	MsgQueue.push_back(tmsg); //No need to lock here.
	if(use_conds) pthread_cond_signal(&msgcond); //Update
}


bool Game::SendMsg(const std::string msg){
	return SendMsg(msg.c_str());
}

void Game::MsgTimer(){
	pthread_mutex_lock(&msgmutex); //Whyever.
	time_t nextevent = 0;
	while(true){
		if(MsgQueue.size() > 0) {
			timespec ts;
			ts.tv_sec = nextevent; 
			pthread_cond_timedwait(&msgcond, &msgmutex, &ts);
		} else pthread_cond_wait(&msgcond, &msgmutex);
		if(cleanup) 
			break;
		nextevent=0;
		int i=MsgQueue.size(); while(i-->0){
			if(MsgQueue[i]->Stamp <= time(NULL)){
				SendMsg(MsgQueue[i]->Msg);
				delete MsgQueue[i];
				MsgQueue.erase(MsgQueue.begin()+i);
			} else {
				if(nextevent == 0 || MsgQueue[i]->Stamp <= nextevent) nextevent = MsgQueue[i]->Stamp;
			}
		}
	}
	use_conds=false;
	pthread_mutex_unlock(&msgmutex);
	pthread_mutex_destroy(&msgmutex);
	pthread_cond_destroy(&msgcond);
}

#endif