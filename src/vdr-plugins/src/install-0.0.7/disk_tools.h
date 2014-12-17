#ifndef DISK_TOOLS_H
#define DISK_TOOLS_H

#include <parted/parted.h>
#include <vector>
#include <string>

typedef PedSector  partition_size_t ; /* long long */


/**
 * @brief The cPartition class : holds info about a disk-partition.
 *  Information about disk partition are got via libparted.
 *  Holds limited information about a partition like:
 *    - partition number
 *    - partition filesystem type
 *    - partition size
 *    - and if it a mountable partition ie. non-extended partition
 */
class cPartition {
public:
    /* name of the device, as given to ped_device_get() 
     * eg. '/dev/sdb' */
    std::string deviceName;
    
    /* partition number, sdb3 ==> number = 3 */
    int number;
    
    /* In Megabytes MiB */
    partition_size_t size;
    
    /* name of filesystem, "ext3", "jfs" etc */
    std::string filesystem; 
    
    /* is it a normal or logical partition? */
    bool mountable;
    
    std::string ToString() const ;
};


/**
 * @brief cPartitions : a list/vector of partitions 
 *        intended to store all partition of a disk
 */
typedef std::vector<cPartition> cPartitions;

cPartitions::const_iterator FindLargestMountablePartition(const cPartitions& partitions);

/**
 * @brief GetPartitions
 * @param deviceName : dev name of the disk eg. '/dev/sdc'
 * @param partitions : a vector of partitions that this function will write to. 
 * @return true if partitions on disk(deviceName) was successfully read.
 */
bool GetPartitions(std::string deviceName, cPartitions& partitions);

#endif /* DISK_TOOLS_H */