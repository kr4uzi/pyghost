An introduction to how the plugins work, and how to make your own

# What is pyGHost++? #
pyGHost++ is simply a port of GHost++ with the added ability to add any functionality you want, dynamically, using python. It achieves this with a plugin-based system. These plugins can be loaded into memory, and can do anything from

  * adding extra commands like "!slap .. <slaps x with a fish>"
  * managing all other plugins (see **pluginManager**)
  * downloading maps (see **getmap**)
  * or changing how fundamental parts of ghost operate...

This all from the comfort of python, a very versatile and powerful open-source programming language.


# So how does it work? #

When you start pyGhost, it loads the plugins by executing all the code in `'plugins/__init__.py'`. Now if you enabled the plugins using pluginManager, this means all the plugins there will be loaded at startup, hooked into GHost, and ready to rumble! The plugins themselves work by listening to events, and reacting accordingly, stuff like players joining, chat, all triggers an event that the plugins can listen to.

This means making your own plugin is easy! All you have to do is write it, drop it in your plugins/directory, make sure it contains an `'__init__.py'`, and then use `!enableplugin name` and you're good to go.



# How do I make my plugins 'official' #

You think your plugin is pretty cool, and everyone should be able to get it with a simple !getplugin command, but you don't know how you can make this happen. Well, basically your plugin needs to be _approved_ by a plugin _mentor_ . They will require that the plugin meets the following criteria


### Criteria ###
  1. Your plugin **MUST** be well documented. The _mentor_ needs to be able to understand what the code is doing, so that he can be sure there is no malicious or buggy code. And documentation promotes the continuation of the plugin! The easiest way to appease the mentors is by following the standards set forth in previously accepted plugins, especially pluginManager :)
  1. You should be the developer of the plugin. Submitting other people's plugins without their permission will definitely not be allowed, and it is optimal if they submit it themselves, so we have someone to go-to if things get hairy
  1. Your plugin has to work!
  1. Your plugin has to work very reliably, within reasonable error bounds, and should not erroneously cause damage to the user's ghost installation or computer
  1. Your plugin has to have some prospect of popularity/utility. Theres nothing wrong with creating plugins that are designed for yourself, or a special community, but if they have no use for anyone else, theres not much reason for them to be added here
  1. You have to release your code under the Apache 2.0 license, or compatible. **No Exceptions**