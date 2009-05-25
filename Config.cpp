#ifndef ConfigCpp
#define ConfigCpp

void ConfigurationStore::Reload(const char * uname, const char * upw, const char * dbname, const char * address){
	pthread_mutex_lock(&mutex);
	Standard();
	mysqlpp::Connection conn(false);
	if (conn.connect(dbname, address, uname, upw)) {
		mysqlpp::Query query1 = conn.query("SELECT Identifier, Value FROM Settings");
		if (mysqlpp::UseQueryResult res = query1.use()) {
			while(mysqlpp::Row row=res.fetch_row()) {
				if(		!row[0].compare("LobbyTime"))    LobbyTime=int(row[1]);
				else if(!row[0].compare("LeagueChance")) League=float(row[1]);
				else if(!row[0].compare("SignOn"))       SignOn=(!row[1].compare("false")?false:true);
				else if(!row[0].compare("Record"))       Record=(!row[1].compare("true")?true:false);
				else if(!row[0].compare("TCPPort"))      Ports.TCP=int(row[1]);
				else if(!row[0].compare("UDPPort"))      Ports.UDP=int(row[1]);
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
			if(Scens != NULL) {
				while(ScenCount >= 0) delete *(Scens + (ScenCount--));
				delete [] Scens;
			}
			mysqlpp::Query query3 = conn.query("SELECT Path, HostChance, LeagueChance, LobbyTime FROM ScenarioList");
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
					Scens[cnt] = new ScenarioSet();
					(Scens + cnt)->SetPath(scn);
					(Scens + cnt)->SetChance(float(row[1]));
					(Scens + cnt)->SetLeague(float(row[2]));
					(Scens + cnt)->SetTime(int(row[3]));
					//Fix later.
					ChanceTotal += (**(Scens + cnt)).GetChance();
				}
			} else {
				delete [] Scens; Scens = NULL;
				std::cerr << "Failed to get scenario list: " << query3.error() << std::endl;
			}
		} else {
			std::cerr << "Failed to get scenario count: " << query2.error() << std::endl;
		}
	} else {
		std::cerr << "DB connection failed: " << conn.error() << std::endl;
	}
	pthread_mutex_unlock(&mutex);
}

const ScenarioSet * ConfigurationStore::GetScen(int index){
	if(0 > index || index > ScenCount) {return NULL;}
	return *(index + Scens);
}

const ScenarioSet * ConfigurationStore::GetScen(){ //Do it by random.
	srand((unsigned)time(NULL)); 
	double rnd=fmod(rand(), ChanceTotal*16) / 16;
	ScenarioSet ** ScenInst = Scens;
	while(rnd >= 0){
		rnd -= (**ScenInst).GetChance();
		ScenInst++;
	}
	return (*(ScenInst-1));
}

#endif