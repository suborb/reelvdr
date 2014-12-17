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


#ifndef __AMDGEODE_H__ 
#define __AMDGEODE_H__

#define	TRUE				1
#define FALSE				0

#define POLLING				0xFF

#define	SET					1
#define	CLEAR				0

typedef enum {
    WAPI_IN = 0, 
    WAPI_OUT
} WAPI_INOUT, *PWAPI_INOUT;

#define NUMBER_OF_CHANNELS	2

#define DMA_PLAYBACK_BUFFERSIZE (128*1024)
#define DMA_CAPTURE_BUFFERSIZE  (64*1024)
//
//  Device and Vendor IDs - Needed like this for OSS only
//
#define CYRIX_VENDOR_ID     0x1078   
#define NATIONAL_VENDOR_ID  0x100B   

//
//       Audio  Device IDs
//
#define	CX5530_DEV_ID    0x0103
#define	SC1200_DEV_ID    0x0503
#define	CS5535_DEV_ID    0x002E

//
//  PCI Config address. 
//  Bits 15..11 specify the PCI device.
//  Bits 7..2 specify reg in PCI device.
//
#define PCI_CADDR         0x0CF8  

//                              
//  PCI Config data reg.
//  After PCI_ADDR set, this reg contains    
//  the corresponding data (the dev reg).
//
#define PCI_CDATA         0x0CFC  

//
// Function 3 of 5530 PCI dev is Audio (ISA idx).
//
#define  PCI_FUNC3_AUDIO     0x300       
#define  PCI_AUDIO_CMD_REG   0x04
                          
typedef unsigned char AUDIO_STATE;

#define AUDIO_STATE_IGNORE          0
#define AUDIO_STATE_IN_RECORDING    0x01
#define AUDIO_STATE_IN_OVERFLOW     0x02
#define AUDIO_STATE_IN_STOPPED      0x03
#define AUDIO_STATE_IN_MASK         0x0F
#define AUDIO_STATE_OUT_PLAYING     0x10
#define AUDIO_STATE_OUT_UNDERFLOW   0x20
#define AUDIO_STATE_OUT_STOPPED     0x30
#define AUDIO_STATE_OUT_MASK        0xF0

#define RECORD_RUNNING		0x01
#define RECORD_OVERFLOW		0x02
#define RECORD_STOPPED		0x03

#define PLAYBACK_RUNNING    0x10
#define PLAYBACK_UNDERFLOW  0x20
#define PLAYBACK_STOPPED    0x30

//-----------------------------------------------------------
//			BEGIN Interrupt-Related code
//-----------------------------------------------------------

//
//   Interrupt IDs
//
#define DMA0_INTERRUPT		0x04
#define DMA1_INTERRUPT		0x08
#define DMA2_INTERRUPT		0x10
#define DMA3_INTERRUPT		0x20
#define DMA4_INTERRUPT		0x40
#define DMA5_INTERRUPT		0x80

//-----------------------------------------------------------
//			END Interrupt-Related code
//-----------------------------------------------------------

//
//   Bit conversions
//
#define BITS_8_TO_16(x) ( ( (long) ((unsigned char) x - 128) ) << 8 )
#define BITS_16_TO_8(x) ( ( (unsigned char) ((long) x >> 8 ) ) + 128 )


//
// The CODEC commands are actually 16-bit words, into which is inserted
// the codec "target" register, identified by a byte. The 5530 Codec
// controller writes a command unsigned short of 32-bits, that includes the codec
// command unsigned short.
//
#define CODEC_COMMAND_MASK       0xFF00FFFF

//
//  The Interaction with the CODEC is a bit cumbersome 
//  because of the serial interface.
//
#define CODEC_STATUS_REG    0x08        // In Audio mem-map.
#define CODEC_CMD_REG       0x0c        // In audio mem-map.
#define CODEC_CMD_VALID     0x00010000
#define CODEC_STATUS_VALID  0x00020000  
#define CODEC_STATUS_NEW    0x00010000  
#define BIT_CODEC_READY     0x00800000  

//
//   Registers for the 5535
//
#define CODEC_STATUS_REG_5535		0x08
#define CODEC_CONTROL_REG_5535      0x0c        

//
//   5535 Bits
//
#define BIT_5535_CODEC_COMMAND_NEW   0x00010000  
#define BIT_5535_CODEC_STATUS_NEW	 0x00020000
#define BIT_5535_ACLINK_SHUTDOWN	 0x00040000	
#define BIT_5535_ACLINK_WARM_RESET	 0x00020000
#define BIT_5535_CODEC_READY_PRIM	 0x00800000

