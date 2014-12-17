#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <string.h>
		
#define INTEL_CE_GBE_MDIO_RCOMP_BASE    0
#define E1000_MDIO_STS  (INTEL_CE_GBE_MDIO_RCOMP_BASE + 0)
#define E1000_MDIO_CMD  (INTEL_CE_GBE_MDIO_RCOMP_BASE + 4)
#define E1000_MDIO_DRV  (INTEL_CE_GBE_MDIO_RCOMP_BASE + 8)
#define E1000_MDC_CMD   (INTEL_CE_GBE_MDIO_RCOMP_BASE + 0xC)
#define E1000_RCOMP_CTL (INTEL_CE_GBE_MDIO_RCOMP_BASE + 0x20)
#define E1000_RCOMP_STS (INTEL_CE_GBE_MDIO_RCOMP_BASE + 0x24)

#define MAX_PHY_REG_ADDRESS        0x1F 

#define E1000_MDIC_DATA_MASK 0x0000FFFF
#define E1000_MDIC_REG_MASK  0x001F0000
#define E1000_MDIC_REG_SHIFT 16
#define E1000_MDIC_PHY_MASK  0x03E00000
#define E1000_MDIC_PHY_SHIFT 21
#define E1000_MDIC_OP_WRITE  0x04000000
#define E1000_MDIC_OP_READ   0x08000000
#define E1000_MDIC_READY     0x10000000
#define E1000_MDIC_INT_EN    0x20000000
#define E1000_MDIC_ERROR     0x40000000

#define INTEL_CE_GBE_MDIC_OP_WRITE      0x04000000
#define INTEL_CE_GBE_MDIC_OP_READ       0x00000000
#define INTEL_CE_GBE_MDIC_GO            0x80000000
#define INTEL_CE_GBE_MDIC_READ_ERROR    0x80000000

typedef int s32;
typedef unsigned int u32;
typedef unsigned short u16;

unsigned char volatile *vaddr;
u32 phy_addr=0;

int phys_base=0xdffe0d00;

void init_pci(void)
{
	int fd;
	off_t base;
	size_t len;
        fd=open("/dev/mem",O_RDWR);
	if (fd<0) {
		perror("open");
		exit(0);
	}

        base=phys_base&~0xfff; // 0xDFFE0d00
        len=4096;
//        printf("FD %i %lx\n",fd, base);
        vaddr=mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);	
	if (vaddr==(void*)-1) {
		perror("mmap");
		exit(-1);
	}
	vaddr+=(phys_base&0xfff);
}

u32 inline readl(u32 x)
{
	volatile u32* zz=(volatile u32*)(vaddr+x);
	return *zz;
}

void inline writel(u32 d, u32 x)
{
	volatile u32* zz=(volatile u32*)(vaddr+x);
	*zz=d;
}

#if 0
s32 read_phy(u32 reg_addr,
                                 u16 *phy_data)
{
        u32 i;
        u32 mdic = 0;

	mdic = ((reg_addr << E1000_MDIC_REG_SHIFT) |
		(phy_addr << E1000_MDIC_PHY_SHIFT) |
		(INTEL_CE_GBE_MDIC_OP_READ) |
		(INTEL_CE_GBE_MDIC_GO));

	writel(mdic, E1000_MDIO_CMD);

	for (i = 0; i < 1000; i++) {
		usleep(1000);
		mdic = readl(E1000_MDIO_CMD);
		if (!(mdic & INTEL_CE_GBE_MDIC_GO))
			break;
	}

	if (mdic & INTEL_CE_GBE_MDIC_GO) {
		printf("MDI Read did not complete\n");
		return -1;
	}

	mdic = readl(E1000_MDIO_STS);
	if (mdic & INTEL_CE_GBE_MDIC_READ_ERROR) {
		printf("MDI Read Error\n");
		return -1;
	}
	*phy_data = (u16) mdic;
               
        return 0;
}
#endif

s32 write_phy(u32 reg_addr, u16 phy_data)
{
        u32 i;
        u32 mdic = 0;
       
	mdic = (((u32) phy_data) |
		(reg_addr << E1000_MDIC_REG_SHIFT) |
		(phy_addr << E1000_MDIC_PHY_SHIFT) |
		(INTEL_CE_GBE_MDIC_OP_WRITE) |
		(INTEL_CE_GBE_MDIC_GO));

	writel(mdic, E1000_MDIO_CMD);

	for (i = 0; i < 100; i++) {
		usleep(1000);
		mdic = readl(E1000_MDIO_CMD);
		if (!(mdic & INTEL_CE_GBE_MDIC_GO))
			break;
	}

	if (mdic & INTEL_CE_GBE_MDIC_GO) {
		printf("MDIO failed\n");
		return -1;
	}
        return 0;
}

void rtl8211_wol(unsigned char *mac)
{
        int regs[]={31,7, 30,0x6d, 25,1,
                    21,0x1000,
                    22,0x9fff,
                    31,7,
                    30,0x6e,
                    -1};
        int n=0;
                
        while(regs[n]!=-1) {                   
                    write_phy(regs[n],regs[n+1]);
                    n+=2;
        }
        
        write_phy(21,mac[0]|(mac[1]<<8));
        write_phy(22,mac[2]|(mac[3]<<8));
        write_phy(23,mac[4]|(mac[5]<<8));
}

int get_mac(char *dev, unsigned char *mac)
{
        int s,r,n;
        struct ifreq ifr;
        
        s=socket(AF_INET, SOCK_DGRAM, 0);
        if (s==-1) 
                return -1;
                
        strcpy(ifr.ifr_name, dev);
        r=ioctl(s, SIOCGIFHWADDR, &ifr);
	if (r)
		return -1;
	memcpy(mac,ifr.ifr_hwaddr.sa_data,6);
	for(n=0;n<6;n++)
		printf("%02x ",mac[n]);
	puts("");
	return 0;
}

int main(int argc, char **argv)
{
	unsigned char mac[6]={0};
	char dev[256]="eth0";
	char cmdline[256];
	if (argc==2)
		strncpy(dev,argv[1],255);
	dev[255]=0;
	snprintf(cmdline,255,"ethtool -s %s wol m",dev);
	system(cmdline);
	init_pci();
	get_mac(dev,mac);
	rtl8211_wol(mac);
	exit(0);
}
