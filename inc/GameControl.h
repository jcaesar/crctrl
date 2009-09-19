class Game;
#ifndef GameControlH
#define GameControlH

#include <boost/regex.hpp>
#include <signal.h>
#include <pthread.h>
#include "Lib.hpp"
#include <stdarg.h>
#include "helpers/StringCollector.hpp"
#include "Config.h"
#include "helpers/StreamReader.hpp"
#include "Control.h"
#include "AutoHost.h"
#include <sys/wait.h>
#include <errno.h>

enum GameStatus {Setting, PreLobby, Lobby, Load, Run, End, Failed};
struct TimedMsg{time_t Stamp; char * Msg; Game * SendTo; ~TimedMsg(){if(Msg != NULL) delete Msg;}};

class Game
{
	private:
		int pipe_out, pipe_err; //from current process view. 
		StreamReader *sr;
		pid_t pid;
		struct{
			bool SignOn;
			bool League;
			bool Record;
			int LobbyTime;
			struct {
				int UDP;
				int TCP;
			} Ports;
			char * Scen;
			char * PW;
		} Settings;
		const char * OutPrefix;
		GameStatus Status;
		unsigned int ExecTrials;
		bool cleanup;
		void Start(const char *);
		bool Fail();
		void Control();
		AutoHost * Parent;
	public:
		Game(AutoHost *);
		void Start();
		bool SendMsg(const char *, ...);
		bool SendMsg(const std::string);
		void Exit(bool);
		void AwaitEnd();
		~Game();
		GameStatus GetStatus(){return Status;}
		bool SetScen(const char *);
		bool SetScen(const ScenarioSet *, bool=true);
		bool SetSignOn(bool signon){if(Status==Setting){Settings.SignOn=signon; return true;} return false;}
		bool SetLeague(bool league){if(Status==Setting){Settings.League=league; return true;} return false;}
		bool SetPorts(int tcp, int udp){if(Status==Setting){Settings.Ports.TCP=tcp; Settings.Ports.UDP=udp; return true;} return false;}
		bool SetLobbyTime(int secs){if(Status==Setting){Settings.LobbyTime=secs; return true;} return false;}
		const char * GetPrefix() const{return OutPrefix;}
		const char * GetScen() const {return Settings.Scen;}
		GameStatus GetStatus() const {return Status;}
		void SendMsg(int, const char *, ...);
	private:
		static pthread_t msgtid;
		static pthread_cond_t msgcond;
		static pthread_mutex_t msgmutex;
		static std::vector <TimedMsg *> MsgQueue;
		static bool msg_ready;
		static void * MsgTimer(void *);
	public:
		static void Init();
		static void Deinit();
};

#endif