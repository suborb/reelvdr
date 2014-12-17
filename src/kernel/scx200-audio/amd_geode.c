/*
 * <LIC_AMD_STD>
 * Copyright (c) <years> Advanced Micro Devices, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * </LIC_AMD_STD>
 * <CTL_AMD_STD>
 * </CTL_AMD_STD>
 * <DOC_AMD_STD>
 *  CS5535 documentation available from AMD.
 * </DOC_AMD_STD>
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <sound/core.h>
#include <sound/info.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include "os_inc.h"
#include "amd_geode.h"


#define SNDRV_GET_ID
#include <sound/initval.h>


#define CARD_NAME "CS5535 Audio"
#define DRIVER_NAME "GEODE"

#define GEODEALSA_BUILD_NUM "0500"
#define DRIVER_VERSION "1.0." GEODEALSA_BUILD_NUM


MODULE_AUTHOR("Jens Altmann <jens.altmann@amd.com>");
MODULE_DESCRIPTION("Geode CS5535/6 sound");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{AMD, Geode CS5535/6}}");


static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;	/* Enable switches */

MODULE_PARM(index, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(index, "Index value for Geode CS5535/6 soundcard.");
MODULE_PARM(id, "1-" __MODULE_STRING(SNDRV_CARDS) "s");
MODULE_PARM_DESC(id, "ID string for Geode CS5535/6 soundcard.");
MODULE_PARM(enable, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(enable, "Enable Geode CS5535/6 soundcard.");


static char version[] __devinitdata =
KERN_INFO "Geode ALSA: version " DRIVER_VERSION " time " __TIME__ " " __DATE__ "\n";

#define chip_t geode_t

#define  BM0_IRQ            0x04
#define  BM1_IRQ            0x08
#define BITS_8_TO_16(x) ( ( (long) ((unsigned char) x - 128) ) << 8 )

/*
 * PCI ids
 */

#ifndef PCI_VENDOR_ID_NS
#define PCI_VENDOR_ID_NS 0x100b
#endif

#ifndef PCI_VENDOR_ID_AMD
#define PCI_VENDOR_ID_AMD 0x1022
#endif

#ifndef PCI_DEVICE_ID_NS_CS5535_AUDIO
#define PCI_DEVICE_ID_NS_CS5535_AUDIO 0x002e
#endif
#ifndef PCI_DEVICE_ID_AMD_CS5536_AUDIO
#define PCI_DEVICE_ID_AMD_CS5536_AUDIO 0x2093
#endif

static struct pci_device_id snd_amd_ids[] =
{
        {PCI_VENDOR_ID_NS, PCI_DEVICE_ID_NS_CS5535_AUDIO, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{PCI_VENDOR_ID_NS, 0x0503, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
	{0,},

};


PGEODEAUDIO     pGeode;
MODULE_DEVICE_TABLE(pci, snd_amd_ids);

/* The actual rates supported by the card. */
static unsigned int samplerates[8] = {
	8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000,
};

static snd_pcm_hw_constraint_list_t constraints_rates =
{
	.count = ARRAY_SIZE(samplerates), 
	.list = samplerates,
	.mask = 0,
};

typedef struct
{
	snd_card_t*     card;
	struct pci_dev* pci;
	unsigned long   iobase;
	unsigned long   irq;
	unsigned long   ioflags;
	unsigned long   port;
	struct resource *res_port;
	snd_pcm_t       *pcm;
	ac97_t          *ac97;
	snd_pcm_substream_t* playbackSubstream;
	snd_pcm_substream_t* recordSubstream;

} snd_amd_t;


/* definition of the chip-specific record */
typedef snd_amd_t geode_t;


/*prototypes*/
static int __devinit snd_geode_new_pcm(geode_t *pAmd);
static int __devinit snd_geode_mixer_new(geode_t *pAmd);
static unsigned short snd_geode_ac97_read(ac97_t *ac97, unsigned short reg);
static void snd_geode_ac97_write(ac97_t *ac97, unsigned short reg, unsigned short val);
static unsigned char snd_geode_wave_open (PGEODEAUDIO pGeode,snd_pcm_substream_t *substream);

static unsigned char snd_hw_InterruptID (PGEODEAUDIO pGeode);
static void snd_hw_ClearIRQ (PGEODEAUDIO pGeode);
static PGEODEAUDIO snd_hw_Initialize (unsigned int Irq, geode_t *pAmd);
static void snd_hw_FreePRD (PGEODEAUDIO pGeode, unsigned long Channel);

static unsigned char snd_hw_AllocatePRD (PGEODEAUDIO pGeode, 
                                         unsigned long Channel, 
                                         unsigned long DMABufferSize,
                                         unsigned long periodSizeInBytes);
static void snd_hw_InitPRD (PGEODEAUDIO     pGeode, 
                            unsigned long Channel, 
                            unsigned long DMAPhys,
                            unsigned long dmasize,
                            unsigned long periodSizeInBytes);
static void snd_hw_ResumeDMA (PGEODEAUDIO pGeode, unsigned long Channel);
static void snd_hw_StartDMA (PGEODEAUDIO pGeode, unsigned long Channel);
static void snd_hw_StopDMA (PGEODEAUDIO pGeode, unsigned long Channel);
static unsigned char snd_hw_ClearStat(PGEODEAUDIO pGeode, unsigned long Channel);
static unsigned short snd_hw_CodecRead ( PGEODEAUDIO pGeode, unsigned char CodecRegister);
static void snd_hw_CodecWrite( PGEODEAUDIO pGeode, unsigned char CodecRegister, unsigned short CodecData);
static unsigned char snd_hw_WaitForBit(PGEODEAUDIO     pGeode, 
                                       unsigned long Offset, 
                                       unsigned long Bit, 
                                       unsigned char Operation,
                                       unsigned long timeout, 
                                       unsigned long *pReturnValue);
static void snd_hw_SetCodecRate(PGEODEAUDIO pGeode, unsigned long Channel, unsigned long SampleRate);
static unsigned long snd_hw_InitAudioRegs(PGEODEAUDIO pGeode);
static void snd_hw_FreeAlignedDMAMemory ( PGEODEAUDIO pGeode, PALLOC_INFO  pAllocInfo );
static unsigned char snd_hw_AllocateAlignedDMAMemory (PGEODEAUDIO      pGeode,
						      unsigned long  Size,
						      PALLOC_INFO    pAllocInfo);
static void snd_hw_SetupIRQ (PGEODEAUDIO pGeode, unsigned long Irq);

static int snd_geode_playback_open(snd_pcm_substream_t *substream);
static int snd_geode_playback_close(snd_pcm_substream_t *substream);
static int snd_geode_capture_open(snd_pcm_substream_t *substream);
static int snd_geode_capture_close(snd_pcm_substream_t *substream);
static int snd_geode_pcm_hw_params(snd_pcm_substream_t *substream,
				   snd_pcm_hw_params_t * hw_params);
static int snd_geode_pcm_hw_free(snd_pcm_substream_t *substream);
static int snd_geode_pcm_prepare(snd_pcm_substream_t *substream);
static int snd_geode_pcm_trigger(snd_pcm_substream_t *substream,int cmd);
static snd_pcm_uframes_t snd_geode_pcm_playback_pointer(snd_pcm_substream_t *substream);
static snd_pcm_uframes_t snd_geode_pcm_capture_pointer(snd_pcm_substream_t *substream);
static int __devinit snd_amd_probe(struct pci_dev *pci,
                                   const struct pci_device_id *pci_id);
static void __devexit snd_amd_remove(struct pci_dev *pci);
static void snd_amd_adjust_prd_table(int channel, snd_pcm_runtime_t* pRuntime);


#ifdef CONFIG_PM

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9))
static int snd_geode_suspend(snd_card_t *card, unsigned int state);
static int snd_geode_resume(snd_card_t *card, unsigned int state);
#else
static int snd_geode_suspend(struct pci_dev *dev, u32 state);
static int snd_geode_resume(struct pci_dev *dev);
#endif 

static unsigned long snd_geode_SetPowerState(PGEODEAUDIO pGeode, GEODEAUDIO_POWER_STATE NewPowerState);
static void snd_hw_ResetCodec (PGEODEAUDIO pGeode);
static unsigned char snd_hw_CheckCodecPowerBit(PGEODEAUDIO pGeode, unsigned short Bit, unsigned char Operation);
static unsigned char snd_hw_CodecFullOn (PGEODEAUDIO pGeode);
static void snd_hw_SaveAudioContext (PGEODEAUDIO pGeode);
static void snd_hw_RestoreAudioContext (PGEODEAUDIO pGeode);
#endif

#define geode_t_magic        0x15052003

/* operators */
static snd_pcm_ops_t snd_geode_playback_ops =
{
	.open =        snd_geode_playback_open,
	.close =       snd_geode_playback_close,
	.ioctl =       snd_pcm_lib_ioctl,
	.hw_params =   snd_geode_pcm_hw_params,
	.hw_free =     snd_geode_pcm_hw_free,
	.prepare =     snd_geode_pcm_prepare,
	.trigger =     snd_geode_pcm_trigger,
	.pointer =     snd_geode_pcm_playback_pointer,
	//.copy    =     snd_geode_pcm_playback_copy,
	//.silence =     snd_geode_pcm_silence,
};

/* operators */
static snd_pcm_ops_t snd_geode_capture_ops =
{
	.open =        snd_geode_capture_open,
	.close =       snd_geode_capture_close,
	.ioctl =       snd_pcm_lib_ioctl,
	.hw_params =   snd_geode_pcm_hw_params,
	.hw_free =     snd_geode_pcm_hw_free,
	.prepare =     snd_geode_pcm_prepare,
	.trigger =     snd_geode_pcm_trigger,
	.pointer =     snd_geode_pcm_capture_pointer,
};

/* hardware definition */

static snd_pcm_hardware_t snd_geode_playback_hw =
{
	.info = (SNDRV_PCM_INFO_MMAP | 
		 SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP_VALID ),
	.formats =          SNDRV_PCM_FMTBIT_S16_LE,
	.rates =            SNDRV_PCM_RATE_8000_48000,
	.rate_min =         8000,
	.rate_max =         48000,
	.channels_min =     2,
	.channels_max =     2,
	.buffer_bytes_max = DMA_PLAYBACK_BUFFERSIZE,
	.period_bytes_min = 64,
	.period_bytes_max = DMA_PLAYBACK_BUFFERSIZE,
	.periods_min =      1,
	.periods_max =      1024,
};

/* hardware definition */
static snd_pcm_hardware_t snd_geode_capture_hw =
{
	.info = (SNDRV_PCM_INFO_MMAP | 
		 SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =          SNDRV_PCM_FMTBIT_S16_LE,
	.rates =            SNDRV_PCM_RATE_48000 ,
	.rate_min =         48000,
	.rate_max =         48000,
	.channels_min =     2,
	.channels_max =     2,
	.buffer_bytes_max = DMA_CAPTURE_BUFFERSIZE,
	.period_bytes_min = 64,
	.period_bytes_max = DMA_CAPTURE_BUFFERSIZE,
	.periods_min =      1,
	.periods_max =      1024,
};


static struct pci_driver driver =
{
	.name = "Geode CS5535",
	.id_table = snd_amd_ids,
	.probe = snd_amd_probe,
	.remove = __devexit_p(snd_amd_remove),
#ifdef CONFIG_PM
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9))
	SND_PCI_PM_CALLBACKS
#else
	.suspend = snd_geode_suspend,    
	.resume = snd_geode_resume, 
#endif // LINUX_VERSION_CODE     
#endif //CONFIG_PM
};

/**
 * \ingroup 
 * \brief
 * Updates the PRD-table to satisfy non integer 
 * relations between buffersize and period size 
 * 
 * 
 * \param    int channel
 *           snd_pcm_runtime_t pRuntime
 * \return 
 */
static void snd_amd_adjust_prd_table(int channel, snd_pcm_runtime_t* pRuntime)
{
	PPRD_ENTRY		pPRDTab;
	int             idx,maxidx,nextidx,overnextidx;
	unsigned long   prdaddr;
	unsigned long   current_ptr,periodBytes,baseaddr,rest,buffersize;


	periodBytes = frames_to_bytes(pRuntime,pRuntime->period_size);
	buffersize= pRuntime->dma_bytes;
	if((periodBytes*pRuntime->periods) == buffersize) 
	{                    
		return; /*Nothing to do - the PRD table is fix*/ 
	}
	OS_DbgMsg("-->snd_amd_adjust_prd_table\n");
	baseaddr=pGeode->AudioChannel[channel].DMA_AllocInfo.PhysicalAddress;
	if(pGeode->AudioChannel[channel].Running==TRUE)
	{
		current_ptr =*(int*)( pGeode->AudioChannel[channel].AudioBusMaster.DMAPointer)-
			baseaddr;
	}
	else
	{  
		current_ptr = 0;
	}
	pPRDTab=&pGeode->AudioChannel[channel].PRDTable[0];
	maxidx=pGeode->AudioChannel[channel].PRDEntriesAllocated-2; 
	idx=maxidx;
	while(idx)
	{
		prdaddr=pPRDTab[idx].ulPhysAddr-baseaddr;
		if(current_ptr>=prdaddr && current_ptr<=prdaddr+0x20) 
		{     
			break;
		}
		idx--;
	}
	if(idx>maxidx)
	{   
		idx=0;
		nextidx=1;
		overnextidx=2;
	}
	else
	{ 
		nextidx=idx+1;  
		if(nextidx>maxidx)
		{ 
			nextidx=0;
			overnextidx=1;
		} 
	}
	overnextidx=nextidx+1;
	if(overnextidx>maxidx)
	{ 
		overnextidx=0;
	} 
    
	OS_DbgMsg("maxidx=%d, idx=%d, nextidx=%d ,overnextidx=%d, cur_ptr=%.4X ,buffersize=%X\n",maxidx, idx, nextidx,overnextidx,current_ptr,buffersize);
     
	if(((pPRDTab[idx].ulPhysAddr-baseaddr)+2*periodBytes) <= buffersize)
	{
		pPRDTab[nextidx].ulPhysAddr = pPRDTab[idx].ulPhysAddr+(pPRDTab[idx].SizeFlags & (~PRD_EOP_BIT)); 
		pPRDTab[nextidx].SizeFlags = periodBytes | PRD_EOP_BIT;
		pPRDTab[overnextidx].ulPhysAddr=(pPRDTab[nextidx].ulPhysAddr-baseaddr) + (pPRDTab[nextidx].SizeFlags & (~PRD_EOP_BIT));
		pPRDTab[overnextidx].ulPhysAddr%=buffersize;
		pPRDTab[overnextidx].ulPhysAddr+=baseaddr;            
		rest=buffersize - (pPRDTab[overnextidx].ulPhysAddr-baseaddr);
		if(rest>=periodBytes)                                              
			pPRDTab[overnextidx].SizeFlags= periodBytes | PRD_EOP_BIT;
		else
			pPRDTab[overnextidx].SizeFlags=rest;  

	}
	else
	{ 
      
		pPRDTab[nextidx].ulPhysAddr = (pPRDTab[idx].ulPhysAddr-baseaddr)+(pPRDTab[idx].SizeFlags & (~PRD_EOP_BIT));
		pPRDTab[nextidx].ulPhysAddr%=buffersize;  

		if((pPRDTab[idx].SizeFlags&PRD_EOP_BIT))
		{
			rest=(buffersize-pPRDTab[nextidx].ulPhysAddr);
			if(rest>periodBytes)
			{ 
				pPRDTab[nextidx].SizeFlags = periodBytes | PRD_EOP_BIT; 
			}
			else
			{
				pPRDTab[nextidx].SizeFlags = rest; 
			}
		}  
		pPRDTab[nextidx].ulPhysAddr+=baseaddr; 

		pPRDTab[overnextidx].ulPhysAddr=(pPRDTab[nextidx].ulPhysAddr-baseaddr) + (pPRDTab[nextidx].SizeFlags & (~PRD_EOP_BIT));
		pPRDTab[overnextidx].ulPhysAddr%=buffersize;
        
		if(!(pPRDTab[nextidx].SizeFlags&PRD_EOP_BIT))
		{
			pPRDTab[overnextidx].SizeFlags=(periodBytes-(pPRDTab[nextidx].SizeFlags & (~PRD_EOP_BIT))) | PRD_EOP_BIT;                
		}
		else
		{    
			if((pPRDTab[overnextidx].ulPhysAddr+periodBytes) <= buffersize)
			{
				pPRDTab[overnextidx].SizeFlags=periodBytes | PRD_EOP_BIT;
			}
			else
			{              
				rest=buffersize-pPRDTab[overnextidx].ulPhysAddr;
				pPRDTab[overnextidx].SizeFlags=rest;
			}
		} 
		pPRDTab[overnextidx].ulPhysAddr+=baseaddr;
	}


	OS_DbgMsg("PRDTable:\n");
	for (idx=0; idx<(pGeode->AudioChannel[channel].PRDEntriesAllocated); ++idx)
	{
		OS_DbgMsg("snd_hw_InitPRD[%.3X] %p ,address: %.8X, flags: %.8X\n",
			  idx,
			  &pGeode->AudioChannel[channel].PRDTable[idx],
			  pGeode->AudioChannel[channel].PRDTable[idx].ulPhysAddr,
			  pGeode->AudioChannel[channel].PRDTable[idx].SizeFlags);
	}
	OS_DbgMsg("<--snd_amd_adjust_prd_table\n");
}

/**
 * \ingroup linux GX layer
 * \brief
 * Called when an interrupt from the DMA controller occurs.
 * The function is bound when the interrupt will be allocated.
 * Notifies the upper alsa layer of finished processing of a
 * sample fragment
 *
 * \param  int irq,
 *         void *dev_id,
 *         struct pt_regs *regs
 * \return IRQ_HANDLED
 */

static irqreturn_t snd_amd_interrupt(int irq, void *dev_id,struct pt_regs *regs)
{
	unsigned char   IntID;
	geode_t *pAmd;

	IntID = snd_hw_InterruptID(pGeode);

	snd_hw_ClearIRQ(pGeode);
	pAmd = dev_id;
	if(IntID & BM0_IRQ)
	{
		OS_DbgMsg(">>BM0_IRQ\n");
		snd_pcm_period_elapsed(pAmd->playbackSubstream);           
		snd_amd_adjust_prd_table(CHANNEL0_PLAYBACK,pAmd->playbackSubstream->runtime); 
	}
	if(IntID & BM1_IRQ)
	{
		snd_pcm_period_elapsed(pAmd->recordSubstream);
		snd_amd_adjust_prd_table(CHANNEL1_RECORD,pAmd->recordSubstream->runtime);
		//OS_DbgMsg(">>BM1_IRQ\n");
	}
	return IRQ_HANDLED;
}

/**
 * \ingroup linux GX layer
 * \brief
 * Called to free hardware related ressources like
 * IO space, IRQ, chip specific data
 *
 * \param  geode_t *pAmd
 * \return int ; errorcode if successful=0
 */
static int snd_amd_free(geode_t *pAmd)
{
	OS_DbgMsg("-->snd_amd_free\n");

	/*release the i/o port*/
	if (pAmd->res_port)
	{
		release_resource(pAmd->res_port);
		kfree_nocheck(pAmd->res_port);
	}
	/*release the irq*/
	if (pAmd->irq >= 0)
	{
		synchronize_irq(pAmd->irq);
		free_irq(pAmd->irq, (void *)pAmd);
	}
	/* release the data*/
	kfree(pAmd);
	OS_DbgMsg("<--snd_amd_free\n");
	return 0;
}

/**
 * \ingroup linux GX layer
 * \brief
 * Callback for the drivers "dev_free" function.
 * Frees port and irq allocation via call to "snd_amd_free".
 * 
 * \param  snd_device_t *device
 * \return int ; errorcode if successful=0
 */
static int snd_amd_dev_free(snd_device_t *device)
{
	int ret;
	geode_t *pAmd;

	OS_DbgMsg("-->snd_amd_dev_free\n");
	pAmd = device->device_data;
	ret=snd_amd_free(pAmd);
	OS_DbgMsg("<--snd_amd_dev_free\n");
	return(ret);
}

/**
 * \ingroup linux GX layer
 * \brief
 * Does all hardware specific allocation and initialization 
 * 1. allocate memory for the HW data structure (geode_t)
 * 2. allocate interrupt and IO-space
 * 3. Initialize the Hardware
 * This function will be called from the drivers probe callback
 *
 * \param snd_card_t*      card,  
 *        struct pci_dev*  pci
 *        geode_t **       rchip)  
 * \return int ;errorcode if succesful=0
 */
static int __devinit snd_amd_create(snd_card_t *card,
                                    struct pci_dev *pci,
                                    geode_t **rchip)
{
	geode_t *pAmd;
	int err;
	static snd_device_ops_t ops =
		{
			.dev_free = snd_amd_dev_free,
		};

	OS_DbgMsg("-->snd_amd_create\n");
	*rchip = NULL;

	if (pci_enable_device(pci))
	{
		return -EIO;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9))
	/*allocate a chip-specific data with magic-alloc*/
	pAmd = (geode_t *)kcalloc(1, sizeof(geode_t), GFP_KERNEL);
#else
	pAmd = snd_magic_kcalloc(geode_t, 0, GFP_KERNEL);
#endif
	if (pAmd == NULL)
	{
		OS_DbgMsg("<--snd_amd_create failed:(..calloc)\n");
		return -ENOMEM;
	}

	card->private_data=pAmd; 
	pAmd->card = card;
	pAmd->pci = pci;
	pAmd->irq = 11;
	pci_resource_start(pci, 0)=0x40011000; //GA
	/*(1) PCI resource allocation*/
/*    pAmd->port = pci_resource_start(pci, 0);
      if ((pAmd->res_port = request_region(pAmd->port, 8,
      "My Chip")) == NULL) { 
      snd_amd_free(pAmd);
      printk(KERN_ERR "cannot allocate the port\n");
      return -EBUSY;
      }*/
	if (request_irq(pAmd->irq, snd_amd_interrupt,
			SA_INTERRUPT|SA_SHIRQ, "snd-geode",(void *)pAmd))
	{
		snd_amd_free(pAmd);
		printk(KERN_ERR "cannot grab irq\n");
		return -EBUSY;
	}
//    pAmd->irq = pci->irq;
	pAmd->iobase=pci_resource_start(pci, 0);   
	pAmd->ioflags=pci_resource_flags(pci, 0);  

	/*(2) initialization of the chip hardware*/
	pGeode=snd_hw_Initialize((unsigned int) pAmd->irq,pAmd);

	if (!pGeode) {
		snd_amd_free(pAmd);
		OS_DbgMsg("<--snd_amd_create failed:(snd_hw_Initialize)\n");
		return -EBUSY;
	}
	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL,pAmd, &ops)) < 0)
	{
		snd_amd_free(pAmd);
		OS_DbgMsg("<--snd_amd_create failed:(snd_device_new)\n");
		return err;
	}
	*rchip = pAmd;
	snd_card_set_dev(card, &pci->dev);