//
// Codec register indexes. Note these are all shifted left by 16 bits.
//
#define RESET                0x00
#define MASTER_VOLUME        0x02
#define LINE_LEV_OUT_VOL     0x04
#define MASTER_VOLUME_MONO   0x06
#define MASTER_TONE_RL       0x08
#define PC_BEEP_VOLUME       0x0a
#define PHONE_VOLUME         0x0c
#define MIC_VOLUME           0x0e
#define LINE_IN_VOLUME       0x10
#define CD_VOLUME            0x12
#define VIDEO_VOLUME         0x14
#define TV_VOLUME            0x16
#define PCM_OUT_VOL          0x18
#define RECORD_SELECT        0x1a
#define RECORD_GAIN          0x1c
#define RECORD_MIC_GAIN      0x1e
#define GENERAL_PURPOSE      0x20
#define CONTROL_3D           0x22
#define MODEM_RATE           0x24
#define POWERDOWN_CTRL_STAT  0x26
#define EXTENDED_AUDIO_ID	 0x28
#define EXT_AUDIO_CTRL_STAT  0x2A
#define PCM_FRONT_DAC_RATE   0x2C
#define PCM_LR_ADC_RATE      0x32
#define VENDOR_ID1           0x7c
#define VENDOR_ID2           0x7e

#define MUTE_MASK			 0x8000
#define HEADHONE_AVAIL       0x0010
#define LINE_LEV_RESET_VOL	 0x0000 // the reset without the mask


// NOTE: The ATTEN_CTL_BITS default is only 5 because some CODECs are
// not compliant with the 2.1 spec.  The value 5 is safe for all Geode
// reference platforms.  For platforms that support 6 bit attenuation control
// uncomment the following line:
//#define AC97_2DOT1_6BIT_COMPLIANT

#ifdef AC97_2DOT1_6BIT_COMPLIANT
#	define MASTER_ATTEN_CTL_BITS		 6
#else
#	define MASTER_ATTEN_CTL_BITS		 5
#endif

#define MASTER_VOLUME_MAX    ( ( 1 << MASTER_ATTEN_CTL_BITS ) - 1 )
#define LINE_LEV_OUT_MAX     ( ( 1 << MASTER_ATTEN_CTL_BITS ) - 1 )

//
// AD1819A registers 
//
#define AD1819A_SER_CONF			0x74
#define AD1819A_SER_CONF_DRQEN		0x08
#define AD1819A_MISC				0x76
#define AD1819A_PCM_SR0				0x78
#define AD1819A_PCM_SR1				0x7A
#define AD1819A_VENDORID1			0x7C
#define AD1819A_VENDORID2			0x7E


#define CHANNEL0_PLAYBACK	0
#define CHANNEL1_RECORD		1
#define MAX_CHANNELS		2

//
//   Interval types
//
#define	INTERVAL_BYTES			0x01
#define INTERVAL_MILLISECONDS	0x02

//
//	Power Management bits
//
#define GEODEAUDIO_PWR_PR0	0x0100      // PCM in ADC's & input Mux Powerdown
#define GEODEAUDIO_PWR_PR1    0x0200      // PCM out DACs Powerdown
#define GEODEAUDIO_PWR_PR2    0x0400      // Analog Mixer powerdown (Vref still on)
#define GEODEAUDIO_PWR_PR3    0x0800      // Analog Mxer powerdown (Vref off)
#define GEODEAUDIO_PWR_PR4    0x1000      // Digital interface (AC-link) powerdown (external clk off)
#define GEODEAUDIO_PWR_PR5    0x2000      // Internal Clk disable
#define GEODEAUDIO_PWR_PR6    0x4000      // HP amp powerdown
#define GEODEAUDIO_PWR_PR7    0x8000      // External Amplifier Power Down

#define GEODEAUDIO_PWR_D0              0x0000      
#define GEODEAUDIO_PWR_D1              GEODEAUDIO_PWR_EXTOFF
#define GEODEAUDIO_PWR_D2              GEODEAUDIO_PWR_PR0|GEODEAUDIO_PWR_PR1|GEODEAUDIO_PWR_PR2|GEODEAUDIO_PWR_PR6|GEODEAUDIO_PWR_PR7
#define GEODEAUDIO_PWR_D3              GEODEAUDIO_PWR_PR0|GEODEAUDIO_PWR_PR1|GEODEAUDIO_PWR_PR2|GEODEAUDIO_PWR_PR6|GEODEAUDIO_PWR_PR7
#define GEODEAUDIO_PWR_D4              GEODEAUDIO_PWR_PR0|GEODEAUDIO_PWR_PR1|GEODEAUDIO_PWR_PR2|GEODEAUDIO_PWR_PR3|GEODEAUDIO_PWR_PR4|GEODEAUDIO_PWR_PR5|GEODEAUDIO_PWR_PR6|GEODEAUDIO_PWR_PR7
#define GEODEAUDIO_PWR_ANLOFF          GEODEAUDIO_PWR_PR2|GEODEAUDIO_PWR_PR3  // Analog section OFF
#define GEODEAUDIO_PWR_EXTOFF          GEODEAUDIO_PWR_PR6|GEODEAUDIO_PWR_PR7  // HP amp and External Amplifier OFF
#define GEODEAUDIO_PWR_D1_HAWK         GEODEAUDIO_PWR_PR0|GEODEAUDIO_PWR_PR1|GEODEAUDIO_PWR_PR2|GEODEAUDIO_PWR_PR3|GEODEAUDIO_PWR_PR4
#define GEODEAUDIO_PWR_DIGOFF			 GEODEAUDIO_PWR_PR0|GEODEAUDIO_PWR_PR1	// Digital section OFF

