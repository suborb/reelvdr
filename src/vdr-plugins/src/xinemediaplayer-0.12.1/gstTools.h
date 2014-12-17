#ifndef _GSTTOOLS__H_
#define _GSTTOOLS__H_

#include <string>

// returns a gstreamer compatible uri string of given mrl
// ie. "prot://<MRL>" 
// "prot://" maybe "file://", "http://", "dvd://" etc
//
// string returned can be empty (in case of error)
std::string make_gst_uri(const char*);

// lists available gstreamer format
void print_available_gst_foramts();


#endif // _GSTTOOLS__H_
