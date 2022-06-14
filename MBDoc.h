#pragma once
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <MBUtility/MBInterfaces.h>
#include <MBUtility/MBErrorHandling.h>
#include <unordered_set>
namespace MBDoc
{

    // Syntax completely line based
    
    
    //@Identifier <- Implicit visible text ,@[VisibleText](LinkIdentifier)

    //Format elements can contain other format elements, and contain block elements, which can contatin text elements, but no intermixin
    
    
        
    //Implicit whitespace normalization, extra spaces and newlines aren't preserved, which means that a sequence of text elements are not ambigous   

    
    enum class ReferenceType
    {
        Null,
        URL,
        MBDocIdentifier   
    };
    struct MBDocIdentifier
    {
        std::string LinkText;//Temp     
    };
    
    //Text elements
    enum class TextModifier
    {
        Null = 0,
        Bold=1,
        Italic=1<<1,
        Underlined=1<<2,
        Highlighted=1<<3,
    };
    inline TextModifier operator&(TextModifier Left,TextModifier Right)
    {
        return(TextModifier(uint64_t(Left)&uint64_t(Right)));
    }
    inline TextModifier operator|(TextModifier Left,TextModifier Right)
    {
        return(TextModifier(uint64_t(Left)|uint64_t(Right)));
    }
    inline TextModifier operator^(TextModifier Left,TextModifier Right)
    {
        return(TextModifier(uint64_t(Left)^uint64_t(Right)));
    }
    inline TextModifier operator~(TextModifier Left)
    {
        return(TextModifier(~uint64_t(Left)));
    }
    struct TextColor
    {
        uint8_t R = 0;   
        uint8_t G = 0;   
        uint8_t B = 0;   
    };
    enum class TextElementType
    {
        Null,
        Regular,
        Literal,     
        Reference,
        Custom,
    };
    struct TextElement
    {
        TextElementType Type;
        TextModifier Modifiers;
        TextColor Color;
    };


    struct DocReference : TextElement
    {
        DocReference()
        {
            Type = TextElementType::Reference;
        }
        std::string VisibleText;
        std::string Identifier;
    };
    struct RegularText : TextElement
    {
        RegularText()
        {
            Type = TextElementType::Regular;
        }
        std::string Text;   
    };
    struct StringLiteral : TextElement
    {
        std::string Text;   
    };

    struct CustomTextElement : TextElement
    {
        std::string ElementName;
        std::vector<std::pair<std::string,std::string>> Attributes; 
        std::unique_ptr<TextElement> Text;
    };
    //Text elements

    //Block elements
    enum class BlockElementType
    {
        Null,
        Paragraph,
        Table,
        CodeBlock,
    };
    struct BlockElement
    {
        BlockElementType Type = BlockElementType::Null;
    };

    struct Paragraph : BlockElement
    {
        Paragraph() 
        {
            Type = BlockElementType::Paragraph; 
        };
        std::vector<std::unique_ptr<TextElement>> TextElements;//Can be sentences, text, inline references etc
    };
    struct Note : BlockElement
    {
        std::unique_ptr<TextElement> Text; 
    };
    struct Image : BlockElement
    {
        std::string ImageResource;     
    };

    struct CustomBlockElement : BlockElement
    {
        std::string ElementName;
        std::vector<std::pair<std::string,std::string>> Attributes;
        std::vector<std::unique_ptr<TextElement>> Content; 
    };
    //Block elements
   
    //Directives
    struct Directive
    {
        std::string DirectiveName;
    };
    //

    //Format elements 
    //Starts with #, default new section name
    //optionally ends with /#
    enum class FormatElementType
    {
        Null,
        Section,
        Default,
        Anchor
    };
    struct FormatElement
    {
        FormatElementType Type;
        std::string Name;
        std::vector<std::pair<std::string,std::string>> Parameters;

        std::vector<std::pair<std::unique_ptr<BlockElement>,size_t>> BlockElements; 
        std::vector<std::pair<Directive, size_t>> Directives;
        std::vector<std::pair<FormatElement,size_t>> NestedFormats;
    };
    //Preprocessors, starts with $
    enum class PreProcessorType
    {
        If,
        Dereference,
        Assignment   
    };
    struct PreProcessorBlock
    {
          
    };
    //
    class DocumentSource
    {
    public:
        std::vector<FormatElement> Contents;
        //[[SemanticallyAuthoritative]]
        //
    };

    class LineRetriever
    {
    private:
        MBUtility::MBOctetInputStream* m_InputStream = nullptr;
        std::string m_CurrentLine;
        std::string m_Buffer;
        size_t m_BufferOffset = 0;
        bool m_Finished = false;
        bool m_StreamFinished = false;
        bool m_LineBuffered = false;
    public:
        LineRetriever(MBUtility::MBOctetInputStream* InputStream);
        bool Finished();
        bool GetLine(std::string& OutLine);
        void DiscardLine();
        std::string& PeekLine();

    };

    class DocumentParsingContext
    {
    private:
        DocReference p_ParseReference(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset);
        std::vector<std::unique_ptr<TextElement>> p_ParseTextElements(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset);
        std::unique_ptr<BlockElement> p_ParseBlockElement(LineRetriever& Retriever);
        Directive p_ParseDirective(LineRetriever& Retriever);
        FormatElement p_ParseFormatElement(LineRetriever& Retriever);
        std::vector<FormatElement> p_ParseFormatElements(LineRetriever& Retriever);
    public:
        DocumentSource ParseSource(MBUtility::MBOctetInputStream& InputStream);
        DocumentSource ParseSource(std::filesystem::path const& InputFile);
        DocumentSource ParseSource(const void* Data,size_t DataSize);
    };
   
    MBError SemanticVerfification(DocumentSource const& DocumentToAnalyze);
    class DocumentCompiler
    {
    
    public:
        virtual void Compile(std::vector<DocumentSource> const& Sources) = 0;
    };

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

    class MarkdownCompiler : DocumentCompiler
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
    };
}
