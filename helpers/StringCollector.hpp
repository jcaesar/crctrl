#ifndef StringCollectorHpp
#define StringCollectorHpp

#ifdef unix
	#include <string.h>
#endif

class StringCollector{
	private:
		const char * data;
		StringCollector * next;
		bool delete_data;
		int length;
	public:
		StringCollector(const char * string = NULL, bool copy = true) : 
			data(string), 
			next(NULL), 
			delete_data(copy), 
			length(-1) 
		{
			if(copy && string) {
				char * tmp = new char [strlen(data)+1]; //note on tmp: We don't inflict the const corrrectness with it.
				strcpy(tmp, string);
				data = tmp;
			}
		}
		~StringCollector() {
			if(next != NULL) delete next;
			if(delete_data) delete [] data;
		}
		int GetLength(bool renew = false){
			if(length == -1 || renew) {
				length = 0;
				for(StringCollector * itr = this; itr; itr = itr->next)
					if(itr->data) length += strlen(itr->data);
			}
			return length;
		}
		const char * GetBlock(){
			if(next) {
				int len=GetLength();
				char * temp = new char[len+1];
				char * jump; jump = temp;
				for(StringCollector * itr = this; itr; itr = itr->next){
					if(itr->data) {
						strcpy(jump, itr->data);
						jump += strlen(itr->data);
					}
				}
				if(delete_data) delete [] data;
				data = temp;
				delete_data = true;
				if(next) {
					delete next;
					next = NULL;
				}
			}
			return data;
		}
		bool Push(const char * push, bool copy = true){
			if(push == NULL) return false;
			StringCollector * pushto = this;
			while(pushto->next) pushto=pushto->next;
			pushto->next = new StringCollector(push, copy);
			length = -1;
			return true;
		}
		bool Push(const int push){
			StringCollector * pushto = this;
			while(pushto->next) pushto=pushto->next;
			char * tmp = new char[11];
			sprintf(tmp, "%d", push);
			pushto->next = new StringCollector(tmp);
			delete [] tmp;
			length = -1;
			return true;
		}
		inline StringCollector& operator+=(const char * push){Push(push); return *this;}
		inline StringCollector& operator+=(const int push)   {Push(push); return *this;}
};

#endif