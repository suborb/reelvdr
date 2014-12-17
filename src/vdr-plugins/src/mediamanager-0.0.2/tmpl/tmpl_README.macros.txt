DBG_x macros are synonyms for xsyslog in vdr.

In distribution package only errors are shown in syslog via esyslog.
In maintainer mode all debuging goes to console via printf.

To enable debugging define MAINTAINER in maintainer.h which is undef by default.
Then define:
	DEBUG_D - enables DBG_D macro
	DEBUG_I - enables DBG_I macro
in your source file and include mediadebug.h after.

This way you can en/disable debugging output for each source file.

