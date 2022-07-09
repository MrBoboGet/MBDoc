#include "RawTerminal.h"
#include "MBCLI/MBCLI.h"
#include "MBDoc.h"
#include "MBUtility/MBInterfaces.h"
#include <fstream>
#include <iostream>
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
            MBParsing::SkipWhitespace(StringToSplit,ParseOffset,&ParseOffset);
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
                    LineOffset++; 
                }
            }
            OutStream.Write("\n",1);
        }
    }
    void RawTerminalCompiler::p_CompileFormat(FormatElement const& FormatToCompile,MBUtility::MBOctetOutputStream& OutStream,CommonCompilationOptions const& Options)
    {
        if(FormatToCompile.Name != "")
        {
            OutStream.Write("\n",1);  
            OutStream.Write(FormatToCompile.Name.data(),FormatToCompile.Name.size());
            OutStream.Write("\n",1);  
        }

        for(auto const& Element : FormatToCompile.Contents)
        {
            if(Element.GetType() == FormatComponentType::Format)
            {
                p_CompileFormat(Element.GetFormatData(),OutStream,Options); 
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
                ANSIOutputter.WriteColorChange(MBCLI::ANSITerminalColor::Blue,OutStream);
                std::string ColorEndString;
                OutStream = MBUtility::MBStringOutputStream(ColorEndString);
                ANSIOutputter.WriteColorChange(MBCLI::ANSITerminalColor::White,OutStream);
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
        auto LiteralOption = Options.CompilerSpecificOptions.CommandOptions.find("text-literal");
        auto Iterator = FilesystemToCompile.begin();
        while(!Iterator.HasEnded())
        {
            if(!Iterator.EntryIsDirectory())
            {
                std::string OutFile = Options.OutputDirectory+Iterator.GetCurrentPath().GetString();
                std::ofstream FileStream = std::ofstream(OutFile,std::ios::binary|std::ios::out);
                MBUtility::MBFileOutputStream OutStream(&FileStream);
                if(LiteralOption != Options.CompilerSpecificOptions.CommandOptions.end())
                {
                    OutStream.Write("R\"1337xd(",9);
                }
                DocumentSource const& SourceToCompile = Iterator.GetDocumentInfo();
                for(FormatElement const& ElementToCompile : SourceToCompile.Contents)
                {
                    p_CompileFormat(ElementToCompile,OutStream,Options);
                }
                if(LiteralOption != Options.CompilerSpecificOptions.CommandOptions.end())
                {
                    OutStream.Write(")1337xd\"",8);
                }
            }
            Iterator++;
        }
    }
}
