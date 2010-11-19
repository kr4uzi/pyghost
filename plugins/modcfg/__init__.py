# Your plugin's version
__version__ = '0.1' 

# The author(s) of the plugin
__author__= "ilaggoodly"  

# The author(s) email's
__author_email__ = 'awknaust@gmail.com' 

# The plugin's mentor (repository management)
__mentor__ = 'ilaggoodly' 

# The mentor's email
__mentor_email__ = __author_email__ 

# The url of your plugin's homepage
__url__ = 'http://code.google.com/p/pyghost/' 

# some tags that identify your plugin
__tags__ = ['cfg', 'admin', 'map'] 

# who this plugin is geared towards
__category__ = 'Root Admins' 

# A list of plugins that this plugin depends on, example doesn't depend on any other plugins!
__depends__= ['configobj'] 

# the required versions (needs to be a dictionary)
__requires__ = {'ghost' : '17.0', 'python' : '2.5'} 

# if this method should actually be loaded, or only does stuff for other plugins
# This defaults to 0 if you leave it out
__islibrary__ = 0 

# A simple description of the plugin, so people know what they're downloading
__description__="A plugin to let you modify ghost.cfg and map.cfgs using simple commands"

# A helpstring, that tells a user how to get started using your plugin
# in this case I just chose to use the description, because its not so complicated.
__help__ = "!cfgset [-o cfg filepath automatically searches mapcfgs/] <key> <newvalue>, !cfgget [-o] <key>, !cfgcreate <name>"

# A long description... could be used to describe the plugin in a website...
__long_description__='''Makes use of the configobj library to open arbitrary config-style files, and edit them.
Meant to be used mainly with ghost.cfg (!reloadcfg after), and mapcfgs, although you can use it with any config
in the pyghost++ folder. You cannot use it to get/set password or cdkeys''' 