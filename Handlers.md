# What are Handlers? #
Handlers are python entry points in C++. Every function you register on a certain event will be called once this entry point is reached in C++.
You can register any function which has the required count of arguments. The handlers you are allowed to register functions and the argument (types) they require are listed below.
Example:
```
import host

def myFunction(arg1, arg2):
    print "hello world from python!"

host.registerHandler("handlerName", myFunction)
```
The explanation:
Every Python-function is saved in the RAM. This function has a adress. In the Python IDLE you can try this:
```
>>> def myFunction():
	return

>>> myFunction
<function myFunction at 0x02784430>
```
The function address (which is the hex value after "<function myFunction at") may vary on every system.<br />
This function can be called by every program running on this computer (in theory). So you call host.registerHandler("handlerName", myFunction) which would now look like this:<br />`host.registerHandler("handlerName", <function myFunction at 0x02784430>)`<br /><br />
The `host.registerHandler`-function now registers `MyFunction` (the second argument) for the event (= handler) "handlerName".<br />
This should now be enough to understand the basic way the python system works in pyGHost++. If you want to add new handlers/events or new methods for certain instances, take a look at CreateNewMethodsAndHandlers.

# The Handlers #
## BNET Handlers ##
| **Handlername** | **(type)identifier** |
|:----------------|:---------------------|
| `BNETConecting` | (instance)BnetObject |
| `BNETConnected` | (instance)BnetObject |
| `BNETDisconnected` | (instance)BnetObject |
| `BNETLoggedIn`  | (instance)BnetObject |
| `BNETRefreshed` | (instance)BnetObject |
| `BNETGameRefreshFailed` | (instance)BnetObject |
| `BNETConnectTimeOut` | (instance)BnetObject |
| `BNETWhisper`   | (instance)BnetObject, (string)`Username`, (string)`Message` |
| `BNETChat`      | (instance)BnetObject, (string)`Username`, (string)`Message` |
| `BNETEmote`     | (instance)BnetObject, (string)`Username`, (string)`Message` |

## Game Handlers ##
| **Handlername** | **(type)identifier** | **Description/Comment** |
|:----------------|:---------------------|:------------------------|
| `GameDeleted`   | (instance)GameObject | This is called when a game has finished, not when its e.g. unhosted |
| `GameRefreshed` | (instance)GameObject, (string)`server`, (bool)`RefreshRehosted` |
| `GameStarted`   | (instance)GameObject |
| `GameLoaded`    |(instance)GameObject  |

## Player Handlers ##
| **Handlername** | **(type)identifier** | **Description/Comment** |
|:----------------|:---------------------|:------------------------|
| `PlayerDeleted` | (instance)GameObject, (instance)PlayerObject |
| `PlayerDisconnectTimedOut` | (instance)GameObject, (instance)PlayerObject |
| `PlayerDisconnectPlayerError` | (instance)GameObject, (instance)PlayerObject |
| `PlayerDisconnectSocketError` | (instance)GameObject, (instance)PlayerObject |
| `PlayerDisconnectConnectionClosed` | (instance)GameObject, (instance)PlayerObject |
| `PlayerJoined`  | (instance)GameObject, (instance)PlayerObject |
| `PlayerJoinedWithScore` | (instance)GameObject, (instance)PlayerObject |
| `PlayerLeft`    | (instance)GameObject, (instance)PlayerObject |
| `PlayerLoaded`  | (instance)GameObject, (instance)PlayerObject |
| `PlayerAction`  | (instance)GameObject, (instance)PlayerObject, (tuple)`action` | the action-tuple is created from "BYTEARRAY action" |
| `PlayerJoinedWithScore` | (instance)GameObject, (instance)PlayerObject, (int)`checksum` |
| `PlayerChatToHost` | (instance)GameObject, (instance)PlayerObject, (string)`Message` |
| `PlayerBotCommand` | (instance)GameObject, (instance)PlayerObject, (string)`command`, (string)payload |
| `PlayerChangeTeam` | (instance)GameObject, (instance)PlayerObject, (int)`team` |
| `PlayerChangeColour` | (instance)GameObject, (instance)PlayerObject, (int)`colour` |
| `PlayerChangeRace` | (instance)GameObject, (instance)PlayerObject, (int)`race` |
| `PlayerChangeHandicap` | (instance)GameObject, (instance)PlayerObject, (int)`handicap` |
| `PlayerPlayerDropRequest` | (instance)GameObject, (instance)PlayerObject |
| `PlayerPongToHost` | (instance)GameObject, (instance)PlayerObject, (int)`pong` |