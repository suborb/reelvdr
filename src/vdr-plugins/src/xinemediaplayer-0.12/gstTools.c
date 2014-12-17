#include <string>
#include <iostream>
#include <stdlib.h>
#include <limits.h>
#include <gst/gst.h>

// -------------make_gst_uri(const char *)-------------------------------------
// takes a char* and tries to guess the source protocol and prepend it to 
// mrl. Need for uri property of gstreamer pipeline (playbin2)
//
// returns an empty string when guess fails
// else "prot://mrl" or mrl itself all as std::string objects

std::string make_gst_uri(const char * mrl) {
  
  //gst uri is absolute file name preceeded by "file://"
  // or "http://" etc
  
  std::string uri (mrl);
  

  /// supported source protocols

  // "file://" in position 0
  if (uri.find("file://") == 0)
    return uri;
  
  // "http://" in position 0
  if (uri.find("http://") == 0)
    return uri;
  
  // "dvd://" in position 0
  if (uri.find("dvd://") == 0)
    return uri;



  /// guess 
  // uri is absolute path without "file://" prefix
  if (uri[0] == '/') 
    return std::string("file://") + uri;

  char abs_path[PATH_MAX+1];
  if (realpath(uri.c_str(), abs_path) != NULL) {
    // got absolute path, prefix "file://"
    return std::string("file://") + std::string(abs_path);
  }
  else // Give up and return empty string 
      return std::string();
    
} // make_gst_uri()


// print all available gstreamer format : like GST_FORMAT_TIME
// some plugins register their own formats. 
// Eg. resindvd (dvd source plugin) has "title" and "chapter" formats 
// through which seek and query maybe requested
void print_available_gst_foramts()
{
    GstIterator *iter;
    gpointer value;

    iter = gst_format_iterate_definitions();

    while (gst_iterator_next(iter, &value) == GST_ITERATOR_OK) 
    {
        GstFormatDefinition *def = (GstFormatDefinition *)value;

        g_print("format : '%s'\nDesc: %s\n\n", 
                def->nick,
                def->description);

    } /* while */

    gst_iterator_free(iter);

} // print_available_gst_foramts
