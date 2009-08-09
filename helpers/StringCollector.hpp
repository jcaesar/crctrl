#ifndef StringCollectorHpp
#define StringCollectorHpp

class StringCollector{
	private:
		const char * data;
		StringCollector * next;
		bool string_complete;
		bool delete_data;
		int length;
	public:
		StringCollector(const char * string = NULL) : data(string), next(NULL), length(-1), string_complete(0), delete_data(false) {} //When data ist NULL, we've got a Problem. Fixme.
		~StringCollector() {
			if(next != NULL) delete next;
			if(string_complete || delete_data) delete [] data;
		}
		int GetLength(bool renew = false){
			if(length == -1 || renew) {
				length = strlen(data);
				if(next) length += next->GetLength();
			}
			return length;
		}
		const char * GetBlock(){
			if(!string_complete){
				int len=GetLength();
				char * temp = new char[len+1];
				char * jump; jump = temp;
				StringCollector * itr; itr=this;
				while(itr){
					strcpy(jump, itr->data);
					jump += strlen(itr->data);
					itr = itr->next;
				}
				data = temp;
				if(next) {
					delete next;
					next = NULL;
				}
				length = GetLength(true);
				string_complete = true;
			}
			return data;
		}
		bool Push(const char * push){
			if(push == NULL || string_complete) return false;
			if(next) return next->Push(push);
			next = new StringCollector(push);
			length = -1;
			return true;
		}
		bool Push(const int push){
			if(string_complete) return false;
			if(next) return next->Push(push);
			char * tmp = new char[11];
			sprintf(tmp, "%d", push);
			next = new StringCollector(tmp);
			next -> delete_data = true;
			length = -1;
			return true;
		}
		StringCollector& operator+=(const char * push){Push(push); return *this;}
		StringCollector& operator+=(const int push)   {Push(push); return *this;}
};

#endif