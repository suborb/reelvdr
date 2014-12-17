/*
 * dpkg Plugin for VDR
 * dpkg.h: Service Interface
 *
 * See the README file for copyright information and how to reach the author.
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#ifndef __DPKG_H
#define __DPKG_H

#define DPKG_MODE_NONE       0
// Search for new packages
#define DPKG_MODE_UPGRADE      (1<<0)
// Enable sources editor
#define DPKG_MODE_SOURCES      (1<<1)
// Shows reel package configuration
#define DPKG_MODE_REEL_PKG     (1<<2)
// Shows opt package configuration
#define DPKG_MODE_OPT_PKG      (1<<3)
// Direct jump to package configuration if packages are available (wont take affect with DPKG_MODE_UPGRADE/needs DPKG_MODE_REEL_PKG or DPKG_MODE_OPT_PKG)
#define DPKG_MODE_PKG_JUMP     (1<<4)
// Prefer to show opt packages if it exists (wont take affect with DPKG_MODE_UPGRADE/needs DPKG_MODE_PKG_JUMP and DPKG_MODE_OPT_PKG)
#define DPKG_MODE_OPT_FIRST    (1<<5)
// Display short description after name
#define DPKG_MODE_DESCR_NAME   (1<<6)
// Display short description in second line
#define DPKG_MODE_DESCR_LINE   (1<<7)
// Display simple
#define DPKG_MODE_AUTO_INSTALL (1<<8)
// Install Wizard
#define DPKG_MODE_INSTALL_WIZARD (1<<9)

#define DPKG_DLG_V1_0 "dpkg-dlg-v1.0"
struct dpkg_dlg_v1_0 {
//	in
    char *title;
	int iMode;
//	out
	cOsdMenu *pMenu;
}; // dpkg_dlg_v1_0

#endif /* __DPKG_H */
