#include "RawTerminal.h"
#include "../MBDoc.h"
#include <MBCLI/MBCLI.h>
#include <MBUtility/MBInterfaces.h>
#include <fstream>
#include <iostream>
#include <MBUtility/MBStrings.h>
namespace MBDoc
{
    
    template<typename IteratorType> 
    std::vector<size_t> h_FormatText(IteratorType Begin,IteratorType End,int Width)
    {
        std::vector<size_t> ReturnValue;
        int CurrentWidth = 0;
        int CurrentIndex = 0;
        while(Begin != End)
        {
            if(CurrentWidth + Begin->size() > Width && CurrentWidth != 0)
            {
                CurrentWidth = 0; 
                ReturnValue.push_back(CurrentIndex);
                continue;
            } 
            if(CurrentWidth + Begin->size() > Width && CurrentWidth == 0)
            {
                //Putting this in separate row
                CurrentWidth = 0;
                ReturnValue.push_back(CurrentIndex+1); 
                Begin++;
                CurrentIndex++;
                continue;
            }
            CurrentWidth += Begin->size()+1;
            CurrentIndex ++;
            Begin++;
        }  
        ReturnValue.push_back(CurrentIndex+1);
        return(ReturnValue);
    }
    void h_ExtractWords(std::string const& StringToSplit,std::vector<i_FormatTextInfo>& OutInfo)
    {
        size_t ParseOffset = 0;
        while(ParseOffset < StringToSplit.size())
        {
            size_t NextWordBegin = 0;
            MBParsing::SkipWhitespace(StringToSplit,ParseOffset,&NextWordBegin);
            if (NextWordBegin >= StringToSplit.size())
            {
                break;
            }
            char Delimiters[] = {' ','\n','\t'};
            size_t NextWordEnd = MBParsing::GetNextDelimiterPosition(Delimiters,Delimiters+sizeof(Delimiters)/sizeof(char),
                    StringToSplit.data(),StringToSplit.size(),NextWordBegin);
            i_FormatTextInfo NewInfo;
            NewInfo.Text = StringToSplit.substr(NextWordBegin,NextWordEnd-NextWordBegin);
            NewInfo.Width = NewInfo.Text.size();
            OutInfo.push_back(std::move(NewInfo));
            ParseOffset = NextWordEnd;
        }
    }    
    void RawTerminalCompiler::p_CompileBlock(BlockElement const& BlockToCompile,MBUtility::MBOctetOutputStream& OutStream,CommonCompilationOptions const& Options)
    {
        
        if(BlockToCompile.Type == BlockElementType::CodeBlock)
        {
            CodeBlock const& CodeBlockData = static_cast<CodeBlock const&>(BlockToCompile); 
            OutStream.Write(CodeBlockData.RawText.data(),CodeBlockData.RawText.size());
        }        
        else if(BlockToCompile.Type == BlockElementType::Paragraph)
        {
            Paragraph const& ParagraphData = static_cast<Paragraph const&>(BlockToCompile);    
            std::vector<i_FormatTextInfo> TotalText;
            for(std::unique_ptr<TextElement> const& Text : ParagraphData.TextElements)
            {
                p_CompileText(*Text,TotalText,Options);   
            }
            std::vector<size_t> LineOffsets = h_FormatText(TotalText.begin(),TotalText.end(),80);
            OutStream.Write("\n",1);
            size_t WordOffset = 0;
            size_t LineOffset = 0;
            while(WordOffset < TotalText.size())
            {
                OutStream.Write(TotalText[WordOffset].Text.data(),TotalText[WordOffset].Text.size()); 
                OutStream.Write(" ",1);
                WordOffset++;
                if(WordOffset >= LineOffsets[LineOffset])
                {
                    OutStream.Write("\n", 1);
                    LineOffset++; 
                }
            }
            OutStream.Write("\n",1);
        }
    }
    void RawTerminalCompiler::p_CompileFormat(FormatElement const& FormatToCompile,MBUtility::MBOctetOutputStream& OutStream,CommonCompilationOptions const& Options, std::string const& Prefix, int Depth,
        std::filesystem::path const& OutPath)
    {
        if(FormatToCompile.Name != "" && Depth != -1)
        {
            OutStream.Write("\n",1);  
            MBCLI::ANSISequenceOutputter ANSIOutputter;
            ANSIOutputter.WriteColorChange(OutStream,MBCLI::ANSITerminalColor::Green);

            std::string NameToWrite = Prefix;
            if (NameToWrite != "")
            {
                NameToWrite += " ";
            }
            NameToWrite += FormatToCompile.Name;
            if (Depth == 0)
            {
                NameToWrite = std::string((80 - NameToWrite.size()) / 2, ' ')+NameToWrite;
            }

            OutStream.Write(NameToWrite.data(), NameToWrite.size());
            ANSIOutputter.WriteColorChange(OutStream,MBCLI::ANSITerminalColor::White);
            OutStream.Write("\n",1);  
        }

        int FormatCount = 1;
        for(auto const& Element : FormatToCompile.Contents)
        {
            if(Element.GetType() == FormatComponentType::Format)
            {
                p_CompileFormat(Element.GetFormatData(), OutStream, Options, Prefix +"." + std::to_string(FormatCount),Depth+1,OutPath);
                FormatCount++;
            }
            else if(Element.GetType() == FormatComponentType::Block)
            {
                p_CompileBlock(Element.GetBlockData(),OutStream,Options); 
            }
            else if(Element.GetType() == FormatComponentType::Directive)
            {
                p_CompileDirective(Element.GetDirectiveData(),OutStream,Options); 
            }
        }
        if (m_CPPCompilation && FormatToCompile.Name != "" && OutPath.filename() != FormatToCompile.Name)
        {
            std::filesystem::path PathToUse = std::filesystem::path(OutPath).replace_filename(FormatToCompile.Name);
            std::ofstream SplitPart = std::ofstream(PathToUse, std::ios::binary | std::ios::out);
            if (SplitPart.is_open() == false)
            {
                throw std::runtime_error("Error compiling output: can't open file");
            }
            MBUtility::MBFileOutputStream SplitOutStream(&SplitPart);
            SplitOutStream.Write("R\"1337xd(", 9);
            p_CompileFormat(FormatToCompile, SplitOutStream, Options, "", -1, PathToUse);
            SplitOutStream.Write(")1337xd\"", 8);
            m_CppIncludePaths[FormatToCompile.Name] = PathToUse;
            SplitPart.flush();
        }
    }
    void RawTerminalCompiler::p_CompileText(TextElement const& TextToCompile,std::vector<i_FormatTextInfo>& OutText,CommonCompilationOptions const& Options)
    {
        if(TextToCompile.Type == TextElementType::Regular)
        {
            RegularText const& TextToWrite = static_cast<RegularText const&>(TextToCompile);  
            h_ExtractWords(TextToWrite.Text,OutText);
        } 
        //else if(TextToCompile.Type == TextElementType::Literal)
        //{
        //    StringLiteral const& LiteralToWrite = static_cast<StringLiteral const&>(TextToCompile); 
        //    i_FormatTextInfo NewInfo;
        //    NewInfo.Text = LiteralToWrite.Text;
        //    NewInfo.Width = LiteralToWrite.Text.size();
        //    OutText.push_back(std::move(NewInfo));
        //}
        else if(TextToCompile.Type == TextElementType::Reference)
        {
            DocReference const& ReferenceToWrite = static_cast<DocReference const&>(TextToCompile);   
            if(ReferenceToWrite.VisibleText != "")
            {
                h_ExtractWords(ReferenceToWrite.VisibleText,OutText);
            }

            i_FormatTextInfo NewInfo;
            NewInfo.Text = "(" + ReferenceToWrite.Identifier + ")";
            NewInfo.Width = NewInfo.Text.size();
            MBError ReferenceResolvable = false;
            m_FilesystemToCompile->ResolveReference(m_CurrentPath,ReferenceToWrite.Identifier,ReferenceResolvable);
            if(ReferenceResolvable)
            {
                MBCLI::ANSISequenceOutputter ANSIOutputter;
                std::string ColorBeginString;
                MBUtility::MBStringOutputStream OutStream(ColorBeginString);
                ANSIOutputter.WriteColorChange(OutStream,MBCLI::ANSITerminalColor::Blue);
                std::string ColorEndString;
                OutStream = MBUtility::MBStringOutputStream(ColorEndString);
                ANSIOutputter.WriteColorChange(OutStream,MBCLI::ANSITerminalColor::White);
                NewInfo.Text = ColorBeginString + NewInfo.Text + ColorEndString;
            }
            OutText.push_back(NewInfo);
        }
    }
    void RawTerminalCompiler::p_CompileDirective(Directive const& TextToCompile,MBUtility::MBOctetOutputStream& OutStream,CommonCompilationOptions const& Options)
    {
         
    }
    void RawTerminalCompiler::Compile(const DocumentFilesystem &FilesystemToCompile, const CommonCompilationOptions &Options)  
    {
        m_CPPCompilation = Options.CompilerSpecificOptions.CommandOptions.find("cpp") != Options.CompilerSpecificOptions.CommandOptions.end();
        auto Iterator = FilesystemToCompile.begin();

        m_CLIInput = Options.CompilerSpecificOptions;
        m_FilesystemToCompile = &FilesystemToCompile;
        while(!Iterator.HasEnded())
        {
            if(!Iterator.EntryIsDirectory())
            {
                m_CurrentPath = Iterator.GetCurrentPath();
                std::filesystem::path OutFile = Options.OutputDirectory+Iterator.GetCurrentPath().GetString();
                m_CppIncludePaths[m_CurrentPath.GetString().substr(1)] = OutFile;
                if (!std::filesystem::exists(OutFile.parent_path()))
                {
                    std::filesystem::create_directories(OutFile.parent_path());
                }
                std::ofstream FileStream = std::ofstream(OutFile,std::ios::binary|std::ios::out);
                if(FileStream.is_open() == false)
                {
                    throw std::runtime_error("Error compiling output: can't open file");
                }
                MBUtility::MBFileOutputStream OutStream(&FileStream);
                if(m_CPPCompilation)
                {
                    OutStream.Write("R\"1337xd(",9);
                }
                DocumentSource const& SourceToCompile = Iterator.GetDocumentInfo();

                int FormatIndex = 1;
                for(FormatElement const& ElementToCompile : SourceToCompile.Contents)
                {
                    p_CompileFormat(ElementToCompile,OutStream,Options,std::to_string(FormatIndex),0,OutFile);
                    FormatIndex++;
                }
                if(m_CPPCompilation)
                {
                    OutStream.Write(")1337xd\"",8);
                }
            }
            Iterator++;
        }
        if (m_CPPCompilation)
        {
            std::filesystem::path OutIncludeIndex = Options.OutputDirectory;
            OutIncludeIndex /= "HelpInclude.i";
            std::string Contents = "{";
            for (auto const& Include : m_CppIncludePaths)
            {
                Contents += "{\"" + Include.first + "\",\n#include \""+
                    MBUtility::ReplaceAll(MBUnicode::PathToUTF8(std::filesystem::relative(Include.second)),"\\","/") + "\"\n},";
            }
            Contents += "}";
            std::ofstream OutIncludeStream = std::ofstream(OutIncludeIndex);
            OutIncludeStream << Contents;
        }
    }
}
