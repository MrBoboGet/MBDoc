#include <iostream>
#include "MBDocCLI.h"

int main(int argc,const char** argv)
{
    //const char* NewArgv[] = { "mbdoc","../../../MBMelee/MBDocBuild.json","-f:html","-o:./" };
    //argc = sizeof(NewArgv) / sizeof(const char*);
    //argv = NewArgv;
    MBDoc::DocCLI CLI;
    CLI.Run(argv, argc);
}
