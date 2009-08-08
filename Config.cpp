
#include "Config.h"

void ConfigurationStore::SetLoginData(const char * usr, const char * pw, const char * db, const char * addr){
	if(usr != NULL) Login.usr = usr;
	if(pw != NULL) Login.pw = pw;
	if(db != NULL) Login.db = db;
	if(addr != NULL) Login.addr = addr;
}

void ConfigurationStore::Reload(){
	pthread_mutex_lock(&mutex);
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
	pthread_mutex_unlock(&mutex);
}

const ScenarioSet * ConfigurationStore::GetScen(int index){
	if(0 > index || index >= ScenCount) {return NULL;}
	return *(index + Scens);
}

const ScenarioSet * ConfigurationStore::GetScen(){ //Do it by random.
	if(ScenCount == 0) return NULL; //No Scen = Silly person.
	if(ScenCount == 1) return *Scens;  //One Scen = Bla, that's not gonna segv anymore, so noone testing could be annoyed.
	srand((unsigned)time(NULL)); 
	double rnd=fmod(rand(), ChanceTotal*16) / 16;
	ScenarioSet ** ScenInst = Scens;
	while(rnd >= 0){
		rnd -= (**ScenInst).GetChance();
		ScenInst++;
	}
	if(ScenInst >= Scens + ScenCount) { //Hmm, what now? SigKill? Reload? Quit?
		raise(SIGSEGV); //If it did not happen earlier.
	}
	return (*(ScenInst-1));
}

ScenarioSet * ConfigurationStore::GetScen(const char * search){
	int cnt=ScenCount;
	ScenarioSet ** ScenInst = Scens;
	while(cnt--){
		const char * name;
		for(int i=0; name=(*ScenInst)->GetName(i); i++){
			if(nocasecmp(search, name)) return *ScenInst;
		}
		ScenInst++;
	}
	return NULL;
}

const char * ConfigurationStore::GetBan(const char * name){
	int cnt = BanCount;
	BanSet ** BanInst; BanInst = Bans;
	while(cnt--){
		if(regex_match(name, *((*BanInst)->NamePattern))){
			return (*BanInst)->Reason;
		}
	}
	return NULL;
}
