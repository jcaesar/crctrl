enum GameStatus {Setting, PreLobby, Lobby, Load, Run, End};
struct TimedMsg{time_t Stamp; char * Msg; ~TimedMsg(){if(Msg != NULL) delete Msg;}};

class Game;

static class GameRegister{
	private:
		std::vector <Game *> Register;
		int Count;
		int Index;
		pthread_mutex_t mutex;
	public:
		int Add(Game *);
		bool Delete(Game *);
		void DelAll();
		Game * Find(const char *);
		bool Exists(Game *);
		GameRegister() :
			Count(0),
			Index(0)
		{
			pthread_mutex_init(&mutex, NULL);
		}
		~GameRegister(){ //This class gets desinitialized, what doesn't happen to non-static classes. Gotta use this °_o
			DelAll();
			pthread_mutex_destroy(&mutex);
		}
} Games;

class Game
{
	private:
		int pipe_out, pipe_err; //from current process view. 
		StreamReader *sr;
		pthread_t tid;
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
		bool Selfkill; //Maybe transfer to struct Status, when it's implemented
		char * OutPrefix;
		GameStatus Status;
		int ExecTrials;
		pthread_cond_t msgcond;
		pthread_mutex_t msgmutex;
		bool use_conds;
		pthread_t msgtid;
		bool cleanup;
		int Start(const char *);
		bool Fail();
		void Control();
		static void * ControlThreadWrapper(void *);
		void MsgTimer();
		static void * MsgThreadWrapper(void *);
	public:
		Game(const char* args){ //Deprecated!
			Start(args);
		}
		Game();
		void Start();
		bool SendMsg(const char *, ...);
		void SendMsg(int, const char *, ...);
		bool SendMsg(const std::string);
		void Exit(bool, bool);
		void AwaitEnd();
		~Game();
		GameStatus GetStatus(){return Status;}
		bool SetScen(const char *);
		bool SetScen(const ScenarioSet *, bool=true);
		bool SetSignOn(bool signon){if(Status==Setting){Settings.SignOn=signon; return true;} return false;}
		bool SetLeague(bool league){if(Status==Setting){Settings.League=league; return true;} return false;}
		bool SetPorts(int tcp, int udp){if(Status==Setting){Settings.Ports.TCP=tcp; Settings.Ports.UDP=udp; return true;} return false;}
		bool SetLobbyTime(int secs){if(Status==Setting){Settings.LobbyTime=secs; return true;} return false;}
		char * GetName() const{return OutPrefix;}
		void KillOnEnd(bool yes=true) {Selfkill=yes;}
};