/****************************************************************************
 * DESCRIPTION:
 *            
 *
 * $Id: debug.h,v 1.2 2006/08/15 20:35:22 ralf Exp $
 *
 * Contact:    ranga@vdrtools.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/
#include <vdr/config.h>

/*
#define LOGERR(str, args...)   {char *s=NULL; asprintf(&s, "XXVAUTOTIMER:ERROR %s",str ); esyslog(s, ##args ); free(s);}


#ifdef DEBUGXXVAUTOTIMER
#define LOGDBG(str, args...)   {char *s=NULL; asprintf(&s, "XXVAUTOTIMER:DEBUG %s",str ); isyslog(s, ##args ); free(s);}
#else
#define LOGDBG(str, args...)
#endif

*/
#define LOGERR(str, args...)
#define LOGDBG(str, args...)
