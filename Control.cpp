#ifndef ControlCpp
#define ControlCpp

StreamControl::StreamControl(int fdin, int fdout):
	fd_out(fdout),
	sr(new StreamReader(fdin))
{
	Out.Add(this);
	pthread_create(&tid, NULL, &StreamControl::ThreadWrapper, this);
	sel=NULL;
}

void StreamControl::Work(){
	std::string cmd;
	while(sr->ReadLine(&cmd)){
		if(!cmd.compare("%auto")) {
			new AutoHost();
		} else if(!cmd.compare("%end")) {
			AutoHosts.DelAll();
			raise(SIGINT);
		} else if(startswith(&cmd,"%stop")) {
			if(AutoHosts.Exists(sel) && sel->GetGame()){
				if(cmd.compare("%stop forced")) delete sel;
				else sel->SoftEnd(false);
			} else {
				Out.Put(this, "No Host selected.", NULL);
			}
		} else if(!cmd.compare("%reload")) {
			Config.Reload();
		} else if(startswith(&cmd,"%sel ")) {
			sel = AutoHosts.Find(atoi(cmd.data() + 5));
			if(sel == NULL) Out.Put(this, "No such host: ", cmd.data() + 5, NULL);
			else PrintStatus(sel);
		} else if(!cmd.compare("%status")) {
			PrintStatus();
		} else if(startswith(&cmd,"%")){
			Out.Put(this, "No such command: ", cmd.data(), NULL);
		} else {
			if(AutoHosts.Exists(sel) && sel->GetGame()) {
				cmd += "\n";
				sel->GetGame()->SendMsg(cmd.c_str(), NULL);
			}
		}
	} //sr is invalid after that.
	sr = NULL;
	delete this;
}

void StreamControl::PrintStatus(AutoHost * stat /*= NULL*/){
	if(AutoHosts.Exists(stat)) Out.Put(this, stat->GetPrefix(), " ", stat->GetGame()->GetScen());
	else{
		int i=0;
		while(AutoHost * ah = AutoHosts.Get(i)) PrintStatus(ah);
	}
}

bool StreamControl::Write(void * context, const char * msg){
	if(context == sel || context == this || context == 0){
		int len=strlen(msg);
		if(len != write(fd_out, msg, len)) return false;
		if(msg[--len] != '\n') if(write(fd_out, "\n", 1) != 1) return false;
		return true;
	} else return false;
}

void * StreamControl::ThreadWrapper(void *p) { //I wonder why I don't need a static here. (but in the .h) Or better: I MAY NOT use it...
	static_cast<StreamControl*>(p)->Work(); 
	return NULL;
}



OutprintControl::OutprintControl(){
	pthread_mutex_init(&mutex, NULL);
}

OutprintControl::~OutprintControl(){
	pthread_mutex_destroy(&mutex);
}

void OutprintControl::Add(StreamControl * ctrl){
	pthread_mutex_lock(&mutex);
	ctrls.push_back(ctrl);
	pthread_mutex_unlock(&mutex);
}

bool OutprintControl::Remove(StreamControl * ctrl){
	pthread_mutex_lock(&mutex);
	int i=ctrls.size();
	while(i--){
		if(ctrls[i]==ctrl) {
			ctrls.erase(ctrls.begin()+i); 
			pthread_mutex_unlock(&mutex); 
			return true;
		}
	}
	pthread_mutex_unlock(&mutex);
	return false;
}

void OutprintControl::Put(void * context, const char * first, ...){
	va_list vl;
	va_start(vl, first);
	StringCollector msg(first);
	const char * str;
	while(str = va_arg(vl, const char *))
		msg.Push(str);
	bool endlbr = true;
	if(*(msg.GetBlock() + msg.GetLength() - 1) == '\n') endlbr = false;
	pthread_mutex_lock(&mutex);
	int cnt=ctrls.size();
	for(int i = 0; i<cnt; i++){
		ctrls[i]->Write(context, msg.GetBlock());
	}
	pthread_mutex_unlock(&mutex);
	va_end(vl);
}

#endif
