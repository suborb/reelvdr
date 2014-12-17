/*
 * i18n.c: Internationalization
 *
 * See the README file for copyright information and how to reach the author.
 * Russian: Vyacheslav Dikonov <sdiconov@mail.ru>
 *
 */

#include "i18n.h"

const tI18nPhrase Phrases[] = {
  { "Drive West",
    "Nach Westen",
    "",// TODO
    "",// TODO
    "Naar West",
    "",// TODO
    "Aller Ouest",
    "",// TODO
    "Ohjaa l‰nteen",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Ω– ◊–ﬂ–‘",
  },
  { "Drive East",
    "Nach Osten",
    "",// TODO
    "",// TODO
    "Naar Oost",
    "",// TODO
    "Aller Est",
    "",// TODO
    "Ohjaa it‰‰n",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Ω– “ﬁ·‚ﬁ⁄",
  },
  { "Halt",
    "Anhalten",
    "",// TODO
    "",// TODO
    "Stop",
    "",// TODO
    "ArrÍt",
    "",// TODO
    "Pys‰yt‰",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "¡‚ﬁﬂ",
  },
  { "Steps",
    "Schritte",
    "",// TODO
    "",// TODO
    "Stappen",
    "",// TODO
    "IncrÈment",
    "",// TODO
    "askelta",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "Ë–”(ﬁ“) ›–",
  },
  { "Recalc",
    "Neuberechnen",
    "",// TODO
    "",// TODO
    "Herbereken",
    "",// TODO
    "Recalc",
    "",// TODO
    "Laske uudelleen",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "ø’‡’·Áÿ‚–‚Ï",
  },
  { "Goto",
    "Gehe zu",
    "",// TODO
    "",// TODO
    "Ga naar",
    "",// TODO
    "Aller ‡",
    "",// TODO
    "Siirry",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "ø’‡’Ÿ‚ÿ",
  },
  { "Store",
    "Speichere",
    "",// TODO
    "",// TODO
    "Opslaan",
    "",// TODO
    "Enregistrer",
    "",// TODO
    "Tallenna",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "∑–ﬂﬁ‹›ÿ‚Ï",
  },
  { "Limits off",
    "Limits aus",
    "",// TODO
    "",// TODO
    "Limiet uit",
    "",// TODO
    "DÈsactiver Limites",
    "",// TODO
    "Ei rajoituksia",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "æ‚⁄€ÓÁÿ‚Ï ﬂ‡’‘’€Î",
  },
  { "Set East Limit",
    "Ost-Limit",
    "",// TODO
    "",// TODO
    "Zet Oost Limiet",
    "",// TODO
    "Limite Est",
    "",// TODO
    "Aseta rajoitus it‰‰n",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "≤ﬁ·‚ﬁÁ›ÎŸ ﬂ‡’‘’€",
  },
  { "Set West Limit",
    "West-Limit",
    "",// TODO
    "",// TODO
    "Zet West Limiet",
    "",// TODO
    "Limite Ouest",
    "",// TODO
    "Aseta rajoitus l‰nteen",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "∑–ﬂ–‘›ÎŸ ﬂ‡’‘’€",
  },
  { "Enable Limits",
    "Limits ein",
    "",// TODO
    "",// TODO
    "Limiet's in",
    "",// TODO
    "Activer Limites",
    "",// TODO
    "K‰yt‰ rajoituksia",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "≤⁄€ÓÁÿ‚Ï ﬂ‡’‘’€Î",
  },
  { "Position",
    "Position",
    "",// TODO
    "",// TODO
    "Positie",
    "",// TODO
    "",
    "",// TODO
    "Sijainti",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",
  },
  { "Tuner",
    "Tuner",
    "",// TODO
    "",// TODO
    "Tuner",
    "",// TODO
    "",
    "",// TODO
    "",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",
  }, 
  { "Card, connected with motor",
    "Karte, die mit Motor verbunden ist",
    "",// TODO
    "",// TODO
    "Kaart, met motor verbonden",
    "",// TODO
    "Carte, connectÈe au moteur",
    "",// TODO
    "Moottoriin kytketty DVB-kortti",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "∫–‡‚– ·ﬁ’‘ÿ›Ò››–Ô · ﬂﬁ‘“’·ﬁ‹",
  },
  { "Repeat DiSEqC-Commands?",
    "DiSEqC-Befehle wiederholen?",
    "",// TODO
    "",// TODO
    "DiSEqC-Commando's herhalen?",
    "",// TODO
    "RÈpÈter Commandes DiSEqc?",
    "",// TODO
    "Toista DiSEqC-komennot",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "øﬁ“‚ﬁ‡Ô‚Ï ⁄ﬁ‹–›‘Î DiSEqC?",
  },
  { "Are you sure?",
    "Sind sie sicher?",
    "",// TODO
    "",// TODO
    "Is het zeker?",
    "",// TODO
    "Etes vous sur?",
    "",// TODO
    "Oletko varma?",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "≤Î „“’‡’›Î?",
  },
  { "Not in DISEQC.CONF",
    "Kein Eintrag in DISEQC.CONF",
    "",// TODO
    "",// TODO
    "Niet in DISEQC.CONF",
    "",// TODO
    "Pas dans DISEQC.CONF",
    "",// TODO
    "Ei lˆydy DISEQC.CONF-tiedostosta",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "æ‚·„‚·‚“„’‚ “ diseqc.conf",
  },
  { "Frequency",
    "Frequenz",
    "",// TODO
    "",// TODO
    "Frequentie",
    "",// TODO
    "FrÈquence",
    "",// TODO
    "Taajuus",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "«–·‚ﬁ‚–",
  },
  { "Symbolrate",
    "Symbolrate",
    "",// TODO
    "",// TODO
    "Symbolrate",
    "",// TODO
    "SymbolRate",
    "",// TODO
    "Symbolinopeus",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "¡ÿ‹“. ·⁄ﬁ‡ﬁ·‚Ï",
  },
  { "Scan",
    "Scannen",
    "",// TODO
    "",// TODO
    "Scannen",
    "",// TODO
    "",
    "",// TODO
    "",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",
  },
  { "Rotor",
    "Rotor",
    "",// TODO
    "",// TODO
    "Rotor",
    "",// TODO
    "Motorisation",
    "",// TODO
    "Moottori",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "ºﬁ‚ﬁﬂﬁ‘“’·",
  },
  { "Steuerung eines Rotors",
    "Steuerung eines Rotors",
    "",// TODO
    "",// TODO
    "Aansturing van een Rotor",
    "",// TODO
    "",// TODO
    "",// TODO
    "Moottorin ohjaus",
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "",// TODO
    "√ﬂ‡–“€’›ÿ’ ‹ﬁ‚ﬁﬂﬁ‘“’·ﬁ‹",
  },
  { "My Longitude",
    "Mein L‰ngengrad",
    "",
    "",
    "Lengtegraad",
    "",
    "Longitude",
    "",
    "Pituusaste",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "¥ﬁ€”ﬁ‚–",
  },
  { "East",
    "Ost",
    "",
    "",
    "Oost",
    "",
    "Est",
    "",
    "it‰ist‰",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "≤ﬁ·‚ﬁ⁄",
  },
  { "To East",
    "Nach Osten",
    "",
    "",
    "Naar Oost",
    "",
    "Est",
    "",
    "It‰‰n",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "≤ﬁ·‚ﬁ⁄",
  },
  { "West",
    "West",
    "",
    "",
    "West",
    "",
    "Ouest",
    "",
    "l‰ntist‰",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "∑–ﬂ–‘",
  },
  { "To West",
    "Nach Westen",
    "",
    "",
    "Naar West",
    "",
    "Ouest",
    "",
    "L‰nteen",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "∑–ﬂ–‘",
  },
  { "My Latitude",
    "Mein Breitengrad",
    "",
    "",
    "Breedtegraad",
    "",
    "Latitude",
    "",
    "Leveysaste",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "»ÿ‡ﬁ‚–",
  },
  { "South",
    "S¸d",
    "",
    "",
    "Zuid",
    "",
    "Sud",
    "",
    "etel‰ist‰",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Œ”",
  },
  { "North",
    "Nord",
    "",
    "",
    "Noord",
    "",
    "Nord",
    "",
    "pohjoista",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "¡’“’‡",
  },
  { "Rotating Dish",
    "Sch¸ssel dreht sich",
    "",
    "",
    "Schotel Draait",
    "",
    "",
    "",
    "K‰‰nnet‰‰n lautasta",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "≤Îﬂﬁ€›Ô’‚·Ô ﬂﬁ“ﬁ‡ﬁ‚ –›‚’››Î",
  },
  { "Add Channel",
    "Add Channel",
    "",
    "",
    "Voeg Kanaal Toe",
    "",
    "",
    "",
    "Lis‰‰ kanava",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Add All",
    "Add All",
    "",
    "",
    "Voeg Allen Toe",
    "",
    "",
    "",
    "Lis‰‰ kaikki",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Satellite-Table",
    "Satelliten-Tabelle",
    "",
    "",
    "Satelliet-Tabelle",
    "",
    "",
    "",
    "Satelliittitaulukko",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Tuners connected with the steerable dish",
    "Tuner die mit der drehbaren Sch¸ssel verbunden sind",
    "",
    "",
    "Tuner's verbonden met de draaibare Schotel",
    "",
    "",
    "",
    "Moottoroituun lautaseen kytketyt kortit",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Satellite",
    "Satellit",
    "",
    "",
    "Satelliet",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Satellite number",
    "Satelliten-Nummer",
    "",
    "",
    "Satelliet nummer",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Type of Control",
    "Steuerungsart",
    "",
    "",
    "Kontrole type",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Motor Control",
    "Motor-Einstellung",
    "",
    "",
    "Motor Kontrole",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Move",
    "Bewegung",
    "",
    "",
    "Verplaats",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Satellite Angle",
    "Satelliten Position",
    "",
    "",
    "Satelliet Positie",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "User",
    "Benutzer",
    "",
    "",
    "Gebruiker",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Installer",
    "Einrichter",
    "",
    "",
    "Installeer",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Goto Reference Position",
    "Referenz-Position",
    "",
    "",
    "Ga naar referentie punt",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "Goto Angular Position",
    "Drehe zur Position",
    "",
    "",
    "Ga naar positie",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "inactive",
    "inaktiv",
    "",
    "",
    "inaktief",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { "shared LNB",
    "mitbenutztes LNB",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
  },
  { NULL }
  };

