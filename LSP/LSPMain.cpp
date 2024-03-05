#include "MBDocLSP.h"

int main(int argc,const char** argv)
{
    MBLSP::RunSTDIOServer(std::make_unique<MBDoc::MBDocLSP>());
}
