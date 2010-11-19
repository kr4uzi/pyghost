#Maximum filesize to DL in kB (default 10MB) kinda dumb because
#you need to have the file to know this :)
max_filesize = 10240

#allowed file extensions
allowed_exts = ('.w3x', '.w3m')

#determines whether all admins can use it, or not
admins_can_use = True

######################Don't edit below here #######################
import os.path
import host
from host import GHost
import re
import random

#getdota.com web address
DOTA_ADDR = "http://www.getdota.com"

#a list of all the mirrors for downloading dota maps
DOTA_HOSTS = {
'MegaJesus' : 'http://dota.megajesus.com/eng/',
'0v1' : 'http://0v1.org/d0tahost/eng/',
'Clan Woof' : 'http://www.clanwoof.com/dota/eng/',
'iCCup' :  'http://dotafiles.iccup.com/eng/',
'Graz' : 'http://81.223.126.91/eng/',
'LinkBG' : 'http://mirrors.joni-b.net/dota/eng/',
'Backup' : 'http://64.20.55.210/eng/'
}

import urllib2
import urlparse
try:
	from configobj.configobj import ConfigObj
except ImportError as E:
	print ('[GETMAP] Getmap depends on configobj plugin')
	raise

has_httplib2 = False

try:
	import httplib2.httplib2 as hl2
	http_con = hl2.Http(".cache")
	has_httplib2 = True
except ImportError:
	print('[GETMAP] You should get httplib2!')

class DownloadError(Exception):
	'''To indicate errors with downloading a map'''
	pass

def create_config( cfgpath, mapname, template='plugins/getmap/template.cfg' ):
	'''Creates a map cfg  from the given template'''
	
	mapcfg = ConfigObj(template)
	
	#set the only two sections that really matter :)
	mapcfg['map_path'] = 'maps\download\{0}'.format(mapname)
	mapcfg['map_localpath'] = mapname
	
	mapcfg.filename = cfgpath
	mapcfg.write()
	
def get_latest_dota_version():
	'''Returns the latest DotA version from getdota.com'''
	f = urllib2.urlopen(DOTA_ADDR)
	for line in f:
		if line.strip().lower().startswith('<div class="header">latest map:'):
			f.close()
			return (line.split('version">')[1]).split('<')[0]
	raise DownloadError("Couldn't get Latest DotA verson")

def download_map ( url_string, output_folder='' ):
	'''Downloads a map, checking the size and extension
	 
	 Keyword Arguments:
	 
	 url_string --the url to download (direct link to map)
	 output_folder --the location to save the map to
	'''
	#get the unquoted filename to save, and the extension
	filename = os.path.basename(urlparse.urlparse(urllib2.unquote(url_string))[2])
	extension = os.path.splitext(filename)[1]
	
	if extension not in allowed_exts:
		raise DownloadError('File is not a valid map to download')

	if has_httplib2:
		response, content = http_con.request(url_string)
		if response.status != 200:
			raise DownloadError('{0} - Error downloading {1}'.format(url_string,response.status))
	else:
		try:
			content = urllib2.urlopen(url_string).read()
		except:
			raise DownloadError('Error downloading {0}'.format(url_string))
	
	if len(content) > max_filesize * 1024:
		raise DownloadError('File too large to download')
		
	with open(os.path.join(output_folder, filename), 'wb') as local_map:
		local_map.write(content)
	
	return filename

def download_dota ( ver, output_folder='', mirror='' ):
	'''Downloads a dota map given only the version, defaults
	   to picking a random mirror
	   
	   Keyword Arguments:
	   
	   ver --the version to download (matching (\d\.\d{2,2}[a-z]?))
	   mirror --the key for the DOTA_HOSTS dictionary, decides where to dl from
	 '''
	
	if not re.match(r'^\d\.\d\d[a-z]?$',ver):
	 raise ValueError('{0} is not a valid version string (i.e. 6.66b)'.format(ver))
	 
	map_name = urllib2.quote('DotA Allstars v{0}.w3x'.format(ver))
	
	if mirror:
		host_url = DOTA_HOSTS[mirror]
	else:
		host_url = DOTA_HOSTS[random.choice(list(DOTA_HOSTS.keys()))]
		print('[GETMAP]Downloading from mirror ' + host_url)
	
	try:
		#call download map with the constructed url and map_name
		return download_map (urlparse.urljoin(host_url,map_name), output_folder)
	except DownloadError:
		for host, url in DOTA_HOSTS.iteritems():
			try:
				filename = download_map (urlparse.urljoin(url, map_name), output_folder)
				if os.path.isfile(os.path.join(output_folder, map_name)):
					return filename
			except Exception as E:
				print('[GETMAP]Failed downloading from {0}'.format(host))
				pass
				
def get_map(reply, payload):
	'''Parses the command and calls download_dota, or download_map
	
	Keyword arguments:
	reply --a function used to reply to the user
	payload --the arguments with which !getmap was called
	'''
	if not payload:
		reply('Downloads a map, given a url (or a version for dota) Usage is :')
		reply('getmap <url [templatename] OR dota [ver] [templatename]>')
		
	else:
		mappath = GHost.mapPath
		mapcfgpath = GHost.mapCFGPath
		
		# !getmap dota 6.66b template.cfg
		if 'dota' in payload.lower():
			if re.match('^dota\s+\d.\d\d[a-z]?\s?\w*',payload.lower()):
				args = payload.split()
				ver = args[1]
				try:
					template = args[2]
				except:
					template = 'template.cfg'
			#!getmap dota
			else:
				ver = get_latest_dota_version()
				try:
					template = payload.split()[1]
				except:
					template = 'template.cfg'
			template = os.path.join('plugins/getmap', template)
			
			map = 'DotA Allstars v{0}.w3x'.format(ver)
			mapcfg = os.path.join(mapcfgpath,'dota'+ver+'.cfg')
			
			#check if we already have the map/mapcfg
			if os.path.isfile(os.path.join(mappath, map)):
				if os.path.isfile(mapcfg):
					reply('Already have mapcfg and map for version {0}'.format(ver))
				else:
					reply('Already have map for version {0}, creating config'.format(ver))
					create_config(mapcfg, map, template )
					reply('Config dota{0}.cfg created'.format(ver))
			else:
				reply('Downloading and creating config for {0}'.format(map))
				mapname = download_dota(ver, mappath)
				if mapname:
					reply('Download complete! config name is dota{0}.cfg'.format(ver))
					create_config(mapcfg, map, template)
				else:
					reply("Error Downloading that map, maybe the servers don't have it?")
		else:
			args = payload.split()
			if len(args)>2:
				template = args[2]
			else: template = 'template.cfg'
			template = os.path.join('plugins/getmap/', template)
			
			try:
				reply('Downloading...')
				downloadedname = download_map( payload, mappath )
				create_config(os.path.join(mapcfgpath, downloadedname), downloadedname, template)
				reply('Successfully downloaded map {0}'.format(downloadedname))
			except:
				reply( 'Error downloading map' )
				raise
				
def init():
	'''Initializes this plugin, by registering a bunch of handlers'''
	import host
	from host import GHost
	host.registerHandler("PlayerCommand", onPlayerCommand)
	host.registerHandler("BNETChat", onMessage)
	host.registerHandler("BNETWhisper", onWhisper)
	host.registerHandler("BNETEmote", onMessage)

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
command_map={'getmap' : get_map, 'dlmap' : get_map}

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
		to use getmap (call command function)
	'''
	if game is GHost.adminGame and player.loggedIn:
		if command in command_map.keys():
			command_map[command](reply('game', game, player), payload)

