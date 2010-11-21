/*

   Copyright [2008] [Trevor Hogan]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   CODE PORTED FROM THE ORIGINAL GHOST PROJECT: http://ghost.pwner.org/

*/

#include "ghost.h"
#include "util.h"
#include "crc32.h"
#include "sha1.h"
#include "csvparser.h"
#include "config.h"
#include "language.h"
#include "socket.h"
#include "ghostdb.h"
#include "ghostdbsqlite.h"
#include "ghostdbmysql.h"
#include "bnet.h"
#include "map.h"
#include "packed.h"
#include "replay.h"
#include "savegame.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "gpsprotocol.h"
#include "game_base.h"
#include "game.h"
#include "game_admin.h"
#include "bnlsprotocol.h"
#include "bnlsclient.h"
#include "bnetprotocol.h"
#include "bncsutilinterface.h"

#include <signal.h>
#include <stdlib.h>
#include <boost/python.hpp>

#ifdef WIN32
 #include <ws2tcpip.h>		// for WSAIoctl
#endif

#define __STORMLIB_SELF__
#include <stormlib/StormLib.h>

/*

#include "ghost.h"
#include "util.h"
#include "crc32.h"
#include "sha1.h"
#include "csvparser.h"
#include "config.h"
#include "language.h"
#include "socket.h"
#include "commandpacket.h"
#include "ghostdb.h"
#include "ghostdbsqlite.h"
#include "ghostdbmysql.h"
#include "bncsutilinterface.h"
#include "warden.h"
#include "bnlsprotocol.h"
#include "bnlsclient.h"
#include "bnetprotocol.h"
#include "bnet.h"
#include "map.h"
#include "packed.h"
#include "savegame.h"
#include "replay.h"
#include "gameslot.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "gpsprotocol.h"
#include "game_base.h"
#include "game.h"
#include "game_admin.h"
#include "stats.h"
#include "statsdota.h"
#include "sqlite3.h"

*/

#ifdef WIN32
 #include <windows.h>
 #include <winsock.h>
#endif

#include <time.h>

#ifndef WIN32
 #include <sys/time.h>
#endif

#ifdef __APPLE__
 #include <mach/mach_time.h>
#endif

string gCFGFile;
string gLogFile;
uint32_t gLogMethod;
ofstream *gLog = NULL;
CGHost *gGHost = NULL;

uint32_t GetTime( )
{
	return GetTicks( ) / 1000;
}

uint32_t GetTicks( )
{
#ifdef WIN32
	// don't use GetTickCount anymore because it's not accurate enough (~16ms resolution)
	// don't use QueryPerformanceCounter anymore because it isn't guaranteed to be strictly increasing on some systems and thus requires "smoothing" code
	// use timeGetTime instead, which typically has a high resolution (5ms or more) but we request a lower resolution on startup

	return timeGetTime( );
#elif __APPLE__
	uint64_t current = mach_absolute_time( );
	static mach_timebase_info_data_t info = { 0, 0 };
	// get timebase info
	if( info.denom == 0 )
		mach_timebase_info( &info );
	uint64_t elapsednano = current * ( info.numer / info.denom );
	// convert ns to ms
	return elapsednano / 1e6;
#else
	uint32_t ticks;
	struct timespec t;
	clock_gettime( CLOCK_MONOTONIC, &t );
	ticks = t.tv_sec * 1000;
	ticks += t.tv_nsec / 1000000;
	return ticks;
#endif
}

void SignalCatcher2( int s )
{
	CONSOLE_Print( "[!!!] caught signal " + UTIL_ToString( s ) + ", exiting NOW" );

	if( gGHost )
	{
		if( gGHost->m_Exiting )
			exit( 1 );
		else
			gGHost->m_Exiting = true;
	}
	else
		exit( 1 );
}

void SignalCatcher( int s )
{
	// signal( SIGABRT, SignalCatcher2 );
	signal( SIGINT, SignalCatcher2 );

	CONSOLE_Print( "[!!!] caught signal " + UTIL_ToString( s ) + ", exiting nicely" );

	if( gGHost )
		gGHost->m_ExitingNice = true;
	else
		exit( 1 );
}

void CONSOLE_Print( string message )
{
	cout << message << endl;

	// logging

	if( !gLogFile.empty( ) )
	{
		if( gLogMethod == 1 )
		{
			ofstream Log;
			Log.open( gLogFile.c_str( ), ios :: app );

			if( !Log.fail( ) )
			{
				time_t Now = time( NULL );
				string Time = asctime( localtime( &Now ) );

				// erase the newline

				Time.erase( Time.size( ) - 1 );
				Log << "[" << Time << "] " << message << endl;
				Log.close( );
			}
		}
		else if( gLogMethod == 2 )
		{
			if( gLog && !gLog->fail( ) )
			{
				time_t Now = time( NULL );
				string Time = asctime( localtime( &Now ) );

				// erase the newline

				Time.erase( Time.size( ) - 1 );
				*gLog << "[" << Time << "] " << message << endl;
				gLog->flush( );
			}
		}
	}
}

void DEBUG_Print( string message )
{
	cout << message << endl;
}

void DEBUG_Print( BYTEARRAY b )
{
	cout << "{ ";

        for( unsigned int i = 0; i < b.size( ); ++i )
		cout << hex << (int)b[i] << " ";

	cout << "}" << endl;
}

//
// host module 
//

map< string, vector<boost::python::object> > gHandlersFirst;
map< string, vector<boost::python::object> > gHandlersSecond;

void RegisterHandler( string HandlerName, boost::python::object nFunction, bool nBool = false )
{
	using namespace boost::python;

	if( !PyFunction_Check( nFunction.ptr() ) )
	{
		string Error = "argument 1 must be function, not ";
		Error += nFunction.ptr()->ob_type->tp_name;
		PyErr_SetString( PyExc_TypeError, Error.c_str() );

		throw_error_already_set();
	}

	if( nBool )
		gHandlersFirst[HandlerName].push_back( nFunction );
	else
		gHandlersSecond[HandlerName].push_back( nFunction );
}

void UnregisterHandler( string HandlerName, boost::python::object nFunction, bool nBool = false )
{
	using namespace boost::python;

	if( !PyFunction_Check( nFunction.ptr() ) )
	{
		string Error = "argument 1 must be function, not ";
		Error += nFunction.ptr()->ob_type->tp_name;
		PyErr_SetString( PyExc_TypeError, Error.c_str() );

		throw_error_already_set();
	}

	vector<object>* Functions = nBool ? &gHandlersFirst[HandlerName] : &gHandlersSecond[HandlerName];
	for( vector<object>::iterator i = Functions->begin(); i != Functions->end(); )
	{
		if( *i == nFunction )
			i = Functions->erase(i);
		else
			i++;
	}
}

BOOST_PYTHON_FUNCTION_OVERLOADS(RegisterHandler_Overloads, RegisterHandler, 2, 3);
BOOST_PYTHON_FUNCTION_OVERLOADS(UnregisterHandler_Overloads, UnregisterHandler, 2, 3);

BOOST_PYTHON_MODULE(host)
{
	using namespace boost::python;

	def( "registerHandler", RegisterHandler, RegisterHandler_Overloads() );
	def( "unregisterHandler", UnregisterHandler, UnregisterHandler_Overloads() );
	def( "log", CONSOLE_Print );
	def( "getTime", GetTime );
}

BOOST_PYTHON_MODULE(replay)
{
	using namespace boost::python;

	enum_<CReplay::BlockID>("blockID")
		.value("REPLAY_LEAVEGAME", CReplay::REPLAY_LEAVEGAME)
		.value("REPLAY_FIRSTSTARTBLOCK", CReplay::REPLAY_FIRSTSTARTBLOCK)
		.value("REPLAY_SECONDSTARTBLOCK", CReplay::REPLAY_SECONDSTARTBLOCK)
		.value("REPLAY_THIRDSTARTBLOCK", CReplay::REPLAY_THIRDSTARTBLOCK)
		.value("REPLAY_TIMESLOT2", CReplay::REPLAY_TIMESLOT2)
		.value("REPLAY_TIMESLOT", CReplay::REPLAY_TIMESLOT)
		.value("REPLAY_CHATMESSAGE", CReplay::REPLAY_CHATMESSAGE)
		.value("REPLAY_CHECKSUM", CReplay::REPLAY_CHECKSUM)
		.value("REPLAY_DESYNC", CReplay::REPLAY_DESYNC)
	;
}

BOOST_PYTHON_MODULE(map)
{
	using namespace boost::python;

	scope().attr("MAPSPEED_SLOW") = MAPSPEED_SLOW;
	scope().attr("MAPSPEED_NORMAL") = MAPSPEED_NORMAL;
	scope().attr("MAPSPEED_FAST") = MAPSPEED_FAST;
	scope().attr("MAPVIS_HIDETERRAIN") = MAPVIS_HIDETERRAIN;
	scope().attr("MAPVIS_EXPLORED") = MAPVIS_EXPLORED;
	scope().attr("MAPVIS_ALWAYSVISIBLE") = MAPVIS_ALWAYSVISIBLE;
	scope().attr("MAPVIS_DEFAULT") = MAPVIS_DEFAULT;
	scope().attr("MAPOBS_NONE") = MAPOBS_NONE;
	scope().attr("MAPOBS_ONDEFEAT") = MAPOBS_ONDEFEAT;
	scope().attr("MAPOBS_ALLOWED") = MAPOBS_ALLOWED;
	scope().attr("MAPOBS_REFEREES") = MAPOBS_REFEREES;
	scope().attr("MAPFLAG_TEAMSTOGETHER") = MAPFLAG_TEAMSTOGETHER;
	scope().attr("MAPFLAG_FIXEDTEAMS") = MAPFLAG_FIXEDTEAMS;
	scope().attr("MAPFLAG_UNITSHARE") = MAPFLAG_UNITSHARE;
	scope().attr("MAPFLAG_RANDOMHERO") = MAPFLAG_RANDOMHERO;
	scope().attr("MAPFLAG_RANDOMRACES") = MAPFLAG_RANDOMRACES;
	scope().attr("MAPOPT_HIDEMINIMAP") = MAPOPT_HIDEMINIMAP;
	scope().attr("MAPOPT_MODIFYALLYPRIORITIES") = MAPOPT_MODIFYALLYPRIORITIES;
	scope().attr("MAPOPT_MELEE") = MAPOPT_MELEE;
	scope().attr("MAPOPT_REVEALTERRAIN") = MAPOPT_REVEALTERRAIN;
	scope().attr("MAPOPT_FIXEDPLAYERSETTINGS") = MAPOPT_FIXEDPLAYERSETTINGS;
	scope().attr("MAPOPT_CUSTOMFORCES") = MAPOPT_CUSTOMFORCES;
	scope().attr("MAPOPT_CUSTOMTECHTREE") = MAPOPT_CUSTOMTECHTREE;
	scope().attr("MAPOPT_CUSTOMABILITIES") = MAPOPT_CUSTOMABILITIES;
	scope().attr("MAPOPT_CUSTOMUPGRADES") = MAPOPT_CUSTOMUPGRADES;
	scope().attr("MAPOPT_WATERWAVESONCLIFFSHORES") = MAPOPT_WATERWAVESONCLIFFSHORES;
	scope().attr("MAPOPT_WATERWAVESONSLOPESHORES") = MAPOPT_WATERWAVESONSLOPESHORES;
	scope().attr("MAPFILTER_MAKER_USER") = MAPFILTER_MAKER_USER;
	scope().attr("MAPFILTER_MAKER_BLIZZARD") = MAPFILTER_MAKER_BLIZZARD;
	scope().attr("MAPFILTER_TYPE_MELEE") = MAPFILTER_TYPE_MELEE;
	scope().attr("MAPFILTER_TYPE_SCENARIO") = MAPFILTER_TYPE_SCENARIO;
	scope().attr("MAPFILTER_SIZE_SMALL") = MAPFILTER_SIZE_SMALL;
	scope().attr("MAPFILTER_SIZE_MEDIUM") = MAPFILTER_SIZE_MEDIUM;
	scope().attr("MAPFILTER_SIZE_LARGE") = MAPFILTER_SIZE_LARGE;
	scope().attr("MAPFILTER_OBS_FULL") = MAPFILTER_OBS_FULL;
	scope().attr("MAPFILTER_OBS_ONDEATH") = MAPFILTER_OBS_ONDEATH;
	scope().attr("MAPFILTER_OBS_NONE") = MAPFILTER_OBS_NONE;
	scope().attr("MAPGAMETYPE_UNKNOWN0") = MAPGAMETYPE_UNKNOWN0;
	scope().attr("MAPGAMETYPE_PRIVATEGAME") = MAPGAMETYPE_PRIVATEGAME;
	scope().attr("MAPGAMETYPE_MAKERUSER") = MAPGAMETYPE_MAKERUSER;
	scope().attr("MAPGAMETYPE_MAKERBLIZZARD") = MAPGAMETYPE_MAKERBLIZZARD;
	scope().attr("MAPGAMETYPE_TYPEMELEE") = MAPGAMETYPE_TYPEMELEE;
	scope().attr("MAPGAMETYPE_TYPESCENARIO") = MAPGAMETYPE_TYPESCENARIO;
	scope().attr("MAPGAMETYPE_SIZESMALL") = MAPGAMETYPE_SIZESMALL;
	scope().attr("MAPGAMETYPE_SIZEMEDIUM") = MAPGAMETYPE_SIZEMEDIUM;
	scope().attr("MAPGAMETYPE_SIZELARGE") = MAPGAMETYPE_SIZELARGE;
	scope().attr("MAPGAMETYPE_OBSFULL") = MAPGAMETYPE_OBSFULL;
	scope().attr("MAPGAMETYPE_OBSONDEATH") = MAPGAMETYPE_OBSONDEATH;
	scope().attr("MAPGAMETYPE_OBSNONE") = MAPGAMETYPE_OBSNONE;

	scope().attr("MAPGAMETYPE_UNKNOWN0") = MAPGAMETYPE_UNKNOWN0;
    scope().attr("MAPGAMETYPE_SAVEDGAME") =	MAPGAMETYPE_SAVEDGAME;
    scope().attr("MAPGAMETYPE_PRIVATEGAME") = MAPGAMETYPE_PRIVATEGAME;
    scope().attr("MAPGAMETYPE_MAKERUSER") = MAPGAMETYPE_MAKERUSER;
    scope().attr("MAPGAMETYPE_MAKERBLIZZARD") = MAPGAMETYPE_MAKERBLIZZARD;
    scope().attr("MAPGAMETYPE_TYPEMELEE") = MAPGAMETYPE_TYPEMELEE;
    scope().attr("MAPGAMETYPE_TYPESCENARIO") = MAPGAMETYPE_TYPESCENARIO;
    scope().attr("MAPGAMETYPE_SIZESMALL") = MAPGAMETYPE_SIZESMALL;
    scope().attr("MAPGAMETYPE_SIZEMEDIUM") = MAPGAMETYPE_SIZEMEDIUM;
    scope().attr("MAPGAMETYPE_SIZELARGE") = MAPGAMETYPE_SIZELARGE;
    scope().attr("MAPGAMETYPE_OBSFULL") = MAPGAMETYPE_OBSFULL;
    scope().attr("MAPGAMETYPE_OBSONDEATH") = MAPGAMETYPE_OBSONDEATH;
    scope().attr("MAPGAMETYPE_OBSNONE") = MAPGAMETYPE_OBSNONE;
}

