
#include "StatusLocks.h"

StatusLocks::StatusLocks() : writercount(0) {
	pthread_mutex_init(&statuslock, NULL);
	pthread_cond_init(&statuscond, NULL);
	readerinfo.count = 0;
	readerinfo.next = NULL;
}

StatusLocks::~StatusLocks() {
	pthread_cond_broadcast(&statuscond);
	pthread_mutex_lock(&statuslock);
	pthread_cond_destroy(&statuscond);
	pthread_mutex_destroy(&statuslock);
	ReaderInfo * itr = readerinfo.next;
	ReaderInfo * itr2;
	while(itr){
		itr2 = itr->next;
		delete itr;
		itr = itr2;
	}
}

void StatusLocks::UnlockStatus() {
	pthread_mutex_lock(&statuslock);
	ReaderInfo * info = &readerinfo;
	while(!pthread_equal(info->thread, pthread_self()))
		info = info->next;
		//Notice that I don't do info == NULL --> throw. Who should catch it?
	--(info->count);
	if(info->count == 0) {
		if(info != &readerinfo) {
			ReaderInfo * itr = &readerinfo;
			while(itr->next != info) itr = itr->next;
			itr->next = info->next;
			delete info;
		} else {
			if(ReaderInfo * del = readerinfo.next) {
				readerinfo.thread = del->thread;
				readerinfo.count = del->count;
				readerinfo.next = del->next;
				delete del;
			}
		}
		pthread_cond_broadcast(&statuscond);
	}
	pthread_mutex_unlock(&statuslock);
}

void StatusLocks::LockStatus() {
	pthread_mutex_lock(&statuslock);
	while(writercount) pthread_cond_wait(&statuscond, &statuslock);
	if(readerinfo.count) {
		ReaderInfo * itr = &readerinfo;
		while(true) {
			if(pthread_equal(pthread_self(), itr->thread)) {
				++(itr->count);
				break;
			}
			if(!itr->next) {
				itr->next = new ReaderInfo;
				itr = itr->next;
				itr->next = NULL;
				itr->count = 1;
				itr->thread = pthread_self();
				break;
			}
			itr = itr->next;
		}
	} else {
		readerinfo.count = 1;
		readerinfo.thread = pthread_self();
	}
	pthread_mutex_unlock(&statuslock);
}
void StatusLocks::StatusUnstable() {
	pthread_mutex_lock(&statuslock);
	while(readerinfo.next || (readerinfo.count && !pthread_equal(pthread_self(), readerinfo.thread)) || (writercount && !pthread_equal(writer,pthread_self())))
		pthread_cond_wait(&statuscond, &statuslock);
	++writercount;
	writer = pthread_self();
	pthread_mutex_unlock(&statuslock);
}

void StatusLocks::StatusStable()  {
	pthread_mutex_lock(&statuslock);
	--writercount;
	if(writercount == 0) pthread_cond_broadcast(&statuscond);
	pthread_mutex_unlock(&statuslock);
}
