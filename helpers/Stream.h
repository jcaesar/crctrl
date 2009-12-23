#ifndef StreamH
#define StreamH

#ifdef WIN32
	#include <windows.h>
#endif
#include <pthread.h>
#include <string>

#ifndef STREAM_MAXCHARS
	#define STREAM_MAXCHARS 128
#endif

class Process;
class ListenSocket;
class Connection;
class Stream;

/*class StreamBase {
	private
		mutable StreamBase * ChainNext;
		mutable StreamBase * ChainPrev;
		virtual void CopyInformationToNext() = 0;
	public:
		StreamBase() : ChainNext(NULL), ChainPrev(NULL) {}
		StreamBase(Connection &const other) {
			other.ChainNext = this;
			ChainPrev = other;
		}
		virtual void Close() = 0;
		~StreamBase() {
			if(!ChainPrev) {
				if(ChainNext)
					CopyInformationToNext();
				else
					Close();
			} else if(ChainNext) {
				ChainNext->ChainPrev = ChainPrev;
				
			}
		}
		bool IsBase() { return ChainPrev!=NULL };
		StreamBase * GetBase 
};*/

class StreamIn /*: private StreamBase */{
	friend class Process;
	friend class ListenSocket;
	friend class Connection;
	friend class Stream;
	private:
		#if defined unix
			int fd_in;
			int rv; //return value of read()
		#elif defined WIN32
			HANDLE fd_in;
			DWORD rv; //return value of read()
		#endif
		char buffadr [STREAM_MAXCHARS];
		char * buff;
		char * buffadr2;
	public:
		StreamIn();
		#ifdef unix
			StreamIn(int fd_in);
		#elif defined WIN32
			StreamIn(HANDLE fd_in);
		#endif
		virtual ~StreamIn();
		bool ReadLine(std::string *, const char [3]);
		#if defined unix
			inline bool ReadLine(std::string * line){return(ReadLine(line, "\10\0"));}
		#elif defined WIN32
			inline bool ReadLine(std::string * line){return(ReadLine(line, "\13\10"));}
		#endif
		void Close();
};

class StreamOut /*: private StreamBase */{
	friend class Process;
	friend class ListenSocket;
	friend class Stream;
	private:
		#if defined unix
			int fd_out;
		#elif defined WIN32
			HANDLE fd_out;
		#endif
	public:
		StreamOut();
		virtual ~StreamOut();
		bool Write(const char *, ...);
		virtual bool WriteList(const char *, va_list);
		virtual void Close();
};

class Stream : public StreamIn, public StreamOut {
	private:
		void Close() {ClosePipes();} //Avoid those functions from I/O for direct public call
	public:
		virtual void ClosePipes();
		static void GetStandardIO(Stream &);
};

#endif