BOOST_PYTHON_MODULE(GPSProtocol)
{
	using namespace boost::python;

	enum_<CGPSProtocol::Protocol>("protocol")
		.value("GPS_INIT", CGPSProtocol::GPS_INIT)
		.value("GPS_RECONNECT", CGPSProtocol::GPS_RECONNECT)
		.value("GPS_ACK", CGPSProtocol::GPS_ACK)
		.value("GPS_REJECT", CGPSProtocol::GPS_REJECT)
	;

	scope().attr("GPS_HEADER_CONSTANT") = GPS_HEADER_CONSTANT;

	scope().attr("REJECTGPS_INVALID") = REJECTGPS_INVALID;
	scope().attr("REJECTGPS_NOTFOUND") = REJECTGPS_NOTFOUND;
}

BOOST_PYTHON_MODULE(gameslot)
{
	using namespace boost::python;

	scope().attr("SLOTSTATUS_OPEN") = SLOTSTATUS_OPEN;
	scope().attr("SLOTSTATUS_CLOSED") = SLOTSTATUS_CLOSED;
	scope().attr("SLOTSTATUS_OCCUPIED") = SLOTSTATUS_OCCUPIED;
	scope().attr("SLOTRACE_HUMAN") = SLOTRACE_HUMAN;
	scope().attr("SLOTRACE_ORC") = SLOTRACE_ORC;
	scope().attr("SLOTRACE_NIGHTELF") = SLOTRACE_NIGHTELF;
	scope().attr("SLOTRACE_UNDEAD") = SLOTRACE_UNDEAD;
	scope().attr("SLOTRACE_RANDOM") = SLOTRACE_RANDOM;
	scope().attr("SLOTRACE_SELECTABLE") = SLOTRACE_SELECTABLE;
	scope().attr("SLOTCOMP_EASY") = SLOTCOMP_EASY;
	scope().attr("SLOTCOMP_NORMAL") = SLOTCOMP_NORMAL;
	scope().attr("SLOTCOMP_HARD") = SLOTCOMP_HARD;
}

BOOST_PYTHON_MODULE(gameProtocol)
{
	using namespace boost::python;

	enum_<CGameProtocol::Protocol>("protocol")
		.value("W3GS_PING_FROM_HOST", CGameProtocol::W3GS_PING_FROM_HOST)
		.value("W3GS_SLOTINFOJOIN", CGameProtocol::W3GS_SLOTINFOJOIN)
		.value("W3GS_REJECTJOIN", CGameProtocol::W3GS_REJECTJOIN)
		.value("W3GS_PLAYERINFO", CGameProtocol::W3GS_PLAYERINFO)
		.value("W3GS_PLAYERLEAVE_OTHERS", CGameProtocol::W3GS_PLAYERLEAVE_OTHERS)
		.value("W3GS_GAMELOADED_OTHERS", CGameProtocol::W3GS_GAMELOADED_OTHERS)
		.value("W3GS_SLOTINFO", CGameProtocol::W3GS_SLOTINFO)
		.value("W3GS_COUNTDOWN_START", CGameProtocol::W3GS_COUNTDOWN_START)
		.value("W3GS_COUNTDOWN_END", CGameProtocol::W3GS_COUNTDOWN_END)
		.value("W3GS_INCOMING_ACTION", CGameProtocol::W3GS_INCOMING_ACTION)
		.value("W3GS_CHAT_FROM_HOST", CGameProtocol::W3GS_CHAT_FROM_HOST)
		.value("W3GS_START_LAG", CGameProtocol::W3GS_START_LAG)
		.value("W3GS_STOP_LAG", CGameProtocol::W3GS_STOP_LAG)
		.value("W3GS_HOST_KICK_PLAYER", CGameProtocol::W3GS_HOST_KICK_PLAYER)
		.value("W3GS_REQJOIN", CGameProtocol::W3GS_REQJOIN)
		.value("W3GS_LEAVEGAME", CGameProtocol::W3GS_LEAVEGAME)
		.value("W3GS_GAMELOADED_SELF", CGameProtocol::W3GS_GAMELOADED_SELF)
		.value("W3GS_OUTGOING_ACTION", CGameProtocol::W3GS_OUTGOING_ACTION)
		.value("W3GS_OUTGOING_KEEPALIVE", CGameProtocol::W3GS_OUTGOING_KEEPALIVE)
		.value("W3GS_CHAT_TO_HOST", CGameProtocol::W3GS_CHAT_TO_HOST)
		.value("W3GS_DROPREQ", CGameProtocol::W3GS_DROPREQ)
		.value("W3GS_SEARCHGAME", CGameProtocol::W3GS_SEARCHGAME)
		.value("W3GS_GAMEINFO", CGameProtocol::W3GS_GAMEINFO)
		.value("W3GS_CREATEGAME", CGameProtocol::W3GS_CREATEGAME)
		.value("W3GS_REFRESHGAME", CGameProtocol::W3GS_REFRESHGAME)
		.value("W3GS_DECREATEGAME", CGameProtocol::W3GS_DECREATEGAME)
		.value("W3GS_CHAT_OTHERS", CGameProtocol::W3GS_CHAT_OTHERS)
		.value("W3GS_PING_FROM_OTHERS", CGameProtocol::W3GS_PING_FROM_OTHERS)
		.value("W3GS_PONG_TO_OTHERS", CGameProtocol::W3GS_PONG_TO_OTHERS)
		.value("W3GS_MAPCHECK", CGameProtocol::W3GS_MAPCHECK)
		.value("W3GS_STARTDOWNLOAD", CGameProtocol::W3GS_STARTDOWNLOAD)
		.value("W3GS_MAPSIZE", CGameProtocol::W3GS_MAPSIZE)
		.value("W3GS_MAPPART", CGameProtocol::W3GS_MAPPART)
		.value("W3GS_MAPPARTOK", CGameProtocol::W3GS_MAPPARTOK)
		.value("W3GS_MAPPARTNOTOK", CGameProtocol::W3GS_MAPPARTNOTOK)
		.value("W3GS_PONG_TO_HOST", CGameProtocol::W3GS_PONG_TO_HOST)
		.value("W3GS_INCOMING_ACTION2", CGameProtocol::W3GS_INCOMING_ACTION2)
	;

	scope().attr("W3GS_HEADER_CONSTANT") = W3GS_HEADER_CONSTANT;
	scope().attr("GAME_NONE") = GAME_NONE;
	scope().attr("GAME_FULL") = GAME_FULL;
	scope().attr("GAME_PUBLIC") = GAME_PUBLIC;
	scope().attr("GAME_PRIVATE") = GAME_PRIVATE;
	scope().attr("GAMETYPE_CUSTOM") = GAMETYPE_CUSTOM;
	scope().attr("GAMETYPE_BLIZZARD") = GAMETYPE_BLIZZARD;
	scope().attr("PLAYERLEAVE_DISCONNECT") = PLAYERLEAVE_DISCONNECT;
	scope().attr("PLAYERLEAVE_LOST") = PLAYERLEAVE_LOST;
	scope().attr("PLAYERLEAVE_LOSTBUILDINGS") = PLAYERLEAVE_LOSTBUILDINGS;
	scope().attr("PLAYERLEAVE_WON") = PLAYERLEAVE_WON;
	scope().attr("PLAYERLEAVE_DRAW") = PLAYERLEAVE_DRAW;
	scope().attr("PLAYERLEAVE_OBSERVER") = PLAYERLEAVE_OBSERVER;
	scope().attr("PLAYERLEAVE_LOBBY") = PLAYERLEAVE_LOBBY;
	scope().attr("PLAYERLEAVE_GPROXY") = PLAYERLEAVE_GPROXY;
	scope().attr("REJECTJOIN_FULL") = REJECTJOIN_FULL;
	scope().attr("REJECTJOIN_STARTED") = REJECTJOIN_STARTED;
	scope().attr("REJECTJOIN_WRONGPASSWORD") = REJECTJOIN_WRONGPASSWORD;
}

BOOST_PYTHON_MODULE(BNLSProtocol)
{
	using namespace boost::python;
	enum_<CBNLSProtocol::Protocol>("protocol")
		.value("BNLS_NULL", CBNLSProtocol::BNLS_NULL)
		.value("BNLS_CDKEY", CBNLSProtocol::BNLS_CDKEY)
		.value("BNLS_LOGONCHALLENGE", CBNLSProtocol::BNLS_LOGONCHALLENGE)
		.value("BNLS_LOGONPROOF", CBNLSProtocol::BNLS_LOGONPROOF)
		.value("BNLS_CREATEACCOUNT", CBNLSProtocol::BNLS_CREATEACCOUNT)
		.value("BNLS_CHANGECHALLENGE", CBNLSProtocol::BNLS_CHANGECHALLENGE)
		.value("BNLS_CHANGEPROOF", CBNLSProtocol::BNLS_CHANGEPROOF)
		.value("BNLS_UPGRADECHALLENGE", CBNLSProtocol::BNLS_UPGRADECHALLENGE)
		.value("BNLS_UPGRADEPROOF", CBNLSProtocol::BNLS_UPGRADEPROOF)
		.value("BNLS_VERSIONCHECK", CBNLSProtocol::BNLS_VERSIONCHECK)
		.value("BNLS_CONFIRMLOGON", CBNLSProtocol::BNLS_CONFIRMLOGON)
		.value("BNLS_HASHDATA", CBNLSProtocol::BNLS_HASHDATA)
		.value("BNLS_CDKEY_EX", CBNLSProtocol::BNLS_CDKEY_EX)
		.value("BNLS_CHOOSENLSREVISION", CBNLSProtocol::BNLS_CHOOSENLSREVISION)
		.value("BNLS_AUTHORIZE", CBNLSProtocol::BNLS_AUTHORIZE)
		.value("BNLS_AUTHORIZEPROOF", CBNLSProtocol::BNLS_AUTHORIZEPROOF)
		.value("BNLS_REQUESTVERSIONBYTE", CBNLSProtocol::BNLS_REQUESTVERSIONBYTE)
		.value("BNLS_VERIFYSERVER", CBNLSProtocol::BNLS_VERIFYSERVER)
		.value("BNLS_RESERVESERVERSLOTS", CBNLSProtocol::BNLS_RESERVESERVERSLOTS)
		.value("BNLS_SERVERLOGONCHALLENGE", CBNLSProtocol::BNLS_SERVERLOGONCHALLENGE)
		.value("BNLS_SERVERLOGONPROOF", CBNLSProtocol::BNLS_SERVERLOGONPROOF)
		.value("BNLS_RESERVED0", CBNLSProtocol::BNLS_RESERVED0)
		.value("BNLS_RESERVED1", CBNLSProtocol::BNLS_RESERVED1)
		.value("BNLS_RESERVED2", CBNLSProtocol::BNLS_RESERVED2)
		.value("BNLS_VERSIONCHECKEX", CBNLSProtocol::BNLS_VERSIONCHECKEX)
		.value("BNLS_RESERVED3", CBNLSProtocol::BNLS_RESERVED3)
		.value("BNLS_VERSIONCHECKEX2", CBNLSProtocol::BNLS_VERSIONCHECKEX2)
		.value("BNLS_WARDEN", CBNLSProtocol::BNLS_WARDEN)
	;
}

