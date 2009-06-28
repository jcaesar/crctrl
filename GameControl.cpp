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
	Settings.Scen = NULL;
	Settings.PW = NULL;
	ExecTrials=0;
	//Standard Settings:
	Settings.Ports.TCP=Config.Ports.TCP;
	Settings.Ports.UDP=Config.Ports.UDP;
	Settings.LobbyTime=Config.LobbyTime;
	Settings.SignOn=Config.SignOn;
	Settings.Record=Config.Record;
	Settings.League = Config.League > static_cast<float>(rand())/RAND_MAX;
	OutPrefix = new char[11];
	sprintf(OutPrefix, "g#%d", Games.Add(this));
	Status=Setting;
}

bool Game::SetScen(const char * scen){
	if(Status==Setting){
		if(Settings.Scen) delete [] Settings.Scen;
		Settings.Scen = new char[strlen(scen)+1];
		strcpy(Settings.Scen,scen);
		return true;
	} 
	return false;
}

bool Game::SetScen(const ScenarioSet * scen, bool changeleague){
	if(Status==Setting){
		delete [] Settings.Scen;
		Settings.Scen = new char[strlen(scen->GetPath())+1];
		strcpy(Settings.Scen,scen->GetPath());
		if(scen->GetPW() && strlen(scen->GetPW())){
			Settings.PW = new char[strlen(scen->GetPW())+1];
			strcpy(Settings.PW,scen->GetPW());
		}
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
	if(Settings.PW && strlen(Settings.PW)>0){
		cmd += " /pass:\"";
		cmd += Settings.PW;
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
	pid_t child, realchild;
	int status;
	Status=PreLobby;
	if ( (pipe(fd1) < 0) || (pipe(fd2) < 0) || (pipe(fderr) < 0) ){
		std::cerr << "Error opening Pipes." << std::endl;
		Fail();
		return -2;
	}
	child = fork();
	if(child < 0) { std::cerr << "Error forking process." << std::endl; Fail(); return -3;}
	if(child == 0){
		child = fork();
		if(child < 0) exit(1);
		if(child == 0){
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
			execl(fullpath, Config.Path, args, NULL); //Think about replacing this by another function. Perhaps you need to get to that exit(child) down there...
			//When execl does fine, it will never return.
			std::cerr << "Could not Start. Error: " << errno << std::endl; //Give parent process a notice.
			close(fd1[0]);
			close(fd2[1]);
			close(fderr[1]);
		}
		exit(child); // damit init (A) als Vater übernimmt und liefert PID vom Enkel als exit status "böser" hack
	}
	waitpid(child, &status, 0);
	pid = WEXITSTATUS(status);
	close(fd1[0]);
	close(fd2[1]);
	close(fderr[1]);
	sr=new StreamReader(fd2[0]);
	pipe_out=fd1[1];
	pipe_err=fderr[0];
	if(tid==0){ //Hu? I don't get the point behind my own thing anymore...
		pthread_create(&tid, NULL, &Game::ControlThreadWrapper, this);
		pthread_create(&msgtid, NULL, &Game::MsgThreadWrapper, this);
	} else {
		Control();
	}	
}

void Game::Control(){
	std::string line;
	boost::smatch regex_ret;
	while(sr->ReadLine(&line)){
		Out.Put(OutPrefix, " ", line.c_str(), NULL); //This is not real logic, but I don't care for now... Perhaps the NULL terminates the list and the c_str.
		//Scan for events
		if(regex_match(line, regex_ret, rx::cm_base)){
			if(!regex_ret[2].compare("hilf") || !regex_ret[2].compare("help")) {
				SendMsg("Liste aller Befehle:\n----%list\n--------Listet alle verfügbaren Szenarien auf\n----%start Szenname -lobby:Sekunden -passwort:\"pw\" -liga\n--------Startet ein Szenario. Alles ab -lobby ist optional.\n----%hilf\n--------Gibt das hier aus.\n", NULL);
			} else if(!regex_ret[2].compare("list")) {
				StringCollector list("Folgende Szenarien koennen gestartet werden:\n");
				const ScenarioSet * scn;
				for(int i=0; scn = Config.GetScen(i); i++){
					const char * name = scn->GetName(0);
					if(name){
						list.Push("-");
						int i=0;
						while(name) {
							list.Push(name);
							name = scn->GetName(++i);
							if(name) list.Push(", ");
						}
						list.Push("\n");
					}
				}
				SendMsg(list.GetBlock(), NULL);
			} else if(!regex_ret[2].compare("start")) {
				AutoHost * ah = AutoHosts.FindByGame(this);
				if(ah){
					char * cmd = new char [regex_ret[4].length() + 1];
					strncpy(cmd, regex_ret[4].str().data(), regex_ret[4].length() + 1); //I know, .str().data() sucks... Do it better.
					char * params = strstr(cmd, " -");
					ScenarioSet * scn;
					if(params){
						*params = 0;
						scn = Config.GetScen(cmd);
						*params = ' ';
					} else scn = Config.GetScen(cmd);
					if(scn != 0) {
						scn = new ScenarioSet(scn); //Make a Copy of it.
						if(strstr(cmd, " -liga")) scn -> SetLeague(1);
						else scn -> SetLeague(0);
						char * pos;
						if(pos = strstr(cmd, " -lobby:")){
							pos += 8;
							int time = atoi(pos);
							if(time > Config.LobbyTime) time = Config.LobbyTime;
							if(time < 10) time = 10;
							scn -> SetTime(time);
						}
						if(pos = strstr(cmd, " -pw:")){
							pos += 5;
							char chr = ' ';
							if(*pos == '"') {chr = '"'; pos++;}
							char * itr = pos;
							while(*itr++ != chr);
							chr = *itr;
							*itr = 0;
							scn -> SetPW(pos);
							*itr = chr; //Actually, that wouldn't be necessary. But I feel it is an itty bit more clean...
						}
						if(ah -> Enqueue(scn)){ //Perhaps I should Fix() the scenario...
							SendMsg("Szenario \"", scn->GetPath(), "\" wurde der Warteliste hinzugefuegt.\n", NULL);
						} else {
							SendMsg("Maximale Groesse der Warteliste ueberschritten.\n", NULL);
							delete scn; //Retour
						}
					} else {
						SendMsg("Szenario nicht gefunden \"", cmd, "\"\n", NULL);
					}
					delete [] cmd;
				} else {
					SendMsg("Dieses Spiel wurde manuell, (vermutlich zu Testzwecken), gestartet, deshalb kann kein weiteres Spiel gestartet werden.\n");
				}
			} else {
				SendMsg("Es gibt kein Kommando: \"", regex_ret[2].str().data(), "\". Gib %hilf ein, um alle Kommandos anzuzeigen.\n", NULL);
			}
		} else if(Status==Lobby){
			if(regex_match(line, rx::gm_gohot)) {
				SendMsg("Jetzt geht es los!\n", NULL);
				SendMsg("Viel Glueck und viel Spass!\n", NULL);
			} else if(regex_match(line, regex_ret, rx::cl_join)){
				SendMsg("Hi! Viel Spass beim Spielen, ", regex_ret[1].str().data(), ".\n", NULL);
				SendMsg(2, "Mehr ueber diesen Server erfaehrst du unter cserv.game-host.org\nJeder kann bestimmen, was gehostet wird. Gib %hilf ein!\n", NULL);
			} else if(regex_match(line, regex_ret, rx::cl_part)){
				SendMsg("Boeh, ", regex_ret[1].str().data(), " ist ein Leaver.\n", NULL);
			} else if(regex_match(line, rx::gm_load)){
				Status=Load;
			}
		} else if(Status==Run) {
			if(regex_match(line, regex_ret, rx::pl_die)){
				SendMsg("Wie die Fliegen, wie die Fliegen. Pass das naechste mal besser auf, ", regex_ret[1].str().data(), ", du Tropf.\n.", NULL);
			} else if(regex_match(line, rx::gm_tick0)){
				SendMsg("/me r0kt!\n", NULL);
				SendMsg(20, "/set maxplayer 1\n", NULL);
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
				SendMsg("/set maxplayer 1337\n", NULL);
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
	else if(pid > 0) kill(pid, SIGKILL);
	if(wait) AwaitEnd();
}

Game::~Game(){
	Games.Delete(this);
	cleanup = true;
	Exit(true);
	if(use_conds) pthread_cond_signal(&msgcond);
	if(pid > 0) kill(pid, SIGKILL); //Make sure, there does nothing rest in the process table
	waitpid(pid, NULL, WNOHANG); //Funny, but Guenther said, it would be usefull against clonk <defunct>. But it is not.
	if(Settings.Scen) delete [] Settings.Scen;
	if(Settings.PW) delete [] Settings.PW;
	delete [] OutPrefix;
	pthread_cancel(msgtid);
	pthread_cond_destroy(&msgcond);
	pthread_mutex_destroy(&msgmutex);
}

bool Game::Fail(){
	ExecTrials++;
	if(ExecTrials >= Config.MaxExecTrials) {
		Out.Put(OutPrefix, " Maximum execution attempts exceeded.", NULL);
		Status = Failed;
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

bool Game::SendMsg(const char * first, ...){
	if(!pipe_out) return false;
	va_list vl;
	va_start(vl, first);
	StringCollector msg(first);
	const char * str;
	while(str = va_arg(vl, const char *))
		msg.Push(str);
	//if(str + strlen(str) -1 != '\n') msg.Push("\n");
	msg.GetBlock();
	va_end(vl);
	if(*(msg.GetBlock())=='/') //Some commands don't have feedback. Just do a notification.
		Out.Put(OutPrefix, " ", msg.GetBlock(), NULL);
	return (write(pipe_out, msg.GetBlock(), msg.GetLength())==msg.GetLength());
}

void Game::SendMsg(int secs, const char * first, ...){
	int stamp = time(NULL) + secs; //Conserv
	va_list vl;
	va_start(vl, first);
	StringCollector msg(first);
	const char * str;
	while(str = va_arg(vl, const char *))
		msg.Push(str);
	//if(str + strlen(str) -1 != '\n') msg.Push("\n");
	msg.GetBlock(); 
	va_end(vl);
	//pthread_mutex_lock(&mutex);
	TimedMsg * tmsg = new TimedMsg; //Deleted in Game::MsgTimer
	tmsg->Stamp = stamp;
	tmsg->Msg = new char[msg.GetLength()+1]; //Deleted in D'tor of struct TimedMsg
	strcpy(tmsg->Msg,msg.GetBlock());
	MsgQueue.push_back(tmsg); //No need to lock here.
	if(use_conds) pthread_cond_signal(&msgcond); //Update
}


bool Game::SendMsg(const std::string msg){
	return SendMsg(msg.c_str(), NULL);
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
				SendMsg(MsgQueue[i]->Msg, NULL);
				delete MsgQueue[i];
				MsgQueue.erase(MsgQueue.begin()+i);
			} else {
				if(nextevent == 0 || MsgQueue[i]->Stamp <= nextevent) nextevent = MsgQueue[i]->Stamp;
			}
		}
	}
	use_conds=false;
	pthread_mutex_unlock(&msgmutex);
}

#endif
