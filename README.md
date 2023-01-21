# MBDoc
- [MBDoc](#mbdoc)
- [Description](#description)
- [Compilation](#compilation)
- [Documentation](#documentation)
- [Examples](#examples)
- [License](#license)
# Description
MBDoc is a command line utility/markup language for creating
documents/documentation. The syntax is very similar to markdown, and with
features intended to be compatible with command line utilites and pure text
documents. 
# Compilation
MBDoc follows the MBBuild style of compilation, which means that compilation
can be done manually by compiling all source files with only an extra include
directory required, the directory containing the dependancies. A CMakeLists.txt
is also provided, and provided that you have The required dependancies are
[MBParsing](https://github.com/MrBoboGet/MBParsing) and
[MBUtility](https://github.com/MrBoboGet/MBUtility) 

# Documentation
Documentation can be found at [mrboboget.github.io](https://mrboboget.github.io/MBDoc/index.html) 
# Examples
The file used to create this readme kan be found [here](/README.mbd).
Compiling any document with MBDoc requires a target format and a target
style/semantics, in this case the format is Markdown, and the semantics is a
Github readme. Semantics take care of for example deciding how references
identifiers are interpreted. 
For complete guide on invocation, go [here](https://mrboboget.github.io/MBDoc/CLI.html) 
# License
The source code, as with any other MB dependancies, is in the public domain.