BOOST_PYTHON_MODULE(BNETProtocol)
{
	using namespace boost::python;

	enum_<CBNETProtocol::Protocol>("protocol")
		.value("SID_NULL", CBNETProtocol::SID_NULL)
		.value("SID_STOPADV", CBNETProtocol::SID_STOPADV)
		.value("SID_GETADVLISTEX", CBNETProtocol::SID_GETADVLISTEX)
		.value("SID_ENTERCHAT", CBNETProtocol::SID_ENTERCHAT)
		.value("SID_JOINCHANNEL", CBNETProtocol::SID_JOINCHANNEL)
		.value("SID_CHATCOMMAND", CBNETProtocol::SID_CHATCOMMAND)
		.value("SID_CHATEVENT", CBNETProtocol::SID_CHATEVENT)
		.value("SID_CHECKAD", CBNETProtocol::SID_CHECKAD)
		.value("SID_STARTADVEX3", CBNETProtocol::SID_STARTADVEX3)
		.value("SID_DISPLAYAD", CBNETProtocol::SID_DISPLAYAD)
		.value("SID_NOTIFYJOIN", CBNETProtocol::SID_NOTIFYJOIN)
		.value("SID_PING", CBNETProtocol::SID_PING)
		.value("SID_LOGONRESPONSE", CBNETProtocol::SID_LOGONRESPONSE)
		.value("SID_NETGAMEPORT", CBNETProtocol::SID_NETGAMEPORT)
		.value("SID_AUTH_INFO", CBNETProtocol::SID_AUTH_INFO)
		.value("SID_AUTH_CHECK", CBNETProtocol::SID_AUTH_CHECK)
		.value("SID_AUTH_ACCOUNTLOGON", CBNETProtocol::SID_AUTH_ACCOUNTLOGON)
		.value("SID_AUTH_ACCOUNTLOGONPROOF", CBNETProtocol::SID_AUTH_ACCOUNTLOGONPROOF)
		.value("SID_WARDEN", CBNETProtocol::SID_WARDEN)
		.value("SID_FRIENDSLIST", CBNETProtocol::SID_FRIENDSLIST)
		.value("SID_FRIENDSUPDATE", CBNETProtocol::SID_FRIENDSUPDATE)
		.value("SID_CLANMEMBERLIST", CBNETProtocol::SID_CLANMEMBERLIST)
		.value("SID_CLANMEMBERSTATUSCHANGE", CBNETProtocol::SID_CLANMEMBERSTATUSCHANGE)
	;

	enum_<CBNETProtocol::KeyResult>("keyResult")
		.value("KR_GOOD", CBNETProtocol::KR_GOOD)
		.value("KR_OLD_GAME_VERSION", CBNETProtocol::KR_OLD_GAME_VERSION)
		.value("KR_INVALID_VERSION", CBNETProtocol::KR_INVALID_VERSION)
		.value("KR_ROC_KEY_IN_USE", CBNETProtocol::KR_ROC_KEY_IN_USE)
		.value("KR_TFT_KEY_IN_USE", CBNETProtocol::KR_TFT_KEY_IN_USE)
	;

	enum_<CBNETProtocol::IncomingChatEvent>("incomingChatEvent")
		.value("EID_SHOWUSER", CBNETProtocol::EID_SHOWUSER)
		.value("EID_JOIN", CBNETProtocol::EID_JOIN)
		.value("EID_LEAVE", CBNETProtocol::EID_LEAVE)
		.value("EID_WHISPER", CBNETProtocol::EID_WHISPER)
		.value("EID_TALK", CBNETProtocol::EID_TALK)
		.value("EID_BROADCAST", CBNETProtocol::EID_BROADCAST)
		.value("EID_CHANNEL", CBNETProtocol::EID_CHANNEL)
		.value("EID_USERFLAGS", CBNETProtocol::EID_USERFLAGS)
		.value("EID_WHISPERSENT", CBNETProtocol::EID_WHISPERSENT)
		.value("EID_CHANNELFULL", CBNETProtocol::EID_CHANNELFULL)
		.value("EID_CHANNELDOESNOTEXIST", CBNETProtocol::EID_CHANNELDOESNOTEXIST)
		.value("EID_CHANNELRESTRICTED", CBNETProtocol::EID_CHANNELRESTRICTED)
		.value("EID_INFO", CBNETProtocol::EID_INFO)
		.value("EID_ERROR", CBNETProtocol::EID_ERROR)
		.value("EID_EMOTE", CBNETProtocol::EID_EMOTE)
	;
}

BOOST_PYTHON_MODULE(incomingChatPlayer)
{
	using namespace boost::python;

	enum_<CIncomingChatPlayer::ChatToHostType>("chatToHostType")
		.value("CTH_MESSAGE", CIncomingChatPlayer::CTH_MESSAGE)
		.value("CTH_MESSAGEEXTRA", CIncomingChatPlayer::CTH_MESSAGEEXTRA)
		.value("CTH_TEAMCHANGE", CIncomingChatPlayer::CTH_TEAMCHANGE)
		.value("CTH_COLOURCHANGE", CIncomingChatPlayer::CTH_COLOURCHANGE)
		.value("CTH_RACECHANGE", CIncomingChatPlayer::CTH_RACECHANGE)
		.value("CTH_HANDICAPCHANGE", CIncomingChatPlayer::CTH_HANDICAPCHANGE)
	;
}

struct BYTEARRAY_to_list
{
	static PyObject* convert( vector<unsigned char> const& vec)
	{
		boost::python::list result;

		for( vector<unsigned char>::const_iterator i = vec.begin(); i != vec.end() ; i++ ) 
		{
			result.append( boost::python::object(*i) );
		}

		return boost::python::incref( result.ptr() );
	}
};

struct BYTEARRAY_from_list
{
	static void* convertible( PyObject* py_object )
	{
		if( !PyList_Check( py_object ) )
			return NULL;

		return py_object;
	}

	static void construct( PyObject* obj_ptr, boost::python::converter::rvalue_from_python_stage1_data* data)
    {
		vector<unsigned char> vec;

		for( Py_ssize_t i = 0; i < PyList_Size(obj_ptr); i++ )
		{
			PyObject* obj = PyList_GetItem(obj_ptr, i);

			if( !PyInt_Check(obj) )
			{
				string Error = "object must be int, not ";
				Error += obj->ob_type->tp_name;
				PyErr_SetString( PyExc_TypeError, Error.c_str() );

				boost::python::throw_error_already_set();
			}

			vec.push_back( (unsigned char)PyInt_AsLong(obj) );
		}
		
		void* storage = ((boost::python::converter::rvalue_from_python_storage< vector<unsigned char> >*)data)->storage.bytes;
		
		new(storage) vector<unsigned char>(vec);
		data->convertible = storage;
    }
 };

//
// main
//

int main( int argc, char **argv )
{
	srand( time( NULL ) );

	gCFGFile = "ghost.cfg";

	if( argc > 1 && argv[1] )
		gCFGFile = argv[1];

	// read config file

	CConfig CFG;
	CFG.Read( "default.cfg" );
	CFG.Read( gCFGFile );
	gLogFile = CFG.GetString( "bot_log", string( ) );
	gLogMethod = CFG.GetInt( "bot_logmethod", 1 );

	if( !gLogFile.empty( ) )
	{
		if( gLogMethod == 1 )
		{
			// log method 1: open, append, and close the log for every message
			// this works well on Linux but poorly on Windows, particularly as the log file grows in size
			// the log file can be edited/moved/deleted while GHost++ is running
		}
		else if( gLogMethod == 2 )
		{
			// log method 2: open the log on startup, flush the log for every message, close the log on shutdown
			// the log file CANNOT be edited/moved/deleted while GHost++ is running

			gLog = new ofstream( );
			gLog->open( gLogFile.c_str( ), ios :: app );
		}
	}

	CONSOLE_Print( "[GHOST] starting up" );

	if( !gLogFile.empty( ) )
	{
		if( gLogMethod == 1 )
			CONSOLE_Print( "[GHOST] using log method 1, logging is enabled and [" + gLogFile + "] will not be locked" );
		else if( gLogMethod == 2 )
		{
			if( gLog->fail( ) )
				CONSOLE_Print( "[GHOST] using log method 2 but unable to open [" + gLogFile + "] for appending, logging is disabled" );
			else
				CONSOLE_Print( "[GHOST] using log method 2, logging is enabled and [" + gLogFile + "] is now locked" );
		}
	}
	else
		CONSOLE_Print( "[GHOST] no log file specified, logging is disabled" );

	// catch SIGABRT and SIGINT

	// signal( SIGABRT, SignalCatcher );
	signal( SIGINT, SignalCatcher );

#ifndef WIN32
	// disable SIGPIPE since some systems like OS X don't define MSG_NOSIGNAL

	signal( SIGPIPE, SIG_IGN );
#endif

#ifdef WIN32
	// initialize timer resolution
	// attempt to set the resolution as low as possible from 1ms to 5ms

	unsigned int TimerResolution = 0;

        for( unsigned int i = 1; i <= 5; ++i )
	{
		if( timeBeginPeriod( i ) == TIMERR_NOERROR )
		{
			TimerResolution = i;
			break;
		}
		else if( i < 5 )
			CONSOLE_Print( "[GHOST] error setting Windows timer resolution to " + UTIL_ToString( i ) + " milliseconds, trying a higher resolution" );
		else
		{
			CONSOLE_Print( "[GHOST] error setting Windows timer resolution" );
			return 1;
		}
	}

	CONSOLE_Print( "[GHOST] using Windows timer with resolution " + UTIL_ToString( TimerResolution ) + " milliseconds" );
#elif __APPLE__
	// not sure how to get the resolution
#else
	// print the timer resolution

	struct timespec Resolution;

	if( clock_getres( CLOCK_MONOTONIC, &Resolution ) == -1 )
		CONSOLE_Print( "[GHOST] error getting monotonic timer resolution" );
	else
		CONSOLE_Print( "[GHOST] using monotonic timer with resolution " + UTIL_ToString( (double)( Resolution.tv_nsec / 1000 ), 2 ) + " microseconds" );
#endif

#ifdef WIN32
	// initialize winsock

	CONSOLE_Print( "[GHOST] starting winsock" );
	WSADATA wsadata;

	if( WSAStartup( MAKEWORD( 2, 2 ), &wsadata ) != 0 )
	{
		CONSOLE_Print( "[GHOST] error starting winsock" );
		return 1;
	}

	// increase process priority

	CONSOLE_Print( "[GHOST] setting process priority to \"above normal\"" );
	SetPriorityClass( GetCurrentProcess( ), ABOVE_NORMAL_PRIORITY_CLASS );
#endif

	// register the builtin modules

	if( PyImport_AppendInittab("host", inithost) == -1 )
		throw std::runtime_error( "Failed to add host to the interpreter's builtin modules" );
	if( PyImport_AppendInittab("replay", initreplay) == -1 )
		throw std::runtime_error( "Failed to add host to the interpreter's builtin modules" );
	if( PyImport_AppendInittab("map", initmap) == -1 )
		throw std::runtime_error( "Failed to add host to the interpreter's builtin modules" );
	if( PyImport_AppendInittab("GPSProtocol", initGPSProtocol) == -1 )
		throw std::runtime_error( "Failed to add host to the interpreter's builtin modules" );
	if( PyImport_AppendInittab("gameslot", initgameslot) == -1 )
		throw std::runtime_error( "Failed to add host to the interpreter's builtin modules" );
	if( PyImport_AppendInittab("gameProtocol", initgameProtocol) == -1 )
		throw std::runtime_error( "Failed to add host to the interpreter's builtin modules" );
	if( PyImport_AppendInittab("BNLSProtocol", initBNLSProtocol) == -1 )
		throw std::runtime_error( "Failed to add host to the interpreter's builtin modules" );
	if( PyImport_AppendInittab("BNETProtocol", initBNETProtocol) == -1 )
		throw std::runtime_error( "Failed to add host to the interpreter's builtin modules" );
	if( PyImport_AppendInittab("incomingChatPlayer", initincomingChatPlayer) == -1 )
		throw std::runtime_error( "Failed to add host to the interpreter's builtin modules" );

#ifdef WIN32
	Py_SetPythonHome(".\\python\\");
#endif

	Py_Initialize( );
	PyEval_InitThreads();
	PyThreadState* MainThreadState;

	boost::python::object global( boost::python::import("__main__").attr("__dict__") );
	boost::python::exec("import sys, host												\n"
						"																\n"
						"class Logger:													\n"	
						"	def __init__(self, name):									\n"
						"		self.name = name										\n"
						"		self.buffer = ''										\n"
						"																\n"
						"	def write(self, string):									\n"
						"		if len(string) == 0: return								\n"
						"		self.buffer += string									\n"
						"																\n"
						"		if string[-1] == '\\n':									\n"
						"			host.log('[PYTHON] ' + self.buffer[:-1])			\n"
						"			self.buffer = ''									\n"
						"																\n"
						"	def flush(self): pass										\n"
						"	def close(self): pass										\n"
						"																\n"
						"#forwarding all python 'prints' to the c++ log ( host.log )	\n"
						"sys.stdout = Logger('stdout')									\n"
						"sys.stderr = Logger('stderr')									\n",
						global, global);

	CSocket::RegisterPythonClass( );
	CTCPSocket::RegisterPythonClass( );
	CTCPClient::RegisterPythonClass( );
	CTCPServer::RegisterPythonClass( );
	CUDPSocket::RegisterPythonClass( );
	CUDPServer::RegisterPythonClass( );
	CPacked::RegisterPythonClass( );
	CSaveGame::RegisterPythonClass( );
	CReplay::RegisterPythonClass( );
	CMap::RegisterPythonClass( );
	CLanguage::RegisterPythonClass( );
	CGPSProtocol::RegisterPythonClass( );
	CGHost::RegisterPythonClass( );
	CGameSlot::RegisterPythonClass( );
	CIncomingMapSize::RegisterPythonClass( );
	CIncomingChatPlayer::RegisterPythonClass( );
	CIncomingAction::RegisterPythonClass( );
	CIncomingJoinPlayer::RegisterPythonClass( );
	CGameProtocol::RegisterPythonClass( );
	CPotentialPlayer::RegisterPythonClass( );
	CGamePlayer::RegisterPythonClass( );
	CBaseGame::RegisterPythonClass( );
	CAdminGame::RegisterPythonClass( );
	CGame::RegisterPythonClass( );
	CBNLSProtocol::RegisterPythonClass( );
	CBNLSClient::RegisterPythonClass( );
	CIncomingClanList::RegisterPythonClass( );
	CIncomingFriendList::RegisterPythonClass( );
	CIncomingChatEvent::RegisterPythonClass( );
	CIncomingGameHost::RegisterPythonClass( );
	CBNETProtocol::RegisterPythonClass( );
	CBNET::RegisterPythonClass( );
	CBNCSUtilInterface::RegisterPythonClass( );
	CConfig::RegisterPythonClass( );

	boost::python::to_python_converter< BYTEARRAY, BYTEARRAY_to_list>();
	boost::python::converter::registry::push_back( &BYTEARRAY_from_list::convertible, &BYTEARRAY_from_list::construct, boost::python::type_id< BYTEARRAY >() );

	MainThreadState = PyGILState_GetThisThreadState( );
	PyEval_ReleaseThread( MainThreadState );

	try
	{
		PyGILState_STATE gstate = PyGILState_Ensure( );
		boost::python::object module = boost::python::import("plugins.python");
		PyGILState_Release( gstate );
	}
	catch(...)
	{
		PyGILState_STATE gstate = PyGILState_Ensure( );
		PyErr_Print( );
		PyGILState_Release( gstate );
		throw;
	}

	EXECUTE_HANDLER("StartUp", false, boost::ref(CFG))
	EXECUTE_HANDLER("StartUp", true, boost::ref(CFG))

	// initialize ghost

	gGHost = new CGHost( &CFG );

	EXECUTE_HANDLER("GHostStarted", false, boost::ref(gGHost))
	try	
	{ 
		EXECUTE_HANDLER("GHostStarted", true, boost::ref(gGHost)) 
	}
	catch(...) 
	{ 

	}

	while( 1 )
	{
		// block for 50ms on all sockets - if you intend to perform any timed actions more frequently you should change this
		// that said it's likely we'll loop more often than this due to there being data waiting on one of the sockets but there aren't any guarantees

		if( gGHost->Update( 50000 ) )
			break;
	}

	// shutdown ghost

	CONSOLE_Print( "[GHOST] shutting down" );
	delete gGHost;
	gGHost = NULL;

	EXECUTE_HANDLER("ShutDown", false)
	try	
	{ 
		EXECUTE_HANDLER("ShutDown", true) 
	}
	catch(...) 
	{ 

	}

	PyEval_AcquireThread( MainThreadState );
	Py_Finalize( );

#ifdef WIN32
	// shutdown winsock

	CONSOLE_Print( "[GHOST] shutting down winsock" );
	WSACleanup( );

	// shutdown timer

	timeEndPeriod( TimerResolution );
#endif

	if( gLog )
	{
		if( !gLog->fail( ) )
			gLog->close( );

		delete gLog;
	}

	return 0;
}

