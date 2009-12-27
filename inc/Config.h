#ifndef ConfigH
#define ConfigH

#include <pthread.h>
#include <boost/regex.hpp>
#include "helpers/StatusLocks.h"

class ScenarioSet;
class ScenarioSet {
	private:
		char * ScenPath;
		char ** ExtraNames;
		char * PW;
		int NameCount;
		int LobbyTime;
		float League;
		float HostChance;
		bool Fixed;
		const int DbIndex;
	public:
		ScenarioSet(int index);
		ScenarioSet(ScenarioSet const& base);
		~ScenarioSet();
		void SetPath(const char * path);
		void SetNames(const char * const * names, int count);
		void SetPW(const char * pw);
		void SetTime(const int time);
		void SetChance(const float chance);
		void SetLeague(const float chance);
		void Fix();
		bool IsFixed() const;
		const char * GetPath() const;
		int GetIndex() const;
		int GetTime() const;
		float GetLeague() const;
		float GetChance() const;
		const char * GetPW() const;
		const char * GetName(int index=0) const;
		const bool operator==(const ScenarioSet & o);
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

class Setting : public StatusLocks {
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