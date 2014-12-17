#include "scanner.h"
#include "proctools/conlogger.h"
#include <iostream>
#include <string>
#include <stdexcept>

using namespace std;
using namespace proctools;
using namespace vdr_burn;

int main(int argc, char* argv[])
{
   conlogger::start();

   if( argc != 2 ) {
      logger::error( format("use: {0} <recording>") % argv[0], false );
      return 1;
   }

   try {
      recording_scanner scanner(argv[1]);
      scanner.scan();
      return 0;
   }
   catch( std::string_error& ex ) {
      logger::error( ex.c_str(), false );
      return 1;
   }
}
