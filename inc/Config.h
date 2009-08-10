#ifndef ConfigH
#define ConfigH

#ifndef DEFAULT_SQL_NAME
	#define DEFAULT_SQL_NAME "crctrl"
#endif
#ifndef DEFAULT_SQL_PW
	#define DEFAULT_SQL_PW "sLZpTCMMHZmnvebA"
#endif
#ifndef DEFAULT_SQL_DB
	#define DEFAULT_SQL_DB "crctrl"
#endif

#include <boost/regex.hpp>
#include <mysql++.h>
#include <string>
#include <signal.h>
#include "helpers/StringFunctions.hpp"
#include <math.h>

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
			ScenPath(NULL),
			ExtraNames(NULL),
			PW(NULL),
			NameCount(0),
			DeleteStrings(del),
			Fixed(false),
			DbIndex(index)
		{}
		ScenarioSet(const ScenarioSet * base) :
			ExtraNames (NULL),
			PW(NULL),
			NameCount (0),
			DeleteStrings(true),
			LobbyTime (base->LobbyTime),
			League (base->League),
			HostChance (base->HostChance),
			Fixed(false),
			DbIndex(base->DbIndex)
		{ //Names and pw will be ignored
			ScenPath = new char[strlen(base->ScenPath) + 1];
			strcpy(ScenPath, base->ScenPath);
		}
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

struct BanSet {
	boost::regex * NamePattern;
	const char * Reason;
	BanSet(boost::regex * namepat, char * reason):
		NamePattern (namepat), Reason (reason)
	{}
	~BanSet(){
		delete NamePattern;
		delete [] Reason;
	}
};

class Setting{
	private:
		pthread_mutex_t mutex;
		struct{
			const char * usr;
			const char * pw;
			const char * db;
			const char * addr;
		} Login;
	public: //FIXME!
		struct{
			int TCP;
			int UDP;
		} Ports;
		int QueryPort;
		int LobbyTime;
		float League;
		bool SignOn;
		bool Record;
		char * Path;
		char * ConfigPath;
		ScenarioSet ** Scens;
		int ScenCount;
		BanSet ** Bans;
		int BanCount;
		float ChanceTotal;
		unsigned int MaxExecTrials;
		unsigned int MaxQueueSize;
	public:
		Setting();
		void Standard();
		void Reload();
		const ScenarioSet * GetScen(int);
		const ScenarioSet * GetScen();
		ScenarioSet * GetScen(const char *);
		void SetLoginData(const char *, const char *, const char *, const char *);
		const char * GetBan(const char *);
};

Setting * GetConfig();

#endif