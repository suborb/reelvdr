#ifndef REEL_LIB_TOOLS_H
#define REEL_LIB_TOOLS_H

#include <string>
#include <cstdlib>

namespace Reel {
namespace Tools {

/**
 * @brief FileExists : returns true if the given abs. path exists
 * @param file : absolute path of a file
 * @return true if file exists else false
 */
bool FileExists(const std::string file);

/**
 * @brief IsNetworkDevicePresent : returns true if /sys/class/net/<dev> exists
 * @param dev : network device name. Example 'eth1'
 * @return true if network device is available
 */
bool IsNetworkDevicePresent(const std::string dev);

/**
 * @brief ProgressBarString  : makes "[|||||    ]" for given percent and length of string
 * @param percent: between 0...100
 * @param length : default 10, max 1000
 * @return progressBar string "[|||   ]"
 */
std::string ProgressBarString(float percent, unsigned int length=10);

/**
 * @brief HasDevice : reads /proc/bus/pci/devices to find if the give pcidev number exists
 * @param deviceID : Example "808627cb",
 * @return true if device exists in pci bus
 */
bool HasDevice(const char *deviceID);

} //namespace Tools
} //namespace Reel

#endif
