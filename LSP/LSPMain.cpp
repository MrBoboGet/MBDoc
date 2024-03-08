#include "MBDocLSP.h"

int main(int argc,const char** argv)
{
    //std::this_thread::sleep_for(std::chrono::seconds(10));
    MBLSP::RunSTDIOServer(std::make_unique<MBDoc::MBDocLSP>());
}
