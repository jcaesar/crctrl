
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
#if (((!defined DEFAULT_SQL_NAME) || (!defined DEFAULT_SQL_PW) || (!defined DEFAULT_SQL_DB)) && (!defined RUNTIME_LOGIN))
	#error "You tricked your build system, eh?"
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

static Setting Config;

inline Setting * GetConfig(){
	return &Config;
}

Setting::Setting(){
	SetLoginData(DEFAULT_SQL_NAME, DEFAULT_SQL_PW, DEFAULT_SQL_DB, NULL); Login.addr = NULL;
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
			mysqlpp::Query query3 = conn.query("SELECT Path, HostChance, LeagueChance, LobbyTime, ScenIndex FROM ScenarioList");
			if(mysqlpp::UseQueryResult res = query3.use()){
				Scens = new ScenarioSet * [cnt];
				ChanceTotal=0;
				while(mysqlpp::Row row=res.fetch_row()) {
					cnt--; 
					if(cnt < 0) break; //goodness knows... and SEG is smarter than SQL.
					ScenCount++;
					int len=row[0].length();
					char * scn = new char [len+1]; *(scn+len)=0; //Gets deleted from Scenclass.
					strncpy(scn,row[0].c_str(),len);
					(*(Scens + cnt)) = new ScenarioSet(int(row[4]));
					(*(Scens + cnt))->SetPath(scn);
					(*(Scens + cnt))->SetChance(float(row[1]));
					(*(Scens + cnt))->SetLeague(float(row[2]));
					(*(Scens + cnt))->SetTime(int(row[3]));
					//Fix later.
					ChanceTotal += (**(Scens + cnt)).GetChance();
				}
				mysqlpp::Query query4 = conn.query("SELECT ScenIndex, Name FROM ScenarioNames"); 
				if(mysqlpp::StoreQueryResult names = query4.store()){ //What follows here is fucking sillily done. But, what works works.
					for(cnt=0; cnt<ScenCount; cnt++){
						int index=Scens[cnt]->GetIndex();
						int namecnt=0;
						for(int i=names.num_rows()-1; i>=0; i--){
							if(index==int(names[i][0])) namecnt++;
						}
						char ** nameptr = new char * [namecnt];
						char ** ptrinst; ptrinst = nameptr;
						for(int i=names.num_rows()-1; i>=0; i--){
							if(index==int(names[i][0])) {
								int len=names[i][1].length()+1;
								*ptrinst = new char[len];
								strncpy(*ptrinst, names[i][1].c_str(), len);
								ptrinst++;
							}
						}
						Scens[cnt]->SetNames(nameptr, namecnt);
						while(namecnt--) delete [] nameptr[namecnt];
						delete [] nameptr;
						Scens[cnt]->Fix();
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

const ScenarioSet * Setting::GetScen(int index){
	if(0 > index || index >= ScenCount) {return NULL;}
	return *(index + Scens);
}

const ScenarioSet * Setting::GetScen(){ //Do it by random.
	LockStatus();
	if(ScenCount == 0) return NULL; //No Scen = Silly person. This is gonna fail later, I should perhaps error earlier when no scen is given.
	if(ScenCount == 1) return *Scens;  //One Scen = Bla, that's not gonna segv anymore, so noone testing could be annoyed.
	srand((unsigned)time(NULL) ^ rand()); 
	double rnd=fmod((float)rand(), ChanceTotal*16) / 16;
	ScenarioSet ** ScenInst = Scens;
	while(rnd >= 0){
		rnd -= (**ScenInst).GetChance();
		ScenInst++;
	}
	if(ScenInst >= Scens + ScenCount) { //Hmm, what now? SigKill? Reload? Quit?
		PanicExit(); //raise(SIGSEGV); //If it did not happen earlier.
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



ScenarioSet::ScenarioSet(int index) :
	ScenPath(NULL),
	ExtraNames(NULL),
	PW(NULL),
	NameCount(0),
	LobbyTime(GetConfig()->LobbyTime),
	League(GetConfig()->League),
	Fixed(false),
	DbIndex(index)
{}

ScenarioSet::ScenarioSet(ScenarioSet const& base) :
	ExtraNames (NULL),
	PW(NULL),
	NameCount (0),
	LobbyTime (base.LobbyTime),
	League (base.League),
	HostChance (base.HostChance),
	Fixed(false),
	DbIndex(base.DbIndex)
{ //Names and pw will be ignored
	ScenPath = new char[strlen(base.ScenPath) + 1];
	strcpy(ScenPath, base.ScenPath);
}

ScenarioSet::~ScenarioSet(){
	if(ScenPath) delete [] ScenPath;
	if(ExtraNames) {
		while(NameCount--) delete [] ExtraNames[NameCount];
		delete [] ExtraNames;
	}
	if(PW) delete [] PW;
}

void ScenarioSet::SetPath(const char * path){
	if(!Fixed) {
		if(ScenPath) delete [] ScenPath;
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
		if(PW) delete [] PW;
		PW = new char [strlen(pw) + 1];
		strcpy(PW, pw);
	}
}

void ScenarioSet::SetTime(int time){ if(!Fixed) LobbyTime=time; }

void ScenarioSet::SetChance(float chance){ if(!Fixed) HostChance=chance; }

void ScenarioSet::SetLeague(float chance){ if(!Fixed) League=chance; }

void ScenarioSet::Fix(){ Fixed=true; }

bool ScenarioSet::IsFixed() const {return Fixed; }

const char * ScenarioSet::GetPath() const {return ScenPath; }

int ScenarioSet::GetIndex() const {return DbIndex;}

int ScenarioSet::GetTime() const {return LobbyTime;}

float ScenarioSet::GetLeague() const {return League;}

float ScenarioSet::GetChance() const {return HostChance;}

const char * ScenarioSet::GetPW() const {return PW;}

const char * ScenarioSet::GetName(int index /*=0*/) const {
	const ScenarioSet * readfrom;
	if(!ExtraNames && (readfrom=GetConfig()->GetScen(index)));
	else readfrom = this;
	if(index < 0 || index >= readfrom->NameCount) return NULL;
	return *(readfrom->ExtraNames + index);
}

const bool ScenarioSet::operator==(const ScenarioSet & o){ //Names & PW & HostChance ignored.
	if(ScenPath && o.ScenPath && !strcmp(ScenPath, o.ScenPath)) return true;
	else return false;
}