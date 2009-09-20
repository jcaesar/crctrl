#ifndef StreamReaderHpp
#define StreamReaderHpp

#ifndef MAXCHARS
	#define MAXCHARS 128
#endif

class StreamReader{
	private:
		int rv;
		char *buff;
		char *buffadr;
		char *buffadr2;
		pthread_t readthread;
	public:
		const int fd;
		StreamReader(const int fd_read) : readthread(NULL), fd(fd_read) {
			buffadr=new char[MAXCHARS];
			buff=buffadr;
			rv=0;
		}
		~StreamReader(){
			if(readthread != pthread_self() && readthread) pthread_cancel(readthread);
			delete [] buffadr;
			close(fd);
		}
		bool ReadLine(std::string *line){return(ReadLine(line, 10));}
		bool ReadLine(std::string *line, const char delim){
			readthread = pthread_self();
			std::stringstream ss;
			do{
				//std::cout << reinterpret_cast<int>(buffadr) << " " << reinterpret_cast<int>(buff) << " " << rv << std::endl;
				if(buffadr+rv > buff){ //I left things behind, last time.
					buffadr2=buff;
					while(*buff and *buff!=delim) buff++;
					if(*buff==delim){ //This is the case, when there was a completed line found.
						*buff=0;
						buff++;
						ss << buffadr2;
						break;
					}
					ss << buffadr2;
				}
				buff = buffadr;
				if((rv = read(fd, buff, MAXCHARS-1)) <= 0) {
					close(fd);
					*line = ss.str();
					return false;
				}
				buff = buffadr;
				*(buff+rv)=0;
			} while(*buff!=0);
			*line = ss.str();
			return true;
		}
};

#endif