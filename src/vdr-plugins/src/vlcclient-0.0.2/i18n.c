#include "i18n.h"

const char *i18n_name = NULL;

const tI18nPhrase myPhrases[] = {
        {       "VideoLAN-Client",            // English
                "VideoLAN-Client",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "IP of remote VLC",            // English
                "IP des entfernten VLC",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "Fetching recordings",            // English
                "Suche gestartet",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "Remote VLC Directory",            // English
                "entferntes VLC-Verzeichnis",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "Client for a remote VLC",            // English
                "Client fuer einen entfernten VLC",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "Streaming Resolution",            // English
                "Stream Auflösung",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "Stream DD (works only if movie has DD)",            // English
                "Stream Dolby Digital (Nur für Filme mit DD)",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "Sync on audio",            // English
                "Syncronisieren für Audio",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "Remote CPU Power",            // English
                "CPU Leistung des entfernten VLC",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "very low",            // English
                "sehr langsam",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "low",            // English
                "langsam",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "ok",            // English
                "normal",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "good",            // English
                "schnell",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "very good",            // English
                "sehr schnell",                     // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        {       "Error fetching recordings",            // English
                "Keine Dateien gefunden",             // Deutsch
                "",                     // Slovenski
                "",                     // Italiano
                "",                     // Nederlands
                "",                     // Portugu<EA>s
                "",                     // Fran<E7>ais
                "",                     // Norsk
                "",                     // suomi
                "",                     // Polski
                "",                     // Espa<F1>ol
                "",                     // Ellinika / Greek
                "",                     // Svenska
                "",                     // Romaneste
                "",                     // Magyar
                "",                     // Catala
                ""                      // Russian
        },
        { NULL }
};

