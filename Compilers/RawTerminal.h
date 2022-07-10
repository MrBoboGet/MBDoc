#include "../MBDoc.h"
#include "MBUtility/MBInterfaces.h"
#include "MBCLI/MBCLI.h"

namespace MBDoc
{
    
    struct i_FormatTextInfo
    {
        int Width = 0;   
        std::string Text;
        i_FormatTextInfo(int NewWidth,std::string NewText)
        {
            Width = NewWidth;
            Text = NewText;   
        }
        i_FormatTextInfo() {};
        int size() const
        {
            return(Width);   
        }

    };
    
    class RawTerminalCompiler : public DocumentCompiler
    {
    private:
        MBCLI::ArgumentListCLIInput m_CLIInput;
        DocumentFilesystem const* m_FilesystemToCompile = nullptr;
        DocumentPath m_CurrentPath;
        bool m_CPPCompilation = false;
        std::unordered_map<std::string, std::filesystem::path> m_CppIncludePaths;
        void p_CompileBlock(BlockElement const& BlockToCompile,MBUtility::MBOctetOutputStream& OutStream,CommonCompilationOptions const& Options);
        void p_CompileFormat(FormatElement const& FormatToCompile,MBUtility::MBOctetOutputStream& OutStream,CommonCompilationOptions const& Options,std::string const& Prefix,int Depth
        , std::filesystem::path const& OutPath);
        void p_CompileText(TextElement const& TextToCompile,std::vector<i_FormatTextInfo>& OutText,CommonCompilationOptions const& Options);
        void p_CompileDirective(Directive const& TextToCompile,MBUtility::MBOctetOutputStream& OutStream,CommonCompilationOptions const& Options);
    public:
        virtual void Compile(DocumentFilesystem const& FilesystemToCompile,CommonCompilationOptions const& Options);
    };
}
