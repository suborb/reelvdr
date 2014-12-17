/*
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

cWebbrowserSetup WebbrowserSetup;

// --- cWebbrowserSetup ---------------------------------------------------------------

cWebbrowserSetup::cWebbrowserSetup(void)
{
    strcpy(startURL, "http://www.reel-multimedia.com");
    strcpy(FFstartURL, "http://www.reel-multimedia.com/de/aktuelles_presse.html");
    fontSize = 14;
    menuFontSize = 14;
    scalepics = 75;
    transparency = 240;
    margin_x = 30;
    margin_y = 30;
    optimize = 1;
    X11alpha = 225;
    mode = 0;
    center_full = 0;
}
