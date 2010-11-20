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
#include "gameslot.h"

//
// CGameSlot
//

CGameSlot :: CGameSlot( BYTEARRAY &n ) : m_PID( 0 ), m_DownloadStatus( 255 ), m_SlotStatus( SLOTSTATUS_OPEN ), m_Computer( 0 ), m_Team( 0 ), m_Colour( 1 ), m_Race( SLOTRACE_RANDOM ), m_ComputerType( SLOTCOMP_NORMAL ), m_Handicap( 100 )
{
	if( n.size( ) >= 7 )
	{
		m_PID = n[0];
		m_DownloadStatus = n[1];
		m_SlotStatus = n[2];
		m_Computer = n[3];
		m_Team = n[4];
		m_Colour = n[5];
		m_Race = n[6];

		if( n.size( ) >= 8 )
			m_ComputerType = n[7];

		if( n.size( ) >= 9 )
			m_Handicap = n[8];
	}
}

CGameSlot :: CGameSlot( unsigned char nPID, unsigned char nDownloadStatus, unsigned char nSlotStatus, unsigned char nComputer, unsigned char nTeam, unsigned char nColour, unsigned char nRace, unsigned char nComputerType, unsigned char nHandicap )
    : m_PID( nPID ), m_DownloadStatus( nDownloadStatus ), m_SlotStatus( nSlotStatus ), m_Computer( nComputer ), m_Team( nTeam ), m_Colour( nColour ), m_Race( nRace ), m_ComputerType( nComputerType ), m_Handicap( nHandicap )
{

}

CGameSlot :: ~CGameSlot( )
{

}

BYTEARRAY CGameSlot :: GetByteArray( ) const
{
	BYTEARRAY b;
	b.push_back( m_PID );
	b.push_back( m_DownloadStatus );
	b.push_back( m_SlotStatus );
	b.push_back( m_Computer );
	b.push_back( m_Team );
	b.push_back( m_Colour );
	b.push_back( m_Race );
	b.push_back( m_ComputerType );
	b.push_back( m_Handicap );
	return b;
}

#include <boost/python.hpp>

void CGameSlot :: RegisterPythonClass( )
{
	using namespace boost::python;

	class_<CGameSlot>("gameSlot", no_init)
		.def_readonly("PID", &CGameSlot::m_PID)
		.def_readonly("m_DownloadStatus", &CGameSlot::m_DownloadStatus)
		.def_readonly("m_SlotStatus", &CGameSlot::m_SlotStatus)
		.def_readonly("m_Computer", &CGameSlot::m_Computer)
		.def_readonly("team", &CGameSlot::m_Team)
		.def_readonly("colour", &CGameSlot::m_Colour)
		.def_readonly("m_Race", &CGameSlot::m_Race)
		.def_readonly("m_ComputerType", &CGameSlot::m_ComputerType)
		.def_readonly("handicap", &CGameSlot::m_Handicap)

		.def("getPID", &CGameSlot::GetPID)
		.def("getDownloadStatus", &CGameSlot::GetDownloadStatus)
		.def("getSlotStatus", &CGameSlot::GetSlotStatus)
		.def("getComputer", &CGameSlot::GetComputer)
		.def("getTeam", &CGameSlot::GetTeam)
		.def("getColour", &CGameSlot::GetColour)
		.def("getRace", &CGameSlot::GetRace)
		.def("getComputerType", &CGameSlot::GetComputerType)
		.def("getHandicap", &CGameSlot::GetHandicap)

		.def("setPID", &CGameSlot::SetPID)
		.def("setDownloadStatus", &CGameSlot::SetDownloadStatus)
		.def("setSlotStatus", &CGameSlot::SetSlotStatus)
		.def("setComputer", &CGameSlot::SetComputer)
		.def("setTeam", &CGameSlot::SetTeam)
		.def("setColour", &CGameSlot::SetColour)
		.def("setRace", &CGameSlot::SetRace)
		.def("setComputerType", &CGameSlot::SetComputerType)
		.def("setHandicap", &CGameSlot::SetHandicap)

		.def("getByteArray", &CGameSlot::GetByteArray)
	;
}