#ifdef CONFIG_PM 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9))
	snd_card_set_pm_callback(card, snd_geode_suspend, snd_geode_resume, pAmd);
#endif
#endif

	OS_DbgMsg("<--snd_amd_create\n");
	return 0;
}


/**
 * \ingroup linux GX layer
 * \brief
 * Called from pci_module_init immediately after loading 
 * the driver.Creates Mixer and PCM instance and
 * registers the driver.
 * 
 * \param  struct pci_dev *pci,              
 *         const struct pci_device_id *pci_id
 * \return int= o if successful
 */
static int __devinit snd_amd_probe(struct pci_dev *pci,
                                   const struct pci_device_id *pci_id)
{
	static int dev;
	snd_card_t *card;
	geode_t *pAmd;
	int err;

	OS_DbgMsg("-->snd_amd_probe\n");
	if (dev >= SNDRV_CARDS)
	{
		OS_DbgMsg("<--snd_amd_probe failed:(dev >= SNDRV_CARDS)\n");
		return -ENODEV;
	}
	if (!enable[dev])
	{
		dev++;
		OS_DbgMsg("<--snd_amd_probe failed:(!enable[dev])\n");
		return -ENOENT;
	}

	card = snd_card_new(index[dev], id[dev], THIS_MODULE, 0);
	if (card == NULL)
	{
		OS_DbgMsg("<--snd_amd_probe failed:(card == NULL)\n");
		return -ENOMEM;
	}

	if ((err = snd_amd_create(card, pci, &pAmd)) < 0)
	{
		snd_card_free(card);
		OS_DbgMsg("<--snd_amd_probe failed:(snd_amd_create)\n");
		return err;
	}
	strcpy(card->driver, "CS553x audio");
	strcpy(card->shortname, "Geode CS553x audio");
	sprintf(card->longname, "%s at 0x%lx irq %i",
		card->shortname, (long)pAmd->iobase, (int)pAmd->irq);

	if ((err = snd_geode_new_pcm(pAmd)) < 0)
	{  
		snd_card_free(card);
		OS_DbgMsg("<--snd_amd_probe failed:(snd_geode_new_pcm)\n");
		return err;
	}

	if ((err = snd_geode_mixer_new(pAmd)) < 0)
	{
		snd_card_free(card);
		OS_DbgMsg("<--snd_amd_probe failed:(snd_geode_mixer_new)\n");
		return err;
	}       

	if ((err = snd_card_register(card)) < 0)
	{
		snd_card_free(card);
		OS_DbgMsg("<--snd_amd_probe failed:(snd_card_register)\n");
		return err;
	}
	pci_set_drvdata(pci, card);
	dev++;
	OS_DbgMsg("<--snd_amd_probe\n");
	return 0;
}



