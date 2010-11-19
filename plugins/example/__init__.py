# =================================================================================
# This file defines some useful information about your plugin
# This one (example's) should be an exhaustive list of all values you can define
# However your plugin doesn't need to define all of them, the ones that don't apply
# Or you don't find useful can simply be left out, and pluginmanager will resort to
# defaults for the plugins. However if you are trying to get your plugin online, 
# its best to include as many as possible, and to make sure they make sense!
# These values can be fetched in pyghost using !plugininfo (!pinfo)
#                                                       -ilaggoodly
#==================================================================================

# Your plugin's version
__version__ = '0.2' 

# The author(s) of the plugin
__author__= "krauzi & ilaggoodly"  

# The author(s) email's
__author_email__ = 'awknaust@gmail.com' 

# The plugin's mentor (repository management)
__mentor__ = 'ilaggoodly' 

# The mentor's email
__mentor_email__ = __author_email__ 

# The url of your plugin's homepage
__url__ = 'http://code.google.com/p/pyghost/' 

# some tags that identify your plugin
__tags__ = ['example', 'test', 'admingame'] 

# who this plugin is geared towards
__category__ = 'developer' 

# A list of plugins that this plugin depends on, example doesn't depend on any other plugins!
__depends__= [] 

# the required versions (needs to be a dictionary)
__requires__ = {'ghost' : '17.0', 'python' : '2.6'} 

# if this method should actually be loaded, or only does stuff for other plugins
# This defaults to 0 if you leave it out
__islibrary__ = 0 

# A simple description of the plugin, so people know what they're downloading
__description__="An example plugin to test the system, and to be used as a basis for new plugins"

# A helpstring, that tells a user how to get started using your plugin
# in this case I just chose to use the description, because its not so complicated.
__help__ = __description__  

# A long description... could be used to describe the plugin in a website...
__long_description__='''This is a very long description about a very short plugin, 
basically on load the plugin will add a listener to the admingame (and current game)
and print a basic statement/ log you in automatically''' 