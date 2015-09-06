# Introduction #
The introduction for [GHostObject](GHostObject.md) applies here too.

# Attributes #
| **`Attribute`** | **`type`** | **`Description`** |
|:----------------|:-----------|:------------------|
| exiting         | bool       | set to true and this class will be deleted next update |
| server          | string     | battle.net server to connect to |
| serverIP        | string     | battle.net server to connect to (the IP address so we don't have to resolve it every time we connect) |
| serverAlias     | string     | battle.net server alias (short name, e.g. "USEast") |
| BNLSServer      | string     | BNLS server to connect to (for warden handling) |
| BNLSPort        | int        | BNLS port         |
| BNLSWardenCookie | int        | BNLS warden cookie |
| CDKeyROC        | string     | ROC CD key        |
| CDKeyTFT        | string     | TFT CD key        |
| countryAbbrev   | string     | country abbreviation |
| country         | string     | country           |
| localeID        | int        | see: http://msdn.microsoft.com/en-us/library/0h88fahh%28VS.85%29.aspx |
| userName        | string     | battle.net username |
| userPassword    | string     | battle.net password |
| firstChannel    | string     | the first chat channel to join upon entering chat (note: we hijack this to store the last channel when entering a game) |
| currentChannel  | string     | the current chat channel |
| rootAdmin       | string     | the root admin    |
| commandTrigger  | string     | the character prefix to identify commands |
| war3Version     | int        | custom warcraft 3 version for PvPGN users |
| EXEVersion      | tuple      | custom exe version for PvPGN users |
| EXEVersionHash  | tuple      | custom exe version hash for PvPGN users |
| passwordHashType | string     | password hash type for PvPGN users |
| PVPGNRealmName  | string     | realm name for PvPGN users (for mutual friend spoofchecks) |
| maxMessageLength | int        | maximum message length for PvPGN users |
| hostCounterID   | int        | the host counter ID to identify players from this realm |
| lastDisconnectedTime | int        | `GetTime` when we were last disconnected from battle.net |
| lastConnectionAttemptTime | int        | `GetTime` when we last attempted to connect to battle.net |
| lastNullTime    | int        | `GetTime` when the last null packet was sent for detecting disconnects |
| lastOutPacketTicks | int        | `GetTicks` when the last packet was sent for the m\_OutPackets queue |
| lastOutPacketSize | int        |
| lastAdminRefreshTime | int        | `GetTime` when the admin list was last refreshed from the database |
| lastBanRefreshTime | int        | `GetTime` when the ban list was last refreshed from the database |
| firstConnect    | bool       | if we haven't tried to connect to battle.net yet |
| waitingToConnect | bool       | if we're waiting to reconnect to battle.net after being disconnected |
| loggedIn        | bool       | if we've logged into battle.net or not |
| inChat          | bool       | if we've entered chat or not (but we're not necessarily in a chat channel yet) |
| holdFriends     | bool       | whether to auto hold friends when creating a game or not |
| holdClan        | bool       | whether to auto hold clan members when creating a game or not |
| publicCommands  | bool       | whether to allow public commands or not |

# Methods #
| **`Method( (type)identifier )`** | **`Description`** |
|:---------------------------------|:------------------|
| isValid()                        | returns True when the object is valid and still existing |