/**
 * \ingroup linux GX layer
 * \brief
 * Called from the module unload callback. Release all
 * data allocated by the driver by calling snd_card_free
 * 
 * \param struct pci_dev *pci
 */
static void __devexit snd_amd_remove(struct pci_dev *pci)
{ 
	OS_DbgMsg("-->snd_amd_remove pci=%X\n",pci);
	snd_card_free(pci_get_drvdata(pci));
	pci_set_drvdata(pci, NULL);
	OS_Free((unsigned char*)pGeode); 
	OS_DbgMsg("<--snd_amd_remove\n");
}


/**
 * \ingroup linux GX layer
 * \brief
 * Callback function called before start of playback.
 * Returns the hardware capabilities to the upper 
 * layer.
 * 
 * \param snd_pcm_substream_t *substream
 * \return always success
 */
static int snd_geode_playback_open(snd_pcm_substream_t *substream)
{
	geode_t *pAmd;

	OS_DbgMsg("-->snd_geode_playback_open\n");
	pAmd = snd_pcm_substream_chip(substream);
	pAmd->playbackSubstream=substream;
	substream->runtime->hw = snd_geode_playback_hw;
   	snd_pcm_hw_constraint_list(substream->runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &constraints_rates);
	OS_DbgMsg("<--snd_geode_playback_open\n");
	return 0;
}

/**
 * \ingroup linux GX layer
 * \brief
 * Callback function called when playback.ends
 * Does actually nothing here.
 *
 * \param snd_pcm_substream_t *substream
 * \return always success
 */
static int snd_geode_playback_close(snd_pcm_substream_t *substream)
{
	geode_t *pAmd;

	OS_DbgMsg("-->snd_geode_playback_close\n");
	pAmd = snd_pcm_substream_chip(substream);
	pAmd->playbackSubstream=NULL;
	OS_DbgMsg("<--snd_geode_playback_close\n");
	return 0;
}

/**
 * \ingroup linux GX layer
 * \brief
 * Callback function called before start of recording.
 * Returns the hardware capabilities to the upper 
 * layer.
 * 
 * \param snd_pcm_substream_t *substream
 * \return always success
 */
static int snd_geode_capture_open(snd_pcm_substream_t *substream)
{
	geode_t *pAmd;
	snd_pcm_runtime_t *runtime;

	OS_DbgMsg("-->snd_geode_capture_open\n");
	pAmd = snd_pcm_substream_chip(substream);
	pAmd->recordSubstream=substream;
	runtime = substream->runtime;
	runtime->hw = snd_geode_capture_hw;
   	snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &constraints_rates);
	OS_DbgMsg("<--snd_geode_capture_open\n");
	return 0;
}

/**
 * \ingroup linux GX layer
 * \brief
 * Callback function called when capturing.ends
 * Does actually nothing here.
 *
 * \param snd_pcm_substream_t *substream
 * \return always success
 */
static int snd_geode_capture_close(snd_pcm_substream_t *substream)
{
	geode_t *pAmd;

	OS_DbgMsg("-->snd_geode_capture_close\n");
	pAmd = snd_pcm_substream_chip(substream);
	pAmd->recordSubstream=NULL;
	OS_DbgMsg("<--snd_geode_capture_close\n");
	return 0;
}


/**
 * \ingroup linux GX layer
 * \brief
 * Callback function called before the PCM interface is used.
 * Calls the allocation function for the dma page allocation
 * 
 * \param snd_pcm_substream_t *substream
 * \return always success
 */
static int snd_geode_pcm_hw_params(snd_pcm_substream_t *substream,
				   snd_pcm_hw_params_t * hw_params)
{
	int pages;

	OS_DbgMsg("-->snd_geode_pcm_hw_params\n"); 
	pages=snd_pcm_lib_malloc_pages(substream,params_buffer_bytes(hw_params));
	OS_DbgMsg("<--snd_geode_pcm_hw_params: pages=%X\n",pages);
	return pages; 
}


/**
 * \ingroup linux GX layer
 * \brief
 * Callback function called before the PCM interface will be closed.
 * Release the driver allocated PRD table and calls
 * the fre function in the ALSA layer to release the DMA buffers 
 * 
 * \param snd_pcm_substream_t *substream
 * \return return value of snd_pcm_lib_free_pages
 */
static int snd_geode_pcm_hw_free(snd_pcm_substream_t *substream)
{
	int pages;
	unsigned long channel;

	OS_DbgMsg("-->snd_geode_pcm_hw_free\n"); 
	pages= snd_pcm_lib_free_pages(substream);
	channel=(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)?CHANNEL0_PLAYBACK:CHANNEL1_RECORD; 
	if(pGeode->AudioChannel[channel].fAllocated==TRUE)
	{
		snd_hw_FreePRD(pGeode,channel);
		pGeode->AudioChannel[channel].fAllocated=FALSE;
	}
	OS_DbgMsg("<--snd_geode_pcm_hw_free\n"); 
	return pages;
}

/**
 * \ingroup linux GX layer
 * \brief
 * Callback function called before the PCM interface opened.
 * Calls snd_geode_wave_open which initialize the PRD table
 * and necessary codec registers.
 * 
 * \param snd_pcm_substream_t *substream
 * \return always success
 */
static int snd_geode_pcm_prepare(snd_pcm_substream_t *substream)
{
	geode_t *pAmd;

	OS_DbgMsg("-->snd_geode_pcm_prepare\n"); 
	pAmd= snd_pcm_substream_chip(substream);
	snd_geode_wave_open( pGeode,substream);
	OS_DbgMsg("<--snd_geode_pcm_prepare\n"); 
	return 0;
}


/**
 * \ingroup linux GX layer
 * \brief
 * Callback function to start or stop the DMA transfer for
 * record or playback
 * 
 * \param snd_pcm_substream_t *substream
 *        int                 cmd 0=STOP,1=START                  
 * \return 0 if successful
 */
static int snd_geode_pcm_trigger(snd_pcm_substream_t *substream,int cmd)
{
	unsigned long channel;

	OS_DbgMsg("-->snd_geode_pcm_trigger=%d ch=%d\n",cmd,substream->stream); 
	channel=(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)?CHANNEL0_PLAYBACK:CHANNEL1_RECORD; 
	switch (cmd)
	{
        case SNDRV_PCM_TRIGGER_START:
		/*start the PCM engine*/
		snd_hw_StartDMA (pGeode,channel);
		break;
        case SNDRV_PCM_TRIGGER_STOP:
		/*stop the PCM engine*/
		snd_hw_StopDMA (pGeode, channel);
		break;
        default:
		OS_DbgMsg("<--snd_geode_pcm_trigger failed:(EINVAL)\n"); 
		return -EINVAL;
	}
	OS_DbgMsg("<--snd_geode_pcm_trigger\n"); 
	return 0;
}
//------------------------------------------------------------------------
//  Returns the Current PRD reg contents, unmodified
//------------------------------------------------------------------------
static unsigned long snd_geode_GetCurrentPRD (unsigned long Channel)
{
	unsigned long	PRDPointer;


	if (Channel==CHANNEL0_PLAYBACK)
		PRDPointer = *((unsigned long *) ( pGeode->F3BAR0 + 0x24 ));    
	else
		PRDPointer = *((unsigned long *) ( pGeode->F3BAR0 + 0x2C ));

	return PRDPointer;
}



//------------------------------------------------------------------------
//  Returns the Current PRD index being played
//------------------------------------------------------------------------
unsigned long snd_geode_GetCurrentPRDIndex (unsigned long Channel)
{
	unsigned char	CommandRegStatus;
	unsigned long	PRDPointer, PRDIndex;

	//
	//    If the BusMaster is not enabled, return 0 as position
	//

	
	CommandRegStatus = *((unsigned char *)  pGeode->AudioChannel[Channel].AudioBusMaster.CommandRegister);


	if (!( CommandRegStatus & ENABLE_BUSMASTER))
		return 0;

	PRDPointer = snd_geode_GetCurrentPRD ( Channel );

	if (PRDPointer==pGeode->AudioChannel[Channel].PRD_AllocInfo.PhysicalAddress)
	{
		PRDIndex=0;
	}
	else
	{
		PRDIndex=(PRDPointer - pGeode->AudioChannel[Channel].PRD_AllocInfo.PhysicalAddress) / sizeof(PRD_ENTRY) - 1;
	}

	return PRDIndex;
}

/**
 * \ingroup linux GX layer
 * \brief
 * This callback is called when the PCM middle layer inquires the current 
 * hardware position on the playback buffer.
 * The position will be returned in frames ranged from 0 to (buffer_size - 1).
 * For playback at sample sizes of 8Bit and/or single channel the pointer
 * is adjusted to the expected range in the ALSA layer.
 *
 * \param snd_pcm_substream_t *substream
 *                                           
 * \return snd_pcm_uframes_t
 */
static snd_pcm_uframes_t snd_geode_pcm_playback_pointer(snd_pcm_substream_t *substream)
{
	unsigned int current_ptr=0;
	snd_pcm_uframes_t    bpf;
	int Channel=CHANNEL0_PLAYBACK;
	int PRDIndex;
	unsigned long OffsetBytePointer;

	PRDIndex=snd_geode_GetCurrentPRDIndex ( Channel);
	OffsetBytePointer = pGeode->AudioChannel[Channel].PRDTable[PRDIndex].ulPhysAddr;
	//
	//   If we are right at the JMP PRD entry, Return 0 (beginning of the buffer)
	//
	if (OffsetBytePointer == pGeode->AudioChannel[Channel].PRD_AllocInfo.PhysicalAddress)
		return 0;

	current_ptr=OffsetBytePointer - pGeode->AudioChannel[Channel].PRDTable[0].ulPhysAddr;	
	
	bpf=current_ptr;
    
	if(substream->runtime->format <= SNDRV_PCM_FORMAT_U8)      
		bpf/=2;          

	if(substream->runtime->channels == 1)
		bpf/=2;          

	bpf=bytes_to_frames(substream->runtime,bpf);
	OS_DbgMsg("current_ptr(bytes)=%X\n",current_ptr);
	OS_DbgMsg("<--snd_geode_pcm_playback_pointer(frames): bpf=%X\n",bpf); 
	return bpf;
}

/**
 * \ingroup linux GX layer
 * \brief
 * This callback is called when the PCM middle layer inquires the current 
 * hardware position on the capture buffer.
 * The position will be returned in frames ranged from 0 to (buffer_size - 1).
 *
 * \param snd_pcm_substream_t *substream
 *                                           
 * \return snd_pcm_uframes_t
 */
