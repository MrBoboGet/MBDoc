#include "LSP.h"
#include <MBLSP/MBLSP.h>
namespace MBDoc
{
    std::unique_ptr<MBLSP::LSP_Client> StartLSPServer(LSPServer const& ServerToStart,MBLSP::InitializeRequest const& InitReq);
    std::unique_ptr<MBLSP::LSP_Client> StartLSPServer(LSPServer const& ServerToStart);
    LSPInfo LoadLSPConfig(std::filesystem::path const& PathToParse);
    LSPInfo LoadLSPConfig();
    LSPInfo LoadLSPConfigIfExists();
}