//
// CGHost
//

CGHost :: CGHost( CConfig *CFG )
{
	m_UDPSocket = new CUDPSocket( );
	m_UDPSocket->SetBroadcastTarget( CFG->GetString( "udp_broadcasttarget", string( ) ) );
	m_UDPSocket->SetDontRoute( CFG->GetInt( "udp_dontroute", 0 ) == 0 ? false : true );
	m_ReconnectSocket = NULL;
	m_GPSProtocol = new CGPSProtocol( );
	m_CRC = new CCRC32( );
	m_CRC->Initialize( );
	m_SHA = new CSHA1( );
	m_CurrentGame = NULL;
	string DBType = CFG->GetString( "db_type", "sqlite3" );
	CONSOLE_Print( "[GHOST] opening primary database" );

	if( DBType == "mysql" )
	{
#ifdef GHOST_MYSQL
		m_DB = new CGHostDBMySQL( CFG );
#else
		CONSOLE_Print( "[GHOST] warning - this binary was not compiled with MySQL database support, using SQLite database instead" );
		m_DB = new CGHostDBSQLite( CFG );
#endif
	}
	else
		m_DB = new CGHostDBSQLite( CFG );

	CONSOLE_Print( "[GHOST] opening secondary (local) database" );
	m_DBLocal = new CGHostDBSQLite( CFG );

	// get a list of local IP addresses
	// this list is used elsewhere to determine if a player connecting to the bot is local or not

	CONSOLE_Print( "[GHOST] attempting to find local IP addresses" );

#ifdef WIN32
	// use a more reliable Windows specific method since the portable method doesn't always work properly on Windows
	// code stolen from: http://tangentsoft.net/wskfaq/examples/getifaces.html

	SOCKET sd = WSASocket( AF_INET, SOCK_DGRAM, 0, 0, 0, 0 );

	if( sd == SOCKET_ERROR )
		CONSOLE_Print( "[GHOST] error finding local IP addresses - failed to create socket (error code " + UTIL_ToString( WSAGetLastError( ) ) + ")" );
	else
	{
		INTERFACE_INFO InterfaceList[20];
		unsigned long nBytesReturned;

		if( WSAIoctl( sd, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList, sizeof(InterfaceList), &nBytesReturned, 0, 0 ) == SOCKET_ERROR )
			CONSOLE_Print( "[GHOST] error finding local IP addresses - WSAIoctl failed (error code " + UTIL_ToString( WSAGetLastError( ) ) + ")" );
		else
		{
			int nNumInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);

                        for( int i = 0; i < nNumInterfaces; ++i )
			{
				sockaddr_in *pAddress;
				pAddress = (sockaddr_in *)&(InterfaceList[i].iiAddress);
				CONSOLE_Print( "[GHOST] local IP address #" + UTIL_ToString( i + 1 ) + " is [" + string( inet_ntoa( pAddress->sin_addr ) ) + "]" );
				m_LocalAddresses.push_back( UTIL_CreateByteArray( (uint32_t)pAddress->sin_addr.s_addr, false ) );
			}
		}

		closesocket( sd );
	}
#else
	// use a portable method

	char HostName[255];

	if( gethostname( HostName, 255 ) == SOCKET_ERROR )
		CONSOLE_Print( "[GHOST] error finding local IP addresses - failed to get local hostname" );
	else
	{
		CONSOLE_Print( "[GHOST] local hostname is [" + string( HostName ) + "]" );
		struct hostent *HostEnt = gethostbyname( HostName );

		if( !HostEnt )
			CONSOLE_Print( "[GHOST] error finding local IP addresses - gethostbyname failed" );
		else
		{
                        for( int i = 0; HostEnt->h_addr_list[i] != NULL; ++i )
			{
				struct in_addr Address;
				memcpy( &Address, HostEnt->h_addr_list[i], sizeof(struct in_addr) );
				CONSOLE_Print( "[GHOST] local IP address #" + UTIL_ToString( i + 1 ) + " is [" + string( inet_ntoa( Address ) ) + "]" );
				m_LocalAddresses.push_back( UTIL_CreateByteArray( (uint32_t)Address.s_addr, false ) );
			}
		}
	}
#endif

	m_Language = NULL;
	m_Exiting = false;
	m_ExitingNice = false;
	m_Enabled = true;
	m_Version = "17.1";
	m_HostCounter = 1;
	m_AutoHostMaximumGames = CFG->GetInt( "autohost_maxgames", 0 );
	m_AutoHostAutoStartPlayers = CFG->GetInt( "autohost_startplayers", 0 );
	m_AutoHostGameName = CFG->GetString( "autohost_gamename", string( ) );
	m_AutoHostOwner = CFG->GetString( "autohost_owner", string( ) );
	m_LastAutoHostTime = GetTime( );
	m_AutoHostMatchMaking = false;
	m_AutoHostMinimumScore = 0.0;
	m_AutoHostMaximumScore = 0.0;
	m_AllGamesFinished = false;
	m_AllGamesFinishedTime = 0;
	m_TFT = CFG->GetInt( "bot_tft", 1 ) == 0 ? false : true;

	if( m_TFT )
		CONSOLE_Print( "[GHOST] acting as Warcraft III: The Frozen Throne" );
	else
		CONSOLE_Print( "[GHOST] acting as Warcraft III: Reign of Chaos" );

	m_HostPort = CFG->GetInt( "bot_hostport", 6112 );
	m_Reconnect = CFG->GetInt( "bot_reconnect", 1 ) == 0 ? false : true;
	m_ReconnectPort = CFG->GetInt( "bot_reconnectport", 6114 );
	m_DefaultMap = CFG->GetString( "bot_defaultmap", "map" );
	m_AdminGameCreate = CFG->GetInt( "admingame_create", 0 ) == 0 ? false : true;
	m_AdminGamePort = CFG->GetInt( "admingame_port", 6113 );
	m_AdminGamePassword = CFG->GetString( "admingame_password", string( ) );
	m_AdminGameMap = CFG->GetString( "admingame_map", string( ) );
	m_LANWar3Version = CFG->GetInt( "lan_war3version", 24 );
	m_ReplayWar3Version = CFG->GetInt( "replay_war3version", 24 );
	m_ReplayBuildNumber = CFG->GetInt( "replay_buildnumber", 6059 );
	SetConfigs( CFG );

	// load the battle.net connections
	// we're just loading the config data and creating the CBNET classes here, the connections are established later (in the Update function)

        for( uint32_t i = 1; i < 10; ++i )
	{
		string Prefix;

		if( i == 1 )
			Prefix = "bnet_";
		else
			Prefix = "bnet" + UTIL_ToString( i ) + "_";

		string Server = CFG->GetString( Prefix + "server", string( ) );
		string ServerAlias = CFG->GetString( Prefix + "serveralias", string( ) );
		string CDKeyROC = CFG->GetString( Prefix + "cdkeyroc", string( ) );
		string CDKeyTFT = CFG->GetString( Prefix + "cdkeytft", string( ) );
		string CountryAbbrev = CFG->GetString( Prefix + "countryabbrev", "USA" );
		string Country = CFG->GetString( Prefix + "country", "United States" );
		string Locale = CFG->GetString( Prefix + "locale", "system" );
		uint32_t LocaleID;

		if( Locale == "system" )
		{
#ifdef WIN32
			LocaleID = GetUserDefaultLangID( );
#else
			LocaleID = 1033;
#endif
		}
		else
			LocaleID = UTIL_ToUInt32( Locale );

		string UserName = CFG->GetString( Prefix + "username", string( ) );
		string UserPassword = CFG->GetString( Prefix + "password", string( ) );
		string FirstChannel = CFG->GetString( Prefix + "firstchannel", "The Void" );
		string RootAdmin = CFG->GetString( Prefix + "rootadmin", string( ) );
		string BNETCommandTrigger = CFG->GetString( Prefix + "commandtrigger", "!" );

		if( BNETCommandTrigger.empty( ) )
			BNETCommandTrigger = "!";

		bool HoldFriends = CFG->GetInt( Prefix + "holdfriends", 1 ) == 0 ? false : true;
		bool HoldClan = CFG->GetInt( Prefix + "holdclan", 1 ) == 0 ? false : true;
		bool PublicCommands = CFG->GetInt( Prefix + "publiccommands", 1 ) == 0 ? false : true;
		string BNLSServer = CFG->GetString( Prefix + "bnlsserver", string( ) );
		int BNLSPort = CFG->GetInt( Prefix + "bnlsport", 9367 );
		int BNLSWardenCookie = CFG->GetInt( Prefix + "bnlswardencookie", 0 );
		unsigned char War3Version = CFG->GetInt( Prefix + "custom_war3version", 24 );
		BYTEARRAY EXEVersion = UTIL_ExtractNumbers( CFG->GetString( Prefix + "custom_exeversion", string( ) ), 4 );
		BYTEARRAY EXEVersionHash = UTIL_ExtractNumbers( CFG->GetString( Prefix + "custom_exeversionhash", string( ) ), 4 );
		string PasswordHashType = CFG->GetString( Prefix + "custom_passwordhashtype", string( ) );
		string PVPGNRealmName = CFG->GetString( Prefix + "custom_pvpgnrealmname", "PvPGN Realm" );
		uint32_t MaxMessageLength = CFG->GetInt( Prefix + "custom_maxmessagelength", 200 );

		if( Server.empty( ) )
			break;

		if( CDKeyROC.empty( ) )
		{
			CONSOLE_Print( "[GHOST] missing " + Prefix + "cdkeyroc, skipping this battle.net connection" );
			continue;
		}

		if( m_TFT && CDKeyTFT.empty( ) )
		{
			CONSOLE_Print( "[GHOST] missing " + Prefix + "cdkeytft, skipping this battle.net connection" );
			continue;
		}

		if( UserName.empty( ) )
		{
			CONSOLE_Print( "[GHOST] missing " + Prefix + "username, skipping this battle.net connection" );
			continue;
		}

		if( UserPassword.empty( ) )
		{
			CONSOLE_Print( "[GHOST] missing " + Prefix + "password, skipping this battle.net connection" );
			continue;
		}

		CONSOLE_Print( "[GHOST] found battle.net connection #" + UTIL_ToString( i ) + " for server [" + Server + "]" );

		if( Locale == "system" )
		{
#ifdef WIN32
			CONSOLE_Print( "[GHOST] using system locale of " + UTIL_ToString( LocaleID ) );
#else
			CONSOLE_Print( "[GHOST] unable to get system locale, using default locale of 1033" );
#endif
		}

		m_BNETs.push_back( new CBNET( this, Server, ServerAlias, BNLSServer, (uint16_t)BNLSPort, (uint32_t)BNLSWardenCookie, CDKeyROC, CDKeyTFT, CountryAbbrev, Country, LocaleID, UserName, UserPassword, FirstChannel, RootAdmin, BNETCommandTrigger[0], HoldFriends, HoldClan, PublicCommands, War3Version, EXEVersion, EXEVersionHash, PasswordHashType, PVPGNRealmName, MaxMessageLength, i ) );
	}

	if( m_BNETs.empty( ) )
		CONSOLE_Print( "[GHOST] warning - no battle.net connections found in config file" );

	// extract common.j and blizzard.j from War3Patch.mpq if we can
	// these two files are necessary for calculating "map_crc" when loading maps so we make sure to do it before loading the default map
	// see CMap :: Load for more information

	ExtractScripts( );

	// load the default maps (note: make sure to run ExtractScripts first)

	if( m_DefaultMap.size( ) < 4 || m_DefaultMap.substr( m_DefaultMap.size( ) - 4 ) != ".cfg" )
	{
		m_DefaultMap += ".cfg";
		CONSOLE_Print( "[GHOST] adding \".cfg\" to default map -> new default is [" + m_DefaultMap + "]" );
	}

	CConfig MapCFG;
	MapCFG.Read( m_MapCFGPath + m_DefaultMap );
	m_Map = new CMap( this, &MapCFG, m_MapCFGPath + m_DefaultMap );

	if( !m_AdminGameMap.empty( ) )
	{
		if( m_AdminGameMap.size( ) < 4 || m_AdminGameMap.substr( m_AdminGameMap.size( ) - 4 ) != ".cfg" )
		{
			m_AdminGameMap += ".cfg";
			CONSOLE_Print( "[GHOST] adding \".cfg\" to default admin game map -> new default is [" + m_AdminGameMap + "]" );
		}

		CONSOLE_Print( "[GHOST] trying to load default admin game map" );
		CConfig AdminMapCFG;
		AdminMapCFG.Read( m_MapCFGPath + m_AdminGameMap );
		m_AdminMap = new CMap( this, &AdminMapCFG, m_MapCFGPath + m_AdminGameMap );

		if( !m_AdminMap->GetValid( ) )
		{
			CONSOLE_Print( "[GHOST] default admin game map isn't valid, using hardcoded admin game map instead" );
			delete m_AdminMap;
			m_AdminMap = new CMap( this );
		}
	}
	else
	{
		CONSOLE_Print( "[GHOST] using hardcoded admin game map" );
		m_AdminMap = new CMap( this );
	}

	m_AutoHostMap = new CMap( *m_Map );
	m_SaveGame = new CSaveGame( );

	// load the iptocountry data

	LoadIPToCountryData( );

	// create the admin game

	if( m_AdminGameCreate )
	{
		CONSOLE_Print( "[GHOST] creating admin game" );
		m_AdminGame = new CAdminGame( this, m_AdminMap, NULL, m_AdminGamePort, 0, "GHost++ Admin Game", m_AdminGamePassword );

		if( m_AdminGamePort == m_HostPort )
			CONSOLE_Print( "[GHOST] warning - admingame_port and bot_hostport are set to the same value, you won't be able to host any games" );

		EventGameCreated( m_AdminGame );
	}
	else
		m_AdminGame = NULL;

	if( m_BNETs.empty( ) && !m_AdminGame )
		CONSOLE_Print( "[GHOST] warning - no battle.net connections found and no admin game created" );

