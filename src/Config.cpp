
#include "Config.h"

#include <signal.h>
#include <math.h>
#include <string>
#include <boost/regex.hpp>
#include <mysql++.h>
#include "helpers/AppManagement.h"
#include "helpers/StringFunctions.h"
#include "Control.h"

//Check login definitions
#if (((!defined DEFAULT_SQL_NAME) || (!defined DEFAULT_SQL_PW) || (!defined DEFAULT_SQL_DB) || (!defined DEFAULT_SQL_ADDR)) && (!defined RUNTIME_LOGIN))
	#error "You tricked your build system, eh? Go and re-cmake!"
#endif
#ifndef DEFAULT_SQL_NAME
	#define DEFAULT_SQL_NAME "crctrl"
#endif
#ifndef DEFAULT_SQL_PW
	#define DEFAULT_SQL_PW "" //Dangerous not to use a password
#endif
#ifndef DEFAULT_SQL_DB
	#define DEFAULT_SQL_DB "crctrl"
#endif
#ifndef DEFAULT_SQL_ADDR
	#define DEFAULT_SQL_ADDR NULL
#endif

static Setting Config;

inline Setting * GetConfig(){
	return &Config;
}

Setting::Setting(){
	Login.addr = NULL; SetLoginData(DEFAULT_SQL_NAME, DEFAULT_SQL_PW, DEFAULT_SQL_DB, DEFAULT_SQL_ADDR);
	Path = NULL;
	ConfigPath = NULL;
	ScenCount=0;
	Scens = NULL;
	BanCount=0;
	Bans = NULL;
	Standard();
}

void Setting::Standard(){
	StatusUnstable();
	Ports.TCP=11112;
	Ports.UDP=11113;
	QueryPort=11110;
	LobbyTime=180;
	League=0.5f;
	ScenCount=0;
	ChanceTotal=0;
	MaxQueueSize=3;
	SignOn=true;
	delete [] Path;
	Path = new char[11];
	strcpy(Path, "/usr/games");
	delete [] ConfigPath;
	ConfigPath = new char[1];
	*ConfigPath = 0;
	if(Bans){
		Bans += BanCount;
		while(BanCount--){
			Bans--;
			delete *Bans;
		}
		delete [] Bans;
		Bans = NULL;
	}
	if(Scens) {
		Scens += ScenCount;
		while(ScenCount--){
			Scens--;
			delete *Scens;
		}
		delete [] Scens;
		Scens = NULL;
	}
	StatusStable();
}


void Setting::SetLoginData(const char * usr, const char * pw, const char * db, const char * addr){
	StatusUnstable();
	if(usr != NULL) Login.usr = usr; //Fixme: Copy that.
	if(pw != NULL) Login.pw = pw;
	if(db != NULL) Login.db = db;
	if(addr != NULL) Login.addr = addr;
	StatusStable();
}

