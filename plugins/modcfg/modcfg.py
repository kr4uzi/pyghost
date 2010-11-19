import shlex
import optparse
import os

import host
from host import GHost

try:
    import configobj.configobj as configobj
    from configobj.configobj import ConfigObj
except ImportError as E:
    print ('[MODCFG] Getmap depends on configobj plugin')
    raise

NOT_ALLOWED = ['cdkey', 'password', 'mysql']
admins_can_use = False

####################################################################################
##                    Command functions                                           ##
## take a reply function (a one-argument function like print()), and a payload    ##
## the first line of their doc string should be a help on how to use the command  ##  
####################################################################################

def parseArgs(cmdstring):
    parser = optparse.OptionParser()
    parser.add_option('-o','--cfg-file',dest='cfgfile', action='store',default='ghost.cfg',type='string')
    (options, args) = parser.parse_args(shlex.split(cmdstring))
    
    return options.cfgfile, args

def createCFG(reply, payload):
    '''Creates an empty config file'''
    
    cfgfile, args = parseArgs(payload)
    if len(args) < 1:
        reply('Need to specify a filename to create')
        return
    
    if 'ghost.cfg' in cfgfile:
        cfgfile = args[0]
    
    if os.path.exists(cfgfile):
        reply('Config file already exists')
        return
    
    if '/' in cfgfile or '\\' in cfgfile:
        with open(cfgfile, 'w') as cfg:
            pass
    else:
        if not os.path.exists(os.path.join(GHost.mapCFGPath, cfgfile)):
            with open(os.path.join(GHost.mapCFGPath, cfgfile),'w') as cfg:
                pass
        else:
            reply('Config file already exists')
            return
    
    reply("Successfully created configfile '%s'" % cfgfile)
    
def setCFG(reply, payload):
    '''Sets a config value, creating it if the key doesn't already exist'''
    
    cfgfile, args = parseArgs(payload.replace("=" , " "))
    if len(args) < 2:
        reply('Need to specify key name and value to set a config value...')
        return
    
    #else its either an absolute path or ghost.cfg, and no need to change cfgfile
    if cfgfile != 'ghost.cfg' and cfgfile in os.listdir(GHost.mapCFGPath):
        cfgfile = os.path.join(GHost.mapCFGPath, cfgfile)
    
    try:    
        cfg = ConfigObj(cfgfile, file_error=True, write_empty_values=True)
    except IOError:
        reply("Couldn't find config file... '%s'" % cfgfile )
    except configobj.ConfigObjError:
        reply("%s is not a valid Configuration file" % cfgfile)
        
    key = args[0]
    for na in NOT_ALLOWED:
        if na in key:
            reply("Not allowed to set that value")
            return
    
    #do work!
    cfg[args[0]] = args[1]
    cfg.write()
    
    reply("Successfully set %s = %s in '%s'" %(args[0], args[1], cfgfile))
    
def getCFG(reply, payload):
    '''Gets, and replies with a config value if it can be found in the given file
    '''
    cfgfile, args = parseArgs(payload)
    if len(args) < 1:
        reply('Need to specify key name to get a cfg value')
        return
    
    #else its either an absolute path or ghost.cfg, and no need to change cfgfile
    if cfgfile != 'ghost.cfg' and cfgfile in os.listdir(GHost.mapCFGPath):
        cfgfile = os.path.join(GHost.mapCFGPath, cfgfile)
    
    try:    
        cfg = ConfigObj(cfgfile, file_error=True)
    except IOError:
        reply("Couldn't find config file... '%s'" % cfgfile )
    except configobj.ConfigObjError:
        reply("%s is not a valid Configuration file" % cfgfile)
    
    key = args[0]
    for na in NOT_ALLOWED:
        if na in key:
            reply("Not allowed to get that value")
            return
    
    if key not in cfg:
        reply("That key does not exist in this configfile")
        return
    else:
        reply(key + " = " + cfg[key])
        

def init():
    '''Initializes this plugin, by registering a bunch of handlers'''
    host.registerHandler("PlayerCommand", onPlayerCommand)
    host.registerHandler("BNETChat", onMessage)
    host.registerHandler("BNETWhisper", onWhisper)

def deinit():
    '''Unregisters all handlers registered in init()'''
    host.unregisterHandler(onPlayerCommand)
    host.unregisterHandler(onMessage)
    host.unregisterHandler(onWhisper)

def reply(type, obj, user, whisper=False):
    '''A high-order function that defines how a command function
       should talk back to the user, returns functions that take one
      argument, a string, and respond appropriately
    '''
    if type is 'bnet':
        return lambda message: obj.queueChatCommand(message, user, whisper)
    else:
        return lambda message: obj.sendChat(user, message)
    
#stupid to have it here, but it needs to be defined after the functions
command_map={'cfgcreate' : createCFG, 'cfgset' : setCFG, 'cfgget' : getCFG}

##############################################################################
################### Event Handlers ###########################################
##############################################################################

def onWhisper(bnet, user, message):
    '''Passes a message along to onMessage'''
    onMessage(bnet, user, message, True)
    
def onMessage(bnet, user, message, whisper=False):
    '''Decides what to do with a chat message (calls the appropriate function
        if it is a command
    Keyword Arguments:
    bnet --a bnet instance
    user --the user who sent the message
    message --the actual message
    whisper --whether to respond with a whisper or not

    '''
    if message[0] is bnet.commandTrigger[0]:
        args = message[1:].split(None, 1)
        command = args[0]
        payload = args[1] if len(args) is 2 else ''
        if command in command_map:
            if admins_can_use:
                if bnet.admincheck(user, bnet.server) or user in bnet.rootAdmin:
                    command_map[command](reply('bnet', bnet, user, whisper), payload)
            else:
                if user in bnet.rootAdmin:
                    command_map[command](reply('bnet', bnet, user, whisper), payload)

def onPlayerCommand(game, player, command, payload):
    '''If the user is in the adminGame and logged in, allow them
        to use (call command function)
    '''
    if game is GHost.adminGame and player.loggedIn:
        if command in command_map.keys():
            command_map[command](reply('game', game, player), payload)