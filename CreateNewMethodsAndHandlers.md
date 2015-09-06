# Introduction #
The basic way how the pyGHost++ python works is described [here](Handlers.md). This doc shows you how to create new Handlers (entry points for python in C++) and how to create you own low-level functions for python.

# Adding new Handlers #
If you want to add new handlers you can simply call the macro `EXECUTE_HANDLER(HandlerName, throw_if_not_all_succeeded, ...)`
Explanation:
HandlerName = the name under which the functions were registered
throw\_if\_not\_all\_succeeded = the macro will throw an error (so use try, catch blocks) if not all functions registered on this handler succeeded.
... = the arguments, the python function are called with

Example:<br />
`EXECUTE_HANDLER("MyNewHandler", false, "hello", "world", "from", "python");`
<br />
A fitting python function for this handler:
```
def exampleFunction(string1, string2, string3, string4):
    print string1, string2, string3, string
```
The output will be: `hello world from python`
This function can be registered via:
```
import host
host.registerHandler("MyNewHandler", exampleFunction) 
host.registerHandler("MyNewHandler", exampleFunction, True)
```

The explanation about the boolean optinal argument for host.registerHandler (False by default):<br />
You have 2 options on how to register a python function. The simplest method is just to call host.registerHandler without the optinal third argument. All the default handlers like `PlayerJoined` etc will then call your python function **AFTER** the c++ function itself has been executed.<br />
The sense if the third argument is to be able to abort this execution. This can be e.g. useful if you want to completely replace the original c++ function. Or if you just want your python function to be executed before the c++ stuff (but then make sure you dont return True or raise an error because this will also raise/throw an error in the c++ funciton and thus abort it; returning Nothing (= return None = if you dont even use `return` in your function) or False will not result in a throwing c++ function)

# Adding new Methods #
Adding new functions became very easy with the API using boost::python.
At first you have do decide weather you want to create a new module or to extend a existing module (e.g. the host module). <br />
If you want to extend a module you can just search for `BOOST_PYTHON_MODULE(<modulename>)` and just add a `def(<method_name_in_python>, <pointer_to_method>)`. The existing defs should last.<br />
Creating a new module is as easy as adding functions but require a few more lines of code:
```
BOOST_PYTHON_MODULE(<module_name>)
{
boost::python::def(...);
```
Be careful: `<module_name>` is NOT a string because this macro defines a init function which you will have to add near `Py_Initialize`. You will have to call this function before Py\_Initialize:
```
PyImport_AppendInittab(<module_name_this_time_as_string>, init<module_name>)
```
So a complete code:
```
bool MyNewFunction( int IntArg, string StringArg1, char* StringArg2 )
{
    if( IntArg == 123 )
        cout << "[MyNewFunction] says: IntArg is exactly 123." << endl;
    else
    {
        cout << "[MyNewFunction] says: IntArg is *not* 123!" << endl;
        return false;
    }

    // reached if IntArg is 123
    cout << "The Strings are:" << endl;
    cout << "[StringArg1] : " << StringArg1 << endl;
    cout << "[StringArg2] : " << StringArg2 << endl;
    return true;
}

BOOST_PYTHON_MODULE(MyNewModule)
{
    using namespace boost::python;
    def("myNewFunction", &MyNewFunction);
}

int main( int argc, char **argv )
{
    ...
    PyImport_AppendInittab("MyNewModule", initMyNewModule)
    ...
    Py_Initialize( );
...
```


An example output:<br>
<pre><code>&gt;&gt;&gt; import MyNewModule<br>
&gt;&gt;&gt; x = MyNewModule.myNew_Function(123, "hello", "world!")<br>
<br>
[MyNew_Function] says: IntArg is exactly 123.<br>
The Strings are:<br>
[StringArg1] : hello<br>
[StringArg2] : world!<br>
&gt;&gt;&gt; print x<br>
<br>
True<br>
&gt;&gt;&gt; x = MyNewModule.myNew_Function(456, "hallo", "world!")<br>
<br>
[MyNew_Function] says: IntArg is *not* 123!<br>
<br>
&gt;&gt;&gt; print x<br>
<br>
False<br>
</code></pre>
The above output is NOT from the IDLE, i only wrote it that way because its more clear what is output and what is a function call.