void Setting::Reload(){
	StatusUnstable();
	Standard();
	mysqlpp::Connection conn(false);
	if (conn.connect(Login.db, Login.addr, Login.usr, Login.pw)) {
		//Parse options
		mysqlpp::Query query1 = conn.query("SELECT Identifier, Value FROM Settings");
		if (mysqlpp::UseQueryResult res = query1.use()) {
			while(mysqlpp::Row row=res.fetch_row()) { //What about an hash-array and clean getters?
				if(		!row[0].compare("LobbyTime"))    LobbyTime=int(row[1]);
				else if(!row[0].compare("LeagueChance")) League=float(row[1]);
				else if(!row[0].compare("SignOn"))       SignOn=(!row[1].compare("false")?false:true);
				else if(!row[0].compare("Record"))       Record=(!row[1].compare("true")?true:false);
				else if(!row[0].compare("TCPPort"))      Ports.TCP=int(row[1]);
				else if(!row[0].compare("UDPPort"))      Ports.UDP=int(row[1]);
				else if(!row[0].compare("QueryPort"))    QueryPort=int(row[1]);
				else if(!row[0].compare("MaxQueueSize")) MaxQueueSize=int(row[1]);
				else if(!row[0].compare("MaxExecTrials"))MaxExecTrials=int(row[1]);
				else if(!row[0].compare("ConfigPath")){
					delete [] ConfigPath;
					ConfigPath = new char[row[1].length()+1];
					strncpy(ConfigPath,row[1].c_str(),row[1].length()+1);
				}
				else if(!row[0].compare("Path")){ //Funny to treat this different to ConfigPath...
					delete [] Path;
					int len=row[1].length();
					Path = new char[len+1]; *(Path + len)=0;
					strncpy(Path,row[1].c_str(),len);
				}
			}
		} else {
			std::cerr << "Failed to get settings list: " << query1.error() << std::endl;
		}
		mysqlpp::Query query2 = conn.query("SELECT Count(*) FROM ScenarioList");
		if (mysqlpp::UseQueryResult res = query2.use()) {
			int cnt = int(res.fetch_row()[0]);
			while(res.fetch_row());
			if(cnt == 0) GetOut()->Put(NULL, "Warning: Loading empty scenario list.");
			//Parse Scens.
			mysqlpp::Query query3 = conn.query("SELECT Path, HostChance, LeagueChance, LobbyTime, ScenIndex FROM ScenarioList ORDER BY ScenIndex DESC");
			if(mysqlpp::UseQueryResult res = query3.use()){
				Scens = new ScenarioSet * [cnt];
				ChanceTotal=0;
				while(mysqlpp::Row row=res.fetch_row()) {
					cnt--; 
					if(cnt < 0) break; //goodness knows... and SEG is smarter than SQL.
					(*(Scens + cnt)) = new ScenarioSet(int(row[4]), ScenCount);
					(*(Scens + cnt))->SetPath(row[0].c_str());
					(*(Scens + cnt))->SetChance(float(row[1]));
					(*(Scens + cnt))->SetLeague(float(row[2]));
					(*(Scens + cnt))->SetTime(int(row[3]));
					ScenCount++;
					//Fix() later.
					ChanceTotal += (**(Scens + cnt)).GetChance();
				}
				//Parse Scen names
				mysqlpp::Query query4 = conn.query("SELECT ScenIndex, Name FROM ScenarioNames ORDER BY ScenIndex DESC"); 
				if(mysqlpp::StoreQueryResult names = query4.store()){
					int i=names.num_rows()-1;
					while(i>=0){
						int curindex = names[i][0];
						int j = i-1;
						while(j>=0 && int(names[j][0]) == curindex) --j;
						char ** nameptr = new char * [i-j];
						char ** ptrinst; ptrinst = nameptr;
						while(i > j) {
							int len=names[i][1].length()+1;
							*ptrinst = new char[len];
							strncpy(*ptrinst, names[i][1].c_str(), len);
							ptrinst++; i--;
						}
						ScenarioSet * scn = GetScenByDbIndex(curindex);
						scn->SetNames(nameptr, ptrinst-nameptr);
						scn->Fix();
						while((ptrinst--)-nameptr) delete [] *ptrinst;
						delete [] nameptr;
					}
				} else {
					std::cerr << "Error in naming scenarios: " << query4.error() << std::endl;
				}
			} else {
				std::cerr << "Failed to get scenario list: " << query3.error() << std::endl;
			}
		} else {
			std::cerr << "Failed to get scenario count: " << query2.error() << std::endl;
		}
		mysqlpp::Query query5 = conn.query("SELECT Count(*) FROM Bans"); 
		if(mysqlpp::UseQueryResult res = query5.use()){
			int cnt = int(res.fetch_row()[0]);
			while(res.fetch_row());
			mysqlpp::Query query6 = conn.query("SELECT Name, Reason FROM Bans"); 
			if(mysqlpp::UseQueryResult bans = query6.use()){
				Bans = new BanSet*[cnt]; //Deleted in Standard()
				BanCount = cnt;
				while(mysqlpp::Row row=bans.fetch_row()) {
					cnt--;
					if(cnt < 0) break;
					int len = row[0].length()+5;
					char * nameps = new char[len]; //Deleted here
					strncpy(nameps+2,row[0].c_str(),len-4);
					strncpy(nameps, ".*", 2); strcpy(nameps+len-3, ".*"); //FIXME These .* should not be nescessary
					len = row[1].length()+1;
					char * reason = new char[len]; //Deleted in d'tor of BanSet
					strncpy(reason,row[1].c_str(),len);
					boost::regex * namepat;
					try {namepat = new boost::regex(nameps, boost::regex_constants::icase);} //Deleted in d'tor of BanSet
					catch (boost::regex_error& e) {
						std::cerr << "Ban error: " << nameps << " is not a valid regular expression: \"" << e.what() << "\"" << std::endl;
						*namepat = "^$";
					}
					delete [] nameps;
					(*(Bans + cnt)) = new BanSet(namepat, reason); //Deleted in Standard()
				}
			} else {
				std::cerr << "Error in retriving Bans: " << query6.error() << std::endl;
			}
		} else {
			std::cerr << "Error in counting Bans: " << query5.error() << std::endl;
		}	
	} else {
		std::cerr << "DB connection failed: " << conn.error() << std::endl;
	}
	StatusStable();
}

ScenarioSet * Setting::GetScen(int index) {
	if(0 > index || index >= ScenCount) {return NULL;}
	return *(index + Scens);
}

ScenarioSet * Setting::GetScenByDbIndex(int index) {
	int i=0;
	while(((Scens[i]->GetIndex()) != index) && (i<ScenCount)) ++i;
	if(i<ScenCount) return Scens[i];
	return NULL;
}

