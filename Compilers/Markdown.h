#include "../MBDoc.h"

namespace MBDoc
{
    class MarkdownReferenceSolver 
    {
    private:
        struct Heading 
        {
            std::string Name;
            std::vector<Heading> SubHeaders;
        };
        std::vector<Heading> m_Headings;
        std::unordered_set<std::string> m_HeadingNames;

        void p_WriteToc(Heading const& CurrentHeading,MBUtility::MBOctetOutputStream& OutStream, size_t Depth) const;
        Heading p_CreateHeading(FormatElement const& Format);
    public:
        void CreateTOC(MBUtility::MBOctetOutputStream& OutStream) const;
        
        void Initialize(DocumentSource const& Documents);
    };
     
    class MarkdownCompiler : public DocumentVisitor, public DocumentCompiler
    {
    private:
        
        MarkdownReferenceSolver m_TocCreator;
        DocumentSource const* m_CurrentDocument = nullptr;
        std::filesystem::path m_OutDir;
        MBUtility::MBOctetOutputStream* m_OutStream;

        int m_FormatDetph = 0;

    public:

        void EnterText(TextElement_Base const& ElementToEnter) override; 
        void LeaveText(TextElement_Base const& ElementToEnter) override; 

        void EnterFormat(FormatElement const& ElementToEnter) override; 
        void LeaveFormat(FormatElement const& ElementToEnter) override; 

        void LeaveBlock(BlockElement const& BlockToLeave) override;
        void EnterBlock(BlockElement const& BlockToEnter) override;

        void Visit(CodeBlock const& BlockToVisit) override;
        void Visit(MediaInclude const& BlockToVisit) override;
        void Visit(Paragraph const& BlockToVisit) override;

        void Visit(URLReference const& BlockToVisit) override;
        void Visit(FileReference const& BlockToVisit) override;
        void Visit(UnresolvedReference const& BlockToVisit) override;
        void Visit(DocReference const& BlockToVisit) override;

        void Visit(RegularText const& BlockToVisit) override;

        void Visit(Directive const& BlockToVisit) override;


        void AddOptions(CommonCompilationOptions const& Options) override;
        void PeekDocumentFilesystem(DocumentFilesystem const& FilesystemToCompile) override;
        void CompileDocument(DocumentPath const& Path,DocumentSource const& Document) override;
    };
};
