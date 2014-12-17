/*------------------------------------------------------------------------*/
// load_8051 - NetClient2/CE42xx: Load firmware into 8051
// 
// (c) 2012 Georg Acher/BayCom GmbH
/*------------------------------------------------------------------------*/

#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/io.h>
#include <string.h>

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

int pci_fd=-1;
/*------------------------------------------------------------------------*/
int pci_init(uint32_t bus, uint32_t dev, uint32_t func)
{
	char path[256];
	if (pci_fd>=0)
		return 0;
	sprintf(path,"/sys/bus/pci/devices/%04x:%02x:%02x.%x/config",0,bus,dev,func);
	pci_fd=open(path,O_RDWR);
	if (pci_fd<0) {
		perror(path);
		exit(-1);
	}
	return 0;
}
/*------------------------------------------------------------------------*/
void pciwrite32(uint32_t bus, uint32_t dev, uint32_t func,  uint32_t reg, uint32_t val) 
{
	pci_init(bus,dev,func);
	pwrite(pci_fd,&val,4,reg);
}
/*------------------------------------------------------------------------*/
uint32_t pciread32(uint32_t bus, uint32_t dev, uint32_t func,  uint32_t reg) 
{
	uint32_t ret;
	pci_init(bus,dev,func);
	pread(pci_fd,&ret,4,reg);
//	printf("read %08x\n",ret);
	return ret;
}
/*------------------------------------------------------------------------*/
void schMsgSend(uint8_t port, uint8_t opcode, uint8_t reg)
{
	uint32_t val=(port<<16)|(opcode<<24)|(reg<<8)|0xf0;
	pciwrite32(0,0,0,0xd0,val);
}
/*------------------------------------------------------------------------*/
uint32_t schMsgGetData()
{
	return pciread32(0,0,0,0xd4);
}
/*------------------------------------------------------------------------*/
void schMsgSetData(uint32_t data)
{
	pciwrite32(0, 0, 0, 0xd4, data);
}
/*------------------------------------------------------------------------*/
uint32_t schRegRead32(uint8_t port, uint8_t reg)
{
	schMsgSend(port&255,0xd0,reg);
	uint32_t ret=schMsgGetData();
	return ret;
}
/*------------------------------------------------------------------------*/
void schRegWrite32(uint8_t port, uint8_t reg, uint32_t val)
{
	schMsgSetData(val); 
	schMsgSend(port&255, 0xe0, reg&255);
}
/*------------------------------------------------------------------------*/
void schRegMod32(uint8_t port, uint8_t reg, uint32_t mask, uint32_t bits)
{
	uint32_t val;
	val=schRegRead32(port,reg);
	val=val&~(mask);
	val=val|bits;
	schRegWrite32(port,reg,val);
}
/*------------------------------------------------------------------------*/
// 64k aligned
#define I8051_BASE 0x00050000

int do_load_8051fw(uint32_t fw_addr)
{
	pciwrite32(0x00, 0x00, 0x00, 0xD4, fw_addr);
	pciwrite32(0x00, 0x00, 0x00, 0xD0, 0xB9040000);
	
	// The DRAM_VALID bit gates the 8051 from fetching code from DDR
	// set DRAM_VALID bit to let 8051 know DRAM is ready
	schRegMod32(0x04,0x71,(1<<24),(1<<24));
	
	// wait for 8051 to acknowledge it has started
	puts("WAIT");
	int n;
	for(n=0;n<100;n++) {
		if ((schRegRead32(0x04,0x71) & (1<<30))) {
			puts("DOWNLOAD DONE");			
			// clear this bit so Linux can boot properly
			schRegMod32(0x04,0x71,(0x01<<30),0x00);
			
			return 0;
		}
		usleep(10*1000);
	}

	puts("***** DOWNLOAD FAILED ******");
	
	return -1;
}
/*------------------------------------------------------------------------*/
char* do_mmap(void)
{
	void* p;
	int fd,base,len;

	fd=open("/dev/mem",O_RDWR);
        base=I8051_BASE;
        len=65536;

        p=mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);
        printf("FD %i %x: %p\n",fd, base, p);	
	return (char*)p;
}
/*------------------------------------------------------------------------*/

int main(int argc, char** argv)
{
	char *fwx;
	volatile int *x;
	int n,sum=0;
	unsigned char data[65536];

	if (argc!=2) {
	        fprintf(stderr,"Usage: load_8051 <fw.bin>\n");
                exit(-1);
        }
        
        int ffd=open(argv[1],O_RDONLY);
        if (ffd<0) {
                perror(argv[1]);
                exit(-1);
        }

        int len;
	len=read(ffd,data,65536);
	if (len<1000) {
	        fprintf(stderr,"Suspicious file length\n");
	        exit(-1);
        }
        printf("FW length: %i\n",len);
	fwx=do_mmap();
	memcpy(fwx,data,len);
	
	// Cache flush
	x=(volatile int*)malloc(8*1024*1024);
	for(n=0;n<8*1024*1024/4;n++) {
		*x=n;
		sum+=*x++;
	}

	int ret=do_load_8051fw(I8051_BASE);
	exit(ret);
}
