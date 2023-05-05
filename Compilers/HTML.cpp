#include "HTML.h"
#include "../MBDoc.h"
#include <MBUtility/MBStrings.h>
namespace MBDoc
{
    //TODO optimize
    std::string h_EscapeHTMLText(std::string const& TextToEscape)
    {
        //Inefficient af, but enough for current demands
        std::string ReturnValue = TextToEscape;
        MBUtility::ReplaceAll(&ReturnValue, "&", "&amp;");
        MBUtility::ReplaceAll(&ReturnValue, "<", "&lt;");
        MBUtility::ReplaceAll(&ReturnValue, ">", "&gt;");
        return(ReturnValue);
    }
    //BEGIN HTMLReferenceSolver
    void HTMLReferenceSolver::SetCurrentPath(DocumentPath CurrentPath)
    {
        m_CurrentPath = std::move(CurrentPath);
    }
    std::string h_PartIdentifierToHTMLID(std::vector<std::string> const& PartIdentifier)
    {
        std::string ReturnValue = "#";
        bool FirstPart = true;
        for(auto const& Part : PartIdentifier)
        {
            if(FirstPart)
            {
                ReturnValue += Part;   
                FirstPart = false;
            }
            else
            {
                ReturnValue += "_"+Part;
            }
        }
        return(ReturnValue);
    }
    std::string HTMLReferenceSolver::GetDocumentPathURL(DocumentPath const& PathToConvert) 
    {
        std::string ReturnValue = DocumentPath::GetRelativePath(PathToConvert, m_CurrentPath).GetString();
        ReturnValue = MBUtility::ReplaceAll(ReturnValue, ".mbd", ".html");
        if (PathToConvert.GetPartIdentifier().size() != 0)
        {
            ReturnValue += h_PartIdentifierToHTMLID(PathToConvert.GetPartIdentifier());
        }
        return(ReturnValue);
    }
    bool HTMLReferenceSolver::DocumentExists(DocumentPath const& CurrentPath,std::string const& PathToTest)
    {
        bool ReturnValue = false;
        MBError ResolveResult = true;
        auto Result = m_AssociatedBuild->ResolveReference(CurrentPath,PathToTest,ResolveResult);
        return(ResolveResult);
    }
    void HTMLReferenceSolver::Visit(URLReference const& Ref)
    {
        m_VisitResultProperties += Ref.URL;
        m_VisitResultProperties += "\" class=\"link\"";
        m_VisitResultText = m_VisitResultProperties;
    }
    void HTMLReferenceSolver::Visit(FileReference const& Ref)
    {
       m_VisitResultProperties = GetDocumentPathURL(Ref.Path);
       m_VisitResultProperties += "\" class=\"link\"";
       m_VisitResultText = m_VisitResultProperties;
    }
    void HTMLReferenceSolver::Visit(UnresolvedReference const& Ref)
    {

        m_VisitResultProperties = "javascript:void(0);\" class=\"errorLink\"";
        if (m_Colorize)
        {
            m_VisitResultProperties += " style=\"color: red;\"";
        }
        else 
        {
            m_VisitResultProperties += " style=\"color: inherit;\"";
        }
       m_VisitResultText = Ref.ReferenceString;
    }
    std::string HTMLReferenceSolver::GetReferenceString(DocReference const& ReferenceIdentifier,bool Colorize)
    {
        m_Colorize = Colorize;
        std::string ReturnValue = "<a href=\"";
        ReferenceIdentifier.Accept(*this);
        ReturnValue += m_VisitResultProperties;
        ReturnValue += ">";
        if(ReferenceIdentifier.VisibleText != "")
        {
            ReturnValue += h_EscapeHTMLText(ReferenceIdentifier.VisibleText);    
        }
        else
        {
            ReturnValue += h_EscapeHTMLText(m_VisitResultText);    
        }
        ReturnValue += "</a>";
        return(ReturnValue);
    }
    void HTMLReferenceSolver::Initialize(DocumentFilesystem const* Build)
    {
        m_AssociatedBuild = Build;
    }
    //END HTMLReferenceSolver

