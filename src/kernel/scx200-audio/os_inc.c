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


#include "os_inc.h"
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9))
  #include <linux/hardirq.h>
#endif

unsigned long   TotalAllocatedMemory = 0;
unsigned long   flags;
spinlock_t      lock;

//
//  OS-Specific calls
//

//#define  AUDIO_DEBUG

void OS_DbgMsg( unsigned char *szFormat, ... )
{ 
#ifdef AUDIO_DEBUG
    char        szBuf[ 128 ];
    va_list     arglist;

    va_start( arglist, szFormat );

    vsprintf( szBuf, szFormat, arglist );

    va_end( arglist );

    printk ( szBuf ); 
#endif
}


void * OS_AllocateMemory (unsigned long Size)
{
    TotalAllocatedMemory+=Size;

    return kmalloc( Size, GFP_KERNEL ); 
}

void OS_Free (unsigned char *pucBuff)
{
    kfree( pucBuff );
}

void * OS_AllocateDMAMemory (void* pAdapterObject, unsigned long Size, unsigned long *PhysicalAddress)
{    
unsigned int    NumberOfPages, order;
void*           VirtualAddress;
struct          page *page, *pend;
        
    VirtualAddress=NULL;
    order=0;
    NumberOfPages=Size/1024;    
    for (order = 0; (1 << order)<(int)NumberOfPages; order++);

    VirtualAddress = (void *) __get_free_pages(GFP_KERNEL, order);

    if(VirtualAddress)
    {
        *PhysicalAddress = virt_to_bus(VirtualAddress);

        //
        // now mark the pages as reserved; otherwise remap_page_range doesn't do what we want 
        //
        pend = virt_to_page(VirtualAddress + (PAGE_SIZE << order) - 1);

        for (page = virt_to_page(VirtualAddress); page <= pend; page++)
            set_bit(PG_reserved, &((page)->flags));

        TotalAllocatedMemory+=Size;
        OS_DbgMsg("Allocating: %d  - TotalAllocatedMemory: %d\n", Size, TotalAllocatedMemory);
    }

    return VirtualAddress;
}

void OS_FreeDMAMemory (void* pAdapterObject, void * VirtualAddress, unsigned long PhysicalAddress, unsigned long Size)
{
unsigned int    NumberOfPages, order=0;
struct          page *page, *pend;

    OS_DbgMsg("OS_INC: Freeing %08X\n", PhysicalAddress);

    NumberOfPages = Size/1024;
    for (order = 0; (1 << order) < (int)NumberOfPages; order++);

    //
    // undo marking the pages as reserved 
    //
    pend = virt_to_page(VirtualAddress + (PAGE_SIZE << order) - 1);

    for (page = virt_to_page(VirtualAddress); page <= pend; page++)
        clear_bit(PG_reserved, &((page)->flags));

    free_pages((unsigned long) VirtualAddress, order);
    TotalAllocatedMemory-=Size;
    OS_DbgMsg("Freeing: %d - TotalAllocatedMemory: %d\n", Size, TotalAllocatedMemory);

    VirtualAddress = NULL;
}

//
//   Copies n bytes of audio data from the user to the DMA buffer space
//
unsigned char OS_CopyFromUser (unsigned char *DMABuffer,  unsigned char *UserBuffer, unsigned long Count)
{
    if (copy_from_user(DMABuffer, UserBuffer, Count)) 
    {
        return 0;
    }

    return 1;
}

unsigned char OS_CopyToUser (unsigned char *UserBuffer, unsigned char *DMABuffer, unsigned long Count)
{
    if (copy_to_user(UserBuffer, DMABuffer, Count)) 
    {
        return 0;
    }

    return 1;
}

unsigned long OS_ReadPortULong(unsigned short Port)
{
    return inl((int) Port);
}

void OS_WritePortULong (unsigned short Port, unsigned long Data)
{
    outl (Data, (int) Port);
}

unsigned short OS_ReadPortUShort (unsigned short Port)
{
    return inw((int) Port);
}

void OS_WritePortUShort (unsigned short Port, unsigned short Data)
{
    outw (Data, (int) Port);
}

unsigned char OS_ReadPortUChar(unsigned short Port)
{
    return inb((int) Port);
}

void OS_WritePortUChar (unsigned short Port, unsigned char Data)
{
    outb (Data, (int) Port);
}

unsigned long OS_VirtualAddress (unsigned long PhysicalAddress)
{
    return (unsigned long) phys_to_virt (PhysicalAddress);
}

unsigned long OS_Get_F3BAR0_Virt(unsigned long BAR_Phys)
{
    unsigned long F3BAR_Virt;

    F3BAR_Virt=(unsigned long) ioremap(BAR_Phys,128);

    return F3BAR_Virt;
}
 
void OS_Sleep (unsigned long milliseconds)
{
  if(!in_atomic())
  {
    current->state = TASK_INTERRUPTIBLE;
    schedule_timeout( ( milliseconds * HZ ) / 1000 );
  }
}

void OS_InitSpinLock (void)
{
    spin_lock_init (&lock);
}

void OS_SpinLock (void)
{
    spin_lock_irqsave (&lock, flags);
}

void OS_SpinUnlock (void)
{
    spin_unlock_irqrestore (&lock, flags);
}