#ifdef GHOST_MYSQL
	CONSOLE_Print( "[GHOST] GHost++ Version " + m_Version + " (with MySQL support)" );
#else
	CONSOLE_Print( "[GHOST] GHost++ Version " + m_Version + " (without MySQL support)" );
#endif
}

CGHost :: ~CGHost( )
{
	delete m_UDPSocket;
	delete m_ReconnectSocket;

        for( vector<CTCPSocket *> :: iterator i = m_ReconnectSockets.begin( ); i != m_ReconnectSockets.end( ); ++i )
		delete *i;

	delete m_GPSProtocol;
	delete m_CRC;
	delete m_SHA;

        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		delete *i;

	delete m_CurrentGame;
	delete m_AdminGame;

        for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); ++i )
		delete *i;

	delete m_DB;
	delete m_DBLocal;

	// warning: we don't delete any entries of m_Callables here because we can't be guaranteed that the associated threads have terminated
	// this is fine if the program is currently exiting because the OS will clean up after us
	// but if you try to recreate the CGHost object within a single session you will probably leak resources!

	if( !m_Callables.empty( ) )
		CONSOLE_Print( "[GHOST] warning - " + UTIL_ToString( m_Callables.size( ) ) + " orphaned callables were leaked (this is not an error)" );

	delete m_Language;
	delete m_Map;
	delete m_AdminMap;
	delete m_AutoHostMap;
	delete m_SaveGame;
}

bool CGHost :: Update( long usecBlock )
{
	// todotodo: do we really want to shutdown if there's a database error? is there any way to recover from this?

	if( m_DB->HasError( ) )
	{
		CONSOLE_Print( "[GHOST] database error - " + m_DB->GetError( ) );
		return true;
	}

	if( m_DBLocal->HasError( ) )
	{
		CONSOLE_Print( "[GHOST] local database error - " + m_DBLocal->GetError( ) );
		return true;
	}

	// try to exit nicely if requested to do so

	if( m_ExitingNice )
	{
		if( !m_BNETs.empty( ) )
		{
			CONSOLE_Print( "[GHOST] deleting all battle.net connections in preparation for exiting nicely" );

                        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
				delete *i;

			m_BNETs.clear( );
		}

		if( m_CurrentGame )
		{
			CONSOLE_Print( "[GHOST] deleting current game in preparation for exiting nicely" );
			delete m_CurrentGame;
			m_CurrentGame = NULL;
		}

		if( m_AdminGame )
		{
			CONSOLE_Print( "[GHOST] deleting admin game in preparation for exiting nicely" );
			delete m_AdminGame;
			m_AdminGame = NULL;
		}

		if( m_Games.empty( ) )
		{
			if( !m_AllGamesFinished )
			{
				CONSOLE_Print( "[GHOST] all games finished, waiting 60 seconds for threads to finish" );
				CONSOLE_Print( "[GHOST] there are " + UTIL_ToString( m_Callables.size( ) ) + " threads in progress" );
				m_AllGamesFinished = true;
				m_AllGamesFinishedTime = GetTime( );
			}
			else
			{
				if( m_Callables.empty( ) )
				{
					CONSOLE_Print( "[GHOST] all threads finished, exiting nicely" );
					m_Exiting = true;
				}
				else if( GetTime( ) - m_AllGamesFinishedTime >= 60 )
				{
					CONSOLE_Print( "[GHOST] waited 60 seconds for threads to finish, exiting anyway" );
					CONSOLE_Print( "[GHOST] there are " + UTIL_ToString( m_Callables.size( ) ) + " threads still in progress which will be terminated" );
					m_Exiting = true;
				}
			}
		}
	}

	// update callables

	for( vector<CBaseCallable *> :: iterator i = m_Callables.begin( ); i != m_Callables.end( ); )
	{
		if( (*i)->GetReady( ) )
		{
			m_DB->RecoverCallable( *i );
			delete *i;
			i = m_Callables.erase( i );
		}
		else
                        ++i;
	}

	// create the GProxy++ reconnect listener

	if( m_Reconnect )
	{
		if( !m_ReconnectSocket )
		{
			m_ReconnectSocket = new CTCPServer( );

			if( m_ReconnectSocket->Listen( m_BindAddress, m_ReconnectPort ) )
				CONSOLE_Print( "[GHOST] listening for GProxy++ reconnects on port " + UTIL_ToString( m_ReconnectPort ) );
			else
			{
				CONSOLE_Print( "[GHOST] error listening for GProxy++ reconnects on port " + UTIL_ToString( m_ReconnectPort ) );
				delete m_ReconnectSocket;
				m_ReconnectSocket = NULL;
				m_Reconnect = false;
			}
		}
		else if( m_ReconnectSocket->HasError( ) )
		{
			CONSOLE_Print( "[GHOST] GProxy++ reconnect listener error (" + m_ReconnectSocket->GetErrorString( ) + ")" );
			delete m_ReconnectSocket;
			m_ReconnectSocket = NULL;
			m_Reconnect = false;
		}
	}

	unsigned int NumFDs = 0;

	// take every socket we own and throw it in one giant select statement so we can block on all sockets

	int nfds = 0;
	fd_set fd;
	fd_set send_fd;
	FD_ZERO( &fd );
	FD_ZERO( &send_fd );

	// 1. all battle.net sockets

        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		NumFDs += (*i)->SetFD( &fd, &send_fd, &nfds );

	// 2. the current game's server and player sockets

	if( m_CurrentGame )
		NumFDs += m_CurrentGame->SetFD( &fd, &send_fd, &nfds );

	// 3. the admin game's server and player sockets

	if( m_AdminGame )
		NumFDs += m_AdminGame->SetFD( &fd, &send_fd, &nfds );

	// 4. all running games' player sockets

        for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); ++i )
		NumFDs += (*i)->SetFD( &fd, &send_fd, &nfds );

	// 5. the GProxy++ reconnect socket(s)

	if( m_Reconnect && m_ReconnectSocket )
	{
		m_ReconnectSocket->SetFD( &fd, &send_fd, &nfds );
                ++NumFDs;
	}

        for( vector<CTCPSocket *> :: iterator i = m_ReconnectSockets.begin( ); i != m_ReconnectSockets.end( ); ++i )
	{
		(*i)->SetFD( &fd, &send_fd, &nfds );
                ++NumFDs;
	}

	// before we call select we need to determine how long to block for
	// previously we just blocked for a maximum of the passed usecBlock microseconds
	// however, in an effort to make game updates happen closer to the desired latency setting we now use a dynamic block interval
	// note: we still use the passed usecBlock as a hard maximum

        for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); ++i )
	{
		if( (*i)->GetNextTimedActionTicks( ) * 1000 < usecBlock )
			usecBlock = (*i)->GetNextTimedActionTicks( ) * 1000;
	}

	// always block for at least 1ms just in case something goes wrong
	// this prevents the bot from sucking up all the available CPU if a game keeps asking for immediate updates
	// it's a bit ridiculous to include this check since, in theory, the bot is programmed well enough to never make this mistake
	// however, considering who programmed it, it's worthwhile to do it anyway

	if( usecBlock < 1000 )
		usecBlock = 1000;

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = usecBlock;

	struct timeval send_tv;
	send_tv.tv_sec = 0;
	send_tv.tv_usec = 0;

#ifdef WIN32
	select( 1, &fd, NULL, NULL, &tv );
	select( 1, NULL, &send_fd, NULL, &send_tv );
#else
	select( nfds + 1, &fd, NULL, NULL, &tv );
	select( nfds + 1, NULL, &send_fd, NULL, &send_tv );
