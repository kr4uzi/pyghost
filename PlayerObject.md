# Introduction #
The definition of the class CPlayer (whose instances are called PlayerObject afterwards) can be found in pyGHost++/python/ghost/Player.py.
To add methods, take a look at CreateNewMethodsAndHandlers.

# Attributes #
| **`Attribute`** | **`type`** | **`Description`** |
|:----------------|:-----------|:------------------|
| PID             | int        |
| name            | string     | the player's name |
| internalIP      | tuple      | the player's internal IP address as reported by the player when connecting |
| leftReason      | string     | the reason the player left the game |
| spoofedRealm    | string     | the realm the player last spoof checked on |
| joinedRealm     | string     | the realm the player joined on (probable, can be spoofed) |
| totalPacketsSent | int        |
| totalPacketsReceived | int        |
| leftCode        | int        | the code to be sent in W3GS\_PLAYERLEAVE\_OTHERS for why this player left the game |
| loginAttempts   | int        | the number of attempts to login (used with CAdminGame only) |
| syncCounter     | int        | the number of keepalive packets received from this player |
| joinTime        | int        | `GetTime` when the player joined the game (used to delay sending the /whois a few seconds to allow for some lag) |
| lastMapPartSent | int        | the last mappart sent to the player (for sending more than one part at a time) |
| lastMapPartAcked | int        | the last mappart acknowledged by the player |
| startedDownloadingTicks | int        | `GetTicks` when the player started downloading the map |
| finishedDownloadingTime | int        | `GetTime` when the player finished downloading the map |
| finishedLoadingTicks | int        | `GetTicks` when the player finished loading the game |
| startedLaggingTicks | int        | `GetTicks` when the player started lagging |
| statsSentTime   | int        | `GetTime` when we sent this player's stats to the chat (to prevent players from spamming !stats) |
| statsDotASentTime | int        | `GetTime` when we sent this player's dota stats to the chat (to prevent players from spamming !statsdota) |
| lastGProxyWaitNoticeSentTime | int        |
| score           | float      | the player's generic "score" for the matchmaking algorithm |
| loggedIn        | bool       | if the player has logged in or not (used with CAdminGame only) |
| spoofed         | bool       | if the player has spoof checked or not |
| reserved        | bool       | if the player is reserved (VIP) or not |
| whoisShouldBeSent | bool       | if a battle.net /whois should be sent for this player or not |
| whoisSent       | bool       | if we've sent a battle.net /whois for this player yet (for spoof checking) |
| downloadAllowed | bool       | if we're allowed to download the map or not (used with permission based map downloads) |
| downloadStarted | bool       | if we've started downloading the map or not |
| downloadFinished | bool       | if we've finished downloading the map or not |
| finishedLoading | bool       | if the player has finished loading or not |
| lagging         | bool       | if the player is lagging or not (on the lag screen) |
| dropVote        | bool       | if the player voted to drop the laggers or not (on the lag screen) |
| kickVote        | bool       | if the player voted to kick a player or not |
| muted           | bool       | if the player is muted or not |
| leftMessageSent | bool       | if the playerleave message has been sent or not |
| GProxy          | bool       | if the player is using GProxy++ |
| GProxyDisconnectNoticeSent | bool       | if a disconnection notice has been sent or not when using GProxy++ |
| GProxyReconnectKey | int        |
| lastGProxyAckTime | int        |

# Methods #
| **`Method( (type)identifier )`** | **`Description`** |
|:---------------------------------|:------------------|
| isValid()                        | returns True when the instance is pointing to an existing and valid c++ instance |