    //BEGIN HTMLNavigationCreator
    HTMLNavigationCreator::Directory HTMLNavigationCreator::p_CreateDirectory(DocumentFilesystemIterator& FileIterator)
    {
        Directory ReturnValue;       
        size_t CurrentDepth = FileIterator.CurrentDepth();
        if(!FileIterator.HasEnded() && FileIterator.EntryIsDirectory())
        {
            ReturnValue.Name = FileIterator.GetEntryName();
            ReturnValue.Path = FileIterator.GetCurrentPath();
            FileIterator.Increment();
        }
        //CurrentDepth == 0 is a special case when we are at top directoy,
        //which isn't explicitly listed
        while(!FileIterator.HasEnded() && (CurrentDepth < FileIterator.CurrentDepth() || CurrentDepth == 0))
        {
            if(!FileIterator.EntryIsDirectory())
            {
                DocumentSource const& CurrentSource = FileIterator.GetDocumentInfo();
                File NewFile;
                NewFile.Name = CurrentSource.Name;
                NewFile.Path = FileIterator.GetCurrentPath();
                ReturnValue.Files.push_back(std::move(NewFile));
                ++FileIterator;
            }
            else
            {
                ReturnValue.SubDirectories.push_back(p_CreateDirectory(FileIterator));  
            } 
        }
        return(ReturnValue);
    }
    HTMLNavigationCreator::HTMLNavigationCreator(DocumentFilesystem const& FilesystemToInspect)
    {
        auto Iterator = FilesystemToInspect.begin();
        m_TopDirectory = p_CreateDirectory(Iterator);
        m_TopDirectory.Open = true;
    }
    void HTMLNavigationCreator::p_ToggleOpen(DocumentPath const& PathToToggle)
    {
        Directory* CurrentDir = &m_TopDirectory;
        for(int i = 0; i < PathToToggle.ComponentCount();i++)
        {
            std::string CurrentComponent = PathToToggle[i];
            if(CurrentComponent  == "/")
            {
                continue;   
            }
            //simple linear search
            for(auto& Dir : CurrentDir->SubDirectories)
            {
                if(Dir.Name == CurrentComponent)
                {
                    CurrentDir = &Dir;    
                    CurrentDir->Open = !CurrentDir->Open;
                    break;
                }
            }
        }
    }
    void HTMLNavigationCreator::SetCurrentPath(DocumentPath CurrentPath)
    {
        m_CurrentPath = std::move(CurrentPath);        
    }
    void HTMLNavigationCreator::SetOpen(DocumentPath NewPath)
    {
        p_ToggleOpen(m_PreviousOpen);
        p_ToggleOpen(NewPath);
        m_PreviousOpen = NewPath;
    }
    void HTMLNavigationCreator::p_WriteDirectory(MBUtility::MBOctetOutputStream& OutStream, HTMLReferenceSolver& ReferenceSolver,Directory const& DirectoryToWrite) const
    {
        std::string ElementHeader = "<details";
        if(DirectoryToWrite.Open)
        {
            ElementHeader += " open=\"\"";   
        }
        ElementHeader += ">";
        OutStream.Write(ElementHeader.data(),ElementHeader.size());
        
        std::string SummaryStringContent = DirectoryToWrite.Name;
        if(ReferenceSolver.DocumentExists(DirectoryToWrite.Path,DirectoryToWrite.Name+"/index.mbd"))
        {
            std::string HrefURL = ReferenceSolver.GetDocumentPathURL(DirectoryToWrite.Path);
            if (HrefURL == "")
            {
                //this means that we are in the same directory as that pointed to by the path, essentially meaning we want to point to the index of the directory
                HrefURL = "index.html";
            }
            else
            {
                HrefURL += "/index.html";
            }
            SummaryStringContent = "<a href=\""+HrefURL+"\">"+DirectoryToWrite.Name+"</a>";
        }
        std::string SummaryString = "<summary>"+ SummaryStringContent+ "</summary>";
        OutStream.Write(SummaryString.data(), SummaryString.size());
        OutStream.Write("<ul>", 4);
        for(File const& FileToWrite : DirectoryToWrite.Files)
        {
            OutStream.Write("<li>", 4);
            p_WriteFile(OutStream,ReferenceSolver,FileToWrite);   
            OutStream.Write("</li>", 5);
        }
        for(Directory const& DirectoryToWrite : DirectoryToWrite.SubDirectories)
        {
            OutStream.Write("<li>", 4);
            p_WriteDirectory(OutStream,ReferenceSolver,DirectoryToWrite);
            OutStream.Write("</li>", 5);
        }
        OutStream.Write("</ul>", 5);
        OutStream.Write("</details>",10);
    }
    void HTMLNavigationCreator::p_WriteFile(MBUtility::MBOctetOutputStream& OutStream, HTMLReferenceSolver& ReferenceSolver,File const& FileToWrite) const
    {
        std::string ElementData = "<a href=\""+ ReferenceSolver.GetDocumentPathURL(FileToWrite.Path) +"\" ";
        ElementData += ">";
        ElementData += FileToWrite.Name;
        ElementData += "</a>";
        if (FileToWrite.Path == m_CurrentPath)
        {
            ElementData = "<mark>" + ElementData + "</mark>";
        }
        OutStream.Write(ElementData.data(),ElementData.size());
    }
    void HTMLNavigationCreator::__PrintDirectoryStructure(Directory const& DirectoryToPrint,int Depth) const
    {
        //std::cout << std::string(4*Depth, ' ')<<DirectoryToPrint.Name<<"\n";
        //for (auto const& File : DirectoryToPrint.Files)
        //{
        //    std::cout << std::string(4*(Depth+1), ' ') << File.Name << "\n";
        //}
        //for (auto const& SubDir : DirectoryToPrint.SubDirectories)
        //{
        //    __PrintDirectoryStructure(SubDir, Depth + 1);
        //}
    }
    void HTMLNavigationCreator::__PrintDirectoryStructure() const
    {
        //__PrintDirectoryStructure(m_TopDirectory,0);

        //std::cout.flush();
    }
    void HTMLNavigationCreator::WriteTableDiv(MBUtility::MBOctetOutputStream& OutStream, HTMLReferenceSolver& ReferenceSolver) const
    {
        std::string ElementHeader = "<div style=\"width: 30%; display: inline-block\">";
        OutStream.Write(ElementHeader.data(),ElementHeader.size()); 
        p_WriteDirectory(OutStream,ReferenceSolver,m_TopDirectory);
        //for(auto const& FileToWrite : m_TopDirectory.Files)
        //{
        //    p_WriteFile(OutStream,FileToWrite);
        //} 
        //for(auto const& DirectoryToWrite : m_TopDirectory.SubDirectories)
        //{
        //    p_WriteDirectory(OutStream,DirectoryToWrite);
        //} 
        OutStream.Write("</div>",6);
    }
    //END HTMLNavigationCreator
    //BEGIN HTMLCompiler
    std::string HTMLCompiler::p_GetNumberLabel(int FormatDepth)
    {
        std::string ReturnValue;        
        for(int i = 0; i < FormatDepth;i++)
        {
            if(i != 0)
            {
                ReturnValue += '.';   
            }
            ReturnValue += std::to_string(m_FormatCounts[i]);   
        }
        return(ReturnValue);
    }
    void HTMLCompiler::EnterText(TextElement_Base const& ElementToEnter)
    {
        TextModifier Modifiers = ElementToEnter.Modifiers;
        if ((Modifiers & TextModifier::Bold) != TextModifier::Null)
        {
            m_OutStream->Write("<b>", 3);
        }
        if ((Modifiers & TextModifier::Italic) != TextModifier::Null)
        {
            m_OutStream->Write("<i>", 3);
        }
        if(!ElementToEnter.Color.IsDefault())
        {
            *m_OutStream<<"<span class=\"color\" style=\"color: #"<<
                MBUtility::HexEncodeByte(ElementToEnter.Color.R)<<MBUtility::HexEncodeByte(ElementToEnter.Color.G)
                <<MBUtility::HexEncodeByte(ElementToEnter.Color.B)<<";\">";
        }
    }
    void HTMLCompiler::LeaveText(TextElement_Base const& ElementToLeave)
    {
        if ((ElementToLeave.Modifiers & TextModifier::Bold) != TextModifier::Null)
        {
            m_OutStream->Write("</b>", 4);
        }
        if ((ElementToLeave.Modifiers & TextModifier::Italic) != TextModifier::Null)
        {
            m_OutStream->Write("</i>", 4);
        }
        if(!ElementToLeave.Color.IsDefault())
        {
            *m_OutStream<<"</span>";
        }
    }
    DocumentPath HTMLCompiler::p_GetUniquePath(std::string const& Extension)
    {
        DocumentPath ReturnValue;
        ReturnValue.AddDirectory("/");
        ReturnValue.AddDirectory("___Resources");
        ReturnValue.AddDirectory(std::to_string(m_ExportedElementsCount) + "." + Extension);
        m_ExportedElementsCount += 1;
        return(ReturnValue);
    }