#endif

	if( NumFDs == 0 )
	{
		// we don't have any sockets (i.e. we aren't connected to battle.net maybe due to a lost connection and there aren't any games running)
		// select will return immediately and we'll chew up the CPU if we let it loop so just sleep for 50ms to kill some time

		MILLISLEEP( 50 );
	}

	bool AdminExit = false;
	bool BNETExit = false;

	// update current game

	if( m_CurrentGame )
	{
		if( m_CurrentGame->Update( &fd, &send_fd ) )
		{
			CONSOLE_Print( "[GHOST] deleting current game [" + m_CurrentGame->GetGameName( ) + "]" );
			delete m_CurrentGame;
			m_CurrentGame = NULL;

                        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
			{
				(*i)->QueueGameUncreate( );
				(*i)->QueueEnterChat( );
			}
		}
		else if( m_CurrentGame )
			m_CurrentGame->UpdatePost( &send_fd );
	}

	// update admin game

	if( m_AdminGame )
	{
		if( m_AdminGame->Update( &fd, &send_fd ) )
		{
			CONSOLE_Print( "[GHOST] deleting admin game" );
			delete m_AdminGame;
			m_AdminGame = NULL;
			AdminExit = true;
		}
		else if( m_AdminGame )
			m_AdminGame->UpdatePost( &send_fd );
	}

	// update running games

	for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); )
	{
		if( (*i)->Update( &fd, &send_fd ) )
		{
			CONSOLE_Print( "[GHOST] deleting game [" + (*i)->GetGameName( ) + "]" );
			EventGameDeleted( *i );
			delete *i;
			i = m_Games.erase( i );
		}
		else
		{
			(*i)->UpdatePost( &send_fd );
                        ++i;
		}
	}

	// update battle.net connections

        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
	{
		if( (*i)->Update( &fd, &send_fd ) )
			BNETExit = true;
	}

	// update GProxy++ reliable reconnect sockets

	if( m_Reconnect && m_ReconnectSocket )
	{
		CTCPSocket *NewSocket = m_ReconnectSocket->Accept( &fd );

		if( NewSocket )
			m_ReconnectSockets.push_back( NewSocket );
	}

	for( vector<CTCPSocket *> :: iterator i = m_ReconnectSockets.begin( ); i != m_ReconnectSockets.end( ); )
	{
		if( (*i)->HasError( ) || !(*i)->GetConnected( ) || GetTime( ) - (*i)->GetLastRecv( ) >= 10 )
		{
			delete *i;
			i = m_ReconnectSockets.erase( i );
			continue;
		}

		(*i)->DoRecv( &fd );
		string *RecvBuffer = (*i)->GetBytes( );
		BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *)RecvBuffer->c_str( ), RecvBuffer->size( ) );

		// a packet is at least 4 bytes

		if( Bytes.size( ) >= 4 )
		{
			if( Bytes[0] == GPS_HEADER_CONSTANT )
			{
				// bytes 2 and 3 contain the length of the packet

				uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

				if( Length >= 4 )
				{
					if( Bytes.size( ) >= Length )
					{
						if( Bytes[1] == CGPSProtocol :: GPS_RECONNECT && Length == 13 )
						{
							unsigned char PID = Bytes[4];
							uint32_t ReconnectKey = UTIL_ByteArrayToUInt32( Bytes, false, 5 );
							uint32_t LastPacket = UTIL_ByteArrayToUInt32( Bytes, false, 9 );

							// look for a matching player in a running game

							CGamePlayer *Match = NULL;

                                                        for( vector<CBaseGame *> :: iterator j = m_Games.begin( ); j != m_Games.end( ); ++j )
							{
								if( (*j)->GetGameLoaded( ) )
								{
									CGamePlayer *Player = (*j)->GetPlayerFromPID( PID );

									if( Player && Player->GetGProxy( ) && Player->GetGProxyReconnectKey( ) == ReconnectKey )
									{
										Match = Player;
										break;
									}
								}
							}

							if( Match )
							{
								// reconnect successful!

								*RecvBuffer = RecvBuffer->substr( Length );
								Match->EventGProxyReconnect( *i, LastPacket );
								i = m_ReconnectSockets.erase( i );
								continue;
							}
							else
							{
								(*i)->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_NOTFOUND ) );
								(*i)->DoSend( &send_fd );
								delete *i;
								i = m_ReconnectSockets.erase( i );
								continue;
							}
						}
						else
						{
							(*i)->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_INVALID ) );
							(*i)->DoSend( &send_fd );
							delete *i;
							i = m_ReconnectSockets.erase( i );
							continue;
						}
					}
				}
				else
				{
					(*i)->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_INVALID ) );
					(*i)->DoSend( &send_fd );
					delete *i;
					i = m_ReconnectSockets.erase( i );
					continue;
				}
			}
			else
			{
				(*i)->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_INVALID ) );
				(*i)->DoSend( &send_fd );
				delete *i;
				i = m_ReconnectSockets.erase( i );
				continue;
			}
		}

		(*i)->DoSend( &send_fd );
                ++i;
	}

	// autohost

	if( !m_AutoHostGameName.empty( ) && m_AutoHostMaximumGames != 0 && m_AutoHostAutoStartPlayers != 0 && GetTime( ) - m_LastAutoHostTime >= 30 )
	{
		// copy all the checks from CGHost :: CreateGame here because we don't want to spam the chat when there's an error
		// instead we fail silently and try again soon

		if( !m_ExitingNice && m_Enabled && !m_CurrentGame && m_Games.size( ) < m_MaxGames && m_Games.size( ) < m_AutoHostMaximumGames )
		{
			if( m_AutoHostMap->GetValid( ) )
			{
				string GameName = m_AutoHostGameName + " #" + UTIL_ToString( m_HostCounter );

				if( GameName.size( ) <= 31 )
				{
					CreateGame( m_AutoHostMap, GAME_PUBLIC, false, GameName, m_AutoHostOwner, m_AutoHostOwner, m_AutoHostServer, false );

					if( m_CurrentGame )
					{
						m_CurrentGame->SetAutoStartPlayers( m_AutoHostAutoStartPlayers );

						if( m_AutoHostMatchMaking )
						{
							if( !m_Map->GetMapMatchMakingCategory( ).empty( ) )
							{
								if( !( m_Map->GetMapOptions( ) & MAPOPT_FIXEDPLAYERSETTINGS ) )
									CONSOLE_Print( "[GHOST] autohostmm - map_matchmakingcategory [" + m_Map->GetMapMatchMakingCategory( ) + "] found but matchmaking can only be used with fixed player settings, matchmaking disabled" );
								else
								{
									CONSOLE_Print( "[GHOST] autohostmm - map_matchmakingcategory [" + m_Map->GetMapMatchMakingCategory( ) + "] found, matchmaking enabled" );

									m_CurrentGame->SetMatchMaking( true );
									m_CurrentGame->SetMinimumScore( m_AutoHostMinimumScore );
									m_CurrentGame->SetMaximumScore( m_AutoHostMaximumScore );
								}
							}
							else
								CONSOLE_Print( "[GHOST] autohostmm - map_matchmakingcategory not found, matchmaking disabled" );
						}
					}
				}
				else
				{
					CONSOLE_Print( "[GHOST] stopped auto hosting, next game name [" + GameName + "] is too long (the maximum is 31 characters)" );
					m_AutoHostGameName.clear( );
					m_AutoHostOwner.clear( );
					m_AutoHostServer.clear( );
					m_AutoHostMaximumGames = 0;
					m_AutoHostAutoStartPlayers = 0;
					m_AutoHostMatchMaking = false;
					m_AutoHostMinimumScore = 0.0;
					m_AutoHostMaximumScore = 0.0;
				}
			}
			else
			{
				CONSOLE_Print( "[GHOST] stopped auto hosting, map config file [" + m_AutoHostMap->GetCFGFile( ) + "] is invalid" );
				m_AutoHostGameName.clear( );
				m_AutoHostOwner.clear( );
				m_AutoHostServer.clear( );
				m_AutoHostMaximumGames = 0;
				m_AutoHostAutoStartPlayers = 0;
				m_AutoHostMatchMaking = false;
				m_AutoHostMinimumScore = 0.0;
				m_AutoHostMaximumScore = 0.0;
			}
		}

		m_LastAutoHostTime = GetTime( );
	}

	return m_Exiting || AdminExit || BNETExit;
}

void CGHost :: EventBNETConnecting( CBNET *bnet )
{
	try	
	{ 
		EXECUTE_HANDLER("BNETConnecting", true, boost::ref(this), boost::ref(bnet)) 
	}
	catch(...) 
	{ 
		return;
	}
	
	if( m_AdminGame )
		m_AdminGame->SendAllChat( m_Language->ConnectingToBNET( bnet->GetServer( ) ) );

	if( m_CurrentGame )
		m_CurrentGame->SendAllChat( m_Language->ConnectingToBNET( bnet->GetServer( ) ) );

	EXECUTE_HANDLER("BNETConnecting", false, boost::ref(this), boost::ref(bnet))
}

void CGHost :: EventBNETConnected( CBNET *bnet )
{
	try	
	{ 
		EXECUTE_HANDLER("BNETConnected", true, boost::ref(this), boost::ref(bnet)) 
	}
	catch(...) 
	{ 
		return;
	}
	
	if( m_AdminGame )
		m_AdminGame->SendAllChat( m_Language->ConnectedToBNET( bnet->GetServer( ) ) );

	if( m_CurrentGame )
		m_CurrentGame->SendAllChat( m_Language->ConnectedToBNET( bnet->GetServer( ) ) );

	EXECUTE_HANDLER("BNETConnected", false, boost::ref(this), boost::ref(bnet))
}

void CGHost :: EventBNETDisconnected( CBNET *bnet )
{
	try	
	{ 
		EXECUTE_HANDLER("BNETDisconnected", true, boost::ref(this), boost::ref(bnet)) 
	}
	catch(...) 
	{ 
		return;
	}	

	if( m_AdminGame )
		m_AdminGame->SendAllChat( m_Language->DisconnectedFromBNET( bnet->GetServer( ) ) );

	if( m_CurrentGame )
		m_CurrentGame->SendAllChat( m_Language->DisconnectedFromBNET( bnet->GetServer( ) ) );

	EXECUTE_HANDLER("BNETDisconnected", false, boost::ref(this), boost::ref(bnet))
}

void CGHost :: EventBNETLoggedIn( CBNET *bnet )
{
	try	
	{ 
		EXECUTE_HANDLER("BNETLoggedIn", true, boost::ref(this), boost::ref(bnet)) 
	}
	catch(...) 
	{ 
		return;
	}
	
	if( m_AdminGame )
		m_AdminGame->SendAllChat( m_Language->LoggedInToBNET( bnet->GetServer( ) ) );

	if( m_CurrentGame )
		m_CurrentGame->SendAllChat( m_Language->LoggedInToBNET( bnet->GetServer( ) ) );

	EXECUTE_HANDLER("BNETLoggedIn", false, boost::ref(this), boost::ref(bnet))
}

void CGHost :: EventBNETGameRefreshed( CBNET *bnet )
{
	try	
	{ 
		EXECUTE_HANDLER("BNETGameRefreshed", true, boost::ref(this), boost::ref(bnet)) 
	}
	catch(...) 
	{ 
		return;
	}
	
	if( m_AdminGame )
		m_AdminGame->SendAllChat( m_Language->BNETGameHostingSucceeded( bnet->GetServer( ) ) );

	if( m_CurrentGame )
		m_CurrentGame->EventGameRefreshed( bnet->GetServer( ) );

	EXECUTE_HANDLER("BNETGameRefreshed", false, boost::ref(this), boost::ref(bnet))
}

void CGHost :: EventBNETGameRefreshFailed( CBNET *bnet )
{
	try	
	{ 
		EXECUTE_HANDLER("BNETGameRefreshFailed", true, boost::ref(this), boost::ref(bnet)) 
	}
	catch(...) 
	{ 
		return;
	}
	
	if( m_CurrentGame )
	{
                for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			(*i)->QueueChatCommand( m_Language->UnableToCreateGameTryAnotherName( bnet->GetServer( ), m_CurrentGame->GetGameName( ) ) );

			if( (*i)->GetServer( ) == m_CurrentGame->GetCreatorServer( ) )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameTryAnotherName( bnet->GetServer( ), m_CurrentGame->GetGameName( ) ), m_CurrentGame->GetCreatorName( ), true );
		}

		if( m_AdminGame )
			m_AdminGame->SendAllChat( m_Language->BNETGameHostingFailed( bnet->GetServer( ), m_CurrentGame->GetGameName( ) ) );

		m_CurrentGame->SendAllChat( m_Language->UnableToCreateGameTryAnotherName( bnet->GetServer( ), m_CurrentGame->GetGameName( ) ) );

		// we take the easy route and simply close the lobby if a refresh fails
		// it's possible at least one refresh succeeded and therefore the game is still joinable on at least one battle.net (plus on the local network) but we don't keep track of that
		// we only close the game if it has no players since we support game rehosting (via !priv and !pub in the lobby)

		if( m_CurrentGame->GetNumHumanPlayers( ) == 0 )
			m_CurrentGame->SetExiting( true );

		m_CurrentGame->SetRefreshError( true );
	}

	EXECUTE_HANDLER("BNETGameRefreshFailed", false, boost::ref(this), boost::ref(bnet))
}

void CGHost :: EventBNETConnectTimedOut( CBNET *bnet )
{
	try	
	{ 
		EXECUTE_HANDLER("BNETConnectTimedOut", true, boost::ref(this), boost::ref(bnet)) 
	}
	catch(...) 
	{ 
		return;
	}
	
	if( m_AdminGame )
		m_AdminGame->SendAllChat( m_Language->ConnectingToBNETTimedOut( bnet->GetServer( ) ) );

	if( m_CurrentGame )
		m_CurrentGame->SendAllChat( m_Language->ConnectingToBNETTimedOut( bnet->GetServer( ) ) );

	EXECUTE_HANDLER("BNETConnectTimedOut", false, boost::ref(this), boost::ref(bnet))
}

void CGHost :: EventBNETWhisper( CBNET *bnet, string user, string message )
{
	try	
	{ 
		EXECUTE_HANDLER("BNETWhisper", true, boost::ref(this), boost::ref(bnet), user, message) 
	}
	catch(...) 
	{ 
		return;
	}
	
	if( m_AdminGame )
	{
		m_AdminGame->SendAdminChat( "[W: " + bnet->GetServerAlias( ) + "] [" + user + "] " + message );

		if( m_CurrentGame )
			m_CurrentGame->SendLocalAdminChat( "[W: " + bnet->GetServerAlias( ) + "] [" + user + "] " + message );

                for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); ++i )
			(*i)->SendLocalAdminChat( "[W: " + bnet->GetServerAlias( ) + "] [" + user + "] " + message );
	}

	EXECUTE_HANDLER("BNETWhisper", false, boost::ref(this), boost::ref(bnet), user, message)
}

void CGHost :: EventBNETChat( CBNET *bnet, string user, string message )
{
	try	
	{ 
		EXECUTE_HANDLER("BNETChat", true, boost::ref(this), boost::ref(bnet), user, message) 
	}
	catch(...) 
	{ 
		return;
	}

	if( m_AdminGame )
	{
		m_AdminGame->SendAdminChat( "[L: " + bnet->GetServerAlias( ) + "] [" + user + "] " + message );

		if( m_CurrentGame )
			m_CurrentGame->SendLocalAdminChat( "[L: " + bnet->GetServerAlias( ) + "] [" + user + "] " + message );

                for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); ++i )
			(*i)->SendLocalAdminChat( "[L: " + bnet->GetServerAlias( ) + "] [" + user + "] " + message );
	}

	EXECUTE_HANDLER("BNETChat", false, boost::ref(this), boost::ref(bnet), user, message)
}

