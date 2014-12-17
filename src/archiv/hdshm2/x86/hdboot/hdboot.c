/*****************************************************************************/
/*
  Decypher PCI/U-Boot Loader
  
  (c) 2007-2008 Georg Acher, BayCom GmbH (acher at baycom dot de)

  #include <GPLv2-header>
 
*/
/*****************************************************************************/

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include "hdshm.h"

#define MAX_IMAGE_SIZE (16*1024*1024)
#define MIN_IMAGE_SIZE (1*1024*1024)
#define IMG_LOAD_ADDR  (0x02001000)
#define U_BOOT_TIMEOUT 10

// Selected Decypher defines

#define PERIPHERAL_BASE         0x0
#define SEM_REG_BASE      (PERIPHERAL_BASE + 0x00080000)
#define SEM_0_SCRATCH_REG       (SEM_REG_BASE + 0x00000100)
#define SEM_3_SCRATCH_REG       (SEM_REG_BASE + 0x0000010c)
#define CCB_REG_BASE      (PERIPHERAL_BASE + 0x00100000)
#define CCB_REG_CCBCTRL   (CCB_REG_BASE + 0x14)

/*------------------------------------------------------------------------*/
int find_decypher(void)
{
	int fd;
	int bar1;
	fd=open("/dev/hdshm",O_RDWR);
	if (fd<0) {
		fprintf(stderr,"Can't open /dev/hdshm to determine Decypher PCI BAR1.\n");
		exit(-1);
	}
	bar1=ioctl(fd, IOCTL_HDSHM_PCIBAR1, 0);
	close(fd);
	printf("Decypher PCI BAR1: %x\n",bar1);
	return bar1;

}
/*------------------------------------------------------------------------*/
void hexdumpi(volatile void *b, int l)
{
        int n;
        int *x=(int*)b;
        for(n=0;n<l;n++)
        {
                if ((n%8)==0) printf("%08x   ",n*4);
                printf("%08x ",*(x+n));
                if ((n%8)==7)
                        puts("");
        }
        puts("");
}


/*------------------------------------------------------------------------*/
unsigned char* pcimap(off_t offset)
{
        int fd1;
	void *base;
	
	
        if ((fd1 = open("/dev/mem", O_RDWR)) < 0) {
                fprintf(stderr, "Failed to open /dev/mem (%s). Are you root?\n",
                           strerror(errno));
		return 0;
        }

        base = mmap(0, 256*1024*1024,
                                    PROT_READ|PROT_WRITE,
                                    MAP_SHARED, fd1,
                                    offset);
        if (base==MAP_FAILED) {       
                fprintf(stderr, " Failed to mmap PCI (%s)\n",
                       strerror(errno));
		return 0;
        }
	
	return (unsigned char*)base;
}
/*------------------------------------------------------------------------*/
int load(char *fname, unsigned char *pci_base, int jump, int verify)
{
        volatile unsigned char * load_addr;
        char *buf;
        int fd;
        int n,l;

        load_addr=pci_base+IMG_LOAD_ADDR;

	fd=open("/dev/hdshm",O_RDWR);
	if (fd>=0) {
		int stat;
		ioctl(fd, IOCTL_HDSHM_SHUTDOWN, 0);	// Clear booted-flag
		close(fd);
	}

        fd=open(fname,O_RDONLY);
	if (fd<0) {
		fprintf(stderr,"File <%s> not found\n",fname);
		return 0;
	}

        buf=malloc(MAX_IMAGE_SIZE); // 16MB are enough
	if (!buf) {
		fprintf(stderr,"Out of memory\n");
		return 0;
	}

        l=read(fd,buf,MAX_IMAGE_SIZE);
        printf("Uploading %s, Length %i, virtual %p, MIPS 0x%x\n",fname,l,load_addr,IMG_LOAD_ADDR);
        if (l>MIN_IMAGE_SIZE) {
		if (*(int*)buf==0x12345678 && *(int*)(buf+4)!=0 && !jump) {
			jump=*(int*)(buf+4);
			printf("Detected kernel entry %x\n",jump);
		}

		memcpy((void*)load_addr,buf,(l+3)&~3);
		printf("memcpy done\n");
		if (verify) {
			volatile int *src1,*src2;
			printf("Verify\n");
			src1=(volatile int*)buf;
			src2=(volatile int*)load_addr;
			for(n=0;n<l;n+=4) {
				if (*src1!=*src2) {
					printf("DDR Verify Error %x is %x, should be %x\n",n+IMG_LOAD_ADDR,*src2,*src1);
					puts("IS:");
					hexdumpi(src2,32);
					puts("SHOULD BE:");
					hexdumpi(src1,32);
					exit(-1);
				}
				src1++;
				src2++;
			}
			printf("Verify OK\n");

		}
		if (!jump)
			jump=*(int*)(buf+128); // First 1K is zero
		if (!jump) {
			fprintf(stderr,"Invalid kernel entry\n");
			return 0;
		}
	

		return jump;		
        }
	else {
		fprintf(stderr,"File too small to be a real boot image\n");
		return 0;
	}
}

