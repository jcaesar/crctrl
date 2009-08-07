enum GameStatus {Setting, PreLobby, Lobby, Load, Run, End, Failed};
struct TimedMsg{time_t Stamp; char * Msg; ~TimedMsg(){if(Msg != NULL) delete Msg;}};

class AutoHost;

class Game
{
	private:
		int pipe_out, pipe_err; //from current process view. 
		StreamReader *sr;
		pid_t pid;
		std::vector <TimedMsg *> MsgQueue;
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
		int ExecTrials;
		pthread_t msgtid;
		pthread_cond_t msgcond;
		pthread_mutex_t msgmutex;
		bool use_conds;
		bool cleanup;
		int Start(const char *);
		bool Fail();
		void Control();
		void MsgTimer();
		static void * MsgThreadWrapper(void *);
		AutoHost * Parent;
	public:
		Game(AutoHost *);
		void Start();
		bool SendMsg(const char *, ...);
		void SendMsg(int, const char *, ...);
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
};
