            g_print("(%d/%d)  %d:%d   Title: %d/%d  Chapter: %d/%d   Volume:%d Mute:%d\n", 
                        i, argc, pos, len,
                        gstlib.DVDTitleNumber(), gstlib.DVDTitleCount(),
                        gstlib.DVDChapterNumber(), gstlib.DVDChapterCount(),
                        gstlib.Volume(),
                        gstlib.IsMute());
