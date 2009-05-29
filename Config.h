class ScenarioSet;
class ScenarioSet {
	private:
		char * ScenPath;
		char ** ExtraNames;
		char * PW;
		int NameCount;
		bool DeleteStrings;
		int LobbyTime;
		float League;
		float HostChance;
		bool Fixed;
		const int DbIndex;
	public:
		ScenarioSet(int index, bool del=true) :
			DbIndex(index),
			NameCount(0),
			DeleteStrings(del),
			ScenPath(NULL),
			Fixed(false),
			PW(NULL)
		{}
		~ScenarioSet(){
			if(DeleteStrings){
				if(ScenPath) delete [] ScenPath;
				if(ExtraNames) {
					while(NameCount--) delete [] *ExtraNames--;
					delete [] ExtraNames;
				}
			}
			if(PW) delete [] PW;
		}
		ScenarioSet & ScenarioSet(const ScenarioSet & base) :
			NameCount (NULL),
			LobbyTime (false),
			League (base.League),
			HostChance (base.HostChance),
			Fixed(false),
			DbIndex(base.DbIndex),
			PW(NULL),
			DeleteStrings(true)
		{ //Names and pw will be ignored
			ScenPath = new char[strlen(base.ScenPath) + 1];
			strcpy(ScenPath, base.ScenPath);
		}
		void SetPath(char * path){
			if(!Fixed) {
				if(ScenPath && DeleteStrings) delete [] ScenPath;
				ScenPath=path;
			}
		}
		void SetNames(char ** names, int count){
			if(!Fixed){
				if(NameCount != 0 && DeleteStrings){
					while(NameCount--) delete [] *ExtraNames++;
					delete [] ExtraNames;
				}
				ExtraNames=names;
				NameCount=count;
			}
		}
		void SetPW(const char * pw){ //I always copy it, so I always delete it. Why? No idea.
			if(!Fixed){
				if(PW) delete [] PW;
				PW = new char [strlen(pw) + 1];
				strcpy(PW, pw);
			}
		}
		void SetTime(int time){ if(!Fixed) LobbyTime=time; }
		void SetChance(float chance){ if(!Fixed) HostChance=chance;}
		void SetLeague(float chance){ if(!Fixed) League=chance;}
		void Fix(){ Fixed=true; }
		bool IsFixed() const {return Fixed;}
		const char * GetPath() const {return ScenPath;}
		int GetIndex() const {return DbIndex;}
		int GetTime() const {return LobbyTime;}
		float GetLeague() const {return League;}
		float GetChance() const {return HostChance;}
		const char * GetPW() const {return PW;}
		const char * GetName(int index=0) const {
			if(index < 0 || index >= NameCount) return NULL;
			return *(ExtraNames + index);
		}
};

static class ConfigurationStore{ //Just a fucking silly name. I only need it for the Constructor.
	private:
		pthread_mutex_t mutex;
	public: //FIXME!
		struct{
			int TCP;
			int UDP;
		} Ports;
		int LobbyTime;
		float League;
		bool SignOn;
		bool Record;
		char * Path;
		char * ConfigPath;
		ScenarioSet ** Scens;
		int ScenCount;
		float ChanceTotal;
		int MaxExecTrials;
	public:
		ConfigurationStore(){
			Path = new char[1];
			ConfigPath = new char[1];
			pthread_mutex_init(&mutex, NULL);
			ScenCount=0;
			Scens = NULL;
			Standard();
		}
		void Standard(){
			Ports.TCP=11112;
			Ports.UDP=11113;
			LobbyTime=180;
			League=0.5f;
			ScenCount=0;
			ChanceTotal=0;
			SignOn=true;
			delete [] Path;
			Path = new char[11];
			strcpy(Path, "/usr/games");
			delete [] ConfigPath;
			ConfigPath = new char[1];
		}
		void Reload(const char *, const char *, const char *, const char * = NULL);
		const ScenarioSet * GetScen(int);
		const ScenarioSet * GetScen();
		ScenarioSet * GetScen(const char *);
} Config;