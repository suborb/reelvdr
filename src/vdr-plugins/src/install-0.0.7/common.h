#ifndef install_common_h
#define install_common_h

// Debugging

#ifdef DEBUG
#define d(x) { (x); }
#else
#define d(x) ;
#endif


#ifdef DEBUG_LOG
#define dl(x) { (x); }
#else
#define dl(x) ;
#endif

#ifdef DEBUG_XML
#define dx(x) { (x); }
#else
#define dx(x) ;
#endif

#endif
