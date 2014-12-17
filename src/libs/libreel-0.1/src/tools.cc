#include "tools.h"
#include <unistd.h>
#include <fstream>
#include <iostream>

namespace Reel {
namespace Tools {


/**
 * @brief FileExists : returns true if the given abs. path exists
 * @param file : absolute path of a file
 * @return true if file exists else false
 */
bool FileExists(const std::string file)
{
    return !file.empty() && (access(file.c_str(), F_OK) == 0);
}


/**
 * @brief IsNetworkDevicePresent : returns true if /sys/class/net/<dev> exists
 * @param dev : network device name. Example 'eth1'
 * @return true if network device is available
 */
bool IsNetworkDevicePresent(const std::string dev)
{
    if (dev.empty()) {
        return false;
    }

    std::string dev_in_sys = std::string("/sys/class/net/") + dev;
    return FileExists(dev_in_sys);
}


/**
 * @brief ProgressBarString  : makes "[|||||    ]" for given percent and length of string
 * @param percent: between 0...100
 * @param length : default 10, max 1000
 * @return progressBar string "[|||   ]"
 */
std::string ProgressBarString(float percent, unsigned int length)
{
  if (percent < 0) return "";

  percent = std::min(percent, 100.0f); // greater than 100 is 100!
  length  = std::min(length, 1000u);  // no more than 1000 chars

  // account for opening and closing brackets
  length -= 2;

  // number of '|' to show
  unsigned int p = (unsigned int)((length*percent)/100);

  std::string progressString = "[";

  // non-zero number of '|'
  if (p)
      progressString += std::string(p, '|');

  // non-zero number of ' '
  if (length - p)
      progressString += std::string(length-p, ' ');

  progressString += "]";

  return progressString;
}


/**
 * @brief HasDevice : reads /proc/bus/pci/devices to find if the give pcidev number exists
 * @param deviceID : Example "808627cb",
 * @return true if device exists in pci bus
 */
bool HasDevice(const char *deviceID)
{
    std::ifstream inFile("/proc/bus/pci/devices");

    if (!inFile.is_open() || !deviceID)
        return  false;

    std::string line;

    bool found = false;
    while (getline(inFile, line)) {
       std::string::size_type pos = line.find(deviceID);
       if (pos != std::string::npos) {
          found = true;
          break;
       }
    }

    inFile.close();

    return found;
}


} //namespace Tools
} //namespace Reel