ScenarioSet * Setting::GetScen(){ //Do it by random.
	LockStatus();
	if(ScenCount == 0) return NULL; //No Scen = Silly person. This is gonna fail later, I should perhaps error earlier when no scen is given.
	if(ScenCount == 1) return *Scens;  //One Scen = Bla, that's not gonna segv directly anymore, so noone testing could be annoyed.
	srand((unsigned)time(NULL) ^ rand()); 
	double rnd=fmod((float)rand(), ChanceTotal*16) / 16;
	ScenarioSet ** ScenInst = Scens;
	while(rnd >= 0){
		assert(ScenInst < Scens + ScenCount);
		rnd -= (**ScenInst).GetChance();
		ScenInst++;
	}
	UnlockStatus();
	return (*(ScenInst-1));
}

ScenarioSet * Setting::GetScen(const char * search){
	LockStatus();
	int cnt=ScenCount;
	ScenarioSet ** ScenInst = Scens;
	while(cnt--){
		const char * name;
		for(int i=0; (name=(*ScenInst)->GetName(i)); i++){
			if(nocasecmp(search, name)) {UnlockStatus(); return *ScenInst;}
		}
		ScenInst++;
	}
	UnlockStatus();
	return NULL;
}

const char * Setting::GetBan(const char * name){
	LockStatus();
	int cnt = BanCount;
	BanSet ** BanInst; BanInst = Bans;
	while(cnt--){
		if(regex_match(name, *((*BanInst)->NamePattern))){
			UnlockStatus();
			return (*BanInst)->Reason;
		}
		BanInst++; //Dereferencing with gdb: print (*(((*BanInst)->NamePattern)->m_pimpl.px)).m_expression
	}
	UnlockStatus();
	return NULL;
}



ScenarioSet::ScenarioSet(int dbindex, int intindex) :
	ScenPath(NULL),
	ExtraNames(NULL),
	PW(NULL),
	NameCount(0),
	LobbyTime(GetConfig()->LobbyTime),
	League(GetConfig()->League),
	Fixed(false),
	DbIndex(dbindex),
	IntIndex(intindex)
{}

ScenarioSet::ScenarioSet(ScenarioSet const& base) :
	ScenPath (NULL),
	ExtraNames (NULL),
	PW(NULL),
	NameCount (0),
	LobbyTime (base.LobbyTime),
	League (base.League),
	HostChance (base.HostChance),
	Fixed(false),
	DbIndex(base.DbIndex),
	IntIndex(base.IntIndex)
{}

ScenarioSet::~ScenarioSet(){
	delete [] ScenPath;
	if(ExtraNames) {
		while(NameCount--) delete [] ExtraNames[NameCount];
		delete [] ExtraNames;
	}
	delete [] PW;
}

void ScenarioSet::SetPath(const char * path){
	if(!Fixed) {
		delete [] ScenPath;
		ScenPath = new char [strlen(path)+1];
		strcpy(ScenPath, path);
	}
}

void ScenarioSet::SetNames(const char * const * names, int count){
	if(!Fixed) {
		if(NameCount != 0) {
			while(NameCount--) delete [] *ExtraNames++;
			delete [] ExtraNames;
		}
		NameCount=count;
		ExtraNames = new char * [NameCount];
		while(count--) {
			ExtraNames[count] = new char [strlen(names[count])+1];
			strcpy(ExtraNames[count], names[count]);
		}
	}
}

void ScenarioSet::SetPW(const char * pw) {
	if(!Fixed){
		delete [] PW;
		PW = new char [strlen(pw) + 1];
		strcpy(PW, pw);
	}
}

void ScenarioSet::SetTime(int time){ if(!Fixed) LobbyTime=time; }

void ScenarioSet::SetChance(float chance){ if(!Fixed) HostChance=chance; }

void ScenarioSet::SetLeague(float chance){ if(!Fixed) League=chance; }

void ScenarioSet::Fix(){ Fixed=true; }

bool ScenarioSet::IsFixed() const {return Fixed; }

const char * ScenarioSet::GetPath() const {
	if(ScenPath) return ScenPath;
	assert(GetConfig()->GetScen(IntIndex));
	return GetConfig()->GetScen(IntIndex)->ScenPath;
}

int ScenarioSet::GetIndex() const {return DbIndex;}

int ScenarioSet::GetTime() const {return LobbyTime;}

float ScenarioSet::GetLeague() const {return League;}

float ScenarioSet::GetChance() const {return HostChance;}

const char * ScenarioSet::GetPW() const {return PW;}

const char * ScenarioSet::GetName(int index /*=0*/) const {
	const ScenarioSet * readfrom;
	if(!ExtraNames && (readfrom=GetConfig()->GetScen(IntIndex)));
	else readfrom = this;
	assert(readfrom && readfrom->IntIndex == IntIndex);
	if(!readfrom || index < 0 || index >= readfrom->NameCount) return NULL;
	return *(readfrom->ExtraNames + index);
}

const bool ScenarioSet::operator==(const ScenarioSet & o){ //Names & PW & HostChance ignored.
	if(ScenPath && o.ScenPath && !strcmp(ScenPath, o.ScenPath)) return true;
	else return false;
}