static snd_pcm_uframes_t snd_geode_pcm_capture_pointer(snd_pcm_substream_t *substream)
{
	unsigned int current_ptr=0;
	snd_pcm_uframes_t    bpf;
	int Channel=CHANNEL1_RECORD;
	int PRDIndex;
	unsigned long OffsetBytePointer;

	PRDIndex=snd_geode_GetCurrentPRDIndex (Channel);
	OffsetBytePointer = pGeode->AudioChannel[Channel].PRDTable[PRDIndex].ulPhysAddr;
	//
	//   If we are right at the JMP PRD entry, Return 0 (beginning of the buffer)
	//
	if (OffsetBytePointer == pGeode->AudioChannel[Channel].PRD_AllocInfo.PhysicalAddress)
		return 0;

	current_ptr=OffsetBytePointer - pGeode->AudioChannel[Channel].PRDTable[0].ulPhysAddr;

	bpf=bytes_to_frames(substream->runtime,current_ptr);
	OS_DbgMsg("current_ptr=%X, PRD-Idx %i\n",current_ptr,PRDIndex);
	OS_DbgMsg("<--snd_geode_pcm_capture_pointer: bpf=%X  buffer_size = 0x%x, period_size = 0x%x\n",
		  bpf,substream->runtime->buffer_size, substream->runtime->period_size); 
	return bpf;
}


/**
 * \ingroup linux GX layer
 * \brief
 * Called from snd_amd_probe to allocate and setup the 
 * AC97 Mixer structure.
 * 
 * \param  geode_t *pAmd
 * \return 0 if successful
 */
static int __devinit snd_geode_mixer_new(geode_t *pAmd)
{
	ac97_bus_t *pbus;
	int err=0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9))
	ac97_template_t     ac97;
	static ac97_bus_ops_t ops = {
		.write = snd_geode_ac97_write,
		.read = snd_geode_ac97_read,
        };
        if ((err = snd_ac97_bus(pAmd->card, 0, &ops, NULL, &pbus)) < 0)
#else
		ac97_t ac97;
	ac97_bus_t bus;
    
        memset(&bus, 0, sizeof(bus));
        bus.write = snd_geode_ac97_write;
        bus.read = snd_geode_ac97_read;
        if ((err = snd_ac97_bus(pAmd->card, &bus, &pbus)) < 0)
#endif

	{
		return err;
	}
	OS_DbgMsg("-->snd_geode_mixer_new\n"); 
	memset(&ac97, 0, sizeof(ac97));
	ac97.private_data = pAmd;
	err = snd_ac97_mixer(pbus, &ac97, &pAmd->ac97);
	OS_DbgMsg("<--snd_geode_mixer_new=%X\n",err); 
	return err;
}


/**
 * \ingroup linux GX layer
 * \brief
 * Callback for read access to the MIXER registers
 *  
 * \param  ac97_t *ac97
 *         unsigned short reg
 *
 * \return 0 if successful
 */
static unsigned short
snd_geode_ac97_read(ac97_t *ac97, unsigned short reg)
{
	geode_t *pAmd;
	unsigned short ret = 0;

//    OS_DbgMsg("-->snd_geode_ac97_read reg=%X\n",reg); 
	pAmd = ac97->private_data;
	ret=snd_hw_CodecRead(pGeode, reg);

	/*special handling for HP-flag*/
#ifndef SUPPORT_HEADPHONE_OUT    
	if(reg==RESET)
	{
		ret&=~(HEADHONE_AVAIL);
	}
#endif
//    OS_DbgMsg("<--snd_geode_ac97_read= [%X]=%X\n",reg, ret); 
	return ret;
}

/**
 * \ingroup linux GX layer
 * \brief
 * Callback for write access to the MIXER registers
 *  
 * \param  ac97_t *ac97
 *         unsigned short reg
 *         unsigned short val
 *
 * \return 0 if successful
 */
static void
snd_geode_ac97_write(ac97_t *ac97, unsigned short reg, unsigned short val)
{
	geode_t *pAmd;

//    OS_DbgMsg("-->snd_geode_ac97_write reg=%x, data=%X\n",reg,val); 

	pAmd = ac97->private_data;
	if (reg==MASTER_VOLUME)
	{   /*handle overflow*/
		if(val & 0x6000)
		{
			val= 0x1F00 | (val & 0x80FF);
		}
		if(val & 0x0060)
		{
			val= 0x001F | (val & 0xFF00);
		}
		snd_hw_CodecWrite(pGeode,reg,val); 
		snd_hw_CodecWrite(pGeode,LINE_LEV_OUT_VOL, val);
	}
	else if(reg>=MIC_VOLUME && reg<=PCM_OUT_VOL)
	{
		if(val & 0x2000)
		{
			val&=0x81FF;
			val|=0x1F00;
		}
		if(val & 0x0020)
		{
			val&=0xFF1F;
			val|=0x001F;
		} 
		if((val&0x1F1F)==0x1F1F)
		{ //mute if set to 0
			val |=MUTE_MASK;
		} 
		snd_hw_CodecWrite(pGeode,reg,val);
	}
	else if(reg==PCM_OUT_VOL)
	{
		if((val&0x1F1F)==0x1F1F)
		{ //mute if set to 0
			val |=MUTE_MASK;
		}  
		snd_hw_CodecWrite(pGeode,reg,val);
	}
	else if(reg==MIC_VOLUME)
	{
		if((val&0x001F)==0x001F)
		{ //mute if set to 0
			val |=MUTE_MASK;
		}  
		snd_hw_CodecWrite(pGeode,reg,val);
	}
	else
	{
		snd_hw_CodecWrite(pGeode,reg,val);
	}
	//OS_DbgMsg("<--snd_geode_ac97_write\n"); 
}



/**
 * \ingroup linux GX layer
 * \brief
 * Called when the driver has been loaded fronm the .probe callback
 * function. Allocate the memory for the DMA transfer.
 * The DMA buffer for playback is 4-times the actually used
 * size by the ALSA layer. This is necessary because we have always to
 * expand all samples to 2channels/16Bit values.
 *  
 * \param   geode_t *pAmd
 * \return  int 0 on success
 */
static int __devinit snd_geode_new_pcm(geode_t *pAmd)
{
	snd_pcm_t *pcm;
	int err;

	OS_DbgMsg("-->snd_geode_new_pcm\n"); 
	if ((err = snd_pcm_new(pAmd->card, "My Chip", 0, 1, 1,&pcm)) < 0)
	{
		OS_DbgMsg("<--snd_geode_new_pcm failed:()\n");    
		return err;
	}
	pcm->private_data = pAmd;
	strcpy(pcm->name, "Geode");
	pAmd->pcm = pcm;
	/* set operators ,depend on sapmle size it might be changed later*/
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&snd_geode_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
			&snd_geode_capture_ops);
	/* pre-allocation of buffers */
	err= snd_pcm_lib_preallocate_pages(pcm->streams[0].substream, SNDRV_DMA_TYPE_DEV,
					   snd_dma_pci_data(pAmd->pci), DMA_PLAYBACK_BUFFERSIZE, DMA_PLAYBACK_BUFFERSIZE);
	if(err!=0)
	{
		OS_DbgMsg("<--snd_geode_new_pcm failed: cannot allocate DMA mem for playback\n");
		return err;
	} 

	err= snd_pcm_lib_preallocate_pages(pcm->streams[1].substream, SNDRV_DMA_TYPE_DEV,
					   snd_dma_pci_data(pAmd->pci), DMA_CAPTURE_BUFFERSIZE, DMA_CAPTURE_BUFFERSIZE);
	if(err!=0)
	{
		OS_DbgMsg("<--snd_geode_new_pcm failed: cannot allocate DMA mem for capture\n");
		return err;
	}  
	OS_DbgMsg("<--snd_geode_new_pcm\n"); 
	return 0;
}


/**
 * \ingroup linux GX layer
 * \brief
 * Callback function from the module_init procedure.
 * Calls pci_module_init which registers the driver
 * in the system.
 * 
 * \return  int; error code, if OK then 0
 */
static int __init alsa_card_geode_init(void)
{
	int err;

	printk(version);
	OS_DbgMsg("-->alsa_card_geode_init\n");

	if ((err =  pci_module_init(&driver)) < 0) {
#ifdef MODULE
		OS_DbgMsg(KERN_ERR "<--Geode CS5535 soundcard not found or device busy\n");
#endif
		return err;
	}
	OS_DbgMsg("<--alsa_card_geode_init\n");
	return 0;
}

/**
 * \ingroup linux GX layer
 * \brief
 *  Callback function for the moduleexit procedure 
 *  Called when the driver will be unloaded.
 * 
 */
static void __exit alsa_card_geode_exit(void)
{    
	OS_DbgMsg("-->alsa_card_geode_exit\n");
	pci_unregister_driver(&driver);
	OS_DbgMsg("<--alsa_card_geode_exit\n");
	printk("Geode ALSA: unloaded\n");
}



/**
 * \ingroup linux GX layer
 * \brief
 * Called from the pcm_prepare callback   
 * when the device will be opened for playback or recording.
 * The function calculates the layout for the PRD table and 
 * configures the HW and all internal variables.
 *
 * \param PGEODEAUDIO pGeode,
 *        snd_pcm_substream_t *substream 
 *  
 * \return TRUE on success
 * 
 */
