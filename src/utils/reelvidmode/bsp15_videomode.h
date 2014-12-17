static drcDlMONITOR_TIMING_t ReelLcdMonitorTiming =
   {  720,                     // Active video width.
       576,                     // Active video height.
       50,                      // Frame rate.
       False,                   // Display is interlaced (True) or progressive (False).
       60,                      // Horizontal sync width.
       14,                      // Horizontal front porch.
       70,                     // Horizontal back porch.
       2,                       // Vertical Sync height.
       26,                      // Vertical sync front porch.
       21,                      // Vertical sync back porch.
       0,                       // Left border (no active video).
       0,                       // Right border (no active video).
       0,                       // Top border (no active video).
       0,                       // Bottom border (no active video).
       0*40000,                 // Pixel clock frequency.
       True,                    // Set if the horzizontal sync is active high.
       True,                    // Set if the vertical sync is active high.
       {4, 3 },                 // Monitor aspect ratio.
       {  1,  1 },              // Pixel aspect ratio.
       24                       // Number of bits per pixel (Used for Digital RGB).
};
                                                                                                                                        