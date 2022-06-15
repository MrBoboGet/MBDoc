#include "MBDoc.h"
#include "MBUtility/MBInterfaces.h"
#include <cstring>
#include <memory>
#include <MBParsing/MBParsing.h>
#include <stdexcept>
#include <assert.h>
#include <stdint.h>

#include <iostream>
#include <MBUtility/MBStrings.h>
#include <MBUnicode/MBUnicode.h>
namespace MBDoc
{

    //BEGIN AttributeList
    bool AttributeList::IsEmpty() const
    {
        return(m_Attributes.size() == 0);   
    }
    void AttributeList::Clear()
    {
        m_Attributes.clear();       
        assert(IsEmpty() == false);
    }
    void AttributeList::AddAttribute(std::string const& AttributeName)
    {
        m_Attributes.insert(AttributeName);    
    }
    bool AttributeList::HasAttribute(std::string const& AttributeToCheck) const
    {
        return(m_Attributes.find(AttributeToCheck) != m_Attributes.end());        
    }
    //END AttributeList
    //BEGIN LineRetriever
    LineRetriever::LineRetriever(MBUtility::MBOctetInputStream* InputStream)
    {
        m_InputStream = InputStream;       
    }
    bool LineRetriever::Finished()
    {
        return(m_Finished);
    }
    bool LineRetriever::GetLine(std::string& OutLine)
    {
        bool ReturnValue = false;
        if (m_LineBuffered)
        {
            OutLine = m_CurrentLine;
            m_LineBuffered = false;
            return(true);
        }
        if(m_StreamFinished && m_Buffer.size() == 0)
        {
            OutLine = "";  
            m_Finished = true;
            return(ReturnValue);
        } 
        else
        {
            size_t NewlinePosition = m_Buffer.find('\n',m_BufferOffset);
            size_t SearchOffset = m_BufferOffset;
            while(m_StreamFinished == false)
            {
                NewlinePosition = m_Buffer.find('\n',SearchOffset);   
                if(NewlinePosition != m_Buffer.npos)
                {
                    break;   
                }
                SearchOffset = m_Buffer.size();
                constexpr size_t RecieveChunkSize = 4096;
                char RecieveBuffer[RecieveChunkSize];
                size_t RecievedBytes = m_InputStream->Read(RecieveBuffer,RecieveChunkSize);
                m_Buffer += std::string(RecieveBuffer,RecieveBuffer+RecievedBytes);
                if(RecievedBytes < RecieveChunkSize)
                {
                    m_StreamFinished = true;
                    NewlinePosition = m_Buffer.find('\n',SearchOffset);
                }
            }
            if(NewlinePosition == m_Buffer.npos)
            {
                m_CurrentLine = m_Buffer.substr(m_BufferOffset);
                m_Buffer.clear();
                m_BufferOffset = 0;
            }
            else
            {
                if(NewlinePosition > 0 && m_Buffer[NewlinePosition-1] == '\r')
                {
                    m_CurrentLine = m_Buffer.substr(m_BufferOffset,NewlinePosition-1-m_BufferOffset);    
                }
                else
                {
                    m_CurrentLine = m_Buffer.substr(m_BufferOffset,NewlinePosition-m_BufferOffset);    
                }
                m_BufferOffset = NewlinePosition+1;
            }
            if(m_BufferOffset == m_Buffer.size() || m_BufferOffset > 1000000)//a bit arbitrary
            {
                m_Buffer = m_Buffer.substr(m_BufferOffset);
                m_BufferOffset = 0;
            }
        }
        OutLine = m_CurrentLine;
        m_LineBuffered = false;
        return(ReturnValue);
    }
    void LineRetriever::DiscardLine()
    {
        m_LineBuffered = false;
        GetLine(m_CurrentLine); 
        m_LineBuffered = true;
    }
    std::string& LineRetriever::PeekLine()
    {
        if(m_LineBuffered == false)
        {
            GetLine(m_CurrentLine);  
            m_LineBuffered = true;
        } 
        return(m_CurrentLine);
    }
    //LineRetriever




