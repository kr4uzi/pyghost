# Introduction #
The `GameObject` is used very often. If the game where it is called is the `AdminGame`, the `GameObject` is a instance of `CAdminGame`, otherwise of `CGame`. This classes are defined in `pyGHost++/python/ghost/Game.py` respectively `AdminGame.py`. <br />
To add new functions, take a look CreateNewMethodsAndHandlers.


# Attributes #
| **`Attribute`** | **`type`** | **`Description`** |
|:----------------|:-----------|:------------------|
| exiting         | bool       | set to true and this class will be deleted next update |
| saving          | bool       | if we're currently saving game data to the database |
| hostPort        | int        | the port to host games on |
| gameState       | int        | game state, public or private |
| virtualHostPID  | int        | virtual host's PID |
| fakePlayerPID   | int        | the fake player's PID (if present) |
| GProxyEmptyActions | int        |
| gameName        | string     | game name         |
| lastGameName    | string     | last game name (the previous game name before it was rehosted) |
| virtualHostName | string     | virtual host's name |
| ownerName       | string     | name of the player who owns this game (should be considered an admin) |
| creatorName     | string     | name of the player who created this game |
| creatorServer   | string     | battle.net server the player who created this game was on |
| announceMessage | string     | a message to be sent every `m_AnnounceInterval` seconds |
| statString      | string     | the stat string when the game started (used when saving replays) |
| kickVotePlayer  | string     | the player to be kicked with the currently running kick vote |
| HCLCommandString | string     | the `"HostBot Command Library"` command string, used to pass a limited amount of data to specially designed maps |
| randomSeed      | int        | the random seed sent to the Warcraft III clients |
| hostCounter     | int        | a unique game number |
| latency         | int        | the number of ms to wait between sending action packets (we queue any received during this time) |
| syncLimit       | int        | the maximum number of packets a player can fall out of sync before starting the lag screen |
| syncCounter     | int        | the number of actions sent so far (for determining if anyone is lagging) |
| gameTicks       | int        | ingame ticks      |
| creationTime    | int        | `GetTime` when the game was created |
| lastPingTime    | int        | `GetTime` when the last ping was sent |
| lastRefreshTime | int        | `GetTime` when the last game refresh was sent |
| lastDownloadTicks | int        | `GetTicks` when the last map download cycle was performed |
| downloadCounter | int        | # of map bytes downloaded in the last second |
| lastDownloadCounterResetTicks | int        | `GetTicks` when the download counter was last reset |
| lastAnnounceTime | int        | `GetTime` when the last announce message was sent |
| announceInterval | int        | how many seconds to wait between sending the `m_AnnounceMessage` |
| lastAutoStartTime | int        | the last time we tried to auto start the game |
| autoStartPlayers | int        | auto start the game when there are this many players or more |
| lastCountDownTicks | int        | `GetTicks` when the last countdown message was sent |
| countDownCounter | int        | the countdown is finished when this reaches zero |
| startedLoadingTicks | int        | `GetTicks` when the game started loading |
| startPlayers    | int        | number of players when the game started |
| lastLagScreenResetTime | int        | `GetTime` when the "lag" screen was last reset |
| lastActionSentTicks | int        | `GetTicks` when the last action packet was sent |
| lastActionLateBy | int        | the number of ticks we were late sending the last action packet by |
| startedLaggingTime | int        | `GetTime` when the last lag screen started |
| lastLagScreenTime | int        | `GetTime` when the last lag screen was active (continuously updated) |
| lastReservedSeen | int        | `GetTime` when the last reserved player was seen in the lobby |
| startedKickVoteTime | int        | `GetTime` when the kick vote was started |
| gameOverTime    | int        | `GetTime` when the game was over |
| lastPlayerLeaveTicks | int        | `GetTicks` when the most recent player left the game |
| minimumScore    | float      | the minimum allowed score for matchmaking mode |
| maximumScore    | float      | the maximum allowed score for matchmaking mode |
| slotInfoChanged | bool       | if the slot info has changed and hasn't been sent to the players yet (optimization) |
| locked          | bool       | if the game owner is the only one allowed to run game commands or not |
| refreshMessages | bool       | if we should display "game refreshed..." messages or not |
| refreshError    | bool       | if there was an error refreshing the game |
| refreshRehosted | bool       | if we just rehosted and are waiting for confirmation that it was successful |
| muteAll         | bool       | if we should stop forwarding ingame chat messages targeted for all players or not |
| muteLobby       | bool       | if we should stop forwarding lobby chat messages |
| countDownStarted | bool       | if the game start countdown has started or not |
| gameLoading     | bool       | if the game is currently loading or not |
| gameLoaded      | bool       | if the game has loaded or not |
| loadInGame      | bool       | if the load-in-game feature is enabled or not |
| lagging         | bool       | if the lag screen is active or not |
| autoSave        | bool       | if we should auto save the game before someone disconnects |
| matchMaking     | bool       | if matchmaking mode is enabled |
| localAdminMessages | bool       | if local admin messages should be relayed or not |
| map             | instance   | a instance of the currently loaded map (read-only) |
| password        | string     | only available in CAdminGame: the `AdminGame` password |

