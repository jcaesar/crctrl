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