static unsigned char snd_geode_wave_open (PGEODEAUDIO pGeode,snd_pcm_substream_t *substream)
{
	unsigned long   InterruptFrequencyInDMABytes;
	unsigned long   channel;           
	unsigned long   SampleRate;        
	unsigned char   nChannels;         
	unsigned char   BitsPerSample;     
	unsigned long   InterruptFrequency;
	unsigned long   DMABufferSize;    
	unsigned char   DirectDMA;         
	unsigned long   physDmaAddr,virtDmaAddr;    
	snd_pcm_runtime_t *pRuntime;

  
	channel=(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? CHANNEL0_PLAYBACK : CHANNEL1_RECORD; 
	pRuntime=substream->runtime;
	SampleRate=pRuntime->rate;
	nChannels=pRuntime->channels;
	BitsPerSample=pRuntime->sample_bits;
	InterruptFrequency=pRuntime->period_size;
    
	DirectDMA=pRuntime->status->state;
	physDmaAddr=pRuntime->dma_addr;
	virtDmaAddr=(unsigned long)pRuntime->dma_area;
 
	OS_DbgMsg("-->snd_geode_wave_open:\n");
	OS_DbgMsg("Channel           =%d\n",channel);
	OS_DbgMsg("SampleRate        =%d\n",SampleRate);
	OS_DbgMsg("nChannels         =%d\n",nChannels);
	OS_DbgMsg("BitsPerSample     =%d\n",BitsPerSample);
	OS_DbgMsg("InterruptFrequency=0x%X\n",InterruptFrequency);
	OS_DbgMsg("pPysDmaAddr       =0x%X\n",physDmaAddr);   
	OS_DbgMsg("pVirtDmaAddr      =0x%X\n",virtDmaAddr);   
	OS_DbgMsg("pRuntime->dma_bytes   (bytes) =0x%X\n",pRuntime->dma_bytes);
	OS_DbgMsg("pRuntime->buffer_size (bytes) =0x%X\n",pRuntime->buffer_size);
	OS_DbgMsg("pRuntime->period_size (frames)  =0x%X\n",pRuntime->period_size);
	OS_DbgMsg("pRuntime->periods    =%d\n",pRuntime->periods);
	OS_DbgMsg("substream->dma_max=0x%X\n",substream->dma_max);
   

 
	/*Clear the Audio buffer*/
	memset(pRuntime->dma_area,0,substream->dma_max);
    

	/*Save the format*/ 
	pGeode->AudioChannel[channel].IndirectDMA.SampleRate    = SampleRate;
	pGeode->AudioChannel[channel].IndirectDMA.nChannels     = nChannels;
	pGeode->AudioChannel[channel].IndirectDMA.BitsPerSample = BitsPerSample;

	/*Set the data pointers to 0*/ 
	pGeode->AudioChannel[channel].fDirectDMA = DirectDMA;

	if ((BitsPerSample==0) || (nChannels==0))
	{
		OS_DbgMsg(":snd_geode_wave_open: Invalid BitsPerSample or nChannels\n");
		return FALSE;
	}
     
	DMABufferSize=pRuntime->dma_bytes;
	OS_DbgMsg("used DMABufferSize by Geode driver  =0x%X\n",DMABufferSize);
	pGeode->AudioChannel[channel].DMA_AllocInfo.PhysicalAddress=physDmaAddr;
	pGeode->AudioChannel[channel].DMA_AllocInfo.VirtualAddress=(unsigned char*)virtDmaAddr;

	InterruptFrequencyInDMABytes = frames_to_bytes(pRuntime,pRuntime->period_size);
    
	/*Allocate PRD */ 
	if(pGeode->AudioChannel[channel].fAllocated == TRUE)
	{ 
		snd_hw_FreePRD (pGeode,channel);
	}
	if(!snd_hw_AllocatePRD(pGeode, channel, DMABufferSize,InterruptFrequencyInDMABytes))
	{
		OS_DbgMsg("<--snd_geode_wave_open: failed AllocatePRD\n");
		return FALSE;
	}
	pGeode->AudioChannel[channel].fAllocated = TRUE;  
	snd_hw_InitPRD (pGeode, channel, pGeode->AudioChannel[channel].DMA_AllocInfo.PhysicalAddress,DMABufferSize,InterruptFrequencyInDMABytes);
	snd_hw_SetCodecRate(pGeode, channel, SampleRate);
	OS_DbgMsg("<--snd_geode_wave_open:\n");
	return TRUE;
}

/**
 * \ingroup linux GX layer
 * \brief
 * This function returns the current Interrupt 
 * from the DMA busmaster
 * 0x04 -   DMA0 interrupt (Wave Out)
 * 0x08 -   DMA1 interrupt (Wave In) 
 * 
 * \param      PGEODEAUDIO      pGeode 
 * \return     unsigned char  interrupt ID
 */
static unsigned char snd_hw_InterruptID (PGEODEAUDIO pGeode)
{
	volatile unsigned char    *TempInterruptID, ID;

    
	TempInterruptID=(unsigned char *) (pGeode->F3BAR0 + 0x12);
	TempInterruptID=(unsigned char *) OS_VirtualAddress(0x000004F0);
	ID=*TempInterruptID;

	return (ID);
}

/**
 * \ingroup linux GX layer
 * \brief
 *  Clear the IRQ in hardware
 * 
 * \param      PGEODEAUDIO      pGeode 
 */
static void snd_hw_ClearIRQ (PGEODEAUDIO pGeode)
{

	{
		snd_hw_ClearStat(pGeode, CHANNEL0_PLAYBACK);
		snd_hw_ClearStat(pGeode, CHANNEL1_RECORD);
	}
}



/**
 * \ingroup linux GX layer
 * \brief
 * Initializes the 5535 hardware.                     
 * It has to be called with the IRQ assigned by the OS
 * Reset and initialize the CODEC 
 *
 * \param      unsigned int   interrupt ID 
 * \return     PGEODEAUDIO      ;pointer to the HW structure
 */
static PGEODEAUDIO snd_hw_Initialize (unsigned int Irq, geode_t *pAmd)
{
	PGEODEAUDIO   pGeode;
    
	OS_DbgMsg("-->snd_hw_Initialize\n");

	pGeode=(PGEODEAUDIO) OS_AllocateMemory(sizeof(GEODEAUDIO));

	if (pGeode==NULL)
	{
		OS_DbgMsg("<--snd_hw_Initialize: failed\n");
		return FALSE;
	}

	memset (pGeode, 0, sizeof(GEODEAUDIO));
    
    
	if (pAmd->ioflags & IORESOURCE_IO) 
	{
		pGeode->fIOAccess=TRUE;
		pGeode->F3BAR0 = pAmd->iobase;
	}
	else
	{
		pGeode->fIOAccess=FALSE;     
		pGeode->F3BAR0=OS_Get_F3BAR0_Virt(pAmd->iobase);
	}
	if(!pGeode->F3BAR0 || !snd_hw_InitAudioRegs(pGeode))
	{  
		OS_DbgMsg("<--snd_hw_Initialize: failed\n");
		return FALSE;
	}
	/*------------------------------------------------------------
	  Interrupt-Related code
	  ------------------------------------------------------------*/

	/*As a default, we assume we are using DirectDMA*/

	pGeode->AudioChannel[CHANNEL0_PLAYBACK].fDirectDMA = TRUE;
	pGeode->AudioChannel[CHANNEL1_RECORD].fDirectDMA   = TRUE;
	printk("-------------------------------------\n");
	snd_hw_SetupIRQ (pGeode, (unsigned long) Irq);

    
	/*   Save the selected IRQ*/ 
	pGeode->SelectedIRQ=Irq;

	/*  Prepare the bit mask (To clear Interrupts)*/ 
	pGeode->uIRQMask=1;
	pGeode->uIRQMask <<= pGeode->SelectedIRQ;
	pGeode->uIRQMask=~pGeode->uIRQMask;

	/*------------------------------------------------------------
	  End of Interrupt-Related code
	  ------------------------------------------------------------*/
        
	pGeode->AudioChannel[CHANNEL0_PLAYBACK].fAllocated  = FALSE;
	pGeode->AudioChannel[CHANNEL1_RECORD].fAllocated    = FALSE;

	OS_DbgMsg("<--snd_hw_Initialize\n");
	return pGeode;
}


/**
 * \ingroup linux GX layer
 * \brief
 * Frees the PRD allocated by snd_hw_AllocatePRD
 *
 * \param      unsigned long Channel ,the channel(0,1) 
 *             PGEODEAUDIO              pGeode 
 */
static void snd_hw_FreePRD (PGEODEAUDIO pGeode, unsigned long Channel)
{
	OS_DbgMsg("-->snd_hw_FreePRD\n");
	snd_hw_FreeAlignedDMAMemory (pGeode, &pGeode->AudioChannel[Channel].PRD_AllocInfo);   
	pGeode->AudioChannel[Channel].PRDTable = NULL;
	OS_DbgMsg("<--snd_hw_FreePRD\n");
}


/**
 * \ingroup linux GX layer
 * \brief
 * Setup the PRD entries of the specified channel with EOP to generate 
 * interrupts based on the Sample rate and the InterruptFrequency      
 * value expressed in Miliseconds.                                     
 *
 * \param  PGEODEAUDIO      pGeode,            
 *         unsigned  long Channel,           
 *         unsigned  long SampleRate,        
 *         unsigned  long InterruptFrequency,
 *         unsigned  char FrequencyType         
 *             
 */

/**
 * \ingroup linux GX layer
 * \brief
 * Allocates the PRD table for a DMA buffer of DMABufferSize bytes for the specified Channel  
 * 
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 unsigned long Channel,       ;0=playback,1=Capture
 unsigned long DMABufferSize  ;size of the DMA buffer
 * \return  unsigned char                ;TRUE if successful else FALSE
 */
static unsigned char snd_hw_AllocatePRD (PGEODEAUDIO pGeode, 
                                         unsigned long Channel, 
                                         unsigned long DMABufferSize,
                                         unsigned long periodSizeInBytes)
{
	unsigned long   PRDsToAllocate;

	OS_DbgMsg("-->snd_hw_AllocatePRD\n");
	PRDsToAllocate=(DMABufferSize/periodSizeInBytes+1);
	if(DMABufferSize%periodSizeInBytes)
	{
		PRDsToAllocate++;
	}
	if(!snd_hw_AllocateAlignedDMAMemory(pGeode, 
					    (PRDsToAllocate * sizeof(PRD_ENTRY)), 
					    &pGeode->AudioChannel[Channel].PRD_AllocInfo))
	{
		OS_DbgMsg("Could not allocate PRD\n");
		return FALSE;
	}

	pGeode->AudioChannel[Channel].PRDTable = (PPRD_ENTRY) pGeode->AudioChannel[Channel].PRD_AllocInfo.VirtualAddress;

	pGeode->AudioChannel[Channel].DMA_AllocInfo.Size    = DMABufferSize;
	pGeode->AudioChannel[Channel].PRDEntriesAllocated   = PRDsToAllocate;
	OS_DbgMsg("<--snd_hw_AllocatePRD\n");
	return TRUE;
}


/**
 * \ingroup linux GX layer
 * \brief
 * Starts the DMA transfer for the channel specified by Channel.  
 * 
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 *          unsigned long Channel,       ;0=playback,1=Capture
 */
static void snd_hw_StartDMA (PGEODEAUDIO pGeode, unsigned long Channel)
{
	if(pGeode->AudioChannel[Channel].Running)
		return;
	disable_hlt();
        /*Sets the PRD physical address*/
	*((unsigned long *) pGeode->AudioChannel[Channel].AudioBusMaster.PRDTableAddress) = pGeode->AudioChannel[Channel].PRD_AllocInfo.PhysicalAddress;

	//OS_DbgMsg("snd_hw_StartDMA: *(%.8X)=%.8X\n",pGeode->AudioChannel[Channel].AudioBusMaster.PRDTableAddress,pGeode->AudioChannel[Channel].PRD_AllocInfo.PhysicalAddress);
	snd_hw_ResumeDMA (pGeode, Channel);
	pGeode->AudioChannel[Channel].Running=TRUE;
}



/**
 * \ingroup linux GX layer
 * \brief
 * Resumes the DMA channel controlled by the specified PRD table 
 * 
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 *          unsigned long Channel,       ;0=playback,1=Capture
 */
static void snd_hw_ResumeDMA (PGEODEAUDIO pGeode, unsigned long Channel)
{   
	OS_DbgMsg("-->snd_hw_ResumeDMA\n");

	*((unsigned char *) pGeode->AudioChannel[Channel].AudioBusMaster.CommandRegister) = pGeode->AudioChannel[Channel].AudioBusMaster.DirectionBit | ENABLE_BUSMASTER;

	OS_DbgMsg("<--snd_hw_ResumeDMA: *(%.8X)=%.8X\n",pGeode->AudioChannel[Channel].AudioBusMaster.CommandRegister,(pGeode->AudioChannel[Channel].AudioBusMaster.DirectionBit & 0xFC) | ENABLE_BUSMASTER);
}

/**
 * \ingroup linux GX layer
 * \brief
 * Stops the DMA channel controlled by the specified PRD table.
 * 
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 *          unsigned long Channel,       ;0=playback,1=Capture
 */
static void snd_hw_StopDMA (PGEODEAUDIO pGeode, unsigned long Channel)
{   
	/*if DMA is not running, don't do anything and return.*/ 
	OS_DbgMsg("-->snd_hw_StopDMA\n");
	if (!pGeode->AudioChannel[Channel].Running)
	{
		return;
	}
	if (pGeode->AudioChannel[Channel].PRDTable)
	{
		int i;
		for (i=0; i<(pGeode->AudioChannel[Channel].PRDEntriesAllocated-1); ++i)
			pGeode->AudioChannel[Channel].PRDTable[i].SizeFlags |= PRD_EOT_BIT;
		
		OS_Sleep(50);
		       
		//   Turn OFF DMA
		*((unsigned char *) pGeode->AudioChannel[Channel].AudioBusMaster.CommandRegister) = STOP_BUSMASTER;	
	} 
	pGeode->AudioChannel[Channel].Running=FALSE;
	OS_Sleep(5);
	enable_hlt();
	OS_DbgMsg("<--snd_hw_StopDMA\n");
}


/**
 * \ingroup linux GX layer
 * \brief
 * Clear the status bit of the respective Channel
 * 
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 *          unsigned long Channel,       ;0=playback,1=Capture
 * \return  unsigned char
 */
static unsigned char snd_hw_ClearStat(PGEODEAUDIO pGeode, unsigned long Channel)
{
	volatile unsigned char  status;      /*Volatile to force read-to-clear.*/ 

	/*Read to clear*/ 
	status = *((unsigned char *) pGeode->AudioChannel[Channel].AudioBusMaster.SMI_StatusRegister);

	return status;
}


void DURAUDIO_WaitFrameAndWrite (PGEODEAUDIO pGeode, unsigned long val)
{
        unsigned long    cmd_val, timeout;
        
        timeout = 30000;
        

        {
                while ((*((unsigned long *)( pGeode->F3BAR0 + CODEC_CMD_REG )) & CODEC_CMD_VALID) && (--timeout));

                cmd_val = val & CODEC_COMMAND_MASK;
                *((unsigned long *)( pGeode->F3BAR0 + CODEC_CMD_REG )) = cmd_val;
        }
}


/**
 * \ingroup linux GX layer
 * \brief
 * Reads the specified codec register
 * 
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 *          unsigned char CodecRegister
 * \return  unsigned short               ;the read value 
 */
static unsigned short snd_hw_CodecRead ( PGEODEAUDIO pGeode, unsigned char CodecRegister )
{
	unsigned long CodecRegister_data = 0;
	unsigned long timeout=10;
	volatile unsigned long val=0;
    
	CodecRegister_data  = ((unsigned long)CodecRegister)<<24;
	CodecRegister_data |= 0x80000000;   /* High-bit set (p.106) is a CODEC reg READ.*/

    
	do 
	{
		int i;
		DURAUDIO_WaitFrameAndWrite (pGeode, CodecRegister_data);

		//
		//   Wait for Status Tag and Status Valid bits in the CODEC Status Register
		//
		for( i=0; i<=3000; i++) 
		{
			val = *((unsigned long *)( pGeode->F3BAR0 + CODEC_STATUS_REG ));
		} 
	} while ((((unsigned long) (0xFF & CodecRegister)) != ((0xFF000000 & val)>>24)) && (--timeout) );
	// Check if the register read is the one we want


	return( (unsigned short)val );
}



/* \ingroup linux GX layer
 * \brief
 * Wait for completion of a clear or set of a bit mask in a codec register
 * 
 * 
 * \param   PGEODEAUDIO     pGeode,      
 unsigned long Offset,       ; register in which the bit is located  
 unsigned long Bit,          ; bitmask
 unsigned char Operation,    ; /CLEAR or SET
 unsigned long timeout,      ;timeout after which the operation will be cancelled
 unsigned long *pReturnValue ;the read bit value
 *          
 * \return  unsigned char               ;success state TURE/FALSE 
 */
static unsigned char snd_hw_WaitForBit(PGEODEAUDIO     pGeode, 
                                       unsigned long Offset, 
                                       unsigned long Bit, 
                                       unsigned char Operation,
                                       unsigned long timeout, 
                                       unsigned long *pReturnValue)
{
	volatile unsigned long  Temp;

        Temp = *((unsigned long *)( pGeode->F3BAR0 + Offset ));

	while (timeout)
	{
		if (Operation==CLEAR)
		{
			if (!(Temp & Bit))
				break;
		}
		else
		{
			if (Temp & Bit)
				break;
		}
		/*If the Bit is not clear yet, we wait for 1 milisecond and try again*/
		OS_Sleep(1);

		Temp = *((unsigned long *)( pGeode->F3BAR0 + Offset ));

		timeout--;
	};

	if (pReturnValue)
	{
		*pReturnValue=Temp;
	} 
	if (!timeout)
	{
		return FALSE;
	}
	return TRUE;
}


/**
 * \ingroup linux GX layer
 * \brief
 * Writes data to the CODEC
 * 
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 *          unsigned char CodecRegister
 * \return  unsigned short               ;CodecData 
 */
static void snd_hw_CodecWrite( PGEODEAUDIO pGeode, unsigned char CodecRegister, unsigned short CodecData  )
{
	unsigned long CodecRegister_data;
	unsigned long Temp, timeout;
    
	CodecRegister_data = ((unsigned long) CodecRegister)<<24; 
	CodecRegister_data |= (unsigned long) CodecData;
	CodecRegister_data &= CODEC_COMMAND_MASK;
    
        {
		int i,val;
                //
                //      Wait for the right time to write
                //
                DURAUDIO_WaitFrameAndWrite( pGeode, CodecRegister_data );
                
                //
                //   Wait for Status Tag and Status Valid bits in the CODEC Status Register
                //
                for( i=0; i<=30000; i++) 
                {
			val = *((unsigned long *)( pGeode->F3BAR0 + CODEC_STATUS_REG ));
                        
                        if( ( val & CODEC_STATUS_VALID ) || ( val & CODEC_STATUS_NEW )) 
                                break;
                } 
                
                // This prevents multiple volume writes to a codec register 
                DURAUDIO_WaitFrameAndWrite( pGeode, 0x80000000 | CodecRegister_data );
        }

}

/**
 * \ingroup linux GX layer
 * \brief
 * Initializes the PRD for the specified channel
 * 
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 *          unsigned long Channel        ;channel 0,1
 *          unsigned long DMAPhys        ;physical address of DMA buffer 
 */
static void snd_hw_InitPRD (PGEODEAUDIO   pGeode, 
                            unsigned long Channel, 
                            unsigned long DMAPhys,
                            unsigned long dmasize,
                            unsigned long InterruptFrequencyInDMABytes) 
{
	//
	//    Fill the PRD Entries with the right sizes and flags=0
	//

	unsigned int    i,rest;
	unsigned long enddAddr;

	OS_DbgMsg("-->snd_hw_InitPRD\n");
	enddAddr=DMAPhys+dmasize;
	OS_DbgMsg("dmasize=0x%X, enddAddr=0x%X\n",dmasize,enddAddr); 
	rest=dmasize%InterruptFrequencyInDMABytes;
	OS_DbgMsg("rest=%X\n",rest);
	for (i=0; i<(pGeode->AudioChannel[Channel].PRDEntriesAllocated-1); ++i)
	{
		pGeode->AudioChannel[Channel].PRDTable[i].ulPhysAddr = DMAPhys + (i * InterruptFrequencyInDMABytes);                
		if((pGeode->AudioChannel[Channel].PRDTable[i].ulPhysAddr+InterruptFrequencyInDMABytes)<=enddAddr)
		{ 
			pGeode->AudioChannel[Channel].PRDTable[i].SizeFlags  = InterruptFrequencyInDMABytes | PRD_EOP_BIT;
		}
		else           
		{
			pGeode->AudioChannel[Channel].PRDTable[i].SizeFlags  =rest;
			OS_DbgMsg("adjust len of last prd entry to %X,\n",pGeode->AudioChannel[Channel].PRDTable[i].SizeFlags);
		}
	}
	/*  And, last, put the JMP to the top*/
	pGeode->AudioChannel[Channel].PRDTable[pGeode->AudioChannel[Channel].PRDEntriesAllocated-1].ulPhysAddr = pGeode->AudioChannel[Channel].PRD_AllocInfo.PhysicalAddress;
	pGeode->AudioChannel[Channel].PRDTable[pGeode->AudioChannel[Channel].PRDEntriesAllocated-1].SizeFlags  = PRD_JMP_BIT;  

	OS_DbgMsg("PRDTable:\n");
	for (i=0; i<(pGeode->AudioChannel[Channel].PRDEntriesAllocated); ++i)
	{
		OS_DbgMsg("snd_hw_InitPRD[%.3X] %p,address: %.8X, flags: %.8X\n",
			  i, &pGeode->AudioChannel[Channel].PRDTable[i], 
			  pGeode->AudioChannel[Channel].PRDTable[i].ulPhysAddr, pGeode->AudioChannel[Channel].PRDTable[i].SizeFlags);
	}
	OS_DbgMsg("<--snd_hw_InitPRD\n");
}

/**
 * \ingroup linux GX layer
 * \brief
 * Set the rate in the codec for the given channel
 * 
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 *          unsigned long Channel        ;channel 0,1
 *          unsigned long SampleRate     ;sample rate e.g. 44100
 */
static void snd_hw_SetCodecRate(PGEODEAUDIO pGeode, unsigned long Channel, unsigned long SampleRate)
{
	unsigned short val;

	OS_DbgMsg ("Rate: %d\n", SampleRate);

	pGeode->AudioChannel[Channel].SampleRate=SampleRate;
	/*If Double-Rate is supported (Bit 2 on register 28h)...*/
	val=snd_hw_CodecRead(pGeode, EXTENDED_AUDIO_ID);

	if (val & 0x02)
	{
		OS_DbgMsg ("Codec supports Double rate.\n");
		val=snd_hw_CodecRead(pGeode, EXT_AUDIO_CTRL_STAT);

		if (SampleRate>48000)
		{
			snd_hw_CodecWrite(pGeode, EXT_AUDIO_CTRL_STAT, (unsigned short) (val|0x0002)|1); // GA VAR
			SampleRate/=2;
		}
		else
			snd_hw_CodecWrite(pGeode, EXT_AUDIO_CTRL_STAT, (unsigned short) (val&0xFFFD)|1);

	}
	if (pGeode->fAD1819A) 
	{
		OS_DbgMsg ("AD1819...\n");
		if (Channel)
			snd_hw_CodecWrite(pGeode, AD1819A_PCM_SR1,(unsigned short) SampleRate);
		else
			snd_hw_CodecWrite(pGeode, AD1819A_PCM_SR0,(unsigned short) SampleRate);
	} 
	else 
	{
		if (Channel)
			snd_hw_CodecWrite(pGeode, PCM_LR_ADC_RATE,   (unsigned short) SampleRate);
		else
			snd_hw_CodecWrite(pGeode, PCM_FRONT_DAC_RATE,(unsigned short) SampleRate);
	}
}

/**
 * \ingroup linux GX layer
 * \brief
 * Initializes the codec registers to it's default
 * values after loading the driver
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 *        
 * \return  unsigned long                ;
 */
static unsigned long snd_hw_InitAudioRegs(PGEODEAUDIO pGeode)
{
	OS_DbgMsg("-->snd_hw_InitAudioRegs\n");

	pGeode->fCS553x=FALSE; //TRUE;

	/*Initialize the Audio BusMaster register pointers*/
	OS_DbgMsg("snd_hw_InitAudioRegs: Audio device base address=0x%.8X\n", pGeode->F3BAR0);

	pGeode->AudioChannel[CHANNEL0_PLAYBACK].AudioBusMaster.CommandRegister      = pGeode->F3BAR0+0x20;
	pGeode->AudioChannel[CHANNEL0_PLAYBACK].AudioBusMaster.PRDTableAddress      = pGeode->F3BAR0+0x24;
	pGeode->AudioChannel[CHANNEL0_PLAYBACK].AudioBusMaster.DMAPointer           = pGeode->F3BAR0+0x60;
	pGeode->AudioChannel[CHANNEL0_PLAYBACK].AudioBusMaster.DirectionBit         = PCI_READS;
	pGeode->AudioChannel[CHANNEL0_PLAYBACK].Running=FALSE;

	pGeode->AudioChannel[CHANNEL1_RECORD].AudioBusMaster.CommandRegister        = pGeode->F3BAR0+0x28;
	pGeode->AudioChannel[CHANNEL1_RECORD].AudioBusMaster.PRDTableAddress        = pGeode->F3BAR0+0x2C;
	pGeode->AudioChannel[CHANNEL1_RECORD].AudioBusMaster.DMAPointer             = pGeode->F3BAR0+0x64;
	pGeode->AudioChannel[CHANNEL1_RECORD].AudioBusMaster.DirectionBit           = PCI_WRITES;
	pGeode->AudioChannel[CHANNEL1_RECORD].Running=FALSE;

	/*-----------------------------------------------------------------------------------------------
	  Interrupt-Related code
	  -----------------------------------------------------------------------------------------------*/

	pGeode->IRQControlRegister                                                  = pGeode->F3BAR0+0x1C;
	pGeode->InternalIRQEnableRegister                                           = pGeode->F3BAR0+0x1A;
	pGeode->AudioChannel[CHANNEL0_PLAYBACK].AudioBusMaster.SMI_StatusRegister   = pGeode->F3BAR0+0x21;
	pGeode->AudioChannel[CHANNEL1_RECORD].AudioBusMaster.SMI_StatusRegister     = pGeode->F3BAR0+0x29;

	/*-----------------------------------------------------------------------------------------------
	  End of Interrupt-Related code
	  -----------------------------------------------------------------------------------------------*/

	/*CODEC - RESET and volumes initalization.*/ 
	if (pGeode->fCS553x)
	{
		/*Set the Warm RESET and CODEC_COMMAND_NEW bits.*/
		OS_WritePortULong ( (unsigned short) (pGeode->F3BAR0 + CODEC_CONTROL_REG_5535), 0x00030000 );
		if (!snd_hw_WaitForBit (pGeode, CODEC_STATUS_REG_5535, BIT_CODEC_READY, SET, 400, NULL))
		{
			OS_DbgMsg("Primary Codec NOT Ready...Aborting\n");
//            return FALSE;
		}
	}

	/*Check which codec is being used */
	if (snd_hw_CodecRead(pGeode, AD1819A_VENDORID1) == 0x4144 &&
	    snd_hw_CodecRead(pGeode, AD1819A_VENDORID2) == 0x5303) 
	{
		pGeode->fAD1819A = TRUE;
		/*  Enable non-48kHz sample rates. */
		snd_hw_CodecWrite (pGeode, AD1819A_SER_CONF, (unsigned short) (snd_hw_CodecRead(pGeode, AD1819A_SER_CONF>>8) | AD1819A_SER_CONF_DRQEN));
	}
	else
	{
		pGeode->fAD1819A = FALSE;
		snd_hw_CodecWrite (pGeode, EXT_AUDIO_CTRL_STAT, (unsigned short) (snd_hw_CodecRead(pGeode,EXT_AUDIO_CTRL_STAT) | 0x0001)); 
		/* set the VRA bit to ON*/
	}

	/* set default volume*/
	snd_hw_CodecWrite(pGeode, MASTER_VOLUME,       0x0F0F);
	snd_hw_CodecWrite(pGeode, PCM_OUT_VOL,         0x0F0F);
	snd_hw_CodecWrite(pGeode, PC_BEEP_VOLUME,      0x0000);
	snd_hw_CodecWrite(pGeode, PHONE_VOLUME,        0x8000);
	snd_hw_CodecWrite(pGeode, MIC_VOLUME,          0x8048);
	snd_hw_CodecWrite(pGeode, LINE_IN_VOLUME,      0x0808);
	snd_hw_CodecWrite(pGeode, CD_VOLUME,           0x8000);
	snd_hw_CodecWrite(pGeode, VIDEO_VOLUME,        0x8000);
	snd_hw_CodecWrite(pGeode, TV_VOLUME,           0x8000);
	snd_hw_CodecWrite(pGeode, RECORD_SELECT,       0x0000);
	snd_hw_CodecWrite(pGeode, RECORD_GAIN,         0x0a0a);
	snd_hw_CodecWrite(pGeode, GENERAL_PURPOSE,     0x0200);
	snd_hw_CodecWrite(pGeode, MASTER_VOLUME_MONO,  0x0000);

	/*Set all the power state bits to 0 (Reg 26h)*/
	snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, 0x0000);
	pGeode->CurrentPowerState=GEODEAUDIO_D0;
	OS_DbgMsg("<--snd_hw_InitAudioRegs\n");
	return TRUE;
}


