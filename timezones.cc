#include <config.h>
#include "timezones.h"

using namespace std;

/* --- timezone names ------------------------------------------------------ */

map<string,int> timezones;

void init_timezones() {
  // from http://www.timeanddate.com/library/abbreviations/timezones/
  timezones["A"] = 60 * 60; // Alpha Time Zone (Military)
  timezones["ACDT"] = +(10 * 60 + 30) * 60; // Australian Central Daylight Time (Australia)
  timezones["ACST"] = +(9 * 60 + 30) * 60; // Australian Central Standard Time (Australia)
  timezones["ADT"] = -3 * 60; // Atlantic Daylight Time (North America)
  timezones["AEDT"] = +11 * 60; // Australian Eastern Daylight Time or Australian Eastern Summer Time (Australia)
  timezones["AEST"] = +10 * 60; // Australian Eastern Standard Time (Australia)
  timezones["AKDT"] = -8 * 60; // Alaska Daylight Time (North America)
  timezones["AKST"] = -9 * 60; // Alaska Standard Time (North America)
  timezones["AST"] = -4 * 60; // Atlantic Standard Time (North America)
  timezones["AWDT"] = +9 * 60; // Australian Western Daylight Time (Australia)
  timezones["AWST"] = +8 * 60; // Australian Western Standard Time (Australia)
  timezones["B"] = +2 * 60; // Bravo Time Zone (Military)
  timezones["BST"] = 60 * 60; // British Summer Time (Europe)
  timezones["C"] = +3 * 60; // Charlie Time Zone (Military)
  timezones["CDT"] = +(10 * 60 + 30) * 60; // Central Daylight Time (Australia)
  timezones["CDT"] = -5 * 60; // Central Daylight Time (North America)
  timezones["CEDT"] = +2 * 60; // Central European Daylight Time (Europe)
  timezones["CEST"] = +2 * 60; // Central European Summer Time (Europe)
  timezones["CET"] = 60 * 60; // Central European Time (Europe)
  timezones["CST"] = +(10 * 60 + 30) * 60; // Central Summer Time (Australia)
  timezones["CST"] = +(9 * 60 + 30) * 60; // Central Standard Time (Australia)
  timezones["CST"] = -6 * 60; // Central Standard Time (North America)
  timezones["CXT"] = +7 * 60; // Christmas Island Time (Australia)
  timezones["D"] = +4 * 60; // Delta Time Zone (Military)
  timezones["E"] = +5 * 60; // Echo Time Zone (Military)
  timezones["EDT"] = +11 * 60; // Eastern Daylight Time (Australia)
  timezones["EDT"] = -4 * 60; // Eastern Daylight Time (North America)
  timezones["EEDT"] = +3 * 60; // Eastern European Daylight Time (Europe)
  timezones["EEST"] = +3 * 60; // Eastern European Summer Time (Europe)
  timezones["EET"] = +2 * 60; // Eastern European Time (Europe)
  timezones["EST"] = +11 * 60; // Eastern Summer Time (Australia)
  timezones["EST"] = +10 * 60; // Eastern Standard Time (Australia)
  timezones["EST"] = -5 * 60; // Eastern Standard Time (North America)
  timezones["F"] = +6 * 60; // Foxtrot Time Zone (Military)
  timezones["G"] = +7 * 60; // Golf Time Zone (Military)
  timezones["GMT"] = 0; // Greenwich Mean Time (Europe)
  timezones["H"] = +8 * 60; // Hotel Time Zone (Military)
  timezones["HAA"] = -3 * 60; // Heure Avancée de l'Atlantique (North America)
  timezones["HAC"] = -5 * 60; // Heure Avancée du Centre (North America)
  timezones["HADT"] = -9 * 60; // Hawaii-Aleutian Daylight Time (North America)
  timezones["HAE"] = -4 * 60; // Heure Avancée de l'Est (North America)
  timezones["HAP"] = -7 * 60; // Heure Avancée du Pacifique (North America)
  timezones["HAR"] = -6 * 60; // Heure Avancée des Rocheuses (North America)
  timezones["HAST"] = -10 * 60; // Hawaii-Aleutian Standard Time (North America)
  timezones["HAT"] = -(2 * 60 + 30) * 60; // Heure Avancée de Terre-Neuve (North America)
  timezones["HAY"] = -8 * 60; // Heure Avancée du Yukon (North America)
  timezones["HNA"] = -4 * 60; // Heure Normale de l'Atlantique (North America)
  timezones["HNC"] = -6 * 60; // Heure Normale du Centre (North America)
  timezones["HNE"] = -5 * 60; // Heure Normale de l'Est (North America)
  timezones["HNP"] = -8 * 60; // Heure Normale du Pacifique (North America)
  timezones["HNR"] = -7 * 60; // Heure Normale des Rocheuses (North America)
  timezones["HNT"] = -(3 * 60 + 30) * 60; // Heure Normale de Terre-Neuve (North America)
  timezones["HNY"] = -9 * 60; // Heure Normale du Yukon (North America)
  timezones["HST"] = -10 * 60; // Hawaii Standard Time (North America)
  timezones["I"] = +9 * 60; // India Time Zone (Military)
  timezones["IST"] = 60 * 60; // Irish Summer Time (Europe)
  timezones["K"] = +10 * 60; // Kilo Time Zone (Military)
  timezones["L"] = +11 * 60; // Lima Time Zone (Military)
  timezones["M"] = +12 * 60; // Mike Time Zone (Military)
  timezones["MDT"] = -6 * 60; // Mountain Daylight Time (North America)
  timezones["MESZ"] = +2 * 60; // Mitteleuroäische Sommerzeit (Europe)
  timezones["MEZ"] = 60 * 60; // Mitteleuropäische Zeit (Europe)
  timezones["MSD"] = +4 * 60; // Moscow Daylight Time (Europe)
  timezones["MSK"] = +3 * 60; // Moscow Standard Time (Europe)
  timezones["MST"] = -7 * 60; // Mountain Standard Time (North America)
  timezones["N"] = -1 * 60; // November Time Zone (Military)
  timezones["NDT"] = -(2 * 60 + 30) * 60; // Newfoundland Daylight Time (North America)
  timezones["NFT"] = +(11 * 60 + 30) * 60; // Norfolk (Island) Time (Australia)
  timezones["NST"] = -(3 * 60 + 30) * 60; // Newfoundland Standard Time (North America)
  timezones["O"] = -2 * 60; // Oscar Time Zone (Military)
  timezones["P"] = -3 * 60; // Papa Time Zone (Military)
  timezones["PDT"] = -7 * 60; // Pacific Daylight Time (North America)
  timezones["PST"] = -8 * 60; // Pacific Standard Time (North America)
  timezones["Q"] = -4 * 60; // Quebec Time Zone (Military)
  timezones["R"] = -5 * 60; // Romeo Time Zone (Military)
  timezones["S"] = -6 * 60; // Sierra Time Zone (Military)
  timezones["T"] = -7 * 60; // Tango Time Zone (Military)
  timezones["U"] = -8 * 60; // Uniform Time Zone (Military)
  timezones["UTC"] = 0; // Coordinated Universal Time (Europe)
  timezones["V"] = -9 * 60; // Victor Time Zone (Military)
  timezones["W"] = -10 * 60; // Whiskey Time Zone (Military)
  timezones["WDT"] = +9 * 60; // Western Daylight Time (Australia)
  timezones["WEDT"] = 60 * 60; // Western European Daylight Time (Europe)
  timezones["WEST"] = 60 * 60; // Western European Summer Time (Europe)
  timezones["WET"] = 0; // Western European Time (Europe)
  timezones["WST"] = +9 * 60; // Western Summer Time (Australia)
  timezones["WST"] = +8 * 60; // Western Standard Time (Australia)
  timezones["X"] = -11 * 60; // X-ray Time Zone (Military)
  timezones["Y"] = -12 * 60; // Yankee Time Zone (Military)
  timezones["Z"] = 0; // Zulu Time Zone (Military)
}

/*
Local Variables:
mode:c++
c-basic-offset:2
comment-column:40
fill-column:79
indent-tabs-mode:nil
End:
*/
