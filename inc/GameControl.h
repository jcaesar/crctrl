class Game;
#ifndef GameControlH
#define GameControlH

#include <pthread.h>
#include <vector>
#include <string>
#include "helpers/StatusLocks.h"
class StreamReader; //#include "helpers/Stream.h"
class Process; //#include "helpers/ExternalApp.h"
class AutoHost; //#include "AutoHost.h"
#include "Config.h" //FIXME: Try not to include it here...

enum GameStatus {Setting, PreLobby, Lobby, Load, Run, End, Failed};
struct TimedMsg{time_t Stamp; char * Msg; Game * SendTo; ~TimedMsg(){if(Msg != NULL) delete Msg;}};

class Game : public StatusLocks
{
	private:
		Process * clonk;
		pthread_t worker;
		struct{
			bool SignOn;
			bool Record;
			struct {
				int UDP;
				int TCP;
			} Ports;
			ScenarioSet * Scen;
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
		Game(AutoHost &);
		void Start();
		bool SendMsg(const char *, ...);
		bool SendMsg(const std::string);
		void Exit(bool = true, bool = true);
		void AwaitEnd();
		~Game();
		GameStatus GetStatus(){return Status;}
		bool SetScen(const char *);
		bool SetScen(const ScenarioSet *);
		bool SetSignOn(bool signon){if(Status==Setting){Settings.SignOn=signon; return true;} return false;}
		bool SetLeague(bool league){if(Status==Setting){Settings.Scen->SetLeague(league?1.f:0.f); return true;} return false;}
		bool SetPorts(int tcp, int udp){if(Status==Setting){Settings.Ports.TCP=tcp; Settings.Ports.UDP=udp; return true;} return false;}
		bool SetLobbyTime(int secs){if(Status==Setting){Settings.Scen->SetTime(secs); return true;} return false;}
		const char * GetPrefix() const {return OutPrefix;}
		const char * GetPath() const {return Settings.Scen->GetPath();}
		GameStatus GetStatus() const {return Status;}
		const ScenarioSet * GetScen() const {return Settings.Scen;}
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