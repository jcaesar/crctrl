class StreamControl{
	private:
		void Work();
		static void * ThreadWrapper(void *);
		StreamReader * sr;
		const int fd_out;
		Game * sel;
		pthread_t tid;
	public:
		StreamControl(int, int);
};
