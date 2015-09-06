# Introduction #
The definition of the class `CMap` (whose instances are called `MapObject` afterwards) can be found in `pyGHost++/python/ghost/Map.py`.<br />
The MapObject can be accessed with GameObject.map.

# Attributes #
| **`Attribute`** | **`type`** | **`Description`** |
|:----------------|:-----------|:------------------|
| valid           | bool       |
| CFGFile         | string     |
| mapPath         | string     | config value: map path |
| mapSize         | tuple      | config value: map size (4 bytes) |
| mapInfo         | tuple      | config value: map info (4 bytes) -> this is the real CRC |
| mapCRC          | tuple      | config value: map crc (4 bytes) -> this is not the real CRC, it's the "xoro" value |
| mapSHA1         | tuple      | config value: map sha1 (20 bytes) |
| mapSpeed        | int        |
| mapVisibility   | int        |
| mapObservers    | int        |
| mapFlags        | int        |
| mapFilterMaker  | int        |
| mapFilterType   | int        |
| mapFilterSize   | int        |
| mapFilterObs    | int        |
| mapOptions      | int        |
| mapWidth        | tuple      | config value: map width (2 bytes) |
| mapHeight       | tuple      | config value: map height (2 bytes) |
| mapType         | string     | config value: map type (for stats class) |
| mapMatchMakingCategory | string     | config value: map matchmaking category (for matchmaking) |
| mapStatsW3MMDCategory | string     | config value: map stats w3mmd category (for saving w3mmd stats) |
| mapDefaultHCL   | string     | config value: map default HCL to use (this should really be specified elsewhere and not part of the map config) |
| mapDefaultPlayerScore | int        | config value: map default player score (for matchmaking) |
| mapLocalPath    | string     | config value: map local path |
| mapLoadInGame   | bool       |
| mapData         | string     | the map data itself, for sending the map to players |
| mapNumPlayers   | int        |
| mapNumTeams     | int        |

# Methods #
| **`Method( (type)identifier )`** | **`Description`** |
|:---------------------------------|:------------------|
| isValid()                        | returns True when the instance is pointing to an existing and valid c++ instance |