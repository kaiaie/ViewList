# ViewList

ViewList is a Windows program that takes lines fed to it on standard output 
and displays them in a list box. It is similar to the Out-GridView cmdlet 
in PowerShell but line-oriented rather than cell-oriented.


## Examples

List files of a particular type, recursively:

```
X:\> DIR /S/B *.txt | ViewList
```

Find all files containing a specific string and list them:

```
X:\> FINDSTR /I /S /M /C: "Search text" %CD%\*.txt | ViewList
```


## Compiling ViewList

You will need [MinGW](http://www.mingw.org) to build using the supplied Makefile 
but the code should be compilable with any Windows C compiler.


## To do/ unimplemented features

* Add "open in text editor" and "open in binary editor" options
* Add options dialog so that settings for text/ binary editor and diff 
  program can be set interactively (diff program can be set via the 
  Registry; see DiffProgram.reg file for example)



