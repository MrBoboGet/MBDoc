#include <iostream>
#include "MBDocCLI.h"

int main(int argc,const char** argv)
{
    MBDoc::DocCLI CLI;
    CLI.Run(argv, argc);
}
