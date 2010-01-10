
#include "Control.h"

#include <stdarg.h>
#include "helpers/StringFunctions.h"
#include "helpers/StringCollector.hpp"
#include "helpers/AppManagement.h"
#include "helpers/Stream.h"
#include "AutoHost.h"
#include "Config.h"
#include "GameControl.h"

OutprintControl Out;

OutprintControl * GetOut(){
	return &Out;
}

UserControl::UserControl(Stream * io) :
	conn(io),
	sel(NULL)
{
	Out.Add(this);
	pthread_create(&tid, NULL, &UserControl::ThreadWrapper, this);
	Halt(0.2); //Hacky, but the AutoHosts will have a status at startup.
	PrintStatus();
}

UserControl::~UserControl() {
	Write(this, "Your connection got Terminated (most probably due to a shutdown).");
	delete conn;
}

void UserControl::Work(){
	std::string cmd;
	while(conn->ReadLine(&cmd)){
		if(!cmd.compare("%auto")) {
			sel = new AutoHost(); //Note that sel is not a permanent saving var.
		} else if(!cmd.compare("%end")) {
			GetAutoHosts()->DelAll();
			exit(0);
		} else if(startswith(&cmd,"%stop")) {
			if(GetAutoHosts()->Exists(sel) && sel->GetGame()){
				if(cmd.compare("%stop forced")) delete sel;
				else sel->SoftEnd(false);
			} else {
				Out.Put(this, "No Host selected.", NULL);
			}
		} else if(!cmd.compare("%fork")) {
			Background();
		} else if(!cmd.compare("%reload")) {
			GetConfig()->Reload();
		} else if(startswith(&cmd,"%sel ")) {
			sel = GetAutoHosts()->Find(atoi(cmd.data() + 5));
			if(sel == NULL) Out.Put(this, "No such host: ", cmd.data() + 5, NULL);
			else PrintStatus(sel);
		} else if(!cmd.compare("%status")) {
			PrintStatus();
		} else if(startswith(&cmd,"%")){
			Out.Put(this, "No such command: ", cmd.data(), NULL);
		} else {
			if(GetAutoHosts()->CatchAndLock(sel)) {
				if(sel->GetGame()) {
					cmd += "\n";
					sel->GetGame()->SendMsg(cmd.c_str(), NULL);
				}
				sel->UnlockStatus();
			}
		}
	}
	delete this;
}

void UserControl::PrintStatus(AutoHost * stat /*= NULL*/){
	if(GetAutoHosts()->Exists(stat)) {
		stat->LockStatus();
		if(stat->GetGame()) Out.Put(this, stat->GetPrefix(), " ", (stat->GetGame())->GetScen()->GetName(), NULL);
		else Out.Put(this, stat->GetPrefix(), " currently empty.", NULL);
		stat->UnlockStatus();
	} else {
		int i=0;
		while(AutoHost * ah = GetAutoHosts()->Get(i++)) {
			ah->LockStatus();
			PrintStatus(ah);
			ah->UnlockStatus();
		}
	}
}

bool UserControl::Write(void * context, const char * msg){
	if(context == sel || context == this || context == 0)
		return conn->Write(msg, NULL);
	else 
		return false;
}

void * UserControl::ThreadWrapper(void *p) {
	static_cast<UserControl*>(p)->Work(); 
	return NULL;
}



OutprintControl::OutprintControl(){
	pthread_mutex_init(&mutex, NULL);
}

OutprintControl::~OutprintControl(){
	pthread_mutex_destroy(&mutex);
}

void OutprintControl::Add(UserControl * ctrl){
	pthread_mutex_lock(&mutex);
	ctrls.push_back(ctrl);
	pthread_mutex_unlock(&mutex);
}

bool OutprintControl::Remove(UserControl * ctrl){
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
	StringCollector msg(first, false);
	const char * str;
	while((str = va_arg(vl, const char *)))
		msg.Push(str, false);
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