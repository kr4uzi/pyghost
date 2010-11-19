"""This is an example plugin that provides some very basic
version info, as well as greeting players when they join the game
It can be handy when it logs you in to the admin game automatically,
or maybe thats dangerous! (With lots of documentation !)

You can visit the wiki for more info about the API, and how to create
your own plugins  @ http://code.google.com/p/pyghost/w/list
          --ilaggoodly
"""

import host #important this is pyGhost++'s object, needed to use handlers
from host import GHost #import the GHost object by name, for convenience
import platform #this is a built-in python module (used to get the version)


def init():
	"""This is the function that gets called when the plugin is loaded
		(docstrings are important!)
	"""
	#register a handler for when players join a game, which calls the function
	#'onPlayerJoined' which is defined below
	host.registerHandler("PlayerJoined", onPlayerJoined)
	
	#registers a handler for in-game commands
	host.registerHandler("PlayerCommand", onPlayerCommand)

def deinit():
	"""
	This function gets called when the function is unloaded, here you
	   unregister any registered handlers
	"""
	#unregisters the handler, by calling host.unregisterhandler with the
	#the 'onPlayerJoined' function as an argument
	host.unregisterHandler(onPlayerJoined)
	
	host.unregisterHandler(onPlayerCommand)
	
def onPlayerJoined(game, player):
	"""Sends some trivial messages when players join the game, or
	   logs them in if it is the admin game
	   
	   Keyword Arguments:
	   game --a host.game object that is passed to this function from its' handler
	   player --the host.player object that joined the game
	  """
	
	#Tell the player some info about python/ghost versions
	game.sendChat(player, 'Python version : {0}, pyGHost++ version : {1}'.format(
			platform.python_version(), GHost.version))
   
    #use the GHost.currentGame attribute to see if he joined the game in lobby (not admin game)
	if game == GHost.currentGame:
		  game.sendChat(player, "Hello Player {0}, you have joined the lobby game: {1}".format(
			player.name, game.gameName))
	
	#else, if they are in the admingame, greet them and log them in	  
	elif game == GHost.adminGame:
		game.sendChat(player, "Hi, " + player.name + "! This is a python test, you should now be logged in!")
		player.loggedIn = True #now the player is logged in !
	
	else:
		print("This is a log for ghost which should never be printed because a player cant join a game except CurrentGame or AdminGame!")

def onPlayerCommand(game, player, command, payload):
	"""Tells everyone in the lobby if a player used a command
	   
	   Keyword arguments :
	
	   game --a host.Game object where this event occurred
	   player --a host.player object that executed the command
	   command --the name of the command ex. 'swap'
	   payload --with what the command was executed, .ex '1 2' in '!swap 1 2'
	"""
	
	if game == GHost.currentGame or game == GHost.adminGame:
		game.sendAllChat("Player {0} just used command '{1}'".format(player.name, command))
		