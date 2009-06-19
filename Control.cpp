#ifndef ControlCpp
#define ControlCpp

StreamControl::StreamControl(int fdin, int fdout):
	fd_out(fdout),
	sr(new StreamReader(fdin))
{
	Out.Add(fdout);
	pthread_create(&tid, NULL, &StreamControl::ThreadWrapper, this);
	sel=NULL;
}

void StreamControl::Work(){
	std::string cmd;
	while(sr->ReadLine(&cmd)){
		if(startswith(&cmd,"%start")){
			sel=new Game;
			int pos;
			{ //Gonna get the Scen name from the end.
				bool inv_comma=false;
				const char *p; const char *a; a=cmd.data(); p=a+cmd.length();
				while(p>a){
					p--;
					if(*p=='"'){
						inv_comma = !inv_comma;
					}
					if(*p==' ' and !inv_comma){
						sel->SetScen(p+1);
						break;
					}
				}
			}
			if((pos=cmd.find(" time:"))!=-1){
				sel->SetLobbyTime(atoi(cmd.data()+pos+6));
			}
			if((pos=cmd.find(" ports:"))!=-1){
				char * end;
				int tcp=strtol(cmd.data()+pos+7, &end, 10);
				sel->SetPorts(tcp,atoi(end+1));
			}
			if((cmd.find(" league "))!=-1){ sel->SetLeague(true);}
			else if((cmd.find(" noleague "))!=-1){ sel->SetLeague(false);	}
			if((cmd.find(" nosignup "))!=-1){
				sel->SetSignOn(false);
			}
			sel->KillOnEnd();
			sel->Start();
		} else if(!cmd.compare("%auto")) {
			new AutoHost();
		} else if(!cmd.compare("%end")) {
			Games.DelAll();
			raise(SIGINT);
		} else if(startswith(&cmd,"%kill")) {
			if(Games.Exists(sel)){
				if(cmd.compare("%kill forced")) sel->Exit(false, false);
				else sel->Exit();
			}
		} else if(!cmd.compare("%reload")) {
			Config.Reload();
		} else if(startswith(&cmd,"%sel ")) {
			sel = Games.Find(cmd.data() + 5);
			if(sel == NULL) Out.Put("No such game: ", cmd.data() + 5, NULL);
		} else if(startswith(&cmd,"%")){
			Out.Put("No such command: ", cmd.data(), NULL);
		} else {
			if(Games.Exists(sel)) {
				cmd += "\n";
				sel->SendMsg(cmd.c_str(), NULL);
			}
		}
	} //sr is invalid after that.
	delete this;
}

void * StreamControl::ThreadWrapper(void *p) { //I wonder why I don't need a static here. (but in the .h) Or better: I MAY NOT use it...
	static_cast<StreamControl*>(p)->Work(); 
	return NULL;
}

#endif