    DocReference DocumentParsingContext::p_ParseReference(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset)
    {
        DocReference ReturnValue;
        //ASSUMPTION reference is not escaped 
        if(!(ParseOffset + 1 < DataSize))
        {
            throw std::runtime_error("Invalid reference, empty name");   
        }
        const char* CharData = (const char*)Data;
        if(CharData[ParseOffset+1] == '[')
        {
            size_t TextEnd = std::find(CharData+ParseOffset+2,CharData+DataSize,']')-(CharData);
            if(TextEnd == DataSize)
            {
                throw std::runtime_error("Unexpected end of visible text in reference");   
            }
            ReturnValue.VisibleText = std::string(CharData+ParseOffset+2,CharData+TextEnd);
            ParseOffset = TextEnd+1;
            if(ParseOffset >= DataSize)
            {
                throw std::runtime_error("Reference with visible requires refernce identifier part");
            }
            if(CharData[ParseOffset] != '(')
            {
                throw std::runtime_error("Invalid start of reference identifier");   
            }
            size_t ReferenceIDEnd = std::find(CharData+ParseOffset+1,CharData+DataSize,')')-(CharData);
            if(ReferenceIDEnd == DataSize)
            {
                throw std::runtime_error("Unexpected end of reference identifier");   
            }
            ReturnValue.Identifier = std::string(CharData+ParseOffset+1,CharData+ReferenceIDEnd);
            ParseOffset = ReferenceIDEnd + 1;
        }
        else
        {
            size_t DelimiterPosition = MBParsing::GetNextDelimiterPosition({'\t',' ','\n'},Data,DataSize,ParseOffset,nullptr);
            ReturnValue.Identifier = std::string(CharData+ParseOffset+1,CharData+DelimiterPosition); 
            ParseOffset = DelimiterPosition;
        }
        *OutParseOffset = ParseOffset;
        return(ReturnValue);    
    }
    struct TextState
    {
        TextColor Color;
        TextModifier Modifiers = TextModifier::Null;   
    };
    TextState h_ParseTextState(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset)
    {
        TextState ReturnValue;
        const char* CharData = (const char*)Data;
        //assumes that characters are not escaped
        bool ResultParsed = false;
        if(ParseOffset + 2 < DataSize)
        {
            if(std::memcmp(CharData+ParseOffset,"***",3) == 0)
            {
                ParseOffset += 3;
                ReturnValue.Modifiers = ReturnValue.Modifiers | TextModifier::Bold | TextModifier::Italic; 
                ResultParsed = true;
            }   
        }
        if(!ResultParsed)
        {
            if(ParseOffset + 1 < DataSize)
            {
                if(std::memcmp(CharData+ParseOffset,"**",2) == 0)
                {
                    ReturnValue.Modifiers = ReturnValue.Modifiers | TextModifier::Bold;
                    ParseOffset += 2;
                    ResultParsed = true;
                }
            }   
        }
        if(!ResultParsed)
        {
            if(CharData[ParseOffset] == '*')
            {
                ReturnValue.Modifiers = ReturnValue.Modifiers | TextModifier::Italic;
                ResultParsed = true;
                ParseOffset += 1;
            }   
            if (CharData[ParseOffset] == '_')
            {
                ReturnValue.Modifiers = ReturnValue.Modifiers | TextModifier::Underlined;
                ResultParsed = true;
                ParseOffset += 1;
            }
        }
        if(OutParseOffset != nullptr)
        {
            *OutParseOffset = ParseOffset;   
        }
        return(ReturnValue);   
    }
    bool h_IsEscaped(void const* Data,size_t DataSize,size_t ParseOffset)
    {
        bool ReturnValue = false;
        const char* CharData = (const char*)Data;
        while(ParseOffset > 0)
        {
            if(CharData[ParseOffset-1] == '\\')
            {
                ReturnValue = !ReturnValue;
            }
            else
            {
                break;  
            } 
            ParseOffset--;
        }
        return(ReturnValue);   
    }
    std::vector<std::unique_ptr<TextElement>> DocumentParsingContext::p_ParseTextElements(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset)
    {
        std::vector<std::unique_ptr<TextElement>> ReturnValue;
        //ASSUMPTION no # or block delimiter present, guaranteed text element data 
        const char* CharData = (const char*)Data;
        TextColor CurrentTextColor;
        TextModifier CurrentTextModifier = TextModifier(0); 
        while(ParseOffset < DataSize)
        {
            size_t FindTextModifierDelimiterOffset = ParseOffset;
            size_t NextReferenceDeclaration = DataSize;
            size_t NextTextModifier = DataSize;
            size_t NextModifier = DataSize;
            while(FindTextModifierDelimiterOffset < DataSize)
            {
                NextReferenceDeclaration = std::find(CharData+FindTextModifierDelimiterOffset,CharData+DataSize,'@')-(CharData);
                NextTextModifier = std::find(CharData+FindTextModifierDelimiterOffset,CharData+DataSize,'*')-(CharData);
                NextTextModifier =std::min(NextTextModifier,size_t(std::find(CharData+FindTextModifierDelimiterOffset,CharData+DataSize,'_')-(CharData)));

                NextModifier = std::min(NextTextModifier,NextReferenceDeclaration);
                if(NextModifier == DataSize)
                {
                    break;   
                }
                if(h_IsEscaped(Data,DataSize,NextModifier))
                {
                    FindTextModifierDelimiterOffset = NextModifier+1;  
                }
                else
                {
                    break;   
                }
            }
                
            if(ParseOffset < NextModifier)
            {
                RegularText NewText; 
                NewText.Modifiers = CurrentTextModifier;
                NewText.Color = CurrentTextColor;
                NewText.Text = std::string(CharData+ParseOffset,NextModifier-ParseOffset);
                ReturnValue.push_back(std::unique_ptr<TextElement>(new RegularText(std::move(NewText))));
                ParseOffset = NextModifier;
            }
            if(NextTextModifier >= DataSize && NextReferenceDeclaration  >= DataSize)
            {
                break;     
            }
            if(NextReferenceDeclaration < NextTextModifier)
            {
                DocReference NewReference = p_ParseReference(Data,DataSize,ParseOffset,&ParseOffset);
                NewReference.Color = CurrentTextColor;
                NewReference.Modifiers = CurrentTextModifier;
                ReturnValue.push_back(std::unique_ptr<TextElement>(new DocReference(std::move(NewReference))));
            }
            if(NextTextModifier < NextReferenceDeclaration)
            {
                TextState NewState = h_ParseTextState(Data,DataSize,NextTextModifier,&ParseOffset);
                CurrentTextColor = NewState.Color;
                CurrentTextModifier = TextModifier(uint64_t(CurrentTextModifier)^uint64_t(NewState.Modifiers));
            }
        }
        return(ReturnValue); 
    }
    std::unique_ptr<BlockElement> DocumentParsingContext::p_ParseBlockElement(LineRetriever& Retriever)
    {
        std::unique_ptr<BlockElement> ReturnValue;
        std::string TotalParagraphData;
        while(!Retriever.Finished())
        {
            std::string const& CurrentLine = Retriever.PeekLine();
            if(CurrentLine.size() == 0)
            {
                break;
            }
            size_t ParseOffset = 0;
            MBParsing::SkipWhitespace(CurrentLine,0,&ParseOffset);
            if(CurrentLine.size() <= ParseOffset)
            {
                break;   
            }
            if(CurrentLine[ParseOffset] == '#')
            {
                break;   
            }
            if (CurrentLine[ParseOffset] == '%')
            {
                break;
            }
            if(ParseOffset + 1 < CurrentLine.size())
            {
                if(std::memcmp(CurrentLine.data()+ParseOffset,"_#",2) == 0)
                {
                    break;
                }   
                if(std::memcmp(CurrentLine.data()+ParseOffset,"/#",2) == 0)
                {
                    break;   
                }
                if(std::memcmp(CurrentLine.data()+ParseOffset,"/_",2) == 0)
                {
                    break;   
                }
                if(std::memcmp(CurrentLine.data()+ParseOffset,"[[",2) == 0)
                {
                    break;   
                }
                if (std::memcmp(CurrentLine.data() + ParseOffset, "#_", 2) == 0)
                {
                    break;
                }
            }
            TotalParagraphData += CurrentLine+" ";
            Retriever.DiscardLine();
        }
        //XOR
        assert((TotalParagraphData.size() > 0) != (ReturnValue != nullptr));
        if(TotalParagraphData.size() > 0)
        {
            Paragraph* NewBlock = new Paragraph();
            NewBlock->TextElements = p_ParseTextElements(TotalParagraphData.data(),TotalParagraphData.size(),0,nullptr);
            ReturnValue = std::unique_ptr<BlockElement>(NewBlock);
        }
        return(ReturnValue); 
    }
    std::string h_NormalizeName(std::string const& NameToNormalize)
    {
        std::string ReturnValue;
        size_t Start = 0;
        MBParsing::SkipWhitespace(NameToNormalize, 0, &Start);
        //Whitespace, not including linebreaks
        size_t End = NameToNormalize.find_last_not_of(" \t");
        ReturnValue = NameToNormalize.substr(Start, 1+End - Start);
        return(ReturnValue);
    }
    Directive DocumentParsingContext::p_ParseDirective(LineRetriever& Retriever)
    {
        Directive ReturnValue;
        std::string CurrentLine;
        Retriever.GetLine(CurrentLine);
        size_t NameBegin = CurrentLine.find('%');
        ReturnValue.DirectiveName = CurrentLine.substr(NameBegin + 1);
        return(ReturnValue);
    }
    AttributeList DocumentParsingContext::p_ParseAttributeList(LineRetriever& Retriever)
    {
        AttributeList ReturnValue;
        std::string CurrentLine = Retriever.PeekLine();
        Retriever.DiscardLine();
        size_t ParseOffset = 0;
        MBParsing::SkipWhitespace(CurrentLine, 0, &ParseOffset);
        if(ParseOffset + 4 >= CurrentLine.size())
        {
            throw std::runtime_error("Invalid attribute list: must begin with [[ and end with ]]");   
        }
        if(std::memcmp(CurrentLine.data()+ParseOffset,"[[",2) != 0)
        {
            throw std::runtime_error("Invalid attribute list: must begin with [[ and end with ]]");   
        }
        size_t AttributesEnd = CurrentLine.find("]]", ParseOffset + 2);
        if (AttributesEnd == CurrentLine.npos)
        {
            throw std::runtime_error("Invalid attribute list: must begin with [[ and end with ]]");
        }
        std::string AttributesData = CurrentLine.substr(ParseOffset+2,AttributesEnd-ParseOffset-2);
        std::vector<std::string> Flags =MBUtility::Split(AttributesData,",");
        for(std::string const& Flag : Flags)
        {
            ReturnValue.HasAttribute(Flag);   
        }
        return(ReturnValue);   
    }
    FormatElement DocumentParsingContext::p_ParseFormatElement(LineRetriever& Retriever,AttributeList* OutAttributes)
    {
        FormatElement ReturnValue;
        size_t CurrentElementCount = 0;
        bool IsTopElement = false;
        //Determine which kind of format element
        while(!Retriever.Finished())
        {
            std::string const& CurrentLine = Retriever.PeekLine();
            if(CurrentLine.size() == 0)
            {
                Retriever.DiscardLine();
            }   
            else
            {
                size_t FirstNonEmptyCharacter = 0;
                MBParsing::SkipWhitespace(CurrentLine,0,&FirstNonEmptyCharacter);
                if(FirstNonEmptyCharacter == CurrentLine.size())
                {
                    Retriever.DiscardLine();
                    continue;   
                }
                if(CurrentLine[FirstNonEmptyCharacter] == '#' || CurrentLine[FirstNonEmptyCharacter] == '_')
                {
                    //Is a specified format    
                    if(FirstNonEmptyCharacter+1 < CurrentLine.size() && CurrentLine[FirstNonEmptyCharacter+1] == '_')
                    {
                        IsTopElement = true;
                        FirstNonEmptyCharacter+=1; 
                    }
                    if (CurrentLine[FirstNonEmptyCharacter] == '_')
                    {
                        FirstNonEmptyCharacter += 1;
                    }
                    size_t FirstNameCharacter = 0;
                    MBParsing::SkipWhitespace(CurrentLine, FirstNonEmptyCharacter + 1, &FirstNameCharacter);
                    ReturnValue.Name = h_NormalizeName(CurrentLine.substr(FirstNameCharacter));
                    if(ReturnValue.Name.size() > 0 && ReturnValue.Name[0] == '.')
                    {
                        std::string NewName = ReturnValue.Name.substr(1);   
                        ReturnValue.Name = NewName;
                        ReturnValue.Attributes.AddAttribute(NewName);
                    }
                    if(ReturnValue.Name == "")
                    {
                        throw std::runtime_error("Invalid format element name, empty string not allowed");   
                    }
                    ReturnValue.Type = FormatElementType::Section;
                    Retriever.DiscardLine();
                    break;
                }
                else
                {
                    ReturnValue.Type = FormatElementType::Default;   
                    break;
                }
            }
        }
        AttributeList CurrentAttributes;
        while(!Retriever.Finished())
        {
            if(Retriever.PeekLine().size() == 0)
            {
                Retriever.DiscardLine();
                continue;   
            }
            std::string CurrentLine = Retriever.PeekLine();
            size_t ParseOffset = 0;
            MBParsing::SkipWhitespace(CurrentLine,0,&ParseOffset);
            if(CurrentLine.size() <= ParseOffset)
            {
                Retriever.DiscardLine();
                continue;     
            }
            bool ParseChildFormat = false;
            if(CurrentLine[ParseOffset] == '#' && !IsTopElement)
            {
                break;   
            }
            if (CurrentLine[ParseOffset] == '#' && IsTopElement)
            {
                ParseChildFormat = true;
            }
            if(ParseOffset + 1 < CurrentLine.size())
            {
                if(std::memcmp(CurrentLine.data()+ParseOffset,"/#",2) == 0)
                {
                    Retriever.DiscardLine();
                    break;   
                }
                if(std::memcmp(CurrentLine.data()+ParseOffset,"[[",2) == 0)
                {
                    CurrentAttributes = p_ParseAttributeList(Retriever);
                    continue;   
                }
                if(std::memcmp(CurrentLine.data()+ParseOffset,"_#",2) == 0)
                {
                    ParseChildFormat = true;
                }
                if (std::memcmp(CurrentLine.data() + ParseOffset, "#_", 2) == 0)
                {
                    if (IsTopElement)
                    {
                        ParseChildFormat = true;
                    }
                    else 
                    {
                        break;
                    }
                }
                if (std::memcmp(CurrentLine.data() + ParseOffset, "/_", 2) == 0)
                {
                    if (IsTopElement)
                    {
                        Retriever.DiscardLine();
                    }
                    break;
                }
            }
            if (ParseChildFormat)
            {
                AttributeList NewAttributes;
                ReturnValue.NestedFormats.push_back({ p_ParseFormatElement(Retriever,&NewAttributes),CurrentElementCount });
                if (CurrentAttributes.IsEmpty() == false)
                {
                    ReturnValue.NestedFormats.back().first.Attributes = CurrentAttributes;
                    CurrentAttributes.Clear();
                }
                if (NewAttributes.IsEmpty() == false)
                {
                    CurrentAttributes = NewAttributes;
                }
                CurrentElementCount++;
                continue;
            }
            if (CurrentLine[ParseOffset] == '%')
            {
                if(CurrentAttributes.IsEmpty() == false)
                {
                    throw std::runtime_error("Attribute list not allowed for directives");   
                }
                ReturnValue.Directives.push_back({ p_ParseDirective(Retriever),CurrentElementCount });
                CurrentElementCount += 1;
                continue;
            }
            ReturnValue.BlockElements.push_back({p_ParseBlockElement(Retriever),CurrentElementCount});
            if(CurrentAttributes.IsEmpty() == false)
            {
                ReturnValue.BlockElements.back().first->Attributes = CurrentAttributes; 
                CurrentAttributes.Clear();
            }
            CurrentElementCount += 1; 
        }
        *OutAttributes = CurrentAttributes;
        return(ReturnValue);   
    }
    std::vector<FormatElement> DocumentParsingContext::p_ParseFormatElements(LineRetriever& Retriever)
    {
        std::vector<FormatElement> ReturnValue;
        AttributeList CurrentAttributes;
        while(!Retriever.Finished())
        {
            AttributeList NewAttributes;
            ReturnValue.push_back(p_ParseFormatElement(Retriever,&NewAttributes));  
            if(CurrentAttributes.IsEmpty() == false)
            {
                ReturnValue.back().Attributes = CurrentAttributes;
                CurrentAttributes.Clear();   
            }
            if(NewAttributes.IsEmpty() == false)
            {
                CurrentAttributes = std::move(NewAttributes);  
            } 
        } 
        return(ReturnValue);   
    }
    