void CGHost :: EventBNETEmote( CBNET *bnet, string user, string message )
{
	try	
	{ 
		EXECUTE_HANDLER("BNETEmote", true, boost::ref(this), boost::ref(bnet), user, message) 
	}
	catch(...) 
	{ 
		return;
	}	

	if( m_AdminGame )
	{
		m_AdminGame->SendAdminChat( "[E: " + bnet->GetServerAlias( ) + "] [" + user + "] " + message );

		if( m_CurrentGame )
			m_CurrentGame->SendLocalAdminChat( "[E: " + bnet->GetServerAlias( ) + "] [" + user + "] " + message );

                for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); ++i )
			(*i)->SendLocalAdminChat( "[E: " + bnet->GetServerAlias( ) + "] [" + user + "] " + message );
	}
	
	EXECUTE_HANDLER("BNETEmote", false, boost::ref(this), boost::ref(bnet), user, message)
}

void CGHost :: EventGameDeleted( CBaseGame *game )
{
	try	
	{ 
		EXECUTE_HANDLER("GameDeleted", true, boost::ref(this), boost::ref(game)) 
	}
	catch(...) 
	{ 
		return;
	}	

        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
	{
		(*i)->QueueChatCommand( m_Language->GameIsOver( game->GetDescription( ) ) );

		if( (*i)->GetServer( ) == game->GetCreatorServer( ) )
			(*i)->QueueChatCommand( m_Language->GameIsOver( game->GetDescription( ) ), game->GetCreatorName( ), true );
	}

	EXECUTE_HANDLER("GameDeleted", false, boost::ref(this), boost::ref(game))
}

void CGHost :: EventGameCreated( CBaseGame* game )
{
	try
	{
		EXECUTE_HANDLER("GameCreated", true, boost::ref(this), boost::ref(game))
	}
	catch(...)
	{
	}

	EXECUTE_HANDLER("GameCreated", false, boost::ref(this), boost::ref(game))
}

void CGHost :: ReloadConfigs( )
{
	CConfig CFG;
	CFG.Read( "default.cfg" );
	CFG.Read( gCFGFile );
	SetConfigs( &CFG );
}

void CGHost :: SetConfigs( CConfig *CFG )
{
	// this doesn't set EVERY config value since that would potentially require reconfiguring the battle.net connections
	// it just set the easily reloadable values

	m_LanguageFile = CFG->GetString( "bot_language", "language.cfg" );
	delete m_Language;
	m_Language = new CLanguage( m_LanguageFile );
	m_Warcraft3Path = UTIL_AddPathSeperator( CFG->GetString( "bot_war3path", "C:\\Program Files\\Warcraft III\\" ) );
	m_BindAddress = CFG->GetString( "bot_bindaddress", string( ) );
	m_ReconnectWaitTime = CFG->GetInt( "bot_reconnectwaittime", 3 );
	m_MaxGames = CFG->GetInt( "bot_maxgames", 5 );
	string BotCommandTrigger = CFG->GetString( "bot_commandtrigger", "!" );

	if( BotCommandTrigger.empty( ) )
		BotCommandTrigger = "!";

	m_CommandTrigger = BotCommandTrigger[0];
	m_MapCFGPath = UTIL_AddPathSeperator( CFG->GetString( "bot_mapcfgpath", string( ) ) );
	m_SaveGamePath = UTIL_AddPathSeperator( CFG->GetString( "bot_savegamepath", string( ) ) );
	m_MapPath = UTIL_AddPathSeperator( CFG->GetString( "bot_mappath", string( ) ) );
	m_SaveReplays = CFG->GetInt( "bot_savereplays", 0 ) == 0 ? false : true;
	m_ReplayPath = UTIL_AddPathSeperator( CFG->GetString( "bot_replaypath", string( ) ) );
	m_VirtualHostName = CFG->GetString( "bot_virtualhostname", "|cFF4080C0GHost" );
	m_HideIPAddresses = CFG->GetInt( "bot_hideipaddresses", 0 ) == 0 ? false : true;
	m_CheckMultipleIPUsage = CFG->GetInt( "bot_checkmultipleipusage", 1 ) == 0 ? false : true;

	if( m_VirtualHostName.size( ) > 15 )
	{
		m_VirtualHostName = "|cFF4080C0GHost";
		CONSOLE_Print( "[GHOST] warning - bot_virtualhostname is longer than 15 characters, using default virtual host name" );
	}

	m_SpoofChecks = CFG->GetInt( "bot_spoofchecks", 2 );
	m_RequireSpoofChecks = CFG->GetInt( "bot_requirespoofchecks", 0 ) == 0 ? false : true;
	m_ReserveAdmins = CFG->GetInt( "bot_reserveadmins", 1 ) == 0 ? false : true;
	m_RefreshMessages = CFG->GetInt( "bot_refreshmessages", 0 ) == 0 ? false : true;
	m_AutoLock = CFG->GetInt( "bot_autolock", 0 ) == 0 ? false : true;
	m_AutoSave = CFG->GetInt( "bot_autosave", 0 ) == 0 ? false : true;
	m_AllowDownloads = CFG->GetInt( "bot_allowdownloads", 0 );
	m_PingDuringDownloads = CFG->GetInt( "bot_pingduringdownloads", 0 ) == 0 ? false : true;
	m_MaxDownloaders = CFG->GetInt( "bot_maxdownloaders", 3 );
	m_MaxDownloadSpeed = CFG->GetInt( "bot_maxdownloadspeed", 100 );
	m_LCPings = CFG->GetInt( "bot_lcpings", 1 ) == 0 ? false : true;
	m_AutoKickPing = CFG->GetInt( "bot_autokickping", 400 );
	m_BanMethod = CFG->GetInt( "bot_banmethod", 1 );
	m_IPBlackListFile = CFG->GetString( "bot_ipblacklistfile", "ipblacklist.txt" );
	m_LobbyTimeLimit = CFG->GetInt( "bot_lobbytimelimit", 10 );
	m_Latency = CFG->GetInt( "bot_latency", 100 );
	m_SyncLimit = CFG->GetInt( "bot_synclimit", 50 );
	m_VoteKickAllowed = CFG->GetInt( "bot_votekickallowed", 1 ) == 0 ? false : true;
	m_VoteKickPercentage = CFG->GetInt( "bot_votekickpercentage", 100 );

	if( m_VoteKickPercentage > 100 )
	{
		m_VoteKickPercentage = 100;
		CONSOLE_Print( "[GHOST] warning - bot_votekickpercentage is greater than 100, using 100 instead" );
	}

	m_MOTDFile = CFG->GetString( "bot_motdfile", "motd.txt" );
	m_GameLoadedFile = CFG->GetString( "bot_gameloadedfile", "gameloaded.txt" );
	m_GameOverFile = CFG->GetString( "bot_gameoverfile", "gameover.txt" );
	m_LocalAdminMessages = CFG->GetInt( "bot_localadminmessages", 1 ) == 0 ? false : true;
	m_TCPNoDelay = CFG->GetInt( "tcp_nodelay", 0 ) == 0 ? false : true;
	m_MatchMakingMethod = CFG->GetInt( "bot_matchmakingmethod", 1 );
}

void CGHost :: ExtractScripts( )
{
	string PatchMPQFileName = m_Warcraft3Path + "War3Patch.mpq";
	HANDLE PatchMPQ;

	if( SFileOpenArchive( PatchMPQFileName.c_str( ), 0, MPQ_OPEN_FORCE_MPQ_V1, &PatchMPQ ) )
	{
		CONSOLE_Print( "[GHOST] loading MPQ file [" + PatchMPQFileName + "]" );
		HANDLE SubFile;

		// common.j

		if( SFileOpenFileEx( PatchMPQ, "Scripts\\common.j", 0, &SubFile ) )
		{
			uint32_t FileLength = SFileGetFileSize( SubFile, NULL );

			if( FileLength > 0 && FileLength != 0xFFFFFFFF )
			{
				char *SubFileData = new char[FileLength];
				DWORD BytesRead = 0;

				if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead ) )
				{
					CONSOLE_Print( "[GHOST] extracting Scripts\\common.j from MPQ file to [" + m_MapCFGPath + "common.j]" );
					UTIL_FileWrite( m_MapCFGPath + "common.j", (unsigned char *)SubFileData, BytesRead );
				}
				else
					CONSOLE_Print( "[GHOST] warning - unable to extract Scripts\\common.j from MPQ file" );

				delete [] SubFileData;
			}

			SFileCloseFile( SubFile );
		}
		else
			CONSOLE_Print( "[GHOST] couldn't find Scripts\\common.j in MPQ file" );

		// blizzard.j

		if( SFileOpenFileEx( PatchMPQ, "Scripts\\blizzard.j", 0, &SubFile ) )
		{
			uint32_t FileLength = SFileGetFileSize( SubFile, NULL );

			if( FileLength > 0 && FileLength != 0xFFFFFFFF )
			{
				char *SubFileData = new char[FileLength];
				DWORD BytesRead = 0;

				if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead ) )
				{
					CONSOLE_Print( "[GHOST] extracting Scripts\\blizzard.j from MPQ file to [" + m_MapCFGPath + "blizzard.j]" );
					UTIL_FileWrite( m_MapCFGPath + "blizzard.j", (unsigned char *)SubFileData, BytesRead );
				}
				else
					CONSOLE_Print( "[GHOST] warning - unable to extract Scripts\\blizzard.j from MPQ file" );

				delete [] SubFileData;
			}

			SFileCloseFile( SubFile );
		}
		else
			CONSOLE_Print( "[GHOST] couldn't find Scripts\\blizzard.j in MPQ file" );

		SFileCloseArchive( PatchMPQ );
	}
	else
		CONSOLE_Print( "[GHOST] warning - unable to load MPQ file [" + PatchMPQFileName + "] - error code " + UTIL_ToString( GetLastError( ) ) );
}

void CGHost :: LoadIPToCountryData( )
{
	ifstream in;
	in.open( "ip-to-country.csv" );

	if( in.fail( ) )
		CONSOLE_Print( "[GHOST] warning - unable to read file [ip-to-country.csv], iptocountry data not loaded" );
	else
	{
		CONSOLE_Print( "[GHOST] started loading [ip-to-country.csv]" );

		// the begin and commit statements are optimizations
		// we're about to insert ~4 MB of data into the database so if we allow the database to treat each insert as a transaction it will take a LONG time
		// todotodo: handle begin/commit failures a bit more gracefully

		if( !m_DBLocal->Begin( ) )
			CONSOLE_Print( "[GHOST] warning - failed to begin local database transaction, iptocountry data not loaded" );
		else
		{
			unsigned char Percent = 0;
			string Line;
			string IP1;
			string IP2;
			string Country;
			CSVParser parser;

			// get length of file for the progress meter

			in.seekg( 0, ios :: end );
			uint32_t FileLength = in.tellg( );
			in.seekg( 0, ios :: beg );

			while( !in.eof( ) )
			{
				getline( in, Line );

				if( Line.empty( ) )
					continue;

				parser << Line;
				parser >> IP1;
				parser >> IP2;
				parser >> Country;
				m_DBLocal->FromAdd( UTIL_ToUInt32( IP1 ), UTIL_ToUInt32( IP2 ), Country );

				// it's probably going to take awhile to load the iptocountry data (~10 seconds on my 3.2 GHz P4 when using SQLite3)
				// so let's print a progress meter just to keep the user from getting worried

				unsigned char NewPercent = (unsigned char)( (float)in.tellg( ) / FileLength * 100 );

				if( NewPercent != Percent && NewPercent % 10 == 0 )
				{
					Percent = NewPercent;
					CONSOLE_Print( "[GHOST] iptocountry data: " + UTIL_ToString( Percent ) + "% loaded" );
				}
			}

			if( !m_DBLocal->Commit( ) )
				CONSOLE_Print( "[GHOST] warning - failed to commit local database transaction, iptocountry data not loaded" );
			else
				CONSOLE_Print( "[GHOST] finished loading [ip-to-country.csv]" );
		}

		in.close( );
	}
}

