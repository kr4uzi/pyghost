# httplib2 library for Pyhthon 2.3 + converted to plugin library
# by ilaggoodly (AWKnaust), all credit go to the original developer(s)
# see README/LICENSE for more information, or visit the development
# page at http://code.google.com/p/httplib2/

__author__='Joe Gregorio'
__mentor__='ilaggoodly'
__version__='0.6.0'
__author_email__='joe@bitworking.org'
__requires__ = {'python' : '2.3'}
__url__='http://code.google.com/p/httplib2/'
__description__ = 'A comprehensive HTTP client library.'
__islibrary__ = 1
__license__ = 'MIT'

__long_description__="""
A comprehensive HTTP client library, ``httplib2`` supports many features left out of other HTTP libraries.

**HTTP and HTTPS**
  HTTPS support is only available if the socket module was compiled with SSL support. 
 
**Keep-Alive**
  Supports HTTP 1.1 Keep-Alive, keeping the socket open and performing multiple requests over the same connection if possible. 

**Authentication**
  The following three types of HTTP Authentication are supported. These can be used over both HTTP and HTTPS.

  * Digest
  * Basic
  * WSSE
  
**Caching**
  The module can optionally operate with a private cache that understands the Cache-Control: 
  header and uses both the ETag and Last-Modified cache validators. Both file system
  and memcached based caches are supported.

**All Methods**
  The module can handle any HTTP request method, not just GET and POST.

**Redirects**
  Automatically follows 3XX redirects on GETs.

**Compression**
  Handles both 'deflate' and 'gzip' types of compression.

**Lost update support**
  Automatically adds back ETags into PUT requests to resources we have already cached. This implements Section 3.2 of Detecting the Lost Update Problem Using Unreserved Checkout

**Unit Tested**
  A large and growing set of unit tests.
"""

