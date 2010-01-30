
#include "GameControl.h"

#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <boost/regex.hpp>
#include "helpers/StringCollector.hpp"
#include "helpers/AppManagement.h"
#include "helpers/Stream.h"
#include "helpers/ExternalApp.h"
#include "Lib.hpp"
#include "Control.h"
#include "AutoHost.h"

pthread_t Game::msgtid;
pthread_cond_t Game::msgcond;
pthread_mutex_t Game::msgmutex;
std::vector <TimedMsg *> Game::MsgQueue;
bool Game::msg_ready;


Game::Game(AutoHost & parent) :
	OutPrefix(parent.GetPrefix()),
	Parent(&parent)
{
	cleanup=false;
	Settings.Scen = 0;
	clonk = NULL;
	ExecTrials=0;
	//Standard Settings:
	Settings.Ports.TCP=GetConfig()->Ports.TCP;
	Settings.Ports.UDP=GetConfig()->Ports.UDP;
	Settings.SignOn=GetConfig()->SignOn;
	Settings.Record=GetConfig()->Record;
	Status=Setting;
}

void Game::Init() {
	#ifndef NO_GAME_TIMED_MSGS
		if(msg_ready) return;
		pthread_mutex_init(&msgmutex, NULL);
		pthread_cond_init(&msgcond, NULL);   // -"-
		pthread_create(&msgtid, NULL, &Game::MsgTimer, NULL);
		msg_ready = true;
		//msgtid=NULL; FIXME: Understand, why that line was here...
	#endif
}

bool Game::SetScen(const char * path){ //DEPRECATED
	if(Settings.Scen && Status==Setting && !Settings.Scen->IsFixed()){
		StatusUnstable();
		Settings.Scen->SetPath(path);
		StatusStable();
		return true;
	}
	return false;
}

bool Game::SetScen(const ScenarioSet * scen){
	if(!scen) return false;
	StatusUnstable();
	if(Settings.Scen) delete Settings.Scen;
	Settings.Scen = new ScenarioSet(*scen);
	StatusStable();
	return true;
}

void Game::Start(){
	if(!Settings.Scen) throw "Trying to start a game: No Scenario was set.";
	Settings.Scen->Fix();
	//std::string cmd;
	StringCollector cmd("\"\"/fullscreen\"");
	if(Settings.SignOn) cmd += " \"/signup\"";
	else cmd += " \"/nosignup\"";
	if(Settings.Scen->GetLeague() > static_cast<float>(rand())/RAND_MAX)cmd += " \"/league\"";
	else cmd += " \"/noleague\"";
	if(Settings.Record) cmd += " \"/record\"";
	cmd += " \"/tcpport:";
	cmd	+= Settings.Ports.TCP;
	cmd	+= "\"";
	cmd	+= " \"/udpport:";
	cmd	+= Settings.Ports.UDP;
	cmd += "\"";
	if(Settings.Scen->GetTime()>0){
		cmd += " \"/lobby:";
		cmd += Settings.Scen->GetTime();
		cmd += "\""; 
	}
	if(*(GetConfig()->ConfigPath)){
		cmd += " /config:\"";
		cmd += GetConfig()->ConfigPath;
		cmd += "\"";
	}
	if(Settings.Scen->GetPW() && *(Settings.Scen->GetPW())){
		cmd += " /pass:\"";
		cmd += Settings.Scen->GetPW();
		cmd += "\"";
	}
	cmd += " \"";
	cmd += Settings.Scen->GetPath();
	cmd += "\"";
	StatusUnstable();
	worker = pthread_self();
	delete clonk;
	clonk = new Process();
	char * fullpath = new char[strlen(GetConfig()->Path) + 7];
	strcpy(fullpath, GetConfig()->Path);
	strcpy(fullpath+strlen(GetConfig()->Path), "clonk");
	clonk -> SetArguments(fullpath, cmd.GetBlock(), GetConfig()->Path);
	delete [] fullpath;
	if(!clonk -> Start()) Fail();
	Status = PreLobby;
	StatusStable();
	if(!msg_ready) Init();
	Control();
}

void Game::Control(){
	std::string line;
	boost::smatch regex_ret;
	while(clonk->ReadLine(&line)){
		StatusUnstable();
		GetOut()->Put(Parent, OutPrefix, " ", line.c_str(), NULL);
		//Scan for events
		if(regex_match(line, regex_ret, rx::cm_base)){
			if(!regex_ret[2].compare("hilf") || !regex_ret[2].compare("help")) {
				SendMsg("Liste aller Befehle:\n----%list\n--------Listet alle verfügbaren Szenarien auf\n----%start Szenname -lobby:Sekunden -passwort:\"pw\" -liga\n--------Startet ein Szenario. Alles ab -lobby ist optional.\n----%hilf\n--------Gibt das hier aus.\n", NULL);
			} else if(!regex_ret[2].compare("list")) {
				StringCollector list("Folgende Szenarien koennen gestartet werden:\n", false);
				const ScenarioSet * scn;
				GetConfig()->LockStatus();
				for(int i=0; (scn = GetConfig()->GetScen(i)); i++){
					const char * name = scn->GetName(0);
					if(name){
						list.Push("-", false);
						int i=0;
						while(name) {
							list.Push(name, false);
							name = scn->GetName(++i);
							if(name) list.Push(", ", false);
						}
						list.Push("\n");
					}
				}
				GetConfig()->UnlockStatus();
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
					scn = new ScenarioSet(*scn); //Make a Copy of it.
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
				if(!Settings.Scen->GetLeague()) {
					#ifdef NO_GAME_TIMED_MSGS
						Halt(10);
					#endif
					SendMsg(20, "/set maxplayer 1\n", NULL);
				}
			}
		} else if(Status==Load) {
			if(regex_match(line, rx::gm_go)){
				Status=Run;
			}
		} else if(Status==PreLobby) {
			if(regex_match(line, rx::ctrl_err) || regex_match(line, rx::gm_exit)) {
				StatusStable(); Fail(); return;
			}
			if(regex_match(line, rx::gm_lobby)){
				Status=Lobby;
				if(!Settings.Scen->GetLeague()) SendMsg("/set maxplayer 1337\n", NULL);
			}
			if(regex_match(line, rx::ms_flood)) {
				GetOut()->Put(Parent, OutPrefix, " Masterserver complained about too much games from this IP.");
				Halt(300);
				Fail();
			}
		}
		StatusStable();
	}
	StatusUnstable();
	delete clonk; clonk = NULL;
	StatusStable();
	if(Status==PreLobby) Fail(); 
	return;
}

