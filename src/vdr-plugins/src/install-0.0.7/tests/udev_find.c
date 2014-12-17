/*
 *  file: udev_find.c
 * 
 *  List all internal harddisks
 * 
 * Compile with:
 * gcc -ludev udev_find.c
 */



#include <stdio.h>
#include <libudev.h>

void
find_internal_harddisks ()
{
  struct udev *udev;
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;
  struct udev_device *dev;

  /* Create the udev object */
  udev = udev_new ();
  if (!udev)
    {
      printf ("Can't create udev\n");
      return;
    }

  enumerate = udev_enumerate_new (udev);

  /* internal harddisk */
  udev_enumerate_add_match_property (enumerate, "ID_TYPE", "disk");
  udev_enumerate_add_nomatch_sysattr (enumerate, "partition", "*");
  udev_enumerate_add_match_property (enumerate, "ID_BUS", "ata");
  udev_enumerate_add_match_sysattr (enumerate, "removable", "0");

  
  udev_enumerate_scan_devices (enumerate);
  devices = udev_enumerate_get_list_entry (enumerate);

  udev_list_entry_foreach (dev_list_entry, devices)
  {
    const char *path;
    const char *size_str;
    long int size = 0;


    path = udev_list_entry_get_name (dev_list_entry);
    dev = udev_device_new_from_syspath (udev, path);

    printf ("\nDevice Node Path: %s\n", udev_device_get_devnode (dev));
    
#if 0
    printf ("dev devpath : %s\n", udev_device_get_devpath (dev));
    printf ("dev subsystem: %s\n", udev_device_get_subsystem (dev));
    printf ("dev devtype: %s\n", udev_device_get_devtype (dev));
    printf ("dev syspath: %s\n", udev_device_get_syspath (dev));
    printf ("Dev sysname: %s\n", udev_device_get_sysname (dev));
    printf ("Dev sysnum %s\n", udev_device_get_sysnum (dev));
#endif

    printf ("Dev bus: %s\n", udev_device_get_property_value (dev, "ID_BUS"));

  }

  /* Free the enumerator object */
  udev_enumerate_unref (enumerate);

  udev_unref (udev);
}


int
main ()
{
  find_internal_harddisks ();
  return 0;
}
