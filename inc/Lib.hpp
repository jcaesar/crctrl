
#include <boost/regex.hpp>

namespace rx{
	static boost::regex ctrl_err	("^Could not Start\\. Error: ");
	//Could not Start. Error: 
	static boost::regex cl_join		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Client (.*) aktiviert\\.$"); 
	//[18:30:52] Client Tinys Pc aktiviert.
	static boost::regex cl_conn		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Client (.*) verbunden\\.$"); 
	//[22:52:05] Client Caesar perform bantest yeah! verbunden.
	static boost::regex pl_join		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Spielerbeitritt: (.*)$");
	//
	static boost::regex cl_part		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Client (.*) entfernt \\(.*\\).$");
	//[18:31:11] Client Tinys Pc entfernt (Verbindungsabbruch).
	static boost::regex gm_gohot	("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Das Spiel beginnt in 10 Sekunden!$");
	//[11:59:56] Das Spiel beginnt in 10 Sekunden!
	static boost::regex gm_load		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Los geht's!$");
	//[11:54:36] Los geht's!
	static boost::regex gm_rec		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Aufnahme nach Records\\.c4f/([0-9])-(.*)\\.c4s\\.\\.\\.$");
	//[20:40:12] Aufnahme nach Records.c4f/1119-MinorMelee.c4s...
	static boost::regex gm_go		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Start!$");
	//[11:54:45] Start!
	static boost::regex gm_tick0	("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Spiel gestartet\\.$");
	//[17:56:36] Spiel gestartet.
	static boost::regex gm_end		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Runde beendet\\.$");
	//[12:23:23] Runde beendet.
	static boost::regex gm_exit		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Engine shut down\\.$");
	//[13:34:25] Engine shut down.
	static boost::regex gm_lobby	("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Teilnehmer werden erwartet...$");
	//[20:41:59] Teilnehmer werden erwartet...
	static boost::regex pl_die		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Spieler (.*) eliminiert.$");
	//[20:56:35] Spieler Test0r eliminiert.
	static boost::regex cm_base		("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] <(.*)> %([^ ]+)( (.*))?$");
	//
	static boost::regex ms_flood	("^\\[[0-9]{2}:[0-9]{2}:[0-9]{2}\\] Spiel konnte nicht registriert werden: Zu viele Versuche, ein Spiel zu starten$");
	//[03:07:27] Spiel konnte nicht registriert werden: Zu viele Versuche, ein Spiel zu starten
}