    void HTMLCompiler::CompileDocument(DocumentPath const& Path,DocumentSource const& Document)
    {
        m_NavigationCreator.SetOpen(Path);
        m_ReferenceSolver.SetCurrentPath(Path);
        m_NavigationCreator.SetCurrentPath(Path);
        m_SourcePath = Document.Path;
        
        std::string OutFile = m_CommonOptions.OutputDirectory+"/"+Path.GetString();
        size_t FirstDot = OutFile.find_last_of('.');
        if (FirstDot == OutFile.npos)
        {
            OutFile += ".html";
        }
        else
        {
            OutFile.replace(FirstDot, 5, ".html");
        }
        std::filesystem::path FilePath = OutFile;
        if (!std::filesystem::exists(FilePath.parent_path()))
        {
            std::filesystem::create_directories(FilePath.parent_path());
        }
        std::ofstream OutStream = std::ofstream(OutFile);
        if (!OutStream.is_open())
        {
            throw std::runtime_error("File not open");
        }
        m_OutStream = std::unique_ptr<MBUtility::MBOctetOutputStream>(new MBUtility::MBFileOutputStream(&OutStream));
        std::string TopInfo = "<!DOCTYPE html><html><head><style>body{background-color: black; color: #00FF00;}table,th,td{border: 1px solid;}table{border-collapse: collapse;} .color .link{color: inherit !important;} .color .errorLink {color: inherit !important; text-decoration: none;}</style></head><body>";
        OutStream.write(TopInfo.data(), TopInfo.size());
        std::string TopFlex = "<div style=\"display: flex\">";
        OutStream.write(TopFlex.data(), TopFlex.size());
        m_NavigationCreator.WriteTableDiv(*m_OutStream,m_ReferenceSolver);

        std::string ContentTop = "<div style=\"width: 80ch; display: inline-block; position: absolute; top: 0; left: 0; right: 0; bottom: 0; margin: auto;\">";
        OutStream.write(ContentTop.data(),ContentTop.size());

        m_FormatDepth = 0;
        m_FormatCounts.resize(0);

        DocumentTraverser Traverser;
        Traverser.Traverse(Document,*this);

        OutStream.write("</div></div></body></html>", 26);
        OutStream.flush();
    }



