// Enables IR Wakeup
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>

#define REG_WAKESM              0x7D
int base=0xb080; // BAR2 of PCI 00:1e.0

#define EVT_IR (1<<28)
#define EVT_CEC (1<<25)

int main(int argc, char** argv)
{
	iopl(3);
	outl(0x7d,base);
	outl((~(EVT_IR|EVT_CEC))&0xffff0000,base+4);
	return 0;
}
