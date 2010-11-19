# This package has been included as a pyghost plugin, because it is very useful for
# reading/editing ghost.cfg as well as mapcfgs (and any other cfgs as a matter of fact).
# Its major advantage over configparser is that it preserves comments and whitespace
# when saving, as well as support for ini's without section headers. All credit for the 
# original implementation goes to the developers (see docs).It also serves as an experiment 
# with dependencies. (Note this plugin will do nothing by itself, its more of a library)

# You can use it with
# from configobj import configobj

__author__ = 'Michael Foord & Nicola Larosa'
__mentor__= 'ilaggoodly'
__mentor_email__ = 'awknaust@gmail.com'
__description__ = 'Config file reading, writing and validation.'
__requires__ = {'python' : "2.6", 'ghost' : "17.0"}
__tags__ = ['config', 'library']
__category__= 'developer'
__islibrary__ = 1
__version__ = "4.7.2"

#the original long description
__long_description__ = """**ConfigObj** is a simple but powerful config file reader and writer: an *ini
file round tripper*. Its main feature is that it is very easy to use, with a
straightforward programmer's interface and a simple syntax for config files.
It has lots of other features though :

* Nested sections (subsections), to any level
* List values
* Multiple line values
* Full Unicode support
* String interpolation (substitution)
* Integrated with a powerful validation system

    - including automatic type checking/conversion
    - and allowing default values
    - repeated sections

* All comments in the file are preserved
* The order of keys/sections is preserved
* Powerful ``unrepr`` mode for storing/retrieving Python data-types

| Release 4.7.2 fixes several bugs in 4.7.1
| Release 4.7.1 fixes a bug with the deprecated options keyword in
| 4.7.0.
| Release 4.7.0 improves performance adds features for validation and
| fixes some bugs."""