#define GEODEAUDIO_PWRUP_STEP1		 0x0F00 // Clear EAPD,PR6 and AC-link to power up external and HP amp and Digital interface
#define GEODEAUDIO_PWRUP_STEP2		 0x0700 // Clear PR3 to power up Analog (Vref off)
#define GEODEAUDIO_PWRUP_STEP3		 0x0300 // Clear PR2 to power up Analog (Vref on)
#define GEODEAUDIO_PWRUP_STEP4		 0x0100 // Clear PR1 to power up DAC
#define GEODEAUDIO_PWRUP_STEP5		 0x0000 // Clear PR0 to power up ADC

#define GEODEAUDIO_CODEC_POWER_ADC	 0x0001
#define GEODEAUDIO_CODEC_POWER_DAC	 0x0002
#define GEODEAUDIO_CODEC_POWER_ANL	 0x0004
#define GEODEAUDIO_CODEC_POWER_REF	 0x0008
//
// Device Power States
//
typedef enum _GEODEAUDIO_POWER_STATE 
{
    GEODEAUDIO_D0 = 0, // Full On: full power,  full functionality
    GEODEAUDIO_D1,     // Low Power On: fully functional at low power/performance
    GEODEAUDIO_D2,     // Standby: partially powered with automatic wake
    GEODEAUDIO_D3,     // Sleep: partially powered with device initiated wake
    GEODEAUDIO_D4,     // Off: unpowered
} GEODEAUDIO_POWER_STATE, *PGEODEAUDIO_POWER_STATE;

//
//   Physical address of the device
//
#define AUDIO_REGS_BUFFER           0x40011000  

typedef enum 
{
	PCM_TYPE_M8,
	PCM_TYPE_M16,
	PCM_TYPE_S8,
	PCM_TYPE_S16
} PCM_TYPE, *PPCM_TYPE;

typedef struct  
{
	unsigned char sample;              // Unsigned 8-bit sample
} SAMPLE_8_MONO;

typedef struct  
{
	signed short sample;               // Signed 16-bit sample
} SAMPLE_16_MONO;

typedef struct  
{
	unsigned char sample_left;         // Unsigned 8-bit sample
	unsigned char sample_right;        // Unsigned 8-bit sample
} SAMPLE_8_STEREO;

typedef struct  
{
	unsigned short sample_left;        // Signed 16-bit sample
	unsigned short sample_right;       // Signed 16-bit sample
} SAMPLE_16_STEREO;

typedef union 
{
	SAMPLE_8_MONO m8;
	SAMPLE_16_MONO m16;
	SAMPLE_8_STEREO s8;
	SAMPLE_16_STEREO s16;
} PCM_SAMPLE, *PPCM_SAMPLE;

#define SAMPLE_SIZE_IN_BYTES	8

//
//   Copy of the DDk's extended waveform format structure used for all non-PCM formats. 
//	 this structure is common to all non-PCM formats.  (Currently used only by the UAM driver)
//
typedef struct tdurWAVEFORMATEX
{
	unsigned short  wFormatTag;         // format type 
	unsigned short  nChannels;          // number of channels (i.e. mono, stereo...) 
	unsigned long   nSamplesPerSec;     // sample rate 
	unsigned long   nAvgBytesPerSec;    // for buffer estimation 
	unsigned short  nBlockAlign;        // block size of data 
	unsigned short  wBitsPerSample;     // number of bits per sample of mono data 
	unsigned short  cbSize;             // the count in bytes of the size of 
                                        // extra information (after cbSize) 
} durWAVEFORMATEX, *durLPWAVEFORMATEX;