# Methods #
| **`Method( (type)identifier )`** | **`Description`** |
|:---------------------------------|:------------------|
| isValid()                        | returns True when the object is valid and still existing |
| getSIDFromPID(int pid)           | -                 |
| getPlayerFromName(string name, bool sensitive=False) | returns the instance of the player with name name. sensitive is False by default, so if you want to pass the exact name, set it to True. |
| getPlayerFromPID(int pid)        | -                 |
| getPlayerFromSID(int sid)        | -                 |
| isOwner(string name)             | -                 |
| isReserved(string name)          | -                 |
| getNumOfPlayers()                | returns the number of players (including computers and virtual players) |
| getNumOfHumanPlayers()           | returns the number of human players |
| getPlayers()                     | returns a tuple of all currently existing players |
| getSlotSize()                    | returns the size of slots |
| setAnounce(int interval, string message) | sets the gameannounce with message message every interval seconds |
| setSlotColour(int sid, int colour) | sets the colour of slot with SlotID sid to colour |
| addToSpoofed(string server, string name, bool sendMessage=False | adds the player with name name to the spoof list on server with name server. |
| addToReserved(string name)       | adds the player with name name to the reserved list |
| sendChat(int toPlayer, string message, int fromPlayer=None) | sends the message message to the player toPlayer. By default this message is send from the game owner or from the `VirtualHost` if existing. If fromPID is set, the player with that index will send the message. |
| sendAllChat(string message, int fromPID=None) | The gameowner or the `VirtualHost` will send the message message if fromPID is NOT set, otherwise the player with that index will send the message |
| sendWelcomeMessage(int pid)      | -                 |
| sendEndMessage()                 | -                 |
| sendAllSlotInfo()                | -                 |
| swapSlots(int sid1, int sid2)    | -                 |
| openSlot(int sid, bool kick=False) | -                 |
| closeSlot(int sid)               | -                 |
| computerSlot(int sid, int skill, bool kick=False) | -                 |
| openAllSlots()                   | -                 |
| closeAllSlots()                  | -                 |
| shuffleSlots()                   | -                 |
| balanceSlots                     | -                 |
| saveGameData()                   | -                 |
| startCountDown(bool force=False) | -                 |
| startCountDownAuto(bool requireSpoofChecks=False) | -                 |
| stopPlayers(string reason)       | -                 |
| stopLaggers(string reason)       | -                 |
| createVirtualHost()              | -                 |
| deleteVirtualHost()              | -                 |
| createFakePlayer()               | -                 |
| deleteVirtualHost()              | -                 |
| sendAdminChat(string message)    | sends the message message to everyone in the `AdminGame` |