    DocumentSource DocumentParsingContext::ParseSource(MBUtility::MBOctetInputStream& InputStream)
    {
        DocumentSource ReturnValue;
        LineRetriever Retriever(&InputStream);
        //Preprocessing step that is skipped here
        try
        {
            ReturnValue.Contents = p_ParseFormatElements(Retriever); 
        }  
        catch(std::exception const& e)
        {
            std::cout << e.what() << std::endl;
            ReturnValue = DocumentSource(); 
        }
        return(ReturnValue);
    }
    DocumentSource DocumentParsingContext::ParseSource(std::filesystem::path const& InputFile)
    {
        std::ifstream FileStream(InputFile.c_str());
        MBUtility::MBFileInputStream InputStream(&FileStream);
        return(ParseSource(InputStream));
    }
    DocumentSource DocumentParsingContext::ParseSource(const void* Data,size_t DataSize)
    {
        MBUtility::MBBufferInputStream BufferStream(Data,DataSize);
        return(ParseSource(BufferStream));
    }

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
        std::vector<std::pair<i_CompileType,size_t>> Types = std::vector<std::pair<i_CompileType,size_t>>(
            FormatToCompile.BlockElements.size()+FormatToCompile.Directives.size()+FormatToCompile.NestedFormats.size());
        for (size_t i = 0; i < FormatToCompile.BlockElements.size(); i++)
        {
            Types[FormatToCompile.BlockElements[i].second].first = i_CompileType::Block;
            Types[FormatToCompile.BlockElements[i].second].second = i;
        }
        for (size_t i = 0; i < FormatToCompile.Directives.size(); i++)
        {
            Types[FormatToCompile.Directives[i].second].first = i_CompileType::Directive;
            Types[FormatToCompile.Directives[i].second].second = i;
        }
        for (size_t i = 0; i < FormatToCompile.NestedFormats.size(); i++)
        {
            Types[FormatToCompile.NestedFormats[i].second].first = i_CompileType::Format;
            Types[FormatToCompile.NestedFormats[i].second].second = i;
        }
        for (auto const& CompileTarget : Types)
        {
            if (CompileTarget.first == i_CompileType::Block)
            {
                p_CompileBlock(FormatToCompile.BlockElements[CompileTarget.second].first.get(), ReferenceSolver, OutStream);
            }
            if (CompileTarget.first == i_CompileType::Directive)
            {
                p_CompileDirective(FormatToCompile.Directives[CompileTarget.second].first, ReferenceSolver, OutStream);
            }
            if (CompileTarget.first == i_CompileType::Format)
            {
                p_CompileFormat(FormatToCompile.NestedFormats[CompileTarget.second].first, ReferenceSolver, OutStream,Depth+1);
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
        for (auto const& Element : Format.NestedFormats)
        {
            if (Element.first.Type != FormatElementType::Default)
            {
                ReturnValue.SubHeaders.push_back(p_CreateHeading(Element.first));
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
}
