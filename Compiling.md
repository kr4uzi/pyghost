## Windows ##
### Preparations ###
## !! THIS WIKI HASN'T BEEN UPDATED FOR pyGHost++ v. 0.4 YET !! ##
Download the latest Boost Libraries, Python (> v.2.3) and Microsoft Visual Studio 2008:
<br />
  * [Boost Libraries](http://www.boost.org/users/download/)
  * [Python 2.x.x](http://www.python.org/download/)
  * [Microsoft Visual C++ 2008 Express Edition](http://www.microsoft.com/express/Downloads/)

### Compiling the base packages ###
  1. Unpack the boost libs anywhere you like.
  1. Install Python
  1. Install MS VC++ 2008
  1. Compile the Boost libraries:
  * Start->Run->cmd.exe (or **Windows Key**+**R**->cmd.exe)
  * PUSHD "path\to\Boost\"
  * bootstrap [[screenshot](http://img97.imageshack.us/img97/270/compilingwindows1.jpg)]
  * bjam --with-filesystem --with-thread --with-date\_time
  * bjam --with-python link=shared
[[screenshot start](http://img528.imageshack.us/img528/1631/800pxcompilingwindows2.jpg)] | [[screenshot end](http://img693.imageshack.us/img693/5682/800pxcompilingwindows3.jpg)]
### Configuring Visual Studio ###
  1. Open ghost.sln in .\pyGHost++\
  1. Tools -> Options -> Projects and Solutions -> VC++ Directories
You can add new entries by clicking on the folder button. Enter this two paths:
  1. Show directories for: Include files
    * C:\path\to\python\include
    * C:\path\to\boost\
  1. Show directories for: Library files
    * C:\path\to\python\libs
    * C:\path\to\boost\stage\lib
### Compiling pyGHost++ ###
  1. Click on Release MySQL in the top middle (if you dont know what MySQL is, select only Release)
  1. Right click "ghost" on the left side and select "Compile"
  1. Navigate to .\pyGHost++\Release MySQL (or .\pyGHost++\Release if you selected this compile option in Step 1)
  1. Copy the file ghost.exe there to .\pyGHost++ and overwrite the existing one
  1. All done :)
<br />
<br />
## Linux ##
Installing pyGhost on linux requires all the same libraries as GHost++, in addition to
Python 2.6 (or greater, but not 3). This guide was written using, http://www.codelain.com/forum/index.php?topic=7941.0 as a reference, and is intended for debian-style pcs, although compiling on non-debian distributions follows the same steps (but with different package names). Important requirements are boostlibs and python2.6, war3patch.mpq, war3.exe, etc.
Also the README.txt includes a short description of how to compile under linux
### Required Packages ###
```
sudo aptitude install build-essential g++, libmysql++-dev libbz2-dev libgmp3-dev
```
### Installing Boost ###
If your distribution has versions of boost greater than 1.39, you can install them using
```
sudo aptitude install libboost-date-time-dev libboost-filesystem-dev libboost-thread-devv libboost-system-dev
```
Otherwise you'll have to install boost yourself (only filesystem,date-time...)
You can do this by using "wget" with the boost.org site, for example
```
wget http://sourceforge.net/projects/boost/files/boost/1.42.0/boost_1_42_0.tar.bz2/download
```
now, extract the downloaded files
```
tar xvjf boost_1_42_0.tar.bz2
cd boost*
sudo ./bootstrap.sh --with-libraries=filesystem,system,thread,date_time
sudo ./bjam
sudo ./bjam install
```

### Installing pyGHost++ ###
You can get the latest version using either "wget" and a link from the Downloads page, or checkout via svn (see source page). After having unpacked the files, you need to compile bncsutil and stormlib

```
cd ghost/bncsutil/src/bncsutil
sudo make
cd ghost/StormLib/stormlib
sudo make
sudo make install
```
Now, compile pyghost itself
```
cd ghost/ghost/
sudo make
cp ghost++ ../
```
You should now have a compiled ghost, You still need to configure it to run properly, read the ghost.cfg or visit http://www.codelain.com/forum/ for more info on how to do that