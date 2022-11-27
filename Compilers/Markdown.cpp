#include "Markdown.h"
namespace MBDoc
{
       
    //BEGIN MarkdownCompiler
    void MarkdownCompiler::p_CompileText(std::vector<std::unique_ptr<TextElement>> const& ElementsToCompile, MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream)
    {
        for(std::unique_ptr<TextElement> const& Text : ElementsToCompile)
        {
            std::string ModifierString = "";
            if((Text->Modifiers & TextModifier::Italic) != TextModifier::Null)
            {
                ModifierString += '*';
            }
            if((Text->Modifiers & TextModifier::Bold) != TextModifier::Null)
            {
                ModifierString += "**";
            }
            OutStream.Write(ModifierString.data(),ModifierString.size());
            if(Text->Type == TextElementType::Regular)
            {
                
                OutStream.Write(static_cast<RegularText const&>(*Text.get()).Text.data(),static_cast<RegularText const&>(*Text.get()).Text.size());
            }
            if(Text->Type == TextElementType::Reference)
            {
                DocReference const& CurrentElement = static_cast<DocReference const&>(*Text.get());
                std::string TextToWrite = ReferenceSolver.GetReferenceString(CurrentElement);
                OutStream.Write(TextToWrite.data(), TextToWrite.size());
            }
            OutStream.Write(ModifierString.data(),ModifierString.size());
        }   
    }
    void MarkdownCompiler::p_CompileBlock(BlockElement const* BlockToCompile, MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream)
    {
        if(BlockToCompile->Type != BlockElementType::Paragraph)
        {
            throw std::runtime_error("Unsupported block type");  
        } 
        Paragraph const& ParagraphToCompile = static_cast<Paragraph const&>(*BlockToCompile);
        p_CompileText(ParagraphToCompile.TextElements, ReferenceSolver,OutStream);
        OutStream.Write("\n\n",2);
    }
    void MarkdownCompiler::p_CompileDirective(Directive const& DirectiveToCompile, MarkdownReferenceSolver const& ReferenceSolver, MBUtility::MBOctetOutputStream& OutStream)
    {
        if (DirectiveToCompile.DirectiveName == "toc")
        {
            ReferenceSolver.CreateTOC(OutStream);
        }
    }
    enum class i_CompileType
    {
        Block,
        Format,
        Directive
    };
    void MarkdownCompiler::p_CompileFormat(FormatElement const& FormatToCompile, MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream,int Depth)
    {
        if(FormatToCompile.Type != FormatElementType::Default)
        {
            std::string StringToWrite = "#";
            for(int i = 0; i < Depth;i++)
            {
                StringToWrite += "#";  
            } 
            StringToWrite += " "+FormatToCompile.Name+"\n\n";
            OutStream.Write(StringToWrite.data(),StringToWrite.size());
        }
        for(FormatElementComponent const& Component : FormatToCompile.Contents)
        {
            if(Component.GetType() == FormatComponentType::Directive)
            {
                p_CompileDirective(Component.GetDirectiveData(),ReferenceSolver,OutStream); 
            }  
            else if(Component.GetType() == FormatComponentType::Block)
            {
                p_CompileBlock(&Component.GetBlockData(),ReferenceSolver,OutStream);
            }
            else if(Component.GetType() == FormatComponentType::Format)
            {
                p_CompileFormat(Component.GetFormatData(),ReferenceSolver,OutStream,Depth+1);
            }
        }
    }
    void MarkdownCompiler::p_CompileSource(DocumentSource const& SourceToCompile, MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream)
    {
        //Joint iterator, a bit ugly         
        for(FormatElement const& Format : SourceToCompile.Contents)
        {
            p_CompileFormat(Format,ReferenceSolver,OutStream,0);        
        }
        OutStream.Flush();
    }
    class Printer : public MBUtility::MBOctetOutputStream
    {
    public:
        size_t Write(const void* DataToWrite,size_t DataSize)
        {
            std::cout.write((const char*)DataToWrite,DataSize); 
            return(0);
        } 
    };
    MarkdownReferenceSolver MarkdownCompiler::p_CreateReferenceSolver(DocumentSource const& Source)
    {
        MarkdownReferenceSolver ReturnValue;
        ReturnValue.Initialize(Source);
        return(ReturnValue);
    }
    //void MarkdownCompiler::Compile(DocumentFilesystem const& BuildToCompile, CommonCompilationOptions const& Options)
    //{
    //    std::cout << "Not implemented B)" << std::endl;
    //}
    void MarkdownCompiler::Compile(std::vector<DocumentSource> const& Sources)
    {
        Printer OutStream;
        for(DocumentSource const& Source : Sources)
        {
            MarkdownReferenceSolver Solver = p_CreateReferenceSolver(Source);
            //MBUtility::MBFileOutputStream OutStream = MBUtility::MBFileOutputStream("README.md");
            p_CompileSource(Source,Solver,OutStream); 
        }    
    }
    //
    //END MarkdownCompiler
    //BEGIN MarkdownReferenceSolver
    std::string h_CreateHeadingReference(std::string const& HeadingName)
    {
        std::string ReturnValue = MBUtility::ReplaceAll(HeadingName, " ", "-");
        ReturnValue = "#"+MBUnicode::UnicodeStringToLower(ReturnValue);
        return(ReturnValue);
    }
    std::string MarkdownReferenceSolver::GetReferenceString(DocReference const& ReferenceIdentifier) const
    {
        //std::string ReturnValue = ReferenceIdentifier;
        std::string ReturnValue;
        std::string IdentifierURL = ReferenceIdentifier.Identifier;
        std::string VisibleText = ReferenceIdentifier.VisibleText;
        if (m_HeadingNames.find(ReferenceIdentifier.Identifier) != m_HeadingNames.end())
        {
            IdentifierURL = h_CreateHeadingReference(ReferenceIdentifier.Identifier);
            if (VisibleText == "")
            {
                VisibleText = ReferenceIdentifier.Identifier;
            }
        }
        else
        {
            size_t NamespaceDelimiterPosition = ReferenceIdentifier.Identifier.find("::");
            if (NamespaceDelimiterPosition != ReferenceIdentifier.Identifier.npos && NamespaceDelimiterPosition + 2 < ReferenceIdentifier.Identifier.size())
            {
                //assumes github markdown
                std::string User = ReferenceIdentifier.Identifier.substr(0, NamespaceDelimiterPosition);
                std::string Repository = ReferenceIdentifier.Identifier.substr(NamespaceDelimiterPosition + 2);
                IdentifierURL = "https://github.com/" + User + "/" + Repository;
                if(VisibleText == "")
                {
                    VisibleText = Repository;
                }
            }
        }
        if (VisibleText != "")
        {
            ReturnValue = "[" + VisibleText + "](" + IdentifierURL + ")";
        }
        else
        {
            ReturnValue = "<" + IdentifierURL+ ">";
        }
        return(ReturnValue);
    }
    void MarkdownReferenceSolver::p_WriteToc(Heading const& CurrentHeading,MBUtility::MBOctetOutputStream& OutStream , size_t Depth) const
    {
        std::string Indent = std::string(Depth * 4, ' ');
        OutStream.Write(Indent.data(), Indent.size());
        std::string HeaderName = "- [" + CurrentHeading.Name + "]" + "(" + h_CreateHeadingReference(CurrentHeading.Name)+")\n";
        OutStream.Write(HeaderName.data(), HeaderName.size());
        for (Heading const& SubHeading : CurrentHeading.SubHeaders)
        {
            p_WriteToc(SubHeading, OutStream, Depth + 1);
        }
    }
    void MarkdownReferenceSolver::CreateTOC(MBUtility::MBOctetOutputStream& OutStream) const
    {
        for (Heading const& CurrentHeading : m_Headings)
        {
            p_WriteToc(CurrentHeading,OutStream, 0);
        }
        OutStream.Flush();
    }
    MarkdownReferenceSolver::Heading MarkdownReferenceSolver::p_CreateHeading(FormatElement const& Format)
    {
        Heading ReturnValue;
        ReturnValue.Name = Format.Name;
        m_HeadingNames.insert(ReturnValue.Name);
        for (auto const& Element : Format.Contents)
        {
            if(Element.GetType() != FormatComponentType::Format)
            {
                continue;   
            }
            if (Element.GetFormatData().Type != FormatElementType::Default)
            {
                ReturnValue.SubHeaders.push_back(p_CreateHeading(Element.GetFormatData()));
            }
        }
        return(ReturnValue);
    }
    void MarkdownReferenceSolver::Initialize(DocumentSource const& Documents)
    {
        for (FormatElement const& Element : Documents.Contents)
        {
            if (Element.Type != FormatElementType::Default)
            {
                m_Headings.push_back(p_CreateHeading(Element));
            }
        }
    }
    //END MarkdownReferenceSolver
};