void Game::Exit(bool soft /*= true*/, bool wait /*= true*/){
	StatusUnstable();
	cleanup = true;
	if(soft) {if(clonk) clonk->ClosePipeTo();}
	else {if(clonk) clonk->Kill();}
	//if(pthread_kill(msgtid, 0) == ESRCH) Deinit(); //This looks like an ugly hack, but the manpage says, kill sends signals, and 0 sends no signal, just tests.
	pthread_mutex_lock(&msgmutex);
	int i=MsgQueue.size(); 
	while(i-->0)
		if(MsgQueue[i]->SendTo == this){
			delete MsgQueue[i];
			MsgQueue.erase(MsgQueue.begin()+i);
		}
	pthread_mutex_unlock(&msgmutex);
	if(wait && clonk){
		clonk->Wait();
	}
	StatusStable();
}

Game::~Game(){
	StatusUnstable();
	Exit();
	if(clonk) clonk->Wait(false);
	delete clonk;
	if(Settings.Scen) delete Settings.Scen;
}

void Game::Deinit(){
	if(!msg_ready) return;
	msg_ready = false;
	pthread_cond_signal(&msgcond);
	pthread_join(msgtid, NULL);
	pthread_cond_destroy(&msgcond);
	pthread_mutex_destroy(&msgmutex);
	MsgQueue.resize(0);
}

bool Game::Fail(){
	StatusUnstable();
	ExecTrials++;
	if(ExecTrials >= GetConfig()->MaxExecTrials) {
		Status = Failed;
		GetOut()->Put(Parent, OutPrefix, " Maximum execution attempts for game exceeded.", NULL);
		StatusStable();
		return true;
	} else {
		StatusStable();
		Halt(5);
		if(cleanup) return true;
		Start();
		return false;
	}
}

bool Game::SendMsg(const char * first, ...){
	StatusUnstable();
	if(!clonk || !clonk->IsRunning() || (Status != Lobby && Status != Run)) {StatusStable(); return false;}
	StatusStable();
	va_list vl;
	va_start(vl, first);
	bool ret;
	ret = clonk->WriteList(first, vl);
	va_end(vl);
	if(!ret) return false;
	/*if(*first=='/') //Some commands don't have feedback. Just do a notification. FIXME: Reimplement that! I am just to lazy now
		GetOut()->Put(Parent, OutPrefix, " ", msg.GetBlock(), NULL);*/
	return true;
}

void Game::SendMsg(int secs, const char * first, ...){
	if(cleanup) return;
	time_t stamp = time(NULL) + secs; //Conserv
	if(!msg_ready) Init();
	va_list vl;
	va_start(vl, first);
	StringCollector msg(first, false);
	const char * str;
	while((str = va_arg(vl, const char *)))
		msg.Push(str, false);
	//if(str + strlen(str) -1 != '\n') msg.Push("\n");
	msg.GetBlock(); 
	va_end(vl);
	//pthread_mutex_lock(&mutex);
	#ifndef NO_GAME_TIMED_MSGS
		if(msg_ready){
			TimedMsg * tmsg = new TimedMsg; //Deleted in Game::MsgTimer
			tmsg->Stamp = stamp;
			tmsg->Msg = new char[msg.GetLength()+1]; //Deleted in D'tor of struct TimedMsg
			strcpy(tmsg->Msg,msg.GetBlock());
			tmsg->SendTo = this;
			pthread_mutex_lock(&msgmutex);
			MsgQueue.push_back(tmsg);
			pthread_mutex_unlock(&msgmutex);
			if(msg_ready) pthread_cond_signal(&msgcond); //Update
		} else {
			SendMsg(msg.GetBlock());
		}
	#else 
		SendMsg(msg.GetBlock());
	#endif
}

bool Game::SendMsg(const std::string msg){
	return SendMsg(msg.c_str(), NULL);
}

void * Game::MsgTimer(void * foo){
	pthread_mutex_lock(&msgmutex); //Whyever.
	time_t nextevent = 0;
	while(true){
		if(MsgQueue.size() > 0) {
			timespec ts;
			ts.tv_sec = (long)nextevent; 
			pthread_cond_timedwait(&msgcond, &msgmutex, &ts);
		} else pthread_cond_wait(&msgcond, &msgmutex);
		if(!msg_ready) 
			break;
		nextevent=0;
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
	pthread_mutex_unlock(&msgmutex);
	pthread_mutex_destroy(&msgmutex);
	return NULL;
}

