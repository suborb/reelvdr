/*
 * AudioRipper plugin to VDR
 *
 * (C) 2005
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#include <string.h>

#include "setup.h"

cUpdateSetup UpdateSetup;

// --- cUpdateSetup ---------------------------------------------------------------

cUpdateSetup::cUpdateSetup(void)
{
  Update_hidden = 0;
  Update_halt = 0;
  Update_noquiet = 0;
  Update_eject = 1;
  Update_fastrip = 0;
  Update_lowbitrate = 6;
  Update_maxbitrate = 9;
  Update_crc = 0;
  Update_preset = 2;
  strcpy(Update_encopts, "");
  strcpy(Update_dev,"/dev/dvd");
  strcpy(Update_dir,"/media/hd/music/RippedCDs");
  Update_remote = 0;
  strcpy(Update_remotevalue,"--sshlist master,bedroom --scp");
  Update_nice = 5;
}
