# MBDoc

MBDoc is a command line utility/markup language for creating writing documents, with a focus on supporting programming documentation. The syntax is very similar to markdown, and with features intended to be compatible with command line utilities and pure text documents. 

- [MBDoc](#mbdoc)
    - [Features](#features)
        - [References](#references)
            - [Examples](#examples)
        - [Multiple formats](#multiple-formats)
        - [Builtin LSP support](#builtin-lsp-support)
        - [Easy build setup](#easy-build-setup)
            - [Example](#example)
        - [Simple syntax](#simple-syntax)
        - [Source level extensability](#source-level-extensability)
- [Documentation](#documentation)
- [Compilation](#compilation)
- [License](#license)
## Features

### References

One of the biggest part of MBDoc is the ability to create references that is external to the current document build, allowing for more modular documentation, and for greater composability. 

References to other documents can either be created with an absolute path in the "document filesystem", or searched for at compilation time with a limited set of query operations. This allows for separate build to reference each other, without an as strict requirement on the resulting output file structure, and without abiding by the semantics of a specific output format like HTML. 

#### Examples

```
#creates reference to a document with an absolute path
@[doc](/MBUtility/Code/Classes/LineRetriever.mbd)
#This reference searches the complete document filesystem for any file or
#directory named LineRetriever.mbd that is located under the directory
#MBUtility.
@[doc](/MBUtility/{LineRetriever.mbd})
#Creates a reference to a document named LineRetriever.mbd, that is the
#subdirectory for directory named class that is located somewhere under
#MBUtility.
@[doc](/MBUtility/{Classes/LineRetriever.mbd})
#Creates a reference named LineRetriever.mbd that is located somewhere under a
#directory named Classes that is somwhere under MBUtility
@[doc](/MBUtility/{Classes}/{LineRetriever.mbd})
#Creates a reference to a file that contains a part named GetLine somewhere
#under MBUtility
@[doc](/MBUtility#GetLine).
```


Arbitrary color selection, image and video includes, tables. 

### Multiple formats

The program includes support for github markdown, HTML and vim help files. They are all specified with the `-s`flag. Compiling this document can for example be done with `mbdoc README.mbd -f:md -o:./` 

### Builtin LSP support

Colorizing code blocks often requires the use of heuristics and approximations, as few colorizers take or can take into account the semantics of your build process. MBDoc make's LSP's a first class member, allowing for user customization with for example of codeblocks are colored, and also automatically creating references to the implementation. An example of this can be seen with the source files for MBDoc that are fully colorized and with references to the functions definitions can be seen here [here](https://mrboboget.github.io/MBDoc/Code/Sources/MBDocCLI.cpp). 

Custom colorization for codeblocks can also be added with regexes, supporting pretty colors for niche and ad-hoc file types. 

### Easy build setup

Build files are configure as pure JSON documents, and can specify both exact control over how the document filesystem is created, and also specify directory on the local filesystem that are mounted as is. 

#### Example

This creates a document filesystem with that has the files `/foo.mbd`, `/bar.mbd`, as well as the directory `/DirName` containing `/DirName/stuff.mbd`, aswell as the the directory `/DirName2/` that directly maps the local filesystem files to the document filesystem. 

```json
{
    "TopDirectory":
    {
        "Files":["foo.mbd","bar.mbd"],
        "SubDirectories":
        {
            "DirName":
            {
                "Files":["unrelated/dir/stuff.mbd"]
            },
            "DirName2":
            {
                "MountDir":"path/to/relative/dir/to/mount"
            }
        }
    }
}
```


### Simple syntax

Taking inspiration from markdown so is the syntax simple, needing limited tool support to verify whether or not a document will compile, but with more convenient special syntax, such as for tables. Tables can be  written as 

```
\table 4 
1 & 2 & 3 & 4
out of alignment content & text & @[ref](#foobar) & ~FF00BB colored text
\end
```


creating a table with 4 columns and 2 rows, compared to the much more tedious syntax for markdown. 

### Source level extensability

The source is decoupled between the operations needed for every build, and what is done by a compiler, making it easy to add compilers for new output formats. Incremental builds are also supported generically, meaning the compiler only has to implement the specific task of writing the contents of the output files. 

# Documentation

Documentation can be found at [mrboboget.github.io](https://mrboboget.github.io/MBDoc/index.html), which is also written in MBDoc. The sources for the different projects are all written separately, and a build containing all of projects is created with [MBPacketManager](https://github.io/MrBoboGet/MBPacketManager), and after that so is the whole build compiled with MBDoc. 

# Compilation

MBDoc is compiled with [MBPacketManager](https://github.com/MrBoboGet/MBPacketManager), and further instructions can be found at that github page. In essence however, all that is needed is to have a include path where the root of [MBPacketManager](https://github.com/MrBoboGet/MBPacketManager), [MBPacketManager](https://github.com/MrBoboGet/MBPacketManager)can be found. Add this directory as an include path, and link to their respective static library. 

# License

The source code is like all my projects, in the public domain. 

