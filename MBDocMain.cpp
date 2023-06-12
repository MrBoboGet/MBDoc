#include <iostream>
#include "MBDocCLI.h"

int main(int argc,const char** argv)
{
    //const char* NewArgv[] = {"mbdoc", "","-o:TestOut","-f:html" };
    //argc = sizeof(NewArgv) / sizeof(const char*);
    //argv = NewArgv;
    MBDoc::DocCLI CLI;
    CLI.Run(argv, argc);
}
