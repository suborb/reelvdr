#include "common.h"
#include "gdwrapper.h"
#include <iostream>

using namespace std;
using namespace gdwrapper;

int main(void)
{
	try {
		image img( 720, 576 );
		img.fill_rectangle( gdrectangle( 0, 0, 720, 576 ), gdcolor( 255, 255, 255, 255 ) );

		//image bg( "/etc/vdr/plugins/burn/menu-bg.png" );
		//img.draw_image( gdpoint( 0, 0 ), bg );

		//image button( "/etc/vdr/plugins/burn/menu-button.png" );
		//img.draw_image( gdpoint( 100, 100 ), button );

		img.draw_text( gdtext( gdrectangle( 100, 150, 100, 200 ), gdfont( "/etc/vdr/plugins/burn/helmetr.ttf", 16 ),
					   gdcolor( 255, 255, 255 ) ),
					   "Hallo, Welt, dieser, Text, ist: relativ, lang, und, muss, gewrapped, werden!" );
					   //"HalloWeltDieseZeileIstScheisseLangUndMachtEinProblem! Und noch etwas Text zum:,------- weiterwrappen...." );

		img.save( "test.png" );
	} catch (vdr_burn::user_exception& ex) {
		cerr << "Fehler: " << ex.what() << endl;
	}
}
