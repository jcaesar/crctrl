#include "GameControl.h"
pthread_t Game::msgtid;
pthread_cond_t Game::msgcond;
pthread_mutex_t Game::foomutex;
pthread_mutex_t Game::msgmutex;
std::vector <TimedMsg *> Game::MsgQueue;
bool Game::msg_ready;


Game::Game(AutoHost * parent) : //FIXME: Better use reference
	OutPrefix(parent->GetPrefix()),
	Parent(parent)
{
	cleanup=false;
	pid=NULL;
	Settings.Scen = NULL;
	Settings.PW = NULL;
	worker = NULL;
	ExecTrials=0;
	//Standard Settings:
	Settings.Ports.TCP=GetConfig()->Ports.TCP;
	Settings.Ports.UDP=GetConfig()->Ports.UDP;
	Settings.LobbyTime=GetConfig()->LobbyTime;
	Settings.SignOn=GetConfig()->SignOn;
	Settings.Record=GetConfig()->Record;
	Settings.League = GetConfig()->League > static_cast<float>(rand())/RAND_MAX;
	Status=Setting;
}

void Game::Init() {
	if(msg_ready) return;
	pthread_mutex_init(&foomutex, NULL); //Deleted in MsgQueue
	pthread_mutex_init(&msgmutex, NULL);
	pthread_cond_init(&msgcond, NULL);   // -"-
	if(msgtid == 0) pthread_create(&msgtid, NULL, &Game::MsgTimer, NULL);
	msg_ready = true;
	msgtid=NULL;
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

bool Game::SetScen(const ScenarioSet * scen, bool changeleague /*==true*/){
	if(Status==Setting && scen != 0){
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
	StringCollector cmd("\"\"/fullscreen\"");
	if(Settings.SignOn) cmd += " \"/signup\"";
	else cmd += " \"/nosignup\"";
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
	if(strlen(GetConfig()->ConfigPath)>0){
		cmd += " /config:\"";
		cmd += GetConfig()->ConfigPath;
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

void Game::Start(const char * args){
	int fd1[2];
	int fd2[2];
	int fderr[2];
	int fdtmp[2];
	pid_t child;
	if(cleanup) return; //Last check...
	worker = pthread_self();
	Status=PreLobby;
	if ( (pipe(fd1) < 0) || (pipe(fd2) < 0) || (pipe(fderr) < 0) || (pipe(fdtmp) < 0) ){
		std::cerr << "Error opening Pipes." << std::endl;
		Fail();
		return;
	}
	child = fork();
	if(child < 0) { std::cerr << "Error forking process." << std::endl; Fail(); return;}
	if(child == 0){
		close(fdtmp[0]);
		child = fork();
		if(child < 0) exit(1);
		if(child == 0){
			close(fd1[1]);
			close(fd2[0]);
			close(fderr[0]);
			if (fd1[0] != STDIN_FILENO){
				if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO) std::cout << "Error with stdin" << std::endl;
				close(fd1[0]);
			}
			if (fd2[1] != STDOUT_FILENO){
				if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO) std::cout << "Error with stdout" << std::endl;
				close(fd2[1]);
			}
			if (fderr[1] != STDERR_FILENO){
				if (dup2(fderr[1], STDERR_FILENO) != STDERR_FILENO) std::cout << "Error with stderr" << std::endl;
				close(fderr[1]);
			}
			char * fullpath = new char[strlen(GetConfig()->Path) + 7];
			strcpy(fullpath, GetConfig()->Path);
			strcpy(fullpath+strlen(GetConfig()->Path), "/clonk");
			execl(fullpath, GetConfig()->Path, args, NULL);
			//When execl does fine, it will never return.
			std::cout << "Could not Start. Error: " << errno << std::endl; //Give parent process a notice.
			close(fd1[0]);
			close(fd2[1]);
			close(fderr[1]);
		}
		char pidchr [12];
		sprintf(pidchr, "%d\n", child);
		write(fdtmp[1], pidchr, strlen(pidchr));
		close(fdtmp[1]);
		raise(SIGKILL);
	}
	close(fdtmp[1]);
	waitpid(child, NULL, 0);
	StreamReader pidread (fdtmp[0]);
	std::string pids;
	if(pidread.ReadLine(&pids)) pid = atoi(pids.c_str());
	else pid = 0;
	close(fdtmp[0]);
	close(fd1[0]);
	close(fd2[1]);
	close(fderr[1]);
	sr=new StreamReader(fd2[0]); //deleted in Game::Control()
	pipe_out=fd1[1];
	pipe_err=fderr[0];
	if(!msg_ready) Init();
	Control();
}

void Game::Control(){
	std::string line;
	boost::smatch regex_ret;
	while(sr->ReadLine(&line)){
		GetOut()->Put(Parent, OutPrefix, " ", line.c_str(), NULL);
		//Scan for events
		if(regex_match(line, regex_ret, rx::cm_base)){
			if(!regex_ret[2].compare("hilf") || !regex_ret[2].compare("help")) {
				SendMsg("Liste aller Befehle:\n----%list\n--------Listet alle verfügbaren Szenarien auf\n----%start Szenname -lobby:Sekunden -passwort:\"pw\" -liga\n--------Startet ein Szenario. Alles ab -lobby ist optional.\n----%hilf\n--------Gibt das hier aus.\n", NULL);
			} else if(!regex_ret[2].compare("list")) {
				StringCollector list("Folgende Szenarien koennen gestartet werden:\n");
				const ScenarioSet * scn;
				for(int i=0; (scn = GetConfig()->GetScen(i)); i++){
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
				char * cmd = new char [regex_ret[4].length() + 1];
				strncpy(cmd, regex_ret[4].str().data(), regex_ret[4].length() + 1); //I know, .str().data() sucks... Do it better.
				char * params = strstr(cmd, " -");
				ScenarioSet * scn;
				if(params){
					*params = 0;
					scn = GetConfig()->GetScen(cmd);
					*params = ' ';
				} else scn = GetConfig()->GetScen(cmd);
				if(scn != 0) {
					scn = new ScenarioSet(scn); //Make a Copy of it.
					if(strstr(cmd, " -liga")) scn -> SetLeague(1);
					else scn -> SetLeague(0);
					char * pos;
					if((pos = strstr(cmd, " -lobby:"))){
						pos += 8;
						int time = atoi(pos);
						if(time > GetConfig()->LobbyTime) time = GetConfig()->LobbyTime;
						if(time < 10) time = 10;
						scn -> SetTime(time);
					}
					if((pos = strstr(cmd, " -pw:"))){
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
					if(Parent -> Enqueue(scn)){ //Perhaps I should Fix() the scenario...
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
				SendMsg("Es gibt kein Kommando: \"", regex_ret[2].str().data(), "\". Gib %hilf ein, um alle Kommandos anzuzeigen.\n", NULL);
			}
		} else if(Status==Lobby){
			if(regex_match(line, rx::gm_gohot)) {
				SendMsg("Jetzt geht es los!\n", NULL);
				SendMsg("Viel Glueck und viel Spass!\n", NULL);
			} else if(regex_match(line, regex_ret, rx::cl_join)){
				if(!GetConfig()->GetBan(regex_ret[1].str().c_str())){	
					SendMsg("Hi! Viel Spass beim Spielen, ", regex_ret[1].str().data(), ".\n", NULL);
					SendMsg(2, "Mehr ueber diesen Server erfaehrst du unter cserv.game-server.cc\nJeder kann bestimmen, was gehostet wird. Gib %hilf ein!\n", NULL);
				}
			} else if(regex_match(line, regex_ret, rx::cl_conn)){
				if(const char * reason = GetConfig()->GetBan(regex_ret[1].str().c_str())){
					//SendMsg("Sorry, ", regex_ret[1].str().data(), " aber du stehst auf meiner Abschussliste. (", reason, ")\n");
					SendMsg("/kick ", regex_ret[1].str().data(), "\n");
				}
			} else if(regex_match(line, regex_ret, rx::cl_part)){
				if(!GetConfig()->GetBan(regex_ret[1].str().data())) SendMsg("Boeh, ", regex_ret[1].str().data(), " ist ein Leaver.\n", NULL);
				/*mysqlpp::Connection conn(false);
				if (conn.connect(Login.db, Login.addr, Login.usr, Login.pw)) {
					std::string querystring;
					
					//Hier werden die einzelnen Bestandteile zusamemngesetzt
					querystring = "INSERT INTO 'banned_people'(`pc-name`, `ip-address`, `cause`, `time`) VALUES ('";
					querystring += regex_ret[1].str().data();
					querystring += "', ";
					querystring += "'255.255.255.0', "; //NOT YET IMPLENTENED!!! TODO
					querystring += "'Leaving', ";
					querystring += "'3')";
					//Ende Bestandteile
					
					mysqlpp::Query query1 = conn.query(querystring.c_str());
				}
				else {
					SendMsg("Hast Glück gehabt, ", regex_ret[1].str().data(), ", der MySQL-Server ist tot.\n", NULL);
				}*/
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
				Fail(); return;
			}
			if(regex_match(line, rx::gm_lobby)){
				Status=Lobby;
				SendMsg("/set maxplayer 1337\n", NULL);
			}
		}
	}
	delete sr;
	sr = NULL;
	if(Status==PreLobby) Fail(); 
	return;
}

void Game::Exit(bool soft /*= true*/, bool wait /*= true*/){
	cleanup = true;
	if(soft){ 
		if(pipe_out != 0) {
			int temp_pipe = pipe_out;
			pipe_out=0;
			close(temp_pipe);
			//I just hope, that CR closes by saying this. If it does not, I will have problems...
		}
	} else if(pid > 0) kill(pid, SIGKILL);
	pthread_mutex_lock(&msgmutex);
	int i=MsgQueue.size(); 
	while(i-->0)
		if(MsgQueue[i]->SendTo == this){
			delete MsgQueue[i];
			MsgQueue.erase(MsgQueue.begin()+i);
		}
	pthread_mutex_unlock(&msgmutex);
	if(wait){
		waitpid(pid, NULL, 0);
		pthread_join(worker, NULL);
	}
}

Game::~Game(){
	Exit();
	if(pid > 0) kill(pid, SIGKILL);
	waitpid(pid, NULL, WNOHANG);
	if(Settings.Scen) delete [] Settings.Scen;
	if(Settings.PW) delete [] Settings.PW;
	if(msgtid) {pthread_cancel(msgtid); msgtid=NULL;}
}

void Game::Deinit(){
	if(!msg_ready) return;
	msg_ready = false;
	pthread_cond_signal(&msgcond);
	if(msgtid) {pthread_join(msgtid, NULL); msgtid=NULL;}
	pthread_cond_destroy(&msgcond);
	pthread_mutex_destroy(&foomutex);
	pthread_mutex_destroy(&msgmutex);
}

bool Game::Fail(){
	ExecTrials++;
	if(ExecTrials >= GetConfig()->MaxExecTrials) {
		Status = Failed;
		GetOut()->Put(Parent, OutPrefix, " Maximum execution attempts for game exceeded.", NULL);
		return true;
	} else {
		sleep(5);
		if(cleanup) return true;
		Start();
		return false;
	}
}

bool Game::SendMsg(const char * first, ...){
	if(!pipe_out || (Status != Lobby && Status != Run)) return false;
	va_list vl;
	va_start(vl, first);
	StringCollector msg(first);
	const char * str;
	while((str = va_arg(vl, const char *))) msg.Push(str);
	//if(str + strlen(str) -1 != '\n') msg.Push("\n");
	msg.GetBlock();
	va_end(vl);
	if(*(msg.GetBlock())=='/') //Some commands don't have feedback. Just do a notification.
		GetOut()->Put(Parent, OutPrefix, " ", msg.GetBlock(), NULL);
	return (write(pipe_out, msg.GetBlock(), msg.GetLength())==msg.GetLength());
}

void Game::SendMsg(int secs, const char * first, ...){
	if(cleanup) return;
	int stamp = time(NULL) + secs; //Conserv
	va_list vl;
	va_start(vl, first);
	StringCollector msg(first);
	const char * str;
	while((str = va_arg(vl, const char *)))
		msg.Push(str);
	//if(str + strlen(str) -1 != '\n') msg.Push("\n");
	msg.GetBlock(); 
	va_end(vl);
	//pthread_mutex_lock(&mutex);
	if(msg_ready){
		TimedMsg * tmsg = new TimedMsg; //Deleted in Game::MsgTimer
		tmsg->Stamp = stamp;
		tmsg->Msg = new char[msg.GetLength()+1]; //Deleted in D'tor of struct TimedMsg
		strcpy(tmsg->Msg,msg.GetBlock());
		tmsg->SendTo = this;
		MsgQueue.push_back(tmsg); //No need to lock here.
		if(msg_ready) pthread_cond_signal(&msgcond); //Update
	} else {
		SendMsg(msg.GetBlock());
	}
}

bool Game::SendMsg(const std::string msg){
	return SendMsg(msg.c_str(), NULL);
}

void * Game::MsgTimer(void * foo){
	pthread_mutex_lock(&foomutex); //Whyever.
	time_t nextevent = 0;
	while(true){
		if(MsgQueue.size() > 0) {
			timespec ts;
			ts.tv_sec = nextevent; 
			pthread_cond_timedwait(&msgcond, &foomutex, &ts);
		} else pthread_cond_wait(&msgcond, &foomutex);
		if(!msg_ready) 
			break;
		nextevent=0;
		pthread_mutex_lock(&msgmutex);
		int i=MsgQueue.size(); while(i-->0){
			if(MsgQueue[i]->Stamp <= time(NULL)){
				(MsgQueue[i]->SendTo)->SendMsg(MsgQueue[i]->Msg, NULL);
				delete MsgQueue[i];
				MsgQueue.erase(MsgQueue.begin()+i);
			} else {
				if(nextevent == 0 || MsgQueue[i]->Stamp <= nextevent) nextevent = MsgQueue[i]->Stamp;
			}
		}
		pthread_mutex_unlock(&msgmutex);
	}
	pthread_mutex_unlock(&foomutex);
	return NULL;
}