/*------------------------------------------------------------------------*/
void warm_reset(unsigned char *pci_base)
{
	int fd;
//	printf("%x\n",CCB_REG_CCBCTRL);
	puts("Warm Reset");
	fd=open("/dev/hdshm",O_RDWR);
	if (fd>=0) {
		ioctl(fd, IOCTL_HDSHM_SHUTDOWN, 0);	// Clear booted-flag
		close(fd);
	}

	*(volatile int*)(pci_base+CCB_REG_CCBCTRL)|=1;
	*(volatile int*)(pci_base+CCB_REG_CCBCTRL)&=~1;
	*(volatile int*)(pci_base+SEM_3_SCRATCH_REG)=0;
}
/*------------------------------------------------------------------------*/
void run_cpu(unsigned char *pci_base, int jump)
{
	printf("Start CPU#0 at %x\n",jump);
	*(volatile int*)(pci_base+SEM_0_SCRATCH_REG)=jump;
	*(volatile int*)(pci_base+SEM_REG_BASE)=0;
}
/*------------------------------------------------------------------------*/
// Wait until u-boot is ready to boot

int wait_for_pciboot(unsigned char *pci_base, int timeout_s)
{
	int n;
	for(n=0;n<1+timeout_s*10;n++) {
//		printf("%x\n",*(volatile int*)(pci_base+SEM_3_SCRATCH_REG));
		if (*(volatile int*)(pci_base+SEM_3_SCRATCH_REG)==0xb001abcd)
			return 0;
		usleep(100*1000);
	}
	return 1;
}
/*------------------------------------------------------------------------*/
void upload_start(unsigned char *pci_base, char *fname, int jump, int verify)
{		
	if (wait_for_pciboot(pci_base,U_BOOT_TIMEOUT)) {
		fprintf(stderr,"Timeout: U-Boot not ready for PCI boot\n");
		exit(-1);
	}
	jump=load(fname, pci_base,jump, verify);

	if (!jump)
		exit(-1);
	run_cpu(pci_base,jump);
}
/*------------------------------------------------------------------------*/
int wait_kernel(int timeout)
{
	int n,fd,stat;
	fd=open("/dev/hdshm",O_RDWR);
	if (fd<0)
		return -1;


	for(n=0;n<timeout;n++) {
		stat=ioctl(fd, IOCTL_HDSHM_GET_STATUS);
		if (!(stat&HDSHM_STATUS_NOT_BOOTED)) {
			close(fd);
			return 0;
		}
		printf("WAIT %i  \r",n);
		fflush(stdout);
		sleep(1);
	}
	close(fd);
	return 1;
}
/*------------------------------------------------------------------------*/
void usage(void)
{
	fprintf(stderr,
		"Usage: hdboot <options>\n"
		"       -r        Force warm reset\n"
		"       -p <hex>  Manually set PCI BAR1 (DANGEROUS!!!)\n"
		"       -i <file> Upload file (default: linux.bin)\n"
		"       -e <hex>  Manually set kernel entry address (default: auto detect)\n"
		"       -v        Verify DDR-RAM after upload\n"
		"       -w <timeout>  Wait until kernel is up\n");
	exit(1);
}
/*------------------------------------------------------------------------*/
int main(int argc, char ** argv)
{
	unsigned char *pci_base;
	int bar1=0;
	int do_reset=0;
	int i;
	char fname[256]="";
	int entry= 0;
	int verify=0;
	int wait=-1;

	while ((i = getopt(argc, argv, "e:i:p:rvw:")) != -1) {
		switch (i) 
		{
		case 'e':
			entry=strtoul(optarg,NULL,16);
			break;
		case 'i':
			strcpy(fname,optarg);
			break;
		case 'p':
			bar1=strtoul(optarg,NULL,16);
			printf("Overriding PCI BAR1 to %08x\n",bar1);
			break;
		case 'r':
			do_reset=1;
			break;
		case 'v':
			verify=1;
			break;
		case 'w':
			wait=atoi(optarg);
			break;
		default:
			usage();
			break;
		}
	}

	if (bar1==0)
		bar1=find_decypher();

	pci_base=pcimap(bar1);

	if (strlen(fname)) {
		if (wait_for_pciboot(pci_base,0) || do_reset) {
			warm_reset(pci_base);
			sleep(2);
		}
		upload_start(pci_base, fname, entry, verify);
	}

	if (wait>=0)
		exit(wait_kernel(wait));

	exit(0);
}
