# Introduction #
The GHostObject is the main instance of the whole program. In this instance every config value is somehow saved. <br />
You can easily create new methods or add new attributes by adding the method in <br />
pyGHost++/python/ghost/GHost.py <br />
and <br />
pyGHost++/ghost/python.cpp (CPython::host\_methods and CPyInterface).<br /><br />
Infos about argument parsing can be found here:
http://docs.python.org/extending/extending.html#providing-a-c-api-for-an-extension-module

# Attributes #

| **`Attribute`** | **`type`** | **`Description`** |
|:----------------|:-----------|:------------------|
| exiting         | bool       | set to true to force ghost to shutdown next update (used by SignalCatcher) |
| exitingNice     | bool       | set to true to force ghost to disconnect from all battle.net connections and wait for all games to finish before shutting down |
| enabled         | bool       | set to false to prevent new games from being created |
| version         | string     | GHost++ version string |
| hostCounter     | int        | the current host counter (a unique number to identify a game, incremented each time a game is created) |
| autoHostGameName | string     | the base game name to auto host with |
| autoHostOwner   | string     |
| autoHostServer  | string     |
| autoHostMaximumGames | int        | maximum number of games to auto host |
| autoHostAutoStartPlayers | int        | when using auto hosting auto start the game when this many players have joined |
| lastAutoHostTime | int        | `GetTime` when the last auto host was attempted |
| autoHostMatchMaking | bool       |
| autoHostMinimumScore | float      |
| autoHostMaximumScore | float      |
| allGamesFinished | bool       | if all games finished (used when exiting nicely) |
| allGamesFinishedTime | int        | `GetTime` when all games finished (used when exiting nicely) |
| languageFile    | string     | config value: language file |
| warcraft3Path   | string     | config value: Warcraft 3 path |
| TFT             | bool       | config value: TFT enabled or not |
| bindAddress     | string     | config value: the address to host games on |
| hostPort        | int        | config value: the port to host games on |
| reconnect       | bool       | config value: GProxy++ reliable reconnects enabled or not |
| reconnectPort   | int        | config value: the port to listen for GProxy++ reliable reconnects on |
| reconnectWaitTime | int        | config value: the maximum number of minutes to wait for a GProxy++ reliable reconnect |
| maxGames        | int        | config value: maximum number of games in progress |
| commandTrigger  | string     | config value: the command trigger inside games |
| mapCFGPath      | string     | config value: map cfg path |
| saveGamePath    | string     | config value: savegame path |
| mapPath         | string     | config value: map path |
| saveReplays     | bool       | config value: save replays |
| replayPath      | string     | config value: replay path |
| virtualHostName | string     | config value: virtual host name |
| hideIPAddresses | bool       | config value: hide IP addresses from players |
| checkMultipleIPUsage | bool       | config value: check for multiple IP address usage |
| spoofChecks     | int        | config value: do automatic spoof checks or not |
| requireSpoofChecks | bool       | config value: require spoof checks or not |
| reserveAdmins   | bool       | config value: consider admins to be reserved players or not |
| refreshMessages | bool       | config value: display refresh messages or not (by default) |
| autoLock        | bool       | config value: auto lock games when the owner is present |
| autoSave        | bool       | config value: auto save before someone disconnects |
| allowDownloads  | int        | config value: allow map downloads or not |
| pingDuringDownloads | bool       | config value: ping during map downloads or not |
| maxDownloaders  | int        | config value: maximum number of map downloaders at the same time |
| maxDownloadSpeed | int        | config value: maximum total map download speed in KB/sec |
| LCPings         | bool       | config value: use LC style pings (divide actual pings by two) |
| autoKickPing    | int        | config value: auto kick players with ping higher than this |
| banMethod       | int        | config value: ban method (ban by name/ip/both) |
| IPBlackListFile | string     | config value: IP blacklist file (ipblacklist.txt) |
| lobbyTimeLimit  | int        | config value: auto close the game lobby after this many minutes without any reserved players |
| latency         | int        | config value: the latency (by default) |
| syncLimit       | int        | config value: the maximum number of packets a player can fall out of sync before starting the lag screen (by default) |
| voteKickAllowed | bool       | config value: if votekicks are allowed or not |
| voteKickPercentage | int        | config value: percentage of players required to vote yes for a votekick to pass |
| defaultMap      | string     | config value: default map (map.cfg) |
| MOTDFile        | string     | config value: motd.txt |
| gameLoadedFile  | string     | config value: gameloaded.txt |
| gameOverFile    | string     | config value: gameover.txt |
| localAdminMessages | bool       | config value: send local admin messages or not |
| adminGameCreate | bool       | config value: create the admin game or not |
| adminGamePort   | int        | config value: the port to host the admin game on |
| adminGamePassword | string     | config value: the admin game password |
| adminGameMap    | string     | config value: the admin game map config to use |
| LANWar3Version  | int        | config value: LAN warcraft 3 version |
| replayWar3Version | int        | config value: replay warcraft 3 version (for saving replays) |
| replayBuildNumber | int        | config value: replay build number (for saving replays) |
| TCPNoDelay      | bool       | config value: use Nagle's algorithm or not |
| matchMakingMethod | int        | config value: the matchmaking method |