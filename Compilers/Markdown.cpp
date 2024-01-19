#include "Markdown.h"
#include <MBUtility/MBStrings.h>
namespace MBDoc
{
       
    //BEGIN MarkdownCompiler
    
    //void MarkdownCompiler::Compile(DocumentFilesystem const& BuildToCompile, CommonCompilationOptions const& Options)
    //{
    //    std::cout << "Not implemented B)" << std::endl;
    //}
    //
    //END MarkdownCompiler
    //BEGIN MarkdownReferenceSolver
    std::string h_CreateHeadingReference(std::string const& HeadingName)
    {
        std::string ReturnValue = MBUtility::ReplaceAll(HeadingName, " ", "-");
        ReturnValue = "#"+MBUnicode::UnicodeStringToLower(ReturnValue);
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
            ReturnValue.SubHeaders.push_back(p_CreateHeading(Element.GetFormatData()));
        }
        return(ReturnValue);
    }
    void MarkdownReferenceSolver::Initialize(DocumentSource const& Documents)
    {
        for (FormatElementComponent const& Element : Documents.Contents)
        {
            if (Element.GetType() == FormatComponentType::Format)
            {
                m_Headings.push_back(p_CreateHeading(Element.GetFormatData()));
            }
        }
    }
    void MarkdownCompiler::EnterText(TextElement_Base const& ElementToEnter)
    {
           
    }
    void MarkdownCompiler::LeaveText(TextElement_Base const& ElementToEnter)
    {
           
    }

    void MarkdownCompiler::EnterFormat(FormatElement const& ElementToEnter) 
    {
        m_FormatDetph +=1; 
        *m_OutStream<<std::string(m_FormatDetph,'#')<<" "<<ElementToEnter.Name<<"\n\n";
    }
    void MarkdownCompiler::LeaveFormat(FormatElement const& ElementToEnter) 
    {
        m_FormatDetph -=1; 
    }

    void MarkdownCompiler::LeaveBlock(BlockElement const& BlockToLeave)
    {
        *m_OutStream<<"\n\n";
    }
    void MarkdownCompiler::EnterBlock(BlockElement const& BlockToEnter)
    {
           
    }

    void MarkdownCompiler::Visit(CodeBlock const& BlockToVisit)
    {
        *m_OutStream << "```"<<BlockToVisit.CodeType<<"\n"; 
        if(std::holds_alternative<std::string>(BlockToVisit.Content))
        {
            *m_OutStream << std::get<std::string>(BlockToVisit.Content)<<"\n";
        }
        else if(std::holds_alternative<ResolvedCodeText>(BlockToVisit.Content))
        {
            for(auto const& Row : std::get<ResolvedCodeText>(BlockToVisit.Content))
            {
                for(auto const& Element : Row)
                {
                    EnterText(Element.GetBase());
                    Element.Accept(*this);
                    LeaveText(Element.GetBase());
                }
                *m_OutStream<<"\n";
            }
        }
        *m_OutStream << "```\n";
    }
    void MarkdownCompiler::Visit(MediaInclude const& BlockToVisit)
    {
           
    }
    void MarkdownCompiler::Visit(Paragraph const& BlockToVisit)
    {
        //just let the  traverser handle
    }

    void MarkdownCompiler::Visit(URLReference const& BlockToVisit)
    {
        std::string VisibleText = BlockToVisit.VisibleText != "" ? BlockToVisit.VisibleText : BlockToVisit.URL;
        *m_OutStream<<"["<<VisibleText<<"]"<<"("<<BlockToVisit.URL<<")";
    }
    void MarkdownCompiler::Visit(FileReference const& BlockToVisit)
    {
        std::string VisibleText = BlockToVisit.VisibleText != "" ? BlockToVisit.VisibleText : BlockToVisit.Path.GetString();
        *m_OutStream<<"["<<VisibleText<<"]"<<"("<<BlockToVisit.Path.GetString()<<")";
    }
    void MarkdownCompiler::Visit(UnresolvedReference const& BlockToVisit)
    {
        std::string VisibleText = BlockToVisit.VisibleText != "" ? BlockToVisit.VisibleText : BlockToVisit.ReferenceString;
        *m_OutStream<<"["<<VisibleText<<"]"<<"("<<BlockToVisit.ReferenceString<<")";
    }
    void MarkdownCompiler::Visit(DocReference const& BlockToVisit)
    {
           
    }

    void MarkdownCompiler::Visit(RegularText const& BlockToVisit)
    {
        *m_OutStream <<BlockToVisit.Text;
    }

    void MarkdownCompiler::Visit(Directive const& BlockToVisit)
    {
        if(BlockToVisit.DirectiveName == "toc")
        {
            m_TocCreator.CreateTOC(*m_OutStream);
        }
        else if(BlockToVisit.DirectiveName == "title")
        {
            *m_OutStream<<"# "<<BlockToVisit.Arguments[0]<<"\n";
        }
    }
    void MarkdownCompiler::AddOptions(CommonCompilationOptions const& Options)
    {
        m_OutDir = Options.OutputDirectory;    
    }
    void MarkdownCompiler::PeekDocumentFilesystem(DocumentFilesystem const& FilesystemToCompile)
    {
           
    }
    void MarkdownCompiler::CompileDocument(DocumentPath const& Path,DocumentSource const& Document)
    {
        m_TocCreator.Initialize(Document);
        std::filesystem::path OutPath = (m_OutDir/Path.GetString().substr(1)).replace_extension(".md");
        std::ofstream OutFile = std::ofstream(OutPath,std::ios::out);
        MBUtility::MBFileOutputStream OutStream(&OutFile);
        m_OutStream = &OutStream;
        DocumentTraverser Traverser;
        Traverser.Traverse(Document,*this);
    }
    //END MarkdownReferenceSolver
};