    HTMLCompiler::HTMLCompiler()
    {
               
    }
    void HTMLCompiler::Visit(CodeBlock const& BlockToWrite)
    {
        m_InCodeBlock = true;
        m_OutStream->Write("<pre>", 5);
        if(std::holds_alternative<std::string>(BlockToWrite.Content))
        {
            *m_OutStream<<h_EscapeHTMLText(std::get<std::string>(BlockToWrite.Content));
        }
        else if(std::holds_alternative<ResolvedCodeText>(BlockToWrite.Content))
        {
            for(auto const& Row : std::get<ResolvedCodeText>(BlockToWrite.Content))
            {
                for(auto const& Text : Row)
                {
                    EnterText(Text.GetBase());
                    Text.Accept(*this);
                    LeaveText(Text.GetBase());
                }
                *m_OutStream<<"<br>";
            } 
        }
        m_InCodeBlock = false;
        m_OutStream->Write("</pre>", 6);
    }
    void HTMLCompiler::Visit(MediaInclude const& MediaToInclude)
    {
        std::filesystem::path MediaToIncludePath = m_SourcePath.parent_path()/MediaToInclude.MediaPath;
        std::string CanonicalString = MBUnicode::PathToUTF8(std::filesystem::canonical(MediaToIncludePath));
        DocumentPath& OutPath = m_MovedResources[CanonicalString];
        std::string Extension;
        size_t DotPosition = MediaToInclude.MediaPath.find_last_of('.');
        if (DotPosition != MediaToInclude.MediaPath.npos)
        {
            Extension = MediaToInclude.MediaPath.substr(DotPosition + 1);
        }
        if (OutPath.Empty())
        {
            //need to actually move the path
            OutPath = p_GetUniquePath(Extension);
            if (!std::filesystem::exists(MediaToIncludePath))
            {
                throw std::runtime_error("Can't find media to include: "+MediaToInclude.MediaPath);
            }
            std::filesystem::copy_options Options = std::filesystem::copy_options::overwrite_existing;
            //innefficent, but / gets wacky with the std::filesystem 
            std::filesystem::copy(MediaToIncludePath,m_CommonOptions.OutputDirectory +"/"+ OutPath.GetString().substr(1),Options);
        }
        MBMIME::MediaType TypeToInclude = MBMIME::GetMediaTypeFromExtension(Extension);
        if (TypeToInclude == MBMIME::MediaType::Video)
        {
            std::string TotalElementData = "<video src=\"" + m_ReferenceSolver.GetDocumentPathURL(OutPath) + "\" controls style=\"display: block; margin: auto\"></video>";
            m_OutStream->Write(TotalElementData.data(), TotalElementData.size());
        }
        else if (TypeToInclude == MBMIME::MediaType::Image)
        {
            std::string TotalElementData = "<img src=\"" + m_ReferenceSolver.GetDocumentPathURL(OutPath) + "\" style=\"display: block; margin: auto\">";
            m_OutStream->Write(TotalElementData.data(), TotalElementData.size());
        }
        else
        {
            throw std::runtime_error("Can't deduce valid media type from extension \"" + Extension + "\"");
        }
    }
    void HTMLCompiler::Visit(Paragraph const& ParagraphToWrite)
    {
        if (ParagraphToWrite.Attributes.HasAttribute("Note"))
        {
            m_OutStream->Write("<mark>Note: </mark>", 19);
        }
    }
    void HTMLCompiler::EnterBlock(BlockElement const& BlockToEnter)
    {
        if(BlockToEnter.IsType<Table>())
        {
            m_InTable = true;   
            m_TableWidth = BlockToEnter.GetType<Table>().ColumnCount();
            m_CurrentColumnCount = 0;
            *m_OutStream << "<table>";
        }
        else if(BlockToEnter.IsType<Paragraph>())
        {
            if(m_InTable)
            {
                if(m_CurrentColumnCount == 0)
                {
                    *m_OutStream << "<tr>"; 
                }  
                *m_OutStream<<"<td>";
                m_CurrentColumnCount++;
                m_CurrentColumnCount = m_CurrentColumnCount%m_TableWidth;
            } 
        }
    }
    void HTMLCompiler::LeaveBlock(BlockElement const& BlockToLeave)
    {
        if(BlockToLeave.IsType<Paragraph>())
        {
            if(m_InTable)
            {
                if(m_CurrentColumnCount == 0)
                {
                    *m_OutStream<<"</tr>";  
                } 
                *m_OutStream<<"</td>";  
            }
            else
            {
                m_OutStream->Write("<br><br>\n\n", 10);
            }
        }
        else if(BlockToLeave.IsType<Table>())
        {
            m_InTable = false;   
            m_TableWidth = 0;
            m_CurrentColumnCount = 0;
            *m_OutStream << "</table>";
        }
    }
    void HTMLCompiler::Visit(URLReference const& BlockToVisit)
    {
        std::string StringToWrite = m_ReferenceSolver.GetReferenceString(BlockToVisit);
        m_OutStream->Write(StringToWrite.data(), StringToWrite.size());
        
    }
    void HTMLCompiler::Visit(FileReference const& BlockToVisit)
    {
        std::string StringToWrite = m_ReferenceSolver.GetReferenceString(BlockToVisit);
        m_OutStream->Write(StringToWrite.data(), StringToWrite.size());
           
    }
    void HTMLCompiler::Visit(UnresolvedReference const& BlockToVisit)
    {
        std::string StringToWrite;
        if (m_InCodeBlock) 
        {
            StringToWrite = m_ReferenceSolver.GetReferenceString(BlockToVisit, false);
        }
        else 
        {
            StringToWrite = m_ReferenceSolver.GetReferenceString(BlockToVisit);
        }
        m_OutStream->Write(StringToWrite.data(), StringToWrite.size());
    }
    void HTMLCompiler::Visit(DocReference const& BlockToVisit)
    {
        BlockToVisit.Accept(*this);
    }
    void HTMLCompiler::Visit(RegularText const& RegularTextElement)
    {
        std::string TextToWrite = h_EscapeHTMLText(RegularTextElement.Text);
        m_OutStream->Write(TextToWrite.data(),TextToWrite.size());
    }
    void HTMLCompiler::EnterFormat(FormatElement const& FormatToCompile)
    {
        m_FormatDepth += 1;
        if(m_FormatCounts.size() < m_FormatDepth)
        {
            m_FormatCounts.push_back(0);
        }
        m_FormatCounts[m_FormatDepth-1] += 1;
        std::string HeadingTag = "h"+std::to_string(m_FormatDepth);
        m_CurrentPartIdentifier.push_back(FormatToCompile.Name);
        //if(FormatToCompile.Type != FormatElementType::Default)
        //{
            std::string StringToWrite = "<" + HeadingTag + " id=\"" + h_PartIdentifierToHTMLID(m_CurrentPartIdentifier).substr(1)+"\" ";
            if (m_FormatDepth == 1)
            {
                StringToWrite += "style = \"text-align: center;\"";
            }
            StringToWrite += ">";

            StringToWrite += p_GetNumberLabel(m_FormatDepth) +" "+ FormatToCompile.Name + "</" + HeadingTag + "><br>\n";

            m_OutStream->Write(StringToWrite.data(),StringToWrite.size());
        //}
    }
    void HTMLCompiler::LeaveFormat(FormatElement const& ElementToEnter)
    {
        if(m_FormatDepth + 1 < m_FormatCounts.size())
        {
            m_FormatCounts.pop_back();    
        }
        if(m_FormatDepth < m_FormatCounts.size())
        {
            m_FormatCounts[m_FormatDepth] = 0;   
        }
        if(m_CurrentPartIdentifier.size() > 0)
        {
            m_CurrentPartIdentifier.pop_back();   
        }
        m_FormatDepth -= 1;
    }
    void HTMLCompiler::Visit(Directive const& DirectiveToCompile)
    {
        if (DirectiveToCompile.DirectiveName == "title")
        {
            if (DirectiveToCompile.Arguments.PositionalArgumentsCount() > 0)
            {
                std::string StringToWrite = "<h1 style=\"text-align: center;\">" + DirectiveToCompile.Arguments[0] + "</h1>\n";
                m_OutStream->Write(StringToWrite.data(), StringToWrite.size());
            }
        }
    }
    void HTMLCompiler::AddOptions(CommonCompilationOptions const& Options)
    {
        m_CommonOptions = Options;
        std::filesystem::create_directories(m_CommonOptions.OutputDirectory +"/___Resources");
    }
    void HTMLCompiler::PeekDocumentFilesystem(DocumentFilesystem const& FilesystemToCompile)
    {
        m_ReferenceSolver.Initialize(&FilesystemToCompile); 
        m_NavigationCreator = HTMLNavigationCreator(FilesystemToCompile);
    }
    //void HTMLCompiler::Compile(DocumentFilesystem const& BuildToCompile,CommonCompilationOptions const& Options)
    //{
    //    HTMLReferenceSolver ReferenceSolver;
    //    ReferenceSolver.Initialize(&BuildToCompile);
    //    HTMLNavigationCreator NavigationCreator(BuildToCompile);
    //    DocumentFilesystemIterator FileIterator = BuildToCompile.begin();

    //    //DEBUG
    //    //std::cout << "Regular info:" << std::endl;
    //    //BuildToCompile.__PrintDirectoryStructure();
    //    //std::cout << "Adapted directory info" << std::endl;
    //    //NavigationCreator.__PrintDirectoryStructure();
    //    //return;
    //    while (!FileIterator.HasEnded())
    //    {
    //        if (!FileIterator.EntryIsDirectory())
    //        {
    //            DocumentPath CurrentPath = FileIterator.GetCurrentPath();
    //            NavigationCreator.SetOpen(CurrentPath);
    //            ReferenceSolver.SetCurrentPath(CurrentPath);
    //            NavigationCreator.SetCurrentPath(CurrentPath);
    //            DocumentSource const& SourceToCompile = FileIterator.GetDocumentInfo();
    //            p_CompileSource(Options.OutputDirectory+"/" + CurrentPath.GetString(), SourceToCompile, ReferenceSolver,NavigationCreator);
    //        }
    //        FileIterator++;
    //    }
    //}

    //END HTMLCompiler
}
