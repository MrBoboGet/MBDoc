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
        std::string GetReferenceString(DocReference const& ReferenceIdentifier) const;
        void CreateTOC(MBUtility::MBOctetOutputStream& OutStream) const;
        
        void Initialize(DocumentSource const& Documents);
    };
     
    class MarkdownCompiler
    {
    private:
        

        void p_CompileText(std::vector<std::unique_ptr<TextElement>> const& ElementsToCompile,MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileBlock(BlockElement const* BlockToCompile, MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileDirective(Directive const& DirectiveToCompile, MarkdownReferenceSolver const& ReferenceSolver, MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileFormat(FormatElement const& SourceToCompile, MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream,int Depth);
        void p_CompileSource(DocumentSource const& SourceToCompile, MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream);

        MarkdownReferenceSolver p_CreateReferenceSolver(DocumentSource const& Source);
    public:
        virtual void Compile(std::vector<DocumentSource> const& Sources);
        //void Compile(DocumentFilesystem const& BuildToCompile,CommonCompilationOptions const& Options) override; 
    };
};