/**
 * \ingroup linux GX layer
 * \brief
 * Programs the IRQ
 * 
 * 
 * \param   PGEODEAUDIO pGeode,            ;hw structure  
 *          unsigned long   Irq
 */
static void snd_hw_SetupIRQ (PGEODEAUDIO pGeode, unsigned long Irq)
{	
	if (!pGeode->fCS553x)
	{
		unsigned long   SetIRQData;

		/*If it is a CS5530, we have to configure VSA...*/
		/* VSA2 IRQ config method*/
		OS_WritePortUShort( 0xAC1C, 0xFC53 );                    /*Unlock virtual registers*/ 
		OS_WritePortUShort( 0xAC1C, 0x108 );                     /*ID the audio config virtual regs*/ 
		OS_WritePortUShort( 0xAC1E, (unsigned short) Irq );      /*Set the value*/ 

		/*VSA1 IRQ config method*/ 
        
		/*Get the Irq and OR it with the command*/ 
		SetIRQData = ((Irq<<16) | 0xA00A);

		OS_WritePortULong((unsigned short) PCI_CADDR, 0x800090D0);  /*Set the address*/ 
		OS_WritePortULong((unsigned short) PCI_CDATA, SetIRQData);  /*Send the command*/ 

		/*Set the InterruptID address for the CS5530*/ 
		pGeode->pInterruptID = (unsigned char *) OS_VirtualAddress(0x000004F0);
	}
}


