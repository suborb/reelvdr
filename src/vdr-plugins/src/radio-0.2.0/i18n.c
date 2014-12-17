/*
 * i18n.c: Internationalization
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "i18n.h"

const tI18nPhrase Phrases[] = {
  { "RDS-Radiotext",	// English
    "RDS-Radiotext",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "RDS",		// Français
    "",// Norsk
    "RDS-teksti",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Radio Background-Image/RDS-Text",			// English
    "Hintergr.Bilder/RDS-Text für Radiosender",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Image de fond pour les radio/RDS-Texte",		// Français
    "",// Norsk
    "Taustakuva ja RDS-teksti radiokanaville",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Show RDS-Radiotext",	// English
    "Zeige RDS-Radiotext",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Afficher RDS",		// Français
    "",// Norsk
    "Näytä RDS-teksti",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Radiotext",	// English
    "Radiotext",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "RDS",		// Français
    "",// Norsk
    "RDS-teksti",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "  [waiting ...]",	// English
    "  [warte ...]",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "  [attente ...]",	// Français
    "",// Norsk
    "  [odota ...]",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Activate",		// English
    "RDSText einschalten",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Activer",		// Français
    "",// Norsk
    "Aktiivinen",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText Function",		// English
    "RDSText Funktion",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Fonction de RDS-Texte",	// Français
    "",// Norsk
    "RDS-tekstin toiminto",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Position",		// English
    "RDSText Position",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Position OSD de RDS-Texte",	// Français
    "",// Norsk
    "RDS-tekstin sijainti",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Titlerow",	// English
    "RDSText Titelzeile",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Titre OSD de RDS-Texte",	// Français
    "",// Norsk
    "RDS-tekstin kappale",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Rows (1-5)",		// English
    "RDSText Zeilen (1-5)",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Ligne OSD de RDS-Texte (1-5)",	// Français
    "",// Norsk
    "RDS-tekstin rivimäärä",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Scrollmode",		// English
    "RDSText Scrollmodus",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Mode Scroll OSD de RDS-Texte ",	// Français
    "",// Norsk
    "RDS-tekstin vieritystapa",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Taginfo",		// English
    "RDSText Taginformation",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Tag info OSD de RDS-Texte",	// Français
    "",// Norsk
    "RDS-tekstin tunnistetiedot",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Backgr.Color",		// English
    "RDSText Hintergrundfarbe",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Couleur de fond OSD de RDS-Texte",	// Français
    "",// Norsk
    "RDS-tekstin taustaväri",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Backgr.Transp.",		// English
    "RDSText Hintergr.Transparenz",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Fond transparent OSD de RDS-Texte",	// Français
    "",// Norsk
    "RDS-tekstin taustan läpinäkyvyys",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Foregr.Color",			// English
    "RDSText Textfarbe",			// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Couleur du texte OSD de RDS-Texte",	// Français
    "",// Norsk
    "RDS-tekstin väri",				// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Display",		// English
    "RDSText Anzeige",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Affichage OSD de RDS-Texte",	// Français
    "",// Norsk
    "RDS-tekstin esitys",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Hide MainMenuEntry",			// English
    "Verstecke Hauptmenu-Eintrag",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Cacher l'entrée dans le menu principal",	// Français
    "",// Norsk
    "Piilota valinta päävalikosta",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Top",		// English
    "Oben",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Haut",		// Français
    "",// Norsk
    "yläreuna",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Bottom",		// English
    "Unten",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Bas",		// Français
    "",// Norsk
    "alareuna",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Black",		// English
    "Schwarz",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Noir",		// Français
    "",// Norsk
    "musta",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "White",		// English
    "Weiss",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Blanc",		// Français
    "",// Norsk
    "valkoinen",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Red",		// English
    "Rot",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Rouge",		// Français
    "",// Norsk
    "punainen",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Green",		// English
    "Grün",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Vert",		// Français
    "",// Norsk
    "vihreä",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Yellow",		// English
    "Gelb",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Jaune",		// Français
    "",// Norsk
    "keltainen",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Magenta",		// English
    "Magenta",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Magenta",		// Français
    "",// Norsk
    "magenta",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Blue",		// English
    "Blau",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Bleu",		// Français
    "",// Norsk
    "sininen",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Cyan",		// English
    "Cyan",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Cyan",		// Français
    "",// Norsk
    "syaani",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Transparent",	// English
    "Transparent",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Transparent",	// Français
    "",// Norsk
    "läpinäkyvä",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Off",		// English
    "Aus",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Off",		// Français
    "",// Norsk
    "pois",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "about MainMenu",			// English
    "über Hauptmenü",			// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "A propos de menu principal",	// Français
    "",// Norsk
    "päävalikosta",			// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Automatic",	// English
    "Automatisch",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Automatique",	// Français
    "",// Norsk
    "automaattisesti",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDS-Text AutoOSD active",		// English
    "RDS-Text AutoOSD aktiviert",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "AutoOSD actif de RDS-Texte",	// Français
    "",// Norsk
    "RDS-tekstin esitys aktivoitu",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "only Text",		// English
    "nur Text",			// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Seulement le texte",	// Français
    "",// Norsk
    "vain teksti",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Text+TagInfo",		// English
    "Text+TagInfos",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Texte+Tag d'info",		// Français
    "",// Norsk
    "teksti+tunniste",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Artist :",		// English
    "Interpret :",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Artiste :",	// Français
    "",// Norsk
    "Esittäjä   :",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Title  :",		// English
    "Titel        :",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Titre :",		// Français
    "",// Norsk
    "Kappale :",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "only, if some",		// English
    "nur, wenn vorhanden",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Seulement, si disponible",	// Français
    "",// Norsk
    "jos saatavilla",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "always",		// English
    "immer",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Toujours",		// Français
    "",// Norsk
    "aina",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "latest at Top",	// English
    "aktuelle Oben",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "En haut",		// Français
    "",// Norsk
    "ylöspäin",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "latest at Bottom",	// English
    "aktuelle Unten",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "En bas",		// Français
    "",// Norsk
    "alaspäin",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "unknown program type",		// English
    "Unbekannte Programmart",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Type de programme inconnu",	// Français
    "",// Norsk
    "tuntematon ohjelmatyyppi",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "News",		// English
    "Nachrichten",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Nouvelles",	// Français
    "",// Norsk
    "uutiset",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Current affairs",		// English
    "Aktuelles",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Affaires courantes",	// Français
    "",// Norsk
    "ajankohtaista",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Information",	// English
    "Info",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Information",	// Français
    "",// Norsk
    "tiedote",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Sport",		// English
    "Sport",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Sport",		// Français
    "",// Norsk
    "urheilua",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Education",	// English
    "Bildung",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Education",	// Français
    "",// Norsk
    "opetusohjelmaa",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Drama",		// English
    "Hörspiel",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Drame",		// Français
    "",// Norsk
    "draamaa",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Culture",		// English
    "Kultur",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Culture",		// Français
    "",// Norsk
    "kulttuuria",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Science",		// English
    "Wissenschaft",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Science",		// Français
    "",// Norsk
    "tiedeohjelma",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Varied",		// English
    "Diverses",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Divers",		// Français
    "",// Norsk
    "sekalaista",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Pop music",	// English
    "Popmusik",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Musique Pop",	// Français
    "",// Norsk
    "pop-musiikkia",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Rock music",	// English
    "Rockmusik",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Musique Rock",	// Français
    "",// Norsk
    "rock-musiikkia",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "M.O.R. music", 		// English
    "Easy Listening u.ä.",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Musique M.O.R.",		// Français
    "",// Norsk
    "taukomusiikkia",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Light classical",		// English
    "Leichte Klassik",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Classique facile",		// Français
    "",// Norsk
    "kevyttä klassista",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Serious classical",	// English
    "Ernste Klassik",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Classique sérieux",	// Français
    "",// Norsk
    "klassista",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Other music",		// English
    "Sonstige Musik",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Autre musique",		// Français
    "",// Norsk
    "musiikkia",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Alarm",		// English
    "Alarm",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Alarme",		// Français
    "",// Norsk
    "hätätiedote",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Timeout (0-1440 min)",		// English
    "RDSText OSD-Timeout (0-1440 min)",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Timeout OSD de RDS-Texte (0-1440 min)",	// Français
    "",// Norsk
    "RDS-tekstin odotusaika (0-1440min)",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "min. timeout",	// English
    "Min. Timeout",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Min. timeout",	// Français
    "",// Norsk
    "min. odotusaika",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "no timeout",	// English
    "kein Timeout",     // Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Pas de timeout",	// Français
    "",// Norsk
    "ei odotusaikaa",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText StatusMsg (lcdproc & co)",			// English
    "RDSText StatusMeld. (lcdproc & co)",     		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Statut message de RDS-Texte (lcdproc & co)",	// Français
    "",// Norsk
    "RDS-tekstin toiminto laajennoksille",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "only Taginfo",		// English
    "nur TagInfos",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Seulement Tag Info",	// Français
    "",// Norsk
    "vain tunniste",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDS-Text: OSD Timeout, disabled ",		// English
    "RDS-Text: OSD Timeout, deaktiviert",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "RDS-Texte : Désactiver timeout OSD",	// Français
    "",// Norsk
    "RDS-teksti: ei odotusaikaa",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RTplus",		// English
    "RTplus",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "RTplus",		// Français
    "",// Norsk
    "RTplus",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Playlist",		// English
    "Titelliste",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Playlist",		// Français
    "",// Norsk
    "Soittolista",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Sports",		// English
    "Sport",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Sports",		// Français
    "",// Norsk
    "Urheilu",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Weather",		// English
    "Wetter",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Météo",		// Français
    "",// Norsk
    "Sää",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Stockmarket",	// English
    "Bösenkurse",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Bourse",		// Français
    "",// Norsk
    "Pörssikurssit",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Other",		// English
    "Sonstiges",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Autre",		// Français
    "",// Norsk
    "Sekalaiset",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Radiotext",	// English
    "Radiotext",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Radio Texte",	// Français
    "",// Norsk
    "Radioteksti",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RTplus Memory  since",	// English
    "RTplus Speicher  seit",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Mémoire de RTplus depuis",	// Français
    "",// Norsk
    "RTplus-muisti  alkaen",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Programme",	// English
    "Pogramminfo",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Programme",	// Français
    "",// Norsk
    "Ohjelma",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Station",		// English
    "Sendername",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Station",		// Français
    "",// Norsk
    "Asema",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Now",		// English
    "Jetzt",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Maintenant",	// Français
    "",// Norsk
    "Nyt",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "...Part",		// English
    "...Detail",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "...Détail",	// Français
    "",// Norsk
    "...osa",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Next",		// English
    "Demnächst",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Suivant",		// Français
    "",// Norsk
    "Seuraavaksi",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Host",		// English
    "Moderator",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Animateur",	// Français
    "",// Norsk
    "Juontaja",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Edit.Staff",	// English
    "Person(en)",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Personne",		// Français
    "",// Norsk
    "Henkilökunta",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Homepage",		// English
    "Homepage",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Page d'accueil",	// Français
    "",// Norsk
    "Kotisivu",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Interactivity",		// English
    "Interaktiv (tu' was :)",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Interactivité",		// Français
    "",// Norsk
    "Interaktiivinen",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Phone-Hotline",		// English
    "Tel.-Hotline",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Téléphone hotline",	// Français
    "",// Norsk
    "Suoralinja puhelimelle",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Phone-Studio",		// English
    "Tel.-Studio",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Téléphone studio",		// Français
    "",// Norsk
    "Puhelin studioon",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Email-Hotline",		// English
    "EMail-Hotline",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "E-mail hotline",		// Français
    "",// Norsk
    "Suoralinja sähköpostille",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Email-Studio",		// English
    "EMail-Studio",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "E-mail studio",		// Français
    "",// Norsk
    "Sähköposti studioon",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Info",			// English
    "weitere Information",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "D'autres information",	// Français
    "",// Norsk
    "Lisätiedot",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "News",		// English
    "Nachrichten",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Informations",	// Français
    "",// Norsk
    "Uutiset",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "NewsLocal",		// English
    "Nachricht.Lokal",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Information local",	// Français
    "",// Norsk
    "Paikallisuutiset",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "DateTime",		// English
    "Datum-Zeit",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Date-Heure",	// Français
    "",// Norsk
    "Ajankohtaista",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Traffic",		// English
    "Verkehr",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Traffic",		// Français
    "",// Norsk
    "Liikenne",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Alarm",		// English
    "Alarm (!)",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Alamre",		// Français
    "",// Norsk
    "Hälytys",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Advertising",	// English
    "Hinweis/Reklame",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Publicité",	// Français
    "",// Norsk
    "Mainos",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Url",		// English
    "Url/Webseite",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Url",		// Français
    "",// Norsk
    "Linkki",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Exit",		// English
    "Beenden",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Sortir",		// Français
    "",// Norsk
    "Lopeta",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Time",		// English
    "Zeit",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Temps",		// Français
    "",// Norsk
    "Kellonaika",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Title",		// English
    "Titel",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Titre",		// Français
    "",// Norsk
    "Kappale",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Artist",		// English
    "Interpret",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Artiste",		// Français
    "",// Norsk
    "Esittäjä",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "last seen Radiotext",		// English
    "die letzten Radiotexte",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Dernier Radio-texte",		// Français
    "",// Norsk
    "viimeksi nähty radioteksti",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Refresh Off",		// English
    "Aktualis. Aus",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Arrêter actualisation",	// Français
    "",// Norsk
    "Älä päivitä",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Refresh On",		// English
    "Aktualis. Ein",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Démarrer actualisation",	// Français
    "",// Norsk
    "Päivitä",			// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Back",		// English
    "Zurück",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Retour",		// Français
    "",// Norsk
    "Takaisin",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSplus Memorynumber (10-99)",	// English
    "RDSplus Speicheranzahl (10-99)",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Mémoire RDSplus (10-99)",		// Français
    "",// Norsk
    "RDSplus-muistipaikka (10-99)",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RTplus-File saved",		// English
    "RTplus-Datei gespeichert",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Sauvé fichier TTplus",		// Français
    "",// Norsk
    "RTplus-tiedosto tallennettu",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText OSD-Skincolors used",		// English
    "RDSText OSD-Skinfarben benutzen",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Couleur du skin utilisé de RDS-texte",	// Français
    "",// Norsk
    "Käytä RDS-tekstille ulkoasun värejä",	// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Lottery",		// English
    "Lotterie",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "Loterie",		// Français
    "",// Norsk
    "Arvonta",		// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDS-RaSS: Records closed, Slideshow activ",	// English
    "RDS-RaSS: Archiv geschlossen, Bildlauf aktiv",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "",// Français
    "",// Norsk
    "",// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDS-RaSS: Slideshow detected & startet",		// English
    "RDS-RaSS: Bildlauf gefunden und gestartet",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "",// Français
    "",// Norsk
    "",// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "[0] Contents",	// English
    "[0] Inhalt",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "",// Français
    "",// Norsk
    "",// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "Records",		// English
    "Archiv",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "",// Français
    "",// Norsk
    "",// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "with <0>",		// English
    "mit  <0>",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "",// Français
    "",// Norsk
    "",// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RDSText RaSS-Function",		// English
    "RDSText RaSS-Funktion",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "",// Français
    "",// Norsk
    "",// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RaSS only",	// English
    "RaSS alleine",	// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "",// Français
    "",// Norsk
    "",// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { "RaSS+Text mixed",		// English
    "Text über RaSS",		// Deutsch
    "",// Slovenski
    "",// Italiano
    "",// Nederlands
    "",// Português
    "",// Français
    "",// Norsk
    "",// suomi
    "",// Polski
    "",// Español
    "",// ÅëëçíéêÜ
    "",// Svenska
    "",// Romaneste
    "",// Magyar
    "",// Català
    "" // ÀãááÚØÙ
  },
  { NULL }
};
