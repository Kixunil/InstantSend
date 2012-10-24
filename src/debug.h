#ifdef DEBUGMSG
	#define D(MSG) fprintf(stderr, MSG "\n"); fflush(stderr);
#else
	#define D(MSG)
#endif