/**
 * \ingroup linux GX layer
 * \brief
 * Returns the allocated memory from the PRD table
 * 
 * 
 * \param   PGEODEAUDIO   pGeode    -   Pointer to the DURAUDIO Structure                               
 *          PALLOC_INFO pAllocInfo  -   Pointer to a ALLOC_INFO strcuture that contains the information 
 *                                      for the space allocated (Physical, Virtual and Size).           
 *          
 */
static void snd_hw_FreeAlignedDMAMemory ( PGEODEAUDIO pGeode, PALLOC_INFO  pAllocInfo )
{
	if(pAllocInfo->VirtualAddress)
	{
		OS_FreeDMAMemory ( pGeode->pAdapterObject, (void *) pAllocInfo->OriginalVirtualAddress, pAllocInfo->OriginalPhysicalAddress, pAllocInfo->OriginalSize);
		pAllocInfo->VirtualAddress = NULL;
	}
}

/**
 * \ingroup linux GX layer
 * \brief
 * Allocates a 32-bytes aligned DMA memory block.                                   
 * Our hardware needs to have DMA and PRD spaces 32-bytes aligned to work correctly.
 *                                                                                  
 * 1. We save the original Physical and virtual addresses in the AllocInfo structure
 *    to be able to release the memory afterwards.                                  
 *                                                                                  
 * 2. We allocate 256 extra bytes to allow the shift.                               
 *                                                                                  
 * 3. If the physical adress is not aligned, keep adding to the physical            
 *    and virtual address until it aligns to 32-byte boundary.                      
 * 
 * 
 * \param   PGEODEAUDIO   pGeode      -   Pointer to the DURAUDIO Structure 
 *          unsigned long    Size,  -   size of memory buffer to allocate                          
 *          PALLOC_INFO pAllocInfo  -   Pointer to a ALLOC_INFO strcuture that contains the information 
 *                                      for the space allocated (Physical, Virtual and Size).           
 * \return   unsigned char       FALSE if failed to allocate, TRUE if succeeds
 * 
 */
static unsigned char snd_hw_AllocateAlignedDMAMemory (PGEODEAUDIO      pGeode,
						      unsigned long  Size,
						      PALLOC_INFO    pAllocInfo)

{
	pAllocInfo->Size         = Size;
	pAllocInfo->OriginalSize = Size + 256;

	pAllocInfo->OriginalVirtualAddress = (void *) OS_AllocateDMAMemory ( pGeode->pAdapterObject, pAllocInfo->OriginalSize, &pAllocInfo->OriginalPhysicalAddress);

	if (!pAllocInfo->OriginalVirtualAddress)
	{
		OS_DbgMsg("Could not allocate DMA Memory.  Size:%d\n", Size);
		return FALSE;
	}

	pAllocInfo->VirtualAddress  = pAllocInfo->OriginalVirtualAddress;
	pAllocInfo->PhysicalAddress = pAllocInfo->OriginalPhysicalAddress;

	if(pAllocInfo->PhysicalAddress & 0x0000001F)
	{
		OS_DbgMsg("DMA Memory is not 32-byte aligned! : 0x%08X\n", pAllocInfo->PhysicalAddress);
		while (pAllocInfo->PhysicalAddress & 0x0000001F)
		{
			pAllocInfo->VirtualAddress++;
			pAllocInfo->PhysicalAddress++;       

		}
		OS_DbgMsg("Fixed to: 0x%08X.\n", pAllocInfo->PhysicalAddress);
	}
	return TRUE;
}

#ifdef CONFIG_PM
/**
 * \ingroup linux GX layer
 * \brief
 * Callback when the system goes to suspend. Set the hardware 
 * in powersave mode. The set State is always D3!
 * 
 * \param  struct pci_dev *dev,
 *                u32     state  
 * \return always success
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9))
static int snd_geode_suspend(snd_card_t *card, unsigned int state)
{
	geode_t *pAmd;

	OS_DbgMsg("-->snd_geode_suspend(>=2.6.9),state=%d\n",state);
	pAmd = card->private_data;

#else

	static int snd_geode_suspend(struct pci_dev *dev, u32 state)
		{
			geode_t *pAmd;
			snd_card_t *card;

			OS_DbgMsg("-->snd_geode_suspend(<2.6.9),state=%d\n",state);
			card = pci_get_drvdata(dev);
			pAmd= card->private_data;
#endif

			if(pGeode->CurrentPowerState != GEODEAUDIO_D0)
			{
				OS_DbgMsg("<--snd_geode_suspend - already in power down!\n");
				return 0;  
			}
			snd_pcm_suspend_all(pAmd->pcm);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9))
			snd_ac97_suspend(pAmd->ac97);
#endif
			snd_geode_SetPowerState(pGeode, GEODEAUDIO_D3); 
			pci_disable_device(pAmd->pci);
			snd_power_change_state(card, SNDRV_CTL_POWER_D3hot);
			OS_DbgMsg("<--snd_geode_suspend\n");
			return 0;
		}

/**
 * \ingroup linux GX layer
 * \brief
 * Callback when the system returns from suspend. Set the hardware 
 * in active mode.
 * 
 * \param  struct pci_dev *dev,
 *                
 * \return always success
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9))
	static int snd_geode_resume(snd_card_t *card, unsigned int state)
		{
			geode_t *pAmd;

			OS_DbgMsg("-->snd_geode_resume(>=2,6,9),state=%d\n",state);
			pAmd = card->private_data;

#else

			static int snd_geode_resume(struct pci_dev *dev)
				{
					geode_t *pAmd;
					snd_card_t *card;

					OS_DbgMsg("-->snd_geode_resume(<2,6,9)\n");
					card = pci_get_drvdata(dev);
					pAmd= card->private_data;

#endif


					if(pci_enable_device(pAmd->pci))
					{
						OS_DbgMsg("<--snd_geode_resume failed\n");
						return (-EIO);
					}
					pci_set_master(pAmd->pci);
    

					if(snd_geode_SetPowerState(pGeode, GEODEAUDIO_D0))
					{
						OS_DbgMsg("<--snd_geode_resume failed\n");
						return (-EIO); 
					}
					snd_ac97_resume(pAmd->ac97);    
					snd_power_change_state(card, SNDRV_CTL_POWER_D0);
					OS_DbgMsg("<--snd_geode_resume\n");
					return 0;
				}

/**
 * \ingroup linux GX layer
 * \brief
 * Function to set the Hardware into the desired power state.
 * 
 * \param  struct pci_dev *dev,
 *                
 * \return 0 on success
 */
			static unsigned long snd_geode_SetPowerState(PGEODEAUDIO pGeode, GEODEAUDIO_POWER_STATE NewPowerState)
				{

					if (NewPowerState==pGeode->CurrentPowerState)
						return 0;
					OS_DbgMsg("-->snd_geode_SetPowerState, state=%d\n",NewPowerState);
					switch (NewPowerState)
					{
					case GEODEAUDIO_D0:
					{ 
						switch (pGeode->CurrentPowerState)
						{
						case GEODEAUDIO_D0:
							break;

						case GEODEAUDIO_D1:
						{ 
							OS_DbgMsg("Coming back from D1.\n");

							if (pGeode->fCS553x) 
							{
								/*We are coming back from D1 on a 5535
								  so, we need to do a AC-Link Warm Reset*/
								OS_WritePortULong ( (unsigned short) (pGeode->F3BAR0 + CODEC_CONTROL_REG_5535), BIT_5535_ACLINK_WARM_RESET );
								if (!snd_hw_WaitForBit (pGeode, CODEC_STATUS_REG_5535, BIT_CODEC_READY, SET, 400, NULL))
								{
									OS_DbgMsg("Primary Codec NOT Ready...Aborting\n");
									return (-EINVAL);
								} 
								/*Reset the codec*/ 
								snd_hw_ResetCodec  (pGeode);
								OS_Sleep (50);
							} 

							if (!snd_hw_CodecFullOn (pGeode))
							{
								OS_DbgMsg("ERROR: CODEC did not power up!\n");
								OS_DbgMsg("<--snd_geode_SetPowerState failed\n");
								return  (-EINVAL);
							}
							break;
						}

						case GEODEAUDIO_D2:
						case GEODEAUDIO_D3: 
						{ 
							OS_DbgMsg("Coming back from D2/D3.\n");

							if (pGeode->fCS553x) 
							{
								/*We are coming back from D2 or D3 on a 5535
								  so, we need to do a AC-Link Warm Reset*/
								OS_DbgMsg("AC-Link Warm Reset\n");
								OS_WritePortULong ( (unsigned short) (pGeode->F3BAR0 + CODEC_CONTROL_REG_5535), BIT_5535_ACLINK_WARM_RESET );
								if (!snd_hw_WaitForBit (pGeode, CODEC_STATUS_REG_5535, BIT_CODEC_READY, SET, 400, NULL))
								{
									OS_DbgMsg("Primary Codec NOT Ready...Aborting\n");
									return (-EINVAL);
								} 
								/*Reset the codec*/ 
								snd_hw_ResetCodec(pGeode);
								OS_Sleep (50);
							} 

							if (!snd_hw_CodecFullOn (pGeode))
							{
								OS_DbgMsg("ERROR: CODEC did not power up!\n");
								OS_DbgMsg("<--snd_geode_SetPowerState failed\n");
								return  (-EINVAL);
							}
							/*If we are coming from D2, D3 or D4, we need to 
							  restore the controller and codec data.*/
							snd_hw_RestoreAudioContext (pGeode);
							break;
						} 

						case GEODEAUDIO_D4:
						{
							OS_DbgMsg("Coming back from D4.\n");
							/*If we are coming from D4 we have to initialize everything again 
							  because we would be coming back from hibernation or total shutdown*/
							snd_hw_SetupIRQ (pGeode, pGeode->SelectedIRQ);
							snd_hw_InitAudioRegs (pGeode);
							snd_hw_RestoreAudioContext (pGeode);
							break;
						}
						}
						pGeode->CurrentPowerState = GEODEAUDIO_D0; 
						break;
					}

					case GEODEAUDIO_D1:
					{
						OS_DbgMsg("In D1.\n");

						if ((pGeode->AudioChannel[CHANNEL0_PLAYBACK].Running == FALSE) &&
						    (pGeode->AudioChannel[CHANNEL1_RECORD].Running == FALSE))
						{
							if (pGeode->fCS553x)
							{
								/*Powerdown ADC*/ 
								snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWR_PR0);
								if (!snd_hw_CheckCodecPowerBit (pGeode, GEODEAUDIO_CODEC_POWER_ADC, CLEAR))
								{
									OS_DbgMsg("ERROR: CODEC ADC bit not cleared!\n");
									OS_DbgMsg("<--snd_geode_SetPowerState failed\n");
									return  (-EINVAL);
								}
								/*Powerdown DAC*/ 
								snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWR_DIGOFF);
								if (!snd_hw_CheckCodecPowerBit (pGeode, GEODEAUDIO_CODEC_POWER_DAC, CLEAR))
								{
									OS_DbgMsg("ERROR: CODEC DAC bit not cleared!\n");
									OS_DbgMsg("<--snd_geode_SetPowerState failed\n");
									return  (-EINVAL);
								}

								/*Powerdown Analog*/ 
								snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWR_DIGOFF|GEODEAUDIO_PWR_ANLOFF);
                        
								/*5535 allows to powerdown AC-link
								  PLUS Powerdown AC_Link 
								  Powerdown Amp. causes unusual "pop" sound. This issue
								  will be further investigated in Castle. Currently we don't
								  power down amplifier in D1. */
								snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWR_D1_HAWK);
								/*Set the AC-Link ShutDown bit*/
								OS_WritePortULong((unsigned short) (pGeode->F3BAR0 + CODEC_CONTROL_REG_5535), BIT_5535_ACLINK_SHUTDOWN);
							}
							else
							{
								/*Power down ADC/DAC and ANL sections in GX1/SCxx00
								  Amp. powerdown causes "pop" sound
								  ADC/DAC powerdown only causes loud static sound, but no static sound
								  is heard when ANL and ADC/DAC power down simultaneously*/
								snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWR_DIGOFF | GEODEAUDIO_PWR_ANLOFF);
							}
							/*We went to D1 */
							pGeode->CurrentPowerState = GEODEAUDIO_D1;
						}
						else
						{
							/*If we are playing, we don't do anything and stay in D0*/
							pGeode->CurrentPowerState = GEODEAUDIO_D0;  
						}

						break;
					}

					case GEODEAUDIO_D2:
					case GEODEAUDIO_D3:
					case GEODEAUDIO_D4:
					{
						OS_DbgMsg("In D2/D3/D4.\n");
						/*We are going into D2, D3 or D4.
						  In this mode the power could be removed or reduced to a point
						  where the AC97 controller looses information, 
						  so we must save the codec registers.*/
						snd_hw_SaveAudioContext(pGeode);
						/*Powerdown CODEC*/ 
						snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWR_D4);

						if (pGeode->fCS553x)
						{
							/*Set the AC-Link ShutDown bit*/ 
							OS_WritePortULong((unsigned short) (pGeode->F3BAR0 + CODEC_CONTROL_REG_5535), BIT_5535_ACLINK_SHUTDOWN);
						}
						pGeode->CurrentPowerState = NewPowerState;  
						break;
					}
					}
					OS_DbgMsg("<--snd_geode_SetPowerState\n");
					return 0;
				}

