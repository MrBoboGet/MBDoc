#include <MBCLI/MBCLI.h>
#include "MBDoc.h" 
#include <memory>
namespace MBDoc
{
    class DocCLI
    {
    private:
        MBCLI::MBTerminal m_AssociatedTerminal;
        LSPInfo m_LSPConf;
        ProcessedColorConfiguration m_ColorConfiguration;
        
        LSPInfo p_ParseLSPConfig(std::filesystem::path const& PathToParse);
        ProcessedColorConfiguration p_ParseColorConfiguration(std::filesystem::path const& PathToParse);
        
      
        bool p_VerifyArguments(MBCLI::ArgumentListCLIInput const& ArgumentsToVerfiy);
        CommonCompilationOptions p_GetOptions(std::vector<MBCLI::ArgumentListCLIInput> const& Inputs);
        std::vector<MBCLI::ArgumentListCLIInput> p_GetInputs(const char** argv,int argc);
        DocumentBuild p_GetBuild(MBCLI::ArgumentListCLIInput const& CommandInput);
        DocumentFilesystem p_GetFilesystem(DocumentBuild const& Build,std::filesystem::path const& OldBuildPath ,MBCLI::ArgumentListCLIInput const& CommandInput,std::vector<IndexType>& UpdatedFiles);
        std::unique_ptr<DocumentCompiler> p_GetCompiler(MBCLI::ArgumentListCLIInput const& CommandInput);  

        void p_Help(MBCLI::ArgumentListCLIInput const& Input);
    public:
        int Run(const char** argv,int argc);
    };
}
