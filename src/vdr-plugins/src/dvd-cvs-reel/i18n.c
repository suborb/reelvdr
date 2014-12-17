/*
 * DVD Player plugin for VDR
 *
 * Copyright (C) 2001.2002 Andreas Schultz <aschultz@warp10.net>
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "i18n.h"

const char *ISO639code[] = {
  // Language ISO 639 codes for DVD
    "en",
    "de",
    "sl",
    "it",
    "nl",
    "pt",
    "fr",
    "no",
    "fi",
    "pl",
    "es",
    "el",
    "se",
    "ro",
    "hu",
    "ca",
    "ru",
    "hr",
    "et",
    "da",
};

const tI18nPhrase DvdPhrases[] = {
    {
    "Plugin.DVD$DVD",                                       // English
        "DVD Spieler",                                              // Deutsch
        "DVD",                                              // Slovenski
        "DVD",                                              // Italiano
        "DVD",                                              // Nederlands
        "DVD",                                              // Português
        "DVD",                                              // Français
        "DVD",                                              // Norsk
        "DVD",                                              // suomi
        "DVD",                                              // Polski
        "DVD",                                              // Español
        "DVD",                                              // ÅëëçíéêÜ (Greek)
        "DVD",                                              // Svenska
        "DVD",                                              // Romaneste
        "DVD",                                              // Magyar
        "DVD",                                              // Català
        "DVD",                                              // ÀãááÚØÙ (Russian)
        "DVD",                                              // Hrvatski (Croatian)
        "DVD",                                              // Eesti
        "DVD",                                              // Dansk
    },
    {
    "Plugin.DVD$turn VDR into an (almost) full featured DVD player",    // English
        "Erweitert den VDR zu einen DVD Spieler",  // Deutsch
        "",                                                             // Slovenski
        "Trasforma il VDR in un lettore DVD (quasi)completo",           // Italiano
        "Maak van de VDR een (bijna) komplete DVD speler",              // Nederlands
        "",                                                             // Português
        "",                                                             // Français
        "",                                                             // Norsk
        "DVD-soitin",                                                   // suomi
        "",                                                             // Polski
        "",                                                             // Español
        "",                                                             // ÅëëçíéêÜ (Greek)
        "",                                                             // Svenska
        "",                                                             // Romaneste
        "",                                                             // Magyar
        "",                                                             // Català
        "",                                                             // ÀãááÚØÙ (Russian)
        "",                                                             // Hrvatski (Croatian)
        "",                                                             // Eesti
        "",                                                             // Dansk
    },
    {
    "Setup.DVD$Preferred menu language",                    // English
        "Bevorzugte Spache für Menüs",                      // Deutsch
        "",                                                 // Slovenski
        "Lingua preferita per il Menu",                      // Italiano
        "Voorkeurs taal voor menu",                          // Nederlands
        "",                                                 // Português
        "Langage préféré pour les menus",                   // Français
        "",                                                 // Norsk
        "Haluttu valikkokieli",                             // suomi
        "",                                                 // Polski
        "Idioma preferido para los menús",                  // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "",                                                 // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    {
    "Setup.DVD$Preferred audio language",                   // English
        "Bevorzugte Sprache für Dialog",                    // Deutsch
        "",                                                 // Slovenski
        "Lingua preferita per l'Audio",                     // Italiano
        "Voorkeurs taal voor audio",                        // Nederlands
        "",                                                 // Português
        "Langage préféré pour le son",                      // Français
        "",                                                 // Norsk
        "Haluttu ääniraita",                                // suomi
        "",                                                 // Polski
        "Idioma preferido para el sonido",                  // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "",                                                 // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    {
    "Setup.DVD$Preferred subtitle language",                // English
        "Bevorzugte Spache für Untertitel",                 // Deutsch
        "",                                                 // Slovenski
        "Lingua preferita per i sottotitoli",               // Italiano
        "Voorkeurs taal voor ondertitels",                   // Nederlands
        "",                                                 // Português
        "Langage préféré pour les sous-titres",             // Français
        "",                                                 // Norsk
        "Haluttu tekstityskieli",                           // suomi
        "",                                                 // Polski
        "Idioma preferido para los subtítulos",             // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "",                                                 // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    {
    "Setup.DVD$Player region code",                         // English
        "Regions Code für Player",                          // Deutsch
        "",                                                 // Slovenski
        "Codice regionale del lettore DVD",                       // Italiano
        "Regio code van Speler",                             // Nederlands
        "",                                                 // Português
        "Code région du lecteur",                           // Français
        "",                                                 // Norsk
        "Soittimen aluekoodi",                              // suomi
        "",                                                 // Polski
        "Código de zona del lector",                        // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "",                                                 // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    {
    "Setup.DVD$Display subtitles",                          // English
        "Untertitel anzeigen",                              // Deutsch
        "",                                                 // Slovenski
        "Visualizza sottotitoli",                           // Italiano
        "Toon ondertitels",                                 // Nederlands
        "",                                                 // Português
        "Affiche les sous-titres",                          // Français
        "",                                                 // Norsk
        "Näytä tekstitys",                                  // suomi
        "",                                                 // Polski
        "Mostrar subtítulos",                               // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "",                                                 // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    {
     "Setup.DVD$Hide Mainmenu Entry",
        "Hauptmenüeintrag verstecken",                      // Deutsch
        "",                                                 // Slovenski
        "Nascondi voce menù",                               // Italiano
        "Verberg Hoofdmenu ingave",                         // Nederlands
        "",                                                 // Português
        "",                                                 // Français
        "",                                                 // Norsk
        "Piilota valinta päävalikosta",                     // suomi
        "",                                                 // Polski
        "",                                                 // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "ÁÚàëâì ÚŞÜĞİÔã Ò ÓÛĞÒİŞÜ ÜÕİî",                    // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    {
     "Setup.DVD$ReadAHead",
        "ReadAHead",                                        // Deutsch
        "ReadAHead",                                        // Slovenski
        "ReadAHead",                                        // Italiano
        "ReadAHead",                                        // Nederlands
        "ReadAHead",                                        // Português
        "ReadAHead",                                        // Français
        "ReadAHead",                                        // Norsk
        "ReadAHead-toiminto",                               // suomi
        "ReadAHead",                                        // Polski
        "ReadAHead",                                        // Español
        "ReadAHead",                                        // ÅëëçíéêÜ (Greek)
        "ReadAHead",                                        // Svenska
        "ReadAHead",                                        // Romaneste
        "ReadAHead",                                        // Magyar
        "ReadAHead",                                        // Català
        "ReadAHead",                                        // ÀãááÚØÙ (Russian)
        "ReadAHead",                                        // Hrvatski (Croatian)
        "ReadAHead",                                        // Eesti
        "ReadAHead",                                        // Dansk
    },
    {
     "Setup.DVD$Gain (analog)",
     "Verstärkung (analog)",                                // Deutsch
     "Gain (analog)",                                       // Slovenski
     "Guadagno (analogico)",                                       // Italiano
     "Gain (analog)",                                       // Nederlands
     "Gain (analog)",                                       // Português
     "Gain (analog)",                                       // Français
     "Gain (analog)",                                       // Norsk
     "Äänen vahvistus (analoginen)",                        // suomi
     "Gain (analog)",                                       // Polski
     "Gain (analog)",                                       // Español
     "Gain (analog)",                                       // ÅëëçíéêÜ (Greek)
     "Gain (analog)",                                       // Svenska
     "Gain (analog)",                                       // Romaneste
     "Gain (analog)",                                       // Magyar
     "Gain (analog)",                                       // Català
     "Gain (analog)",                                       // ÀãááÚØÙ (Russian)
     "Gain (analog)",                                       // Hrvatski (Croatian)
     "Gain (analog)",                                       // Eesti
     "Gain (analog)",                                       // Dansk
    },
    {
    "Error.DVD$Error opening DVD!",                         // English
        "Fehler beim öffnen der DVD!",                      // Deutsch
        "",                                                 // Slovenski
        "Errore di apertura DVD",                                                 // Italiano
        "Fout bij openen van de DVD",                       // Nederlands
        "",                                                 // Português
        "",                                                 // Français
        "",                                                 // Norsk
        "DVD:n avaaminen epäonnistui!",                     // suomi
        "",                                                 // Polski
        "",                                                 // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "",                                                 // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    {
    "Error.DVD$Error fetching data from DVD!",              // English
        "Fehler beim lesen von der DVD!",                   // Deutsch
        "",                                                 // Slovenski
        "Errore lettura dati DVD",                                                 // Italiano
        "Fout bij het lezen van de DVD",                 // Nederlands
        "",                                                 // Português
        "",                                                 // Français
        "",                                                 // Norsk
        "DVD:n lukeminen epäonnistui",                      // suomi
        "",                                                 // Polski
        "",                                                 // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "",                                                 // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    {
    "Error.DVD$Current subp stream not seen!",              // English
        "Der ausgewählte Untertitel ist nicht vorhanden!",  // Deutsch
        "",                                                 // Slovenski
        "I sottotitoli selezionati non esistono!",          // Italiano
        "De gekozen ondertitels niet gevonden",             // Nederlands
        "",                                                 // Português
        "",                                                 // Français
        "",                                                 // Norsk
        "Tekstitysraitaa ei havaita!",                      // suomi
        "",                                                 // Polski
        "",                                                 // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "",                                                 // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    {
    "Error.DVD$Current audio track not seen!",              // English
        "Die ausgewählte Audiospur ist nicht vorhanden!",   // Deutsch
        "",                                                 // Slovenski
        "La traccia audio selezionata non esiste!",         // Italiano
        "Het gekozen audiospoor niet gevonden",             // Nederlands
        "",                                                 // Português
        "",                                                 // Français
        "",                                                 // Norsk
        "Ääniraitaa ei havaita!",                           // suomi
        "",                                                 // Polski
        "",                                                 // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "",                                                 // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    {
    "DVD Player",              // English
        "DVD Spieler",   // Deutsch
        "",                                                 // Slovenski
        "",         // Italiano
        "",             // Nederlands
        "",                                                 // Português
        "",                                                 // Français
        "",                                                 // Norsk
        "",                           // suomi
        "",                                                 // Polski
        "",                                                 // Español
        "",                                                 // ÅëëçíéêÜ (Greek)
        "",                                                 // Svenska
        "",                                                 // Romaneste
        "",                                                 // Magyar
        "",                                                 // Català
        "",                                                 // ÀãááÚØÙ (Russian)
        "",                                                 // Hrvatski (Croatian)
        "",                                                 // Eesti
        "",                                                 // Dansk
    },
    { NULL }
};
