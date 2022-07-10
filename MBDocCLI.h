#include "MBCLI/MBCLI.h"
#include "MBDoc.h" 
#include <memory>
namespace MBDoc
{
    class DocCLI
    {
    private:
        MBCLI::MBTerminal m_AssociatedTerminal;
      
        bool p_VerifyArguments(MBCLI::ArgumentListCLIInput const& ArgumentsToVerfiy);
        CommonCompilationOptions p_GetOptions(std::vector<MBCLI::ArgumentListCLIInput> const& Inputs);
        std::vector<MBCLI::ArgumentListCLIInput> p_GetInputs(const char** argv,int argc);
        DocumentBuild p_GetBuild(MBCLI::ArgumentListCLIInput const& CommandInput);
        DocumentFilesystem p_GetFilesystem(DocumentBuild const& Build,MBCLI::ArgumentListCLIInput const& CommandInput);
        std::unique_ptr<DocumentCompiler> p_GetCompiler(MBCLI::ArgumentListCLIInput const& CommandInput);  

        void p_Help(MBCLI::ArgumentListCLIInput const& Input);
    public:
        int Run(const char** argv,int argc);
    };
}