//
// Copy of the DDK's wave data block header 
//
typedef struct tdurWAVEHDR 
{
	char *              lpData;                 // pointer to locked data buffer 
	unsigned long       dwBufferLength;         // length of data buffer 
	unsigned long       dwBytesRecorded;        // used for input only 
	unsigned long	    dwBytesRead;			// Number of bytes Read in this buffer
	unsigned long       dwUser;                 // for client's use 
	unsigned long       dwFlags;                // assorted flags (see defines) 
	unsigned long       dwLoops;                // loop control counter 
	struct tdurWAVEHDR	*lpNext;                 // reserved for driver 
	unsigned long       reserved;               // reserved for driver 
} durWAVEHDR, *durPWAVEHDR;

// PRD table flags
#define PRD_JMP_BIT                     0x20000000
#define PRD_EOP_BIT                     0x40000000
#define PRD_EOT_BIT                     0x80000000

typedef struct tagPRDEntry
{
    unsigned long  ulPhysAddr;
    unsigned long  SizeFlags;
} PRD_ENTRY, *PPRD_ENTRY;

typedef struct tagALLOC_INFO
{
	unsigned long	 PhysicalAddress;
	unsigned char	*VirtualAddress;
	unsigned long	 Size;
	unsigned long	 OriginalPhysicalAddress;
	unsigned char	*OriginalVirtualAddress;
	unsigned long	 OriginalSize;
} ALLOC_INFO, *PALLOC_INFO;

//
//   Command register bits
//
#define	 PCI_READS			0x00
#define	 PCI_WRITES			0x08

#define  ENABLE_BUSMASTER	0x01
#define  PAUSE_BUSMASTER	0x03
#define  STOP_BUSMASTER		0x00

typedef struct tagGEODEAUDIO
{
	//
	// PCI audio functions base register.(r/w via i/o).
	//
	unsigned long	PCI_Header_Registers;  

	//
	// Pointer to memory mapped 5530 Audio regs.                        
	//
	unsigned long			F3BAR0;      

	//
	//   Power management
	//
	GEODEAUDIO_POWER_STATE	CurrentPowerState;	
	unsigned short			CODECRegisters[0x38];

	unsigned char		F3BARSave[0x80];
	unsigned long		AudioBusMaster_PRDTableAddress[MAX_CHANNELS];

	//
	//   Flags
	//
	unsigned char			fAD1819A;			
	unsigned char			fCS553x;
	unsigned char			fIOAccess;
	unsigned char			fPolling;


	void				   *pAdapterObject;	
	unsigned long		    v_nVolume;					//  Initialize with 0!

//-----------------------------------------------------------
//			BEGIN Interrupt-Related code
//-----------------------------------------------------------

	unsigned char		   *pInterruptID;
	unsigned int			SelectedIRQ;

	volatile unsigned long	IRQControlRegister;
	volatile unsigned long  InternalIRQEnableRegister;
	unsigned short			uIRQMask;

//-----------------------------------------------------------
//			END Interrupt-Related code
//-----------------------------------------------------------

	struct tagAudioChannel
	{
		unsigned char	fDirectDMA;

		struct tagIndirectDMA
		{
			unsigned long	CurrentTransferPointer;
			volatile long	BytesRemainingInDMABuffer; 
			unsigned long	SampleConversionFactor;
            unsigned long   DmaBufferSize;
			//
			//   Data Format
			//

			unsigned long	SampleRate;
			unsigned char	nChannels;
			unsigned char	BitsPerSample;
			unsigned long	FragmentSize;

			//
			//	Old stuff!
			//
			unsigned char	*dma_page[4]; 
			unsigned long	*prd_array;
			unsigned long	prd_array_phys_adr;

		} IndirectDMA;

		PPRD_ENTRY			PRDTable;	 
		unsigned long		PRDOriginalVirtualAddress;
		unsigned long		PRDOriginalPhysicalAddress;

		ALLOC_INFO			PRD_AllocInfo;
		ALLOC_INFO			DMA_AllocInfo;
		ALLOC_INFO			Work_AllocInfo;

		unsigned long		PRDEntriesAllocated;
		unsigned long		StopDMAWaitInMiliseconds;

		unsigned char		Running;		  
		unsigned char		fInUse;	
		unsigned long		SampleRate;
		unsigned char		fAllocated;
		
		struct tagAudioBusMaster
		{
			unsigned long	CommandRegister;

//-----------------------------------------------------------
//			BEGIN Interrupt-Related code
//-----------------------------------------------------------
			unsigned long	SMI_StatusRegister;
//-----------------------------------------------------------
//			END Interrupt-Related code
//-----------------------------------------------------------

			unsigned long	PRDTableAddress;
			unsigned char	DirectionBit;
			unsigned long	CurrentPRDPointer;
			unsigned long	DMAPointer;
		} AudioBusMaster;
	} AudioChannel[NUMBER_OF_CHANNELS];

} GEODEAUDIO, *PGEODEAUDIO;

#endif // __AMDGEODE_H__
