#include "disk_tools.h"
#include <sstream>

#ifdef REELVDR/*DEBUG*/
#include <vdr/tools.h>
#define LOG_PARTED_ERROR(fmt, vargs ...) esyslog(fmt, vargs)
#define LOG_PARTED_WARN(fmt, vargs ...) isyslog(fmt, vargs)
#else
#define LOG_PARTED_ERROR(fmt, vargs ...) printf("%s:%d \033[7;91mERROR " fmt "\033[0m\n", __FILE__, __LINE__, vargs)
#define LOG_PARTED_WARN(fmt, vargs ...) printf("%s:%d \033[0;95mWARNING " fmt "\033[0m\n",__FILE__, __LINE__, vargs)
#endif /*DEBUG*/


/** ****************************************************************************
 *  class cPartition
 ******************************************************************************/

/**
 * @brief cPartition::ToString
 * @return a string representation of the partition information.
 */
std::string cPartition::ToString() const 
{
    std::stringstream stream;
    stream << deviceName << number << " " 
           << size << "MiB" << " " << filesystem 
           << " " << (mountable?"mountable":"notmountable");
    
    return stream.str();
} // ToString()



cPartitions::const_iterator FindLargestMountablePartition(const cPartitions &partitions)
{
    cPartitions::const_iterator it = partitions.begin();
    cPartitions::const_iterator it_max = it;
    
    /* find the first mountable partition, and set it_max */
    while (it != partitions.end()) 
    {
        if (it->mountable)
            break;
        
        ++it;
    } // while
    
    it_max = it;
        
    while (it != partitions.end())
    {
        /* mountable and larger ? */
        if (it_max->size < it->size && it->mountable)
            it_max = it;

        ++it;
    } // while
    
    return it_max;
    
} // FindLargestMountablePartition()


/**
 * @brief GetPartitions
 * @param deviceName : dev name of the disk eg. '/dev/sdc'
 * @param partitions : a vector of partitions that this function will write to. 
 * @return true if partitions on disk(deviceName) was successfully read.
 */
bool GetPartitions(std::string deviceName, cPartitions& partitions)
{
    PedDevice* device = NULL;
    PedDisk* disk = NULL;
    PedPartition* part = NULL;
    bool result = false;
    cPartition partition;
    ped_device_probe_all ();
    
    device = ped_device_get(deviceName.c_str());
    if (device == NULL) 
    {
        LOG_PARTED_ERROR("ped_device_get('%s') returned NULL", deviceName.c_str());
        
        result = false;
        goto cleanup;
    } // if
    
    disk = ped_disk_new(device);
    if (disk == NULL) 
    {
        LOG_PARTED_ERROR("ped_disk_new(%p) returned NULL for device %s",
                  disk, deviceName.c_str());
        
        result = false;
        goto cleanup;
    } // if
    
    
    result = true;
    partitions.clear();
    
    // iterate over found paritions
    for (part = ped_disk_next_partition (disk, NULL); 
         part;
         part = ped_disk_next_partition (disk, part))
    {
        if (part->num < 0) 
        {
            LOG_PARTED_WARN("got negative partition number := %d! in device %s", 
                     part->num,
                     deviceName.c_str());
            continue;
        }
        
        /* device name to which this partition belongs */
        partition.deviceName = deviceName;
        
        partition.number = part->num;

        /* convert from number of blocks to number of Megabytes MiB*/
        partition.size = (part->geom.length * device->sector_size) /(1024 * 1024);
        
        partition.filesystem = (part->fs_type) ? part->fs_type->name : "";
        
        /* partition type , avoid extended partitions and freespace ...*/
        partition.mountable = ((part->type&PED_PARTITION_LOGICAL)
                ||(part->type==PED_PARTITION_NORMAL));
        
        /* add to partition list */
        partitions.push_back (partition);
    } // for
    
    
cleanup:
    /* clean up */
    if (disk)
        ped_disk_destroy (disk);
    
    if (device)
        ped_device_destroy (device);

    return result;
} // GetPartitions()

