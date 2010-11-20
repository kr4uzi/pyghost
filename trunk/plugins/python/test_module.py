import host

def init( ):
    host.registerHandler( "StartUp", TestFunc1 )
    host.registerHandler( "StartUp", TestFunc2, True )

def TestFunc1( CFG ):
    print( "Hello World!" )

def TestFunc2( CFG ):
    raise RuntimeError("Error from TestFunc2")