void CGHost :: CreateGame( CMap *map, unsigned char gameState, bool saveGame, string gameName, string ownerName, string creatorName, string creatorServer, bool whisper )
{
	if( !m_Enabled )
	{
                for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == creatorServer )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameDisabled( gameName ), creatorName, whisper );
		}

		if( m_AdminGame )
			m_AdminGame->SendAllChat( m_Language->UnableToCreateGameDisabled( gameName ) );

		return;
	}

	if( gameName.size( ) > 31 )
	{
                for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == creatorServer )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameNameTooLong( gameName ), creatorName, whisper );
		}

		if( m_AdminGame )
			m_AdminGame->SendAllChat( m_Language->UnableToCreateGameNameTooLong( gameName ) );

		return;
	}

	if( !map->GetValid( ) )
	{
                for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == creatorServer )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameInvalidMap( gameName ), creatorName, whisper );
		}

		if( m_AdminGame )
			m_AdminGame->SendAllChat( m_Language->UnableToCreateGameInvalidMap( gameName ) );

		return;
	}

	if( saveGame )
	{
		if( !m_SaveGame->GetValid( ) )
		{
                        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
			{
				if( (*i)->GetServer( ) == creatorServer )
					(*i)->QueueChatCommand( m_Language->UnableToCreateGameInvalidSaveGame( gameName ), creatorName, whisper );
			}

			if( m_AdminGame )
				m_AdminGame->SendAllChat( m_Language->UnableToCreateGameInvalidSaveGame( gameName ) );

			return;
		}

		string MapPath1 = m_SaveGame->GetMapPath( );
		string MapPath2 = map->GetMapPath( );
		transform( MapPath1.begin( ), MapPath1.end( ), MapPath1.begin( ), (int(*)(int))tolower );
		transform( MapPath2.begin( ), MapPath2.end( ), MapPath2.begin( ), (int(*)(int))tolower );

		if( MapPath1 != MapPath2 )
		{
			CONSOLE_Print( "[GHOST] path mismatch, saved game path is [" + MapPath1 + "] but map path is [" + MapPath2 + "]" );

                        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
			{
				if( (*i)->GetServer( ) == creatorServer )
					(*i)->QueueChatCommand( m_Language->UnableToCreateGameSaveGameMapMismatch( gameName ), creatorName, whisper );
			}

			if( m_AdminGame )
				m_AdminGame->SendAllChat( m_Language->UnableToCreateGameSaveGameMapMismatch( gameName ) );

			return;
		}

		if( m_EnforcePlayers.empty( ) )
		{
                        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
			{
				if( (*i)->GetServer( ) == creatorServer )
					(*i)->QueueChatCommand( m_Language->UnableToCreateGameMustEnforceFirst( gameName ), creatorName, whisper );
			}

			if( m_AdminGame )
				m_AdminGame->SendAllChat( m_Language->UnableToCreateGameMustEnforceFirst( gameName ) );

			return;
		}
	}

	if( m_CurrentGame )
	{
                for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == creatorServer )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameAnotherGameInLobby( gameName, m_CurrentGame->GetDescription( ) ), creatorName, whisper );
		}

		if( m_AdminGame )
			m_AdminGame->SendAllChat( m_Language->UnableToCreateGameAnotherGameInLobby( gameName, m_CurrentGame->GetDescription( ) ) );

		return;
	}

	if( m_Games.size( ) >= m_MaxGames )
	{
                for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == creatorServer )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameMaxGamesReached( gameName, UTIL_ToString( m_MaxGames ) ), creatorName, whisper );
		}

		if( m_AdminGame )
			m_AdminGame->SendAllChat( m_Language->UnableToCreateGameMaxGamesReached( gameName, UTIL_ToString( m_MaxGames ) ) );

		return;
	}

	CONSOLE_Print( "[GHOST] creating game [" + gameName + "]" );

	if( saveGame )
		m_CurrentGame = new CGame( this, map, m_SaveGame, m_HostPort, gameState, gameName, ownerName, creatorName, creatorServer );
	else
		m_CurrentGame = new CGame( this, map, NULL, m_HostPort, gameState, gameName, ownerName, creatorName, creatorServer );

	EventGameCreated( m_CurrentGame );

	// todotodo: check if listening failed and report the error to the user

	if( m_SaveGame )
	{
		m_CurrentGame->SetEnforcePlayers( m_EnforcePlayers );
		m_EnforcePlayers.clear( );
	}

        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
	{
		if( whisper && (*i)->GetServer( ) == creatorServer )
		{
			// note that we send this whisper only on the creator server

			if( gameState == GAME_PRIVATE )
				(*i)->QueueChatCommand( m_Language->CreatingPrivateGame( gameName, ownerName ), creatorName, whisper );
			else if( gameState == GAME_PUBLIC )
				(*i)->QueueChatCommand( m_Language->CreatingPublicGame( gameName, ownerName ), creatorName, whisper );
		}
		else
		{
			// note that we send this chat message on all other bnet servers

			if( gameState == GAME_PRIVATE )
				(*i)->QueueChatCommand( m_Language->CreatingPrivateGame( gameName, ownerName ) );
			else if( gameState == GAME_PUBLIC )
				(*i)->QueueChatCommand( m_Language->CreatingPublicGame( gameName, ownerName ) );
		}

		if( saveGame )
			(*i)->QueueGameCreate( gameState, gameName, string( ), map, m_SaveGame, m_CurrentGame->GetHostCounter( ) );
		else
			(*i)->QueueGameCreate( gameState, gameName, string( ), map, NULL, m_CurrentGame->GetHostCounter( ) );
	}

	if( m_AdminGame )
	{
		if( gameState == GAME_PRIVATE )
			m_AdminGame->SendAllChat( m_Language->CreatingPrivateGame( gameName, ownerName ) );
		else if( gameState == GAME_PUBLIC )
			m_AdminGame->SendAllChat( m_Language->CreatingPublicGame( gameName, ownerName ) );
	}

	// if we're creating a private game we don't need to send any game refresh messages so we can rejoin the chat immediately
	// unfortunately this doesn't work on PVPGN servers because they consider an enterchat message to be a gameuncreate message when in a game
	// so don't rejoin the chat if we're using PVPGN

	if( gameState == GAME_PRIVATE )
	{
                for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetPasswordHashType( ) != "pvpgn" )
				(*i)->QueueEnterChat( );
		}
	}

	// hold friends and/or clan members

        for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
	{
		if( (*i)->GetHoldFriends( ) )
			(*i)->HoldFriends( m_CurrentGame );

		if( (*i)->GetHoldClan( ) )
			(*i)->HoldClan( m_CurrentGame );
	}
}

void CGHost :: RegisterPythonClass( )
{
	using namespace boost::python;

	class_<CGHost>("GHost", no_init)
		.add_property("UDPSocket", make_getter(&CGHost::m_UDPSocket, return_value_policy<reference_existing_object>()))
		.add_property("reconnectSocket", make_getter(&CGHost::m_ReconnectSocket, return_value_policy<reference_existing_object>()))
		.def_readonly("reconnectSockets", &CGHost::m_ReconnectSockets)
		.add_property("GPSProtocol", make_getter(&CGHost::m_GPSProtocol, return_value_policy<reference_existing_object>()))
		.add_property("CRC", make_getter(&CGHost::m_CRC, return_value_policy<reference_existing_object>()))
		.add_property("SHA", make_getter(&CGHost::m_SHA, return_value_policy<reference_existing_object>()))
		.def_readonly("BNETs", &CGHost::m_BNETs)
		.add_property("currentGame", make_getter(&CGHost::m_CurrentGame, return_value_policy<reference_existing_object>()))
		.add_property("adminGame", make_getter(&CGHost::m_AdminGame	, return_value_policy<reference_existing_object>()))
		.def_readonly("games", &CGHost::m_Games)
		.add_property("DB", make_getter(&CGHost::m_DB, return_value_policy<reference_existing_object>()))
		.add_property("DBLocal", make_getter(&CGHost::m_DBLocal, return_value_policy<reference_existing_object>()))
		.def_readonly("callables", &CGHost::m_Callables)
		.def_readonly("localAddresses", &CGHost::m_LocalAddresses)
		.add_property("language", make_getter(&CGHost::m_Language, return_value_policy<reference_existing_object>()))
		.add_property("map", make_getter(&CGHost::m_Map, return_value_policy<reference_existing_object>()))
		.add_property("adminMap", make_getter(&CGHost::m_AdminMap, return_value_policy<reference_existing_object>()))
		.add_property("autoHostMap", make_getter(&CGHost::m_AutoHostMap, return_value_policy<reference_existing_object>()))
		.add_property("saveGame", make_getter(&CGHost::m_SaveGame, return_value_policy<reference_existing_object>()))
		.def_readonly("enforcePlayers", &CGHost::m_EnforcePlayers)
		.def_readonly("exiting", &CGHost::m_Exiting)
		.def_readonly("exitingNice", &CGHost::m_ExitingNice)
		.def_readonly("enabled", &CGHost::m_Enabled)
		.def_readonly("version", &CGHost::m_Version)
		.def_readonly("hostCounter", &CGHost::m_HostCounter)
		.def_readonly("autoHostGameName", &CGHost::m_AutoHostGameName)
		.def_readonly("autoHostOwner", &CGHost::m_AutoHostOwner)
		.def_readonly("autoHostServer", &CGHost::m_AutoHostServer)
		.def_readonly("autoHostMaximumGames", &CGHost::m_AutoHostMaximumGames)
		.def_readonly("autoHostAutoStartPlayers", &CGHost::m_AutoHostAutoStartPlayers)
		.def_readonly("lastAutoHostTime", &CGHost::m_LastAutoHostTime)
		.def_readonly("autoHostMatchMaking", &CGHost::m_AutoHostMatchMaking)
		.def_readonly("autoHostMinimumScore", &CGHost::m_AutoHostMinimumScore)
		.def_readonly("autoHostMaximumScore", &CGHost::m_AutoHostMaximumScore)
		.def_readonly("allGamesFinished", &CGHost::m_AllGamesFinished)
		.def_readonly("allGamesFinishedTime", &CGHost::m_AllGamesFinishedTime)
		.def_readonly("languageFile", &CGHost::m_LanguageFile)
		.def_readonly("warcraft3Path", &CGHost::m_Warcraft3Path)
		.def_readonly("TFT", &CGHost::m_TFT)
		.def_readonly("bindAddress", &CGHost::m_BindAddress)
		.def_readonly("hostPort", &CGHost::m_HostPort)
		.def_readonly("reconnect", &CGHost::m_Reconnect)
		.def_readonly("reconnectPort", &CGHost::m_ReconnectPort)
		.def_readonly("reconnectWaitTime", &CGHost::m_ReconnectWaitTime)
		.def_readonly("maxGames", &CGHost::m_MaxGames)
		.def_readonly("commandTrigger", &CGHost::m_CommandTrigger)
		.def_readonly("mapCFGPath", &CGHost::m_MapCFGPath)
		.def_readonly("saveGamePath", &CGHost::m_SaveGamePath)
		.def_readonly("mapPath", &CGHost::m_MapPath)
		.def_readonly("saveReplays", &CGHost::m_SaveReplays)
		.def_readonly("replayPath", &CGHost::m_ReplayPath)
		.def_readonly("virtualHostName", &CGHost::m_VirtualHostName)
		.def_readonly("hideIPAddresses", &CGHost::m_HideIPAddresses)
		.def_readonly("checkMultipleIPUsage", &CGHost::m_CheckMultipleIPUsage)
		.def_readonly("spoofChecks", &CGHost::m_SpoofChecks)
		.def_readonly("requireSpoofChecks", &CGHost::m_RequireSpoofChecks)
		.def_readonly("reserveAdmins", &CGHost::m_ReserveAdmins)
		.def_readonly("refreshMessages", &CGHost::m_RefreshMessages)
		.def_readonly("autoLock", &CGHost::m_AutoLock)
		.def_readonly("autoSave", &CGHost::m_AutoSave)
		.def_readonly("allowDownloads", &CGHost::m_AllowDownloads)
		.def_readonly("pingDuringDownloads", &CGHost::m_PingDuringDownloads)
		.def_readonly("maxDownloaders", &CGHost::m_MaxDownloaders)
		.def_readonly("maxDownloadSpeed", &CGHost::m_MaxDownloadSpeed)
		.def_readonly("LCPings", &CGHost::m_LCPings)
		.def_readonly("autoKickPing", &CGHost::m_AutoKickPing)
		.def_readonly("banMethod", &CGHost::m_BanMethod)
		.def_readonly("IPBlackListFile", &CGHost::m_IPBlackListFile)
		.def_readonly("lobbyTimeLimit", &CGHost::m_LobbyTimeLimit)
		.def_readonly("latency", &CGHost::m_Latency)
		.def_readonly("syncLimit", &CGHost::m_SyncLimit)
		.def_readonly("voteKickAllowed", &CGHost::m_VoteKickAllowed)
		.def_readonly("voteKickPercentage", &CGHost::m_VoteKickPercentage)
		.def_readonly("defaultMap", &CGHost::m_DefaultMap)
		.def_readonly("MOTDFile", &CGHost::m_MOTDFile)
		.def_readonly("gameLoadedFile", &CGHost::m_GameLoadedFile)
		.def_readonly("gameOverFile", &CGHost::m_GameOverFile)
		.def_readonly("localAdminMessages", &CGHost::m_LocalAdminMessages)
		.def_readonly("adminGameCreate", &CGHost::m_AdminGameCreate)
		.def_readonly("adminGamePort", &CGHost::m_AdminGamePort)
		.def_readonly("adminGamePassword", &CGHost::m_AdminGamePassword)
		.def_readonly("adminGameMap", &CGHost::m_AdminGameMap)
		.def_readonly("LANWar3Version", &CGHost::m_LANWar3Version)
		.def_readonly("replayWar3Version", &CGHost::m_ReplayWar3Version)
		.def_readonly("replayBuildNumber", &CGHost::m_ReplayBuildNumber)
		.def_readonly("TCPNoDelay", &CGHost::m_TCPNoDelay)
		.def_readonly("matchMakingMethod", &CGHost::m_MatchMakingMethod)

		.def("reloadConfigs", &CGHost::ReloadConfigs)
		.def("setConfigs", &CGHost::SetConfigs)
		.def("extractScripts", &CGHost::ExtractScripts)
		.def("loadIPToCountryData", &CGHost::LoadIPToCountryData)
		.def("createGame", &CGHost::CreateGame)
	;
}