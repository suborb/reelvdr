/*
 * i18n.c: Internationalization
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: i18n.c,v 1.4 2005/01/01 22:11:13 schmitzj Exp $
 *
 */

#include "i18n.h"

const tI18nPhrase tlPhrases[] = {
  { "Timer Timeline", // English
    "Timer Zeitleiste", // Deutsch
    "", // Slovenski
    "Tabella Timers", // Italiano
    "Timers tijdlijst", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "Aikajana", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { "Show timer overview and collisions", // English
    "", // Deutsch
    "", // Slovenski
    "Mostra controllo Iimer ed eventuali conflitti", // Italiano
    "Toon timers overzicht en conflicten", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "Näyttää päällekkäiset ajastimet", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { "No conflicts", // English
    "Keine Konflikte", // Deutsch
    "", // Slovenski
    "Non ci sono conflitti", // Italiano
    "Geen conflicten", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "Ei päällekkäisyyksiä", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { "Conflict on", // English
    "Konflikt am", // Deutsch
    "", // Slovenski
    "Conflitto al", // Italiano
    "Conflict op", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "Päällekkäisyys", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { "Repeating conflict on", // English
    "Wiederh. Konflikt an", // Deutsch
    "", // Slovenski
    "Conflitto ripetuto al", // Italiano
    "Herhaald conflict op", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "Toistuva päällekkäisyys", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { "same input device", // English
    "gleiche Eingangs-Device", // Deutsch
    "", // Slovenski
    "stessa periferica in ingresso", // Italiano
    "Zelfde ingangsdevice", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "sama vastaanotin", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { "(today)", // English
    "(heute)", // Deutsch
    "", // Slovenski
    "(oggi)", // Italiano
    "(vandaag)", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "(tänään)", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { "Cursor up/down/left/right+Nums", // English
    "Taste auf/ab/links/rechts+Zahlen", // Deutsch
    "", // Slovenski
    "Cursore su/giu'/sinistra/destra+Numeri", // Italiano
    "Cursor op/neer/links/rechts+nummers", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "Ylös/Alas/Vasen/Oikea/Numeronäppäimet", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { "Ignore primary interface", // English
    "Ignoriere primäres Interface", // Deutsch
    "", // Slovenski
    "Ignorare interfaccia primaria", // Italiano
    "Negeer primaire interface", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "Jätä päävastaanotin huomioimatta", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { "Prio", // English
    "Priorität", // Deutsch
    "", // Slovenski
    "Priorita'", // Italiano
    "Prioriteit", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "prioriteetti", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { "Timeline", // English
    "Timer Zeitleiste", // Deutsch
    "", // Slovenski
    "", // Italiano
    "", // Nederlands
    "", // Português
    "", // Français
    "", // Norsk
    "", // suomi
    "", // Polski
    "", // Español
    "", // ÅëëçíéêÜ
    "", // Svenska
    "", // Romaneste
    "", // Magyar
    "", // Català
    ""  // ÀãááÚØÙ (Russian)
    "", // Hrvatski (Croatian)
    "", // Eesti
    "", // Dansk
    "",
  },
  { NULL }
  };
