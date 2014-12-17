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

cRipitSetup RipitSetup;

// --- cRipitSetup ---------------------------------------------------------------

cRipitSetup::cRipitSetup(void)
{
    Ripit_hidden = false;
    Ripit_halt = false;
    Ripit_noquiet = true;
    Ripit_eject = true;
    Ripit_fastrip = false;
    Ripit_lowbitrate = 6;
    Ripit_maxbitrate = 9;
    Ripit_bitrate = 7;
    Ripit_preset = 3;
    strcpy(Ripit_encopts, "");
    strcpy(Ripit_dev, "/dev/dvd");
    strcpy(Ripit_dir, "/media/hd/music/AudioCD-Archiv");
    Ripit_remote = false;
    strcpy(Ripit_remotevalue, "--sshlist master,bedroom --scp");
    Ripit_nice = 5;
    Ripit_encoder = RIPIT_OGG;
    Ripit_allSecretOptions = false;
}