/**
 * \ingroup linux GX layer
 * \brief
 * Access for  reset to the Codeccontrol register
 * 
 * \param  PGEODEAUDIO pGeode
 *  
 */
			static void snd_hw_ResetCodec (PGEODEAUDIO pGeode)
				{
					OS_DbgMsg("-->snd_hw_ResetCodec\n");
					/*Reset codec*/ 
    
					*((unsigned long *)( pGeode->F3BAR0 + CODEC_CMD_REG )) = 0L;
    
					OS_DbgMsg("<--snd_hw_ResetCodec\n");
				}

/**
 * \ingroup linux GX layer
 * \brief
 * Test the actual power state of the codec against
 * the value of "Bit"
 *
 * \param  PGEODEAUDIO pGeode
 *         unsigned short Bit 
 *         unsigned char Operation
 *
 *  
 */
			static unsigned char snd_hw_CheckCodecPowerBit(PGEODEAUDIO pGeode, unsigned short Bit, unsigned char Operation)
				{
					unsigned short CodecData, CodecTimeOut;

					CodecTimeOut = 10;
					do
					{
						/*Read the power management register.*/ 
						CodecData=snd_hw_CodecRead (pGeode, POWERDOWN_CTRL_STAT);
						/*Check the REF status. Should be ready.*/
						if (Operation == CLEAR)
						{
							if (!(CodecData & Bit))
								break;
							else 
								CodecTimeOut--;
						}
						else 
						{
							if (CodecData & Bit) 
								break;
							else
								CodecTimeOut--;
						}
						/*Let's wait a little, 10ms and then try again.*/
						OS_Sleep(10);
					} while (CodecTimeOut);
    
					/*Check if we timed out.*/ 
					if (CodecTimeOut == 0)
					{
						return  FALSE;
					}  
					return TRUE;
				}


/**
 * \ingroup linux GX layer
 * \brief
 * Power up the codec in 5 steps.
 *  
 * 
 * \param  PGEODEAUDIO pGeode
 * \return TRUE on success
 */
			static unsigned char snd_hw_CodecFullOn (PGEODEAUDIO pGeode)
				{
					snd_hw_CodecRead (pGeode, POWERDOWN_CTRL_STAT);

					/*Clear EAPD,PR6 and AC-link to power up external and HP amp and Digital interface*/ 
					snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWRUP_STEP1);
					/*Clear PR3 to power up Analog (Vref off)*/ 
					snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWRUP_STEP2);

					if (!snd_hw_CheckCodecPowerBit (pGeode, GEODEAUDIO_CODEC_POWER_REF, SET))
					{
						OS_DbgMsg("REF timed out. CoDec not powered up.\n");
						return FALSE;
					}
        
					/*A loud "pop" sound is heard without sufficient delay.
					  It means the REF ready bit being set doesn't reflect 
					  the real reference voltage status when this bit is being checked.
					  It is codec issue. Adding approximate 1 second delay is
					  the current workaround.*/
					OS_Sleep(1200); 

					/*Clear PR2 to power up Analog (Vref on)*/
					snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWRUP_STEP3);

					if (!snd_hw_CheckCodecPowerBit (pGeode, GEODEAUDIO_CODEC_POWER_ANL, SET))
					{
						OS_DbgMsg("ANL timed out. CoDec not powered up.\n");
						return FALSE;
					}

					/*Clear PR1 to power up DAC*/ 
					snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWRUP_STEP4);

					if (!snd_hw_CheckCodecPowerBit (pGeode, GEODEAUDIO_CODEC_POWER_DAC, SET))
					{
						OS_DbgMsg("DAC timed out. CoDec not powered up.\n");
						return FALSE;
					}

					/*Clear PR0 to power up ADC*/
					snd_hw_CodecWrite (pGeode, POWERDOWN_CTRL_STAT, GEODEAUDIO_PWRUP_STEP5);
        
					if (!snd_hw_CheckCodecPowerBit (pGeode, GEODEAUDIO_CODEC_POWER_ADC, SET))
					{
						OS_DbgMsg("ADC timed out. CoDec not powered up.\n");
						return FALSE;
					}
					return TRUE;
				}

/**
 * \ingroup linux GX layer
 * \brief
 * Stores the content of all necessary codec registers before   
 * the codec goes to powersave.
 * 
 * \param  PGEODEAUDIO pGeode
 * 
 */
			static void snd_hw_SaveAudioContext (PGEODEAUDIO pGeode)
				{
					unsigned char   i, RegIndex = 0;
					unsigned long   Channel;
					unsigned long   lTemp;
					unsigned short  sTemp;  

					/*Check if DMA is running for each channel.
					  If it is, save PRD table address and turn it OFF*/
    
					for (Channel=0;Channel<MAX_CHANNELS; Channel++)
					{
						if (pGeode->AudioChannel[Channel].Running==TRUE)
						{
            
							pGeode->AudioBusMaster_PRDTableAddress[Channel]  = *((unsigned long  *) (pGeode->F3BAR0+0x24 + (Channel*0x08)));
							/*Stop DMA*/
							snd_hw_StopDMA (pGeode, Channel);
						}
						pGeode->AudioChannel[Channel].Running=TRUE;
					}
				}

			/*Save Mixer volumes and settings*/ 
			for (i=0x02; i<=0x1E; i+=2)
			{
				pGeode->CODECRegisters[RegIndex++] = snd_hw_CodecRead(pGeode, i);
			}

			/*Save Extended registers   (DAC/ADC rates etc...)*/ 
			for (i=0x28; i<=0x38; i+=2)
			{
				pGeode->CODECRegisters[RegIndex++] = snd_hw_CodecRead(pGeode, i);
			}

			memcpy ( pGeode->F3BARSave, (unsigned long *) pGeode->F3BAR0, 0x50 );
			pGeode->F3BARSave[20] &= 0xFE; /*Disable Bus Master again*/    
		}


/**
 * \ingroup linux GX layer
 * \brief
 * Restores the content of all necessary codec registers
 * after awake from powersave. USing this funktion requires 
 * calling snd_hw_SaveAudioContext before
 * 
 * \param  PGEODEAUDIO pGeode
 * 
 */
	static void snd_hw_RestoreAudioContext (PGEODEAUDIO pGeode)
		{
			unsigned char   i, RegIndex = 0;
			unsigned long   Channel;
			unsigned long   ulTemp, ulTemp1, ulTemp2, ulTemp3, ulTemp4;
			unsigned short  usTemp;                                    

			OS_DbgMsg ("Restoring Context.\n");

			/*Restore F3BAR0 registers   */ 

			memcpy ( (unsigned long *) pGeode->F3BAR0, pGeode->F3BARSave, 0x50 );

    
			/*Restore Mixer volumes and settings*/ 
			for (i=0x02; i<=0x1E; i+=2)
			{ 
				snd_hw_CodecWrite(pGeode, i, pGeode->CODECRegisters[RegIndex++]);
			}
    
			/*Restore Extended registers   (DAC/ADC rates etc...)*/ 
			for (i=0x28; i<=0x38; i+=2)
			{
				snd_hw_CodecWrite(pGeode, i, pGeode->CODECRegisters[RegIndex++]);
			}

			/*Check, for each channel, if DMA was running before suspend.
			  If it was, turn it back ON.*/
			for (Channel=0;Channel<MAX_CHANNELS; Channel++)
			{
				if (pGeode->AudioChannel[Channel].Running==TRUE)
				{
					*(unsigned long  *) (pGeode->F3BAR0 + 0x24 + (Channel*0x08)) = pGeode->AudioBusMaster_PRDTableAddress[Channel];

					/*Start DMA*/ 
					snd_hw_ResumeDMA  (pGeode, Channel);
				}
			}
		}

#endif

	module_init(alsa_card_geode_init)
		module_exit(alsa_card_geode_exit)


		
