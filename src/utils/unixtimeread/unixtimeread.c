#include "time.h"
#include "stdio.h"

/* convert from unix-ts to MMDDhhmmCCYY */
int main(int argc, char** argv){

  int res = -1;
  if(argc!=2){
    printf("usage: ./unixtimeread <unixtimestamp>\n");
    return res;
  }
    
  time_t t = atoi(argv[1]);
  char buf[15];
  struct tm* t_tm;
  if(t)
    t_tm = localtime(&t); 
 
  if(t_tm){
    int r = strftime((char*)&buf, 256, "%m%d%H%M%Y.%S\n", t_tm);
    if(r){
      printf((char*)&buf);
      res = 0;
    } 
  }

  return res;
}

