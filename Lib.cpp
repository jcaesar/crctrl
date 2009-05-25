
namespace rx{
	static boost::regex ctrl_err	("^Could not Start\\. Error: ");
	//Could not Start. Error: 
	static boost::regex cl_join		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Client (.*) aktiviert\\.$"); 
	//[18:30:52] Client Tinys Pc aktiviert.
	static boost::regex pl_join		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Spielerbeitritt: (.*)$");
	//
	static boost::regex cl_part		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Client (.*) entfernt \\(.*\\).$");
	//[18:31:11] Client Tinys Pc entfernt (Verbindungsabbruch).
	static boost::regex gm_gohot	("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Das Spiel beginnt in 10 Sekunden!$");
	//[11:59:56] Das Spiel beginnt in 10 Sekunden!
	static boost::regex gm_load		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Los geht's!$");
	//[11:54:36] Los geht's!
	static boost::regex gm_rec		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Aufnahme nach Records\\.c4f/([0-9])-(.*)\\.c4s\\.\\.\\.$");
	//[20:40:12] Aufnahme nach Records.c4f/1119-MinorMelee.c4s...
	static boost::regex gm_go		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Start!$");
	//[11:54:45] Start!
	static boost::regex gm_tick0	("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Spiel gestartet\\.$");
	//[17:56:36] Spiel gestartet.
	static boost::regex gm_end		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Runde beendet\\.$");
	//[12:23:23] Runde beendet.
	static boost::regex gm_exit		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Engine shut down\\.$");
	//[13:34:25] Engine shut down.
	static boost::regex gm_lobby	("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Teilnehmer werden erwartet...$");
	//[20:41:59] Teilnehmer werden erwartet...
	static boost::regex pl_die		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Spieler (.*) eliminiert.$");
	//[20:56:35] Spieler Test0r eliminiert.
	static boost::regex cm_base		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] <(.*)> %([^ ]+)( (.*))?$");
	//
/*	static boost::regex cm_help		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] <(.*)> %help$");
	//[16:58:32] <CServ> %help
	static boost::regex cm_list		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] <(.*)> %list$");
	//[16:58:32] <CServ> %list
	static boost::regex cm_start	("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] <(.*)> %start (.*)$");
	//[16:58:32] <CServ> %start sty*/
}

bool startswith(const std::string *str1, const std::string *str2){
	int len=str2->length();
	if(len > str1->length()) return 0;
	return (memcmp(str1->data(), str2->data(), len)==0);
}

bool startswith(const std::string *str1, const char *str2){
	int len=strlen(str2);
	if(len > str1->length()) return 0;
	return (memcmp(str1->data(), str2, len)==0);
}

bool startswith(const std::string *str1, const char str2_2){
	const char *str2; str2=&str2_2;
	int len=strlen(str2);
	if(len > str1->length()) return 0;
	return (memcmp(str1->data(), str2, len)==0);
}

class StringCollector{
	private:
		const char * data;
		StringCollector * next;
		bool string_complete;
		bool delete_data;
		int length;
	public:
		StringCollector(const char * string = NULL) : data(string), next(NULL), length(-1), string_complete(0), delete_data(false) {}
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
			if(string_complete) return false;
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

#ifndef MAXCHARS
	#define MAXCHARS 128
#endif

class StreamReader{
	private:
		int rv;
		char *buff;
		char *buffadr;
		char *buffadr2;
	public:
		const int fd;
		StreamReader(const int fd_read) : fd(fd_read) {
			buffadr=new char[MAXCHARS];
			buff=buffadr;
			rv=0;
		}
		~StreamReader(){delete[] buffadr; close(fd);}
		bool ReadLine(std::string *line){return(ReadLine(line, 10));}
		bool ReadLine(std::string *line, const char delim){
			*line="";
			do{
				//std::cout << reinterpret_cast<int>(buffadr) << " " << reinterpret_cast<int>(buff) << " " << rv << std::endl;
				if(buffadr+rv > buff){ //I left things behind, last time.
					buffadr2=buff;
					while(*buff and *buff!=delim) buff++;
					if(*buff==delim){ //This is the case, when there was found a completed line.
						*buff=0;
						buff++;
						*line += buffadr2;
						break;
					}
					*line += buffadr2;
				}
				buff = buffadr;
				if((rv = read(fd, buff, MAXCHARS-1)) <= 0) {
					close(fd);
					delete this; //This happens, when fd has EoF.
					return false;
				}
				buff = buffadr;
				*(buff+rv)=0;
			} while(*buff!=0);
			return true;
		}
};

class OutprintControl;
static class OutprintControl{
	private:
		std::vector <int> fds;
		pthread_mutex_t mutex;
	public:
		OutprintControl(){
			pthread_mutex_init(&mutex, NULL);
		}
		~OutprintControl(){
			pthread_mutex_destroy(&mutex);
		}
		void Add(int fd){
			pthread_mutex_lock(&mutex);
			fds.push_back(fd);
			pthread_mutex_unlock(&mutex);
		}
		bool Remove(int fd){
			pthread_mutex_lock(&mutex);
			int i=fds.size();
			while(i--){
				if(fds[i]==fd) {
					fds.erase(fds.begin()+i); 
					pthread_mutex_unlock(&mutex); 
					return true;
				}
			}
			pthread_mutex_unlock(&mutex);
			return false;
		}
		void Put(const char * first, ...){
			va_list vl;
			va_start(vl, first);
			StringCollector msg(first);
			const char * str;
			while(str = va_arg(vl, const char *))
				msg.Push(str);
			bool endlbr = true;
			if(*(msg.GetBlock() + msg.GetLength() - 1) == '\n') endlbr = false;
			pthread_mutex_lock(&mutex);
			int cnt=fds.size();
			for(int i = 0; i<cnt; i++){
				write(fds[i], msg.GetBlock(), msg.GetLength());
				if(endlbr) write(fds[i], "\n", 1);
			}
			pthread_mutex_unlock(&mutex);
			va_end(vl);
		}
} Out;