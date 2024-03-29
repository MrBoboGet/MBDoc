#include "MBDoc.h"
#include "MBLSP/LSP_Structs.h"
#include <MBUtility/MBErrorHandling.h>
#include <MBUtility/MBInterfaces.h>
#include <cstring>
#include <exception>
#include <memory>
#include <MBParsing/MBParsing.h>
#include <stdexcept>
#include <assert.h>
#include <stdint.h>

#include <iostream>
#include <MBUtility/MBStrings.h>
#include <MBUnicode/MBUnicode.h>
#include <MBCLI/MBCLI.h>

#include <MBMime/MBMime.h>

#include <MBLSP/MBLSP.h>
#include <MBSystem/BiDirectionalSubProcess.h>
#include <numeric>


#include <MBUtility/InterfaceAdaptors.h>
#include <MBUtility/Merge.h>

#include <regex>

#include "LSPUtils.h"


namespace MBDoc
{
    std::string h_UnescapeText(const char* Data,size_t DataSize,size_t InParseOffset,size_t TextEnd)
    {
        assert(TextEnd <= DataSize);
        std::string ReturnValue;
        size_t ParseOffset = InParseOffset;
        while(ParseOffset < TextEnd)
        {
            size_t NextEscapePosition = std::find(Data+ParseOffset,Data+TextEnd,'\\')-Data;
            if(NextEscapePosition == TextEnd)
            {
                ReturnValue.insert(ReturnValue.end(),Data+ParseOffset,Data+TextEnd);
                break;
            }
            else
            {
                ReturnValue.insert(ReturnValue.end(),Data+ParseOffset,Data+NextEscapePosition);
                if(NextEscapePosition+1 < TextEnd)
                {
                    ReturnValue += Data[NextEscapePosition+1];
                }
                ParseOffset = NextEscapePosition+2;
            }
        }
        return(ReturnValue);
    }
    std::string h_UnescapeText(const char* Data,size_t DataSize,size_t InParseOffset,size_t TextEnd,char EscapedCharacter)
    {
        assert(TextEnd <= DataSize);
        std::string ReturnValue;
        size_t ParseOffset = InParseOffset;
        while(ParseOffset < TextEnd)
        {
            size_t NextEscapePosition = std::find(Data+ParseOffset,Data+TextEnd,'\\')-Data;
            if(NextEscapePosition == TextEnd)
            {
                ReturnValue.insert(ReturnValue.end(),Data+ParseOffset,Data+TextEnd);
                break;
            }
            else if(NextEscapePosition + 1 >= DataSize || Data[NextEscapePosition+1] != EscapedCharacter)
            {
                ReturnValue.insert(ReturnValue.end(),Data+ParseOffset,Data+NextEscapePosition+1);
                if(NextEscapePosition + 1 < DataSize)
                {
                    ReturnValue += Data[NextEscapePosition+1];   
                }
                ParseOffset = NextEscapePosition+2;
            }
            else
            {
                ReturnValue.insert(ReturnValue.end(),Data+ParseOffset,Data+NextEscapePosition);
                if(NextEscapePosition+1 < TextEnd)
                {
                    ReturnValue += Data[NextEscapePosition+1];
                }
                ParseOffset = NextEscapePosition+2;
            }
        }
        return(ReturnValue);
    }
    bool h_IsEscaped(void const* Data,size_t DataSize,size_t ParseOffset)
    {
        bool ReturnValue = false;
        const char* CharData = (const char*)Data;
        while(ParseOffset > 0)
        {
            if (CharData[ParseOffset - 1] == '\\')
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
    size_t h_FindNextUnescaped(const char* Data,size_t DataSize,size_t InParseOffset,char EscapedCharacter)
    {
        size_t ReturnValue = DataSize;    
        size_t ParseOffset = InParseOffset; 
        while(ParseOffset < DataSize)
        {
            size_t NextCandidate = std::find(Data+ParseOffset,Data+DataSize,EscapedCharacter)-Data;
            if(NextCandidate == DataSize)
            {
                break;
            }
            if(!h_IsEscaped(Data,DataSize,NextCandidate))
            {
                ReturnValue = NextCandidate;     
                break;
            }
            ParseOffset = NextCandidate+1;
        }
        return(ReturnValue);
    }
    //BEGIN LineRetriever
    LineRetriever::LineRetriever(MBUtility::MBOctetInputStream* InputStream) 
        : m_UnderlyingRetriever(InputStream)
    {
           
    }
    void LineRetriever::p_IteratorNextNonComment()
    {
        while(!m_UnderlyingRetriever.Finished())
        {
            std::string const& CurrentLine = m_UnderlyingRetriever.PeekLine();
            size_t FirstNonWS = 0; 
            MBParsing::SkipWhitespace(CurrentLine,0,&FirstNonWS);
            if(FirstNonWS+1 < CurrentLine.size() && (CurrentLine[FirstNonWS] == '/' && CurrentLine[FirstNonWS+1] == '/'))
            {
                m_UnderlyingRetriever.DiscardLine();   
            }
            else
            {
                break;
            }

        }    
    }
    bool LineRetriever::Finished()
    {
        p_IteratorNextNonComment(); 
        return(m_UnderlyingRetriever.Finished());
    }
    int LineRetriever::GetLineIndex() const
    {
        return m_UnderlyingRetriever.GetLineIndex();
    }
    bool LineRetriever::GetLine(std::string& OutLine)
    {
        p_IteratorNextNonComment(); 
        return(m_UnderlyingRetriever.GetLine(OutLine));
    }
    void LineRetriever::DiscardLine()
    {
        m_UnderlyingRetriever.DiscardLine();
    }
    std::string& LineRetriever::PeekLine()
    {
        p_IteratorNextNonComment(); 
        return(m_UnderlyingRetriever.PeekLine());
    }
    //END LineRetriever
    TextColor h_ParseTextColor(const char* DataBegin,const char* DataEnd)
    {
        TextColor ReturnValue;    
        if(DataEnd-DataBegin != 6)
        {
            throw std::runtime_error("Error parsing color: unrecognized color \""+std::string(DataBegin,DataEnd)+"\"");
        }
        bool Result = true;
        char Data[3];
        for(int i = 0; i < 3;i++)
        {
            Data[i] = MBUtility::HexValueToByte(DataBegin[i*2],DataBegin[i*2+1],&Result);
            if(!Result)
            {
                throw std::runtime_error("Error parsing color: unrecognized color \""+std::string(DataBegin,DataEnd)+"\"");
            }
        }
        ReturnValue = TextColor(Data[0],Data[1],Data[2]);
        return(ReturnValue);
    }




    void URLReference::Accept(ReferenceVisitor& Visitor) const
    {
        Visitor.Visit(*this);
    }
    void URLReference::Accept(ReferenceVisitor& Visitor)
    {
        Visitor.Visit(*this);
    }
    void UnresolvedReference::Accept(ReferenceVisitor& Visitor) const
    {
        Visitor.Visit(*this);
    }
    void UnresolvedReference::Accept(ReferenceVisitor& Visitor) 
    {
        Visitor.Visit(*this);
    }
    void FileReference::Accept(ReferenceVisitor& Visitor) const
    {
        Visitor.Visit(*this);
    }
    void FileReference::Accept(ReferenceVisitor& Visitor) 
    {
        Visitor.Visit(*this);
    }
    void ResourceReference::Accept(ReferenceVisitor& Visitor) const
    {
        Visitor.Visit(*this);
    }
    void ResourceReference::Accept(ReferenceVisitor& Visitor) 
    {
        Visitor.Visit(*this);
    }
    //BEGIN TextElement
    void TextElement::Accept(TextVisitor& Visitor) const
    {
        if(IsType<DocReference>())
        {
            Visitor.Visit(GetType<DocReference>());
        } 
        else if(IsType<RegularText>())
        {
            Visitor.Visit(GetType<RegularText>());
        }
    }
    void TextElement::Accept(TextVisitor& Visitor) 
    {
        if(IsType<DocReference>())
        {
            Visitor.Visit(GetType<DocReference>());
        } 
        else if(IsType<RegularText>())
        {
            Visitor.Visit(GetType<RegularText>());
        }
    }
    //END TextElement 
    //
    void BlockElement::Accept(BlockVisitor& Visitor) const
    {
        if(IsType<CodeBlock>())
        {
            Visitor.Visit(static_cast<CodeBlock const&>(*this));
        }       
        else if(IsType<MediaInclude>())
        {
            Visitor.Visit(static_cast<MediaInclude const&>(*this));
        }
        else if(IsType<Paragraph>())
        {
            Visitor.Visit(static_cast<Paragraph const&>(*this));
        }
        else if(IsType<Table>())
        {
            Visitor.Visit(static_cast<Table const&>(*this));
        }
        else if(IsType<List>())
        {
            Visitor.Visit(static_cast<List const&>(*this));
        }
    }
    void BlockElement::Accept(BlockVisitor& Visitor) 
    {
        if(IsType<CodeBlock>())
        {
            Visitor.Visit(static_cast<CodeBlock&>(*this));
        }       
        else if(IsType<MediaInclude>())
        {
            Visitor.Visit(static_cast<MediaInclude&>(*this));
        }
        else if(IsType<Paragraph>())
        {
            Visitor.Visit(static_cast<Paragraph&>(*this));
        }
        else if(IsType<Table>())
        {
            Visitor.Visit(static_cast<Table&>(*this));
        }
        else if(IsType<List>())
        {
            Visitor.Visit(static_cast<List&>(*this));
        }
    }
    //BEGIN AttributeList
    bool AttributeList::IsEmpty() const
    {
        return(m_Attributes.size() == 0);   
    }
    void AttributeList::Clear()
    {
        m_Attributes.clear();       
        assert(IsEmpty() == true);
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
    
    //BEGIN ArgumentList
    std::string const& ArgumentList::operator[](size_t Index) const
    {
        return(m_PositionalArguments[Index]);
    }
    std::string const& ArgumentList::operator[](std::string const& AttributeName) const
    {
        return(m_NamedArguments.at(AttributeName));
    }
    bool ArgumentList::HasArgument(std::string const& AttributeName) const
    {
        return(m_NamedArguments.find(AttributeName) != m_NamedArguments.end());
    }
    size_t ArgumentList::PositionalArgumentsCount() const
    {
        return(m_PositionalArguments.size());
    }
    void ArgumentList::AddArgument(std::pair<std::string, std::string> NewAttribute)
    {
        m_NamedArguments[NewAttribute.first] = std::move(NewAttribute.second);
    }
    void ArgumentList::AddArgument(std::string ArgumentToAdd)
    {
        m_PositionalArguments.push_back(std::move(ArgumentToAdd));
    }
    //END ArgumentList



    //BEGIN FormatElementComponent
    FormatElementComponent::FormatElementComponent(FormatElementComponent&& ElementToSteal) noexcept
    {
        std::swap(ElementToSteal.m_Data, m_Data);
        std::swap(ElementToSteal.m_Type, m_Type);
    }
    FormatElementComponent& FormatElementComponent::operator=(FormatElementComponent FormatToCopy)
    {
        std::swap(m_Type,FormatToCopy.m_Type);    
        std::swap(m_Data,FormatToCopy.m_Data);
        return(*this);
    }
    FormatElementComponent::FormatElementComponent(std::unique_ptr<BlockElement> BlockData)
    {
        m_Type = FormatComponentType::Block;
        m_Data = new std::unique_ptr<BlockElement>(std::move(BlockData));
    }
    FormatElementComponent::FormatElementComponent(Directive DirectiveData)
    {
        m_Type = FormatComponentType::Directive;
        m_Data = new Directive(std::move(DirectiveData));
    }
    FormatElementComponent::FormatElementComponent(FormatElement FormatData)
    {
        m_Type = FormatComponentType::Format;
        m_Data = new FormatElement(std::move(FormatData));
    }

    FormatComponentType FormatElementComponent::GetType() const
    {
        return(m_Type);
    }
    void FormatElementComponent::SetAttributes(AttributeList NewAttributes)
    {
        if(m_Type == FormatComponentType::Block)
        {
            GetBlockData().Attributes = std::move(NewAttributes); 
        } 
        else if(m_Type == FormatComponentType::Format)
        {
            GetFormatData().Attributes = std::move(NewAttributes); 
        }
        else
        {
            throw std::runtime_error("Can't set attributes for directive or empty format type");    
        }

    }
    BlockElement& FormatElementComponent::GetBlockData()
    {
        if (m_Type != FormatComponentType::Block)
        {
            throw std::runtime_error("Format component not of block type");
        }
        return(**(std::unique_ptr<BlockElement>*)m_Data);
    }
    BlockElement const& FormatElementComponent::GetBlockData() const
    {
        if (m_Type != FormatComponentType::Block)
        {
            throw std::runtime_error("Format component not of block type");
        }
        return(**(std::unique_ptr<BlockElement> const*)m_Data);
    }
    Directive& FormatElementComponent::GetDirectiveData()
    {
        if (m_Type != FormatComponentType::Directive)
        {
            throw std::runtime_error("Format component not of directive type");
        }
        return(*(Directive*)m_Data);
    }
    Directive const& FormatElementComponent::GetDirectiveData() const
    {
        if (m_Type != FormatComponentType::Directive)
        {
            throw std::runtime_error("Format component not of directive type");
        }
        return(*(Directive const*)m_Data);
    }
    FormatElement& FormatElementComponent::GetFormatData() 
    {
        if (m_Type != FormatComponentType::Format)
        {
            throw std::runtime_error("Format component not of format type");
        }
        return(*(FormatElement*)m_Data);
    }
    FormatElement const& FormatElementComponent::GetFormatData() const
    {
        if (m_Type != FormatComponentType::Format)
        {
            throw std::runtime_error("Format component not of format type");
        }
        return(*(FormatElement const*)m_Data);
    }
    void FormatElementComponent::Accept(FormatVisitor& Visitor) const
    {
        if(m_Type == FormatComponentType::Block)
        {
            Visitor.Visit(GetBlockData());
        } 
        else if(m_Type == FormatComponentType::Format)
        {
            Visitor.Visit(GetFormatData());
        }
        else if(m_Type == FormatComponentType::Directive)
        {
            Visitor.Visit(GetDirectiveData());
        }
        else 
        {
            assert(false);
        }
    }
    void FormatElementComponent::Accept(FormatVisitor& Visitor) 
    {
        if(m_Type == FormatComponentType::Block)
        {
            Visitor.Visit(GetBlockData());
        } 
        else if(m_Type == FormatComponentType::Format)
        {
            Visitor.Visit(GetFormatData());
        }
        else if(m_Type == FormatComponentType::Directive)
        {
            Visitor.Visit(GetDirectiveData());
        }
        else 
        {
            assert(false);
        }
    }
    FormatElementComponent::~FormatElementComponent()
    {
        if (m_Type == FormatComponentType::Block)
        {
            delete static_cast<std::unique_ptr<BlockElement>*>(m_Data);
        }
        else if (m_Type == FormatComponentType::Directive)
        {
            delete static_cast<Directive*>(m_Data);
        }
        else if (m_Type == FormatComponentType::Format)
        {
            delete static_cast<FormatElement*>(m_Data);
        }
    }
    //END FormatElementComponent
    TextElement DocumentParsingContext::p_ParseReference(int InitialLine,int InitialLineOffset,char const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset)
    {
        TextElement ReturnValue;
        MBLSP::Position Begin;
        Begin.line = InitialLine;
        Begin.character = InitialLineOffset;
        MBLSP::Position RefBegin = Begin;
        std::unique_ptr<DocReference> Reference;
        size_t InitialParseOffset = ParseOffset;
        //ASSUMPTION reference is not escaped 
        if(!(ParseOffset + 1 < DataSize))
        {
            throw std::runtime_error("Invalid reference, empty name");   
        }
        std::string VisibleText;
        std::string ReferenceString;
        const char* CharData = (const char*)Data;
        if(CharData[ParseOffset+1] == '[')
        {
            //size_t TextEnd = std::find(CharData+ParseOffset+2,CharData+DataSize,']')-(CharData);
            size_t TextEnd = h_FindNextUnescaped(CharData,DataSize,ParseOffset+2,']');
            if(TextEnd == DataSize)
            {
                throw std::runtime_error("Unexpected end of visible text in reference");   
            }
            VisibleText = h_UnescapeText(CharData,TextEnd,ParseOffset+2,TextEnd);
            ParseOffset = TextEnd+1;
            MBParsing::SkipWhitespace(Data,DataSize,ParseOffset,&ParseOffset);
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
            RefBegin.character = Begin.character+(ParseOffset+1-InitialParseOffset);
            ReferenceString = std::string(CharData+ParseOffset+1,CharData+ReferenceIDEnd);
            ParseOffset = ReferenceIDEnd + 1;
        }
        else
        {
            if (CharData[ParseOffset + 1] != '(') 
            {
                size_t DelimiterPosition = MBParsing::GetNextDelimiterPosition({ '\t',' ','\n',',','.' }, Data, DataSize, ParseOffset, nullptr);
                RefBegin.character = Begin.character+(ParseOffset+1-InitialParseOffset);
                ReferenceString = std::string(CharData + ParseOffset + 1, CharData + DelimiterPosition);
                ParseOffset = DelimiterPosition;
            }
            else 
            {
                size_t IdentifierEnd = std::find(CharData + ParseOffset + 2, CharData + DataSize,')') - CharData;
                if (IdentifierEnd == DataSize)
                {
                    throw std::runtime_error("Unexpected end of reference identifier, missing )");
                }
                RefBegin.character = Begin.character+(ParseOffset+2-InitialParseOffset);
                ReferenceString = std::string(CharData + ParseOffset + 2, CharData + IdentifierEnd);
                ParseOffset = IdentifierEnd + 1;
            }
        }
        if(ReferenceString.find("://") == ReferenceString.npos)
        {
             UnresolvedReference NewRef;
             NewRef.ReferenceString = ReferenceString;
             NewRef.VisibleText = VisibleText;
             Reference = std::unique_ptr<DocReference>(new UnresolvedReference(std::move(NewRef)));
             //ReturnValue = TextElement(std::unique_ptr<DocReference>(new UnresolvedReference(std::move(NewRef))));
        }
        else
        {
             URLReference NewRef;
             NewRef.VisibleText = VisibleText;
             NewRef.URL = ReferenceString;
             Reference = std::unique_ptr<DocReference>(new URLReference(std::move(NewRef)));
             //ReturnValue = TextElement(std::unique_ptr<DocReference>(new URLReference(std::move(NewRef))));
               
        }
        Reference->Begin = Begin;
        Reference->End.line = InitialLine;
        Reference->End.character = Reference->Begin.character+(ParseOffset-InitialParseOffset);
        Reference->ReferenceBegin = RefBegin;
        ReturnValue = TextElement(std::move(Reference));
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
            //if (CharData[ParseOffset] == '_')
            //{
            //    ReturnValue.Modifiers = ReturnValue.Modifiers | TextModifier::Underlined;
            //    ResultParsed = true;
            //    ParseOffset += 1;
            //}
        }
        if(OutParseOffset != nullptr)
        {
            *OutParseOffset = ParseOffset;   
        }
        return(ReturnValue);   
    }
    TextColor h_ParseColorModifier(const char* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset)
    {
        TextColor ReturnValue;
        size_t NextSpace = std::find(Data+ParseOffset,Data+DataSize,' ')-Data;
        if(NextSpace >= DataSize)
        {
            throw std::runtime_error("Error parsing color modifier: need delimiting space");   
        }
        //reset color, is on the form ~00FFBB @[visible text](Format.mbd) ~ <-- empty to reset coloring
        if(NextSpace-ParseOffset == 0)
        {
            *OutParseOffset = ParseOffset+1;
            return(ReturnValue);
        }
        if(NextSpace-ParseOffset != 6)
        {
            throw  std::runtime_error("Error parsing color modifier: non-reset modifer requires exactly 6 characters to form a valid hex color");
        }
        ReturnValue = h_ParseTextColor(Data+ParseOffset,Data+ParseOffset+6);
        *OutParseOffset = ParseOffset+7;
        return(ReturnValue);
    }
    std::string h_NormalizeText(std::string const& TextToNormalize,bool KeepSpace,bool& OutKeepSpace)
    {
        std::string ReturnValue;
        size_t ParseOffset = 0;
        if(KeepSpace && TextToNormalize.size() > 0 && (TextToNormalize[0] == ' ' || TextToNormalize[0] == '\t' || TextToNormalize[0] == '\r' || TextToNormalize[0] == '\n'))
        {
            ReturnValue += ' '; 
        }
        OutKeepSpace = false;
        while(ParseOffset < TextToNormalize.size())
        {
            MBParsing::SkipWhitespace(TextToNormalize,ParseOffset,&ParseOffset);
            size_t LineEnd = TextToNormalize.find_first_of("\r\n",ParseOffset);
            size_t LastNonWhitespace = TextToNormalize.find_last_not_of("\r\n \t",LineEnd);
            if(LastNonWhitespace == TextToNormalize.npos)
            {
                LastNonWhitespace = ParseOffset;
            }
            else
            {
                LastNonWhitespace += 1;
            }
            if(LastNonWhitespace < ParseOffset)
            {
                break;   
            }
            if(LineEnd == TextToNormalize.npos)
            {
                //kinda hacky, but we allow one space between every element
                ReturnValue.insert(ReturnValue.end(),TextToNormalize.data()+ParseOffset,TextToNormalize.data()+LastNonWhitespace);
                if(LastNonWhitespace < TextToNormalize.size())
                {
                    ReturnValue += ' ';
                    OutKeepSpace = false;
                }
                else
                {
                    OutKeepSpace = true;   
                }
                break;
            }
            else
            {
                ReturnValue.insert(ReturnValue.end(),TextToNormalize.data()+ParseOffset,TextToNormalize.data()+LastNonWhitespace);
                if(TextToNormalize[LineEnd] == '\n') 
                {
                    ParseOffset = LineEnd+1;   
                }
                else if(TextToNormalize[LineEnd] == '\r')
                {
                    ParseOffset = LineEnd+2;
                }
                ReturnValue += ' ';
                OutKeepSpace = false;
            }
        }
        return(ReturnValue);
    }
    std::vector<TextElement> DocumentParsingContext::p_ParseTextElements(int InitialLine,int InitialLineOffset,char const* Data, size_t DataSize, size_t ParseOffset, size_t* OutParseOffset)
    {
        std::vector<TextElement> ReturnValue;
        //ASSUMPTION no # or block delimiter present, guaranteed text element data 
        const char* CharData = (const char*)Data;
        TextColor CurrentTextColor;
        TextModifier CurrentTextModifier = TextModifier(0);
        bool KeepSpace = false;
        int CurrentExtraLines = 0;
        size_t InitialParseOffset = ParseOffset;
        while (ParseOffset < DataSize)
        {
            size_t FindTextModifierDelimiterOffset = ParseOffset;

            size_t NextReferenceDeclaration = DataSize;
            size_t NextTextModifier = DataSize;
            size_t NextColorModifier = DataSize;

            size_t NextModifier = DataSize;
            while (FindTextModifierDelimiterOffset < DataSize)
            {
                NextReferenceDeclaration = std::find(CharData + FindTextModifierDelimiterOffset, CharData + DataSize, '@') - (CharData);
                NextTextModifier = std::find(CharData + FindTextModifierDelimiterOffset, CharData + DataSize, '*') - (CharData);
                NextColorModifier = std::find(CharData + FindTextModifierDelimiterOffset, CharData + DataSize, '~') - (CharData);
                //NextTextModifier = std::min(NextTextModifier, size_t(std::find(CharData + FindTextModifierDelimiterOffset, CharData + DataSize, '_') - (CharData)));

                NextModifier = std::min(NextTextModifier, NextReferenceDeclaration);
                NextModifier = std::min(NextModifier,NextColorModifier);
                if (NextModifier == DataSize)
                {
                    break;
                }
                if (h_IsEscaped(Data, DataSize, NextModifier))
                {
                    FindTextModifierDelimiterOffset = NextModifier + 1;
                }
                else
                {
                    break;
                }
            }

            if (ParseOffset < NextModifier)
            {
                RegularText NewText;
                NewText.Modifiers = CurrentTextModifier;
                NewText.Color = CurrentTextColor;
                NewText.Text = h_NormalizeText(h_UnescapeText(CharData,DataSize,ParseOffset, NextModifier),KeepSpace,KeepSpace);
                //Unescape text

                //TODO fix efficiently, jank
                ReturnValue.push_back(std::move(NewText));
                //increase line count
                CurrentExtraLines += std::count(Data+ParseOffset,Data+NextModifier,'\n');
                ParseOffset = NextModifier;
            }
            if (NextModifier >= DataSize)
            {
                break;
            }
            if (NextReferenceDeclaration == NextModifier)
            {
                //kinda cringe, but easiest way to backport it
                size_t LastNewLinePosition = ParseOffset;
                int NewLineOffset = 0;
                while(LastNewLinePosition > InitialParseOffset)
                {
                    if(Data[LastNewLinePosition] == '\n')
                    {
                        NewLineOffset = (ParseOffset-LastNewLinePosition)-1;
                        break;
                    }
                    LastNewLinePosition--;
                }
                if(LastNewLinePosition == InitialParseOffset)
                {
                    NewLineOffset = InitialLineOffset;
                }
                TextElement NewReference = p_ParseReference( InitialLine + CurrentExtraLines,NewLineOffset,Data, DataSize, ParseOffset, &ParseOffset);
                KeepSpace = true;
                NewReference.GetBase().Color = CurrentTextColor;
                NewReference.GetBase().Modifiers = CurrentTextModifier;
                ReturnValue.push_back(std::move(NewReference));
            }
            else if (NextTextModifier == NextModifier)
            {
                TextState NewState = h_ParseTextState(Data, DataSize, NextTextModifier, &ParseOffset);
                CurrentTextColor = NewState.Color;
                CurrentTextModifier = TextModifier(uint64_t(CurrentTextModifier) ^ uint64_t(NewState.Modifiers));
            }
            else if(NextColorModifier == NextModifier)
            {
                CurrentTextColor = h_ParseColorModifier(CharData,DataSize,NextColorModifier+1,&ParseOffset);
            }
        }
        return(ReturnValue);
    }
    std::unique_ptr<BlockElement> DocumentParsingContext::p_ParseMediaInclude(LineRetriever& Retriever)
    {
        std::unique_ptr<BlockElement> ReturnValue = std::make_unique<MediaInclude>();
        MediaInclude& NewElement = (MediaInclude&)*ReturnValue;
        NewElement.Position.line = Retriever.GetLineIndex();
        std::string IncludeString;
        Retriever.GetLine(IncludeString);
        size_t ParseOffset = 0; 
        MBParsing::SkipWhitespace(IncludeString,0,&ParseOffset);
        if(ParseOffset + 1 >= IncludeString.size())
        {
            throw std::runtime_error("Invalid MediaInclude header: must start with !#"); 
        }
        if(std::memcmp(IncludeString.data()+ParseOffset,"!#",2) != 0)
        {
            throw std::runtime_error("Invalid MediaInclude header: must start with !#"); 
        }
        ParseOffset += 2;
        MBParsing::SkipWhitespace(IncludeString,ParseOffset,&ParseOffset);
        if(ParseOffset >= IncludeString.size() || IncludeString[ParseOffset] != '(')
        {
            throw std::runtime_error("Invalid MediaInclude: Need delimitng ( and ending ) for path");  
        }
        auto EndIt = std::find(IncludeString.begin()+ParseOffset,IncludeString.end(),')');
        if(EndIt == IncludeString.end())
        {
            throw std::runtime_error("Invalid MediaInclude: Need delimitng ( and ending ) for path");  
        }
        NewElement.Position.character = ParseOffset+1;
        NewElement.Length = (EndIt-IncludeString.begin()) -NewElement.Position.character;
        NewElement.MediaPath = std::string(IncludeString.begin()+ParseOffset+1,EndIt); 
        return(ReturnValue);
    }
    std::unique_ptr<BlockElement> DocumentParsingContext::p_ParseCodeBlock(LineRetriever& Retriever)
    {
        std::unique_ptr<BlockElement> ReturnValue = std::make_unique<CodeBlock>();
        CodeBlock& NewCodeBlock = static_cast<CodeBlock&>(*ReturnValue.get());
        NewCodeBlock.LineBegin = Retriever.GetLineIndex();
        std::string CodeBlockHeader;
        Retriever.GetLine(CodeBlockHeader);
        size_t ParseOffset = 0;
        MBParsing::SkipWhitespace(CodeBlockHeader, 0, &ParseOffset);
        if (ParseOffset + 2 >= CodeBlockHeader.size() || std::memcmp(CodeBlockHeader.data()+ParseOffset, "```", 3) != 0)
        {
            throw std::runtime_error("Invalid code block header: must start with ```");
        }
        if (ParseOffset + 4 < CodeBlockHeader.size())
        {
            size_t LastCodeName = CodeBlockHeader.find_last_not_of(" \t");
            if (LastCodeName != CodeBlockHeader.npos)
            {
                NewCodeBlock.CodeType = CodeBlockHeader.substr(ParseOffset + 3, LastCodeName - (ParseOffset + 2));
            }
        }
        //NewCodeBlock.Content = std::string();
        while (!Retriever.Finished())
        {
            std::string NewLine;
            Retriever.GetLine(NewLine);
            MBParsing::SkipWhitespace(NewLine, 0, &ParseOffset);
            if (ParseOffset + 2 < NewLine.size() && std::memcmp(NewLine.data()+ParseOffset, "```", 3) == 0)
            {
                break;
            }
            NewCodeBlock.Content += NewLine;
            NewCodeBlock.Content += '\n';
        }
        //there is always 1 extra newline
        //if (NewCodeBlock.Content.size() > 0)
        //{
        //    NewCodeBlock.Content.resize(NewCodeBlock.Content.size() - 1);
        //}
        NewCodeBlock.LineEnd = Retriever.GetLineIndex()-1;
        return ReturnValue;
    }


    std::unique_ptr<BlockElement> DocumentParsingContext::p_ParseTable(ArgumentList const& Arguments,LineRetriever& Retriever)
    {
        std::unique_ptr<BlockElement> ReturnValue;
        if(Arguments.PositionalArgumentsCount() < 1)
        {
            throw std::runtime_error("Error parsing table: The first positional argument has to be the width of the table");
        }
        int Width = 0;
        try
        {
            Width = std::stoi(Arguments[0]);         
        } 
        catch(std::exception const& e)
        {
            throw std::runtime_error("Error parsing table: Error parsing table width as integer");
        }
        std::vector<std::string> ColumnNames;
        for(int i = 1; i < Arguments.PositionalArgumentsCount();i++)
        {
            ColumnNames.push_back(Arguments[i]);
        }
        if(ColumnNames.size() != 0 && ColumnNames.size() != Width)
        {
            throw std::runtime_error("Error parsing table: All column must be given names as positional arguments if atleast one is specified");
        }
        std::string TotalContent;
        struct TableElementInfo
        {
            int LineIndex = 0;
            int LineCharacterOffset = 0;
            std::string Text;
        };
        std::vector<TableElementInfo> TextElementData;
        std::vector<std::vector<TextElement>> TableElements;
        std::string CurrentLine;

        int CurrentLineIndex = Retriever.GetLineIndex();
        while(Retriever.GetLine(CurrentLine))
        {
            size_t ParseOffset = 0;
            MBParsing::SkipWhitespace(CurrentLine,0,&ParseOffset);
            if(ParseOffset < CurrentLine.size() && CurrentLine.compare(ParseOffset,4,"\\end") == 0)
            {
                break; 
            }
            TotalContent += CurrentLine;
            TotalContent += '\n';
        }
        size_t ParseOffset = 0;
        while(ParseOffset < TotalContent.size())
        {
            size_t NextAmpersand = TotalContent.find('&',ParseOffset);
            while(NextAmpersand != TotalContent.npos)
            {
                if(h_IsEscaped(TotalContent.data(),TotalContent.size(),NextAmpersand))
                {
                    NextAmpersand = TotalContent.find('&',NextAmpersand+1);
                }   
                else
                {
                    break;   
                }
            }
            if(NextAmpersand == TotalContent.npos)
            {
                TableElementInfo& NewInfo = TextElementData.emplace_back();
                NewInfo.LineIndex = CurrentLineIndex;
                NewInfo.Text = TotalContent.substr(ParseOffset);
                size_t LastNewLine = ParseOffset;
                while(LastNewLine > 0)
                {
                    if(TotalContent[LastNewLine] == '\n')
                    {
                        NewInfo.LineCharacterOffset = (ParseOffset-LastNewLine)-1;   
                    }
                    LastNewLine--;
                }

                break;
            }
            else
            {
                TableElementInfo& NewInfo = TextElementData.emplace_back();
                NewInfo.LineIndex = CurrentLineIndex;

                size_t LastNewLine = ParseOffset;
                while(LastNewLine > 0)
                {
                    if(TotalContent[LastNewLine] == '\n')
                    {
                        NewInfo.LineCharacterOffset = (ParseOffset-LastNewLine)-1;   
                    }
                    LastNewLine--;
                }

                NewInfo.Text = h_UnescapeText(TotalContent.data(),TotalContent.size(),ParseOffset,NextAmpersand,'&');
            }
            CurrentLineIndex += std::count(TotalContent.data()+ParseOffset,TotalContent.data()+NextAmpersand,'\n');
            ParseOffset = NextAmpersand+1;
        }
        TableElements.reserve(TextElementData.size());
        size_t TempOffset = 0;
        for(auto const& Element : TextElementData)
        {
            TableElements.push_back(p_ParseTextElements(Element.LineIndex,Element.LineCharacterOffset,Element.Text.data(),Element.Text.size(),0,&TempOffset));
        }
        ReturnValue = std::make_unique<Table>(std::move(ColumnNames),Width,std::move(TableElements));
        return(ReturnValue);
    }
    Paragraph DocumentParsingContext::p_ParseParagraph(int InitialLine,int InitialLineCharacterOffset,std::string const& TotalParagraphData)
    {
        size_t ParseOffset = 0;
        Paragraph ReturnValue = Paragraph(
                p_ParseTextElements(InitialLine,InitialLineCharacterOffset,TotalParagraphData.data(),TotalParagraphData.size(),0,&ParseOffset));
        return(ReturnValue);
    }
    std::unique_ptr<List> DocumentParsingContext::p_ParseList(ArgumentList const& Arguments,LineRetriever& Retriever)
    {
        std::unique_ptr<List> ReturnValue = std::make_unique<List>();
        std::string CurrentLine;
        std::string CurrentParagraphContent;
        bool EndRecieved = false;

        int CurrentLineIndex = 0;
        int CurrentLineOffset = 0;
        while(Retriever.GetLine(CurrentLine))
        {
            size_t ParseOffset = 0;
            MBParsing::SkipWhitespace(CurrentLine,0,&ParseOffset);
            if(ParseOffset < CurrentLine.size() && CurrentLine.compare(ParseOffset,4,"\\end") == 0)
            {
                EndRecieved = true;
                break; 
            }
            //empty line, new paragraph
            if(ParseOffset == CurrentLine.size())
            {
                if(CurrentParagraphContent != "")
                {
                    //create new paragraph
                    ReturnValue->AddParagraph(p_ParseParagraph(CurrentLineIndex,CurrentLineOffset, CurrentParagraphContent));
                }
                CurrentParagraphContent = "";
            }
            else if(ParseOffset < CurrentLine.size() && CurrentLine.compare(ParseOffset,5,"\\list") == 0)
            {
                //create sublist
                List::ListContent SubList;
                if(CurrentParagraphContent != "")
                {
                    SubList.Text = p_ParseParagraph(CurrentLineIndex,CurrentLineOffset,CurrentParagraphContent);
                }
                ArgumentList Args;//empty, no currently used
                SubList.SubList = p_ParseList(Arguments,Retriever);
                ReturnValue->Content.push_back(std::move(SubList));
            }
            else
            {
                //add content to current paragraph
                if(CurrentParagraphContent.size() == 0)
                {
                    CurrentLine = Retriever.GetLineIndex() - 1;   
                    CurrentLineOffset = ParseOffset;
                }
                CurrentParagraphContent += CurrentLine;
                CurrentParagraphContent += "\n";
            }
        }
        if(!EndRecieved)
        {
            throw std::runtime_error("no corresponding \\end to \\list definition");
        }
        if(CurrentParagraphContent != "")
        {
            ReturnValue->AddParagraph(p_ParseParagraph(CurrentLineIndex,CurrentLineOffset,CurrentParagraphContent));
        }
        return(ReturnValue);
    }

    std::unique_ptr<BlockElement> DocumentParsingContext::p_ParseNamedBlockElement(LineRetriever& Retriever)
    {
        std::unique_ptr<BlockElement> ReturnValue;
        std::string HeaderLine;
        Retriever.GetLine(HeaderLine);
        size_t FirstSlash = HeaderLine.find('\\');
        assert(FirstSlash != HeaderLine.npos);
        size_t FirstSpace = HeaderLine.find(' ',FirstSlash+1);
        if(FirstSpace == HeaderLine.npos)
        {
            FirstSpace = HeaderLine.size();
        }
        std::string Name = std::string(HeaderLine.data()+FirstSlash+1,HeaderLine.data()+FirstSpace);
        ArgumentList Arguments = p_ParseArgumentList(HeaderLine.data(),HeaderLine.size(),FirstSpace);
        if(Name == "table")
        {
            ReturnValue = p_ParseTable(Arguments,Retriever); 
        }
        else if(Name == "list")
        {
            ReturnValue = p_ParseList(Arguments,Retriever); 
        }
        else
        {
            throw std::runtime_error("Unkown block element with name \""+Name+"\"");
        }
        return(ReturnValue);
    }
    std::unique_ptr<BlockElement> DocumentParsingContext::p_ParseBlockElement(LineRetriever& Retriever)
    {
        std::unique_ptr<BlockElement> ReturnValue;
        std::string TotalParagraphData;
        int InitialLine = Retriever.GetLineIndex();
        int InitialLineOffset = -1;
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
            if (ParseOffset + 2 < CurrentLine.size())
            {
                if (std::memcmp(CurrentLine.data() + ParseOffset, "```", 3) == 0) 
                {
                    if (TotalParagraphData.size() > 0) 
                    {
                        break;
                    }
                    else 
                    {
                        ReturnValue = p_ParseCodeBlock(Retriever);
                        break;
                    }
                }
            }
            if(ParseOffset + 1 < CurrentLine.size())
            {
                if(std::memcmp(CurrentLine.data()+ParseOffset,"!#",2) == 0)
                {
                    if(TotalParagraphData.size() > 0)
                    {
                        break;
                    }   
                    else
                    {
                        ReturnValue = p_ParseMediaInclude(Retriever);   
                        break;
                    }
                } 
            }
            if(ParseOffset + 1 < CurrentLine.size())
            {
                if( CurrentLine[ParseOffset] == '\\' && !(std::memcmp(CurrentLine.data()+ParseOffset,"\\\\",2) == 0))
                {
                    if(TotalParagraphData.size() > 0)
                    {
                        break;
                    }   
                    else
                    {
                        ReturnValue = p_ParseNamedBlockElement(Retriever);   
                        break;
                    }
                } 
            }
            if(TotalParagraphData.size() == 0)
            {
                InitialLineOffset = ParseOffset;   
            }
            TotalParagraphData += CurrentLine+"\n";
            Retriever.DiscardLine();
        }
        //XOR
        assert((TotalParagraphData.size() > 0) != (ReturnValue != nullptr));
        if(TotalParagraphData.size() > 0)
        {
            std::unique_ptr<Paragraph> NewBlock = std::make_unique<Paragraph>();
            NewBlock->TextElements = p_ParseTextElements(InitialLine,InitialLineOffset,TotalParagraphData.data(),TotalParagraphData.size(),0,nullptr);
            ReturnValue = std::move(NewBlock);
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
    ArgumentList DocumentParsingContext::p_ParseArgumentList(char const* Data, size_t DataSize,size_t InOffset)
    {
        ArgumentList ReturnValue;
        size_t ParseOffset = InOffset;
        char const* CharData = (char const*)Data;
        MBError ParseResult = true;
        MBParsing::SkipWhitespace(Data, DataSize, ParseOffset, &ParseOffset);
        while (ParseOffset < DataSize)
        {
            if (CharData[ParseOffset] == '\"')
            {
                ReturnValue.AddArgument(MBParsing::ParseQuotedString(Data, DataSize, ParseOffset, &ParseOffset,&ParseResult));
                if (!ParseResult)
                {
                    throw std::runtime_error("invalid quoted string in directive argument list: " + ParseResult.ErrorMessage);
                }
            }
            else 
            {
                size_t NamedEnd = MBParsing::GetNextDelimiterPosition({ ' ','\t' }, Data, DataSize,ParseOffset);
                std::string Name = std::string(CharData + ParseOffset, CharData + NamedEnd);
                MBParsing::SkipWhitespace(Data,DataSize, NamedEnd, &ParseOffset);
                if (ParseOffset < DataSize && CharData[ParseOffset] == '=')
                {
                    std::string Value;
                    MBParsing::SkipWhitespace(Data, DataSize, ParseOffset+1, &ParseOffset);
                    if (ParseOffset >= DataSize)
                    {
                        throw std::runtime_error("Invalid named argument in argument list: no argument value provided");
                    }
                    if (CharData[ParseOffset] == '"')
                    {
                        Value = MBParsing::ParseQuotedString(Data,DataSize, ParseOffset, &ParseOffset,&ParseResult);
                        if (!ParseResult) 
                        {
                            throw std::runtime_error("Invalid string in named argument value: " + ParseResult.ErrorMessage);
                        }
                    }
                    else
                    {
                        size_t ValueEnd = MBParsing::GetNextDelimiterPosition({ ' ','\t' }, Data,DataSize, ParseOffset);
                        Value = std::string(CharData + ParseOffset, ValueEnd - ParseOffset);
                        ParseOffset = ValueEnd;
                    }
                    ReturnValue.AddArgument({ std::move(Name),std::move(Value) });
                }
                else
                {
                    ReturnValue.AddArgument(std::move(Name));
                }
            }
            MBParsing::SkipWhitespace(Data, DataSize, ParseOffset, &ParseOffset);
        }

        return(ReturnValue);
    }
    Directive DocumentParsingContext::p_ParseDirective(LineRetriever& Retriever)
    {
        Directive ReturnValue;
        std::string CurrentLine;
        Retriever.GetLine(CurrentLine);
        size_t NameBegin = CurrentLine.find('%');
        size_t NameEnd = MBParsing::GetNextDelimiterPosition({ ' ','\t' },CurrentLine,NameBegin+1);
        ReturnValue.DirectiveName = CurrentLine.substr(NameBegin + 1,NameEnd-NameBegin-1);
        ReturnValue.Arguments = p_ParseArgumentList(CurrentLine.data(), CurrentLine.size(), NameEnd);
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
            ReturnValue.AddAttribute(Flag);   
        }
        return(ReturnValue);   
    }
    enum class i_FormatType
    {
        Null,
        Regular,
        SubFormat,
        TopSubFormat      
    };
    FormatElementComponent DocumentParsingContext::p_ParseFormatElement(LineRetriever& Retriever,AttributeList* OutAttributes)
    {
        FormatElement TopFormat;
        FormatElementComponent ReturnValue;
        size_t CurrentElementCount = 0;
        i_FormatType Type = i_FormatType::Null;
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
                if(FirstNonEmptyCharacter+1 < CurrentLine.size() && 
                    (std::memcmp(&CurrentLine[FirstNonEmptyCharacter],"/_",2) == 0 || 
                     std::memcmp(&CurrentLine[FirstNonEmptyCharacter],"/#",2) == 0))
                {
                    if(std::memcmp(&CurrentLine[FirstNonEmptyCharacter],"/_",2) == 0)
                    {
                        throw std::runtime_error("Missmatched #_ /_ pair");
                    }
                    if(std::memcmp(&CurrentLine[FirstNonEmptyCharacter],"/#",2) == 0)
                    {
                        throw std::runtime_error("Missmatched _# /# pair");
                    }
                }
                if(CurrentLine[FirstNonEmptyCharacter] == '#' || CurrentLine[FirstNonEmptyCharacter] == '_')
                {
                    //Is a specified format    
                    Type = i_FormatType::Regular;
                    if(CurrentLine[FirstNonEmptyCharacter] == '_')
                    {
                        Type = i_FormatType::SubFormat; 
                    }
                    else if(FirstNonEmptyCharacter+1 < CurrentLine.size() && CurrentLine[FirstNonEmptyCharacter+1] == '_')
                    {
                        Type = i_FormatType::TopSubFormat;
                        FirstNonEmptyCharacter+=1; 
                    }
                    if (CurrentLine[FirstNonEmptyCharacter] == '_')
                    {
                        FirstNonEmptyCharacter += 1;
                    }
                    size_t FirstNameCharacter = 0;
                    MBParsing::SkipWhitespace(CurrentLine, FirstNonEmptyCharacter + 1, &FirstNameCharacter);
                    TopFormat.DefinitionRange.start.line = Retriever.GetLineIndex();
                    TopFormat.DefinitionRange.start.character = FirstNameCharacter;
                    TopFormat.DefinitionRange.end.line = Retriever.GetLineIndex();
                    TopFormat.DefinitionRange.end.character = CurrentLine.size();
                    TopFormat.Name = h_NormalizeName(CurrentLine.substr(FirstNameCharacter));
                    if(TopFormat.Name.size() > 0 && TopFormat.Name[0] == '.')
                    {
                        std::string NewName = TopFormat.Name.substr(1);   
                        TopFormat.Name = NewName;
                        TopFormat.Attributes.AddAttribute(NewName);
                    }
                    if(TopFormat.Name == "")
                    {
                        throw std::runtime_error("Invalid format element name, empty string not allowed");   
                    }
                    Retriever.DiscardLine();
                    break;
                }
                else
                {
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
            //Kinda ugly, this whole function deserve a complete overhaul
            bool ParseChildFormat = false;
            if(CurrentLine[ParseOffset] == '#' && Type == i_FormatType::Regular)
            {
                break;   
            }
            if (CurrentLine[ParseOffset] == '#')
            {
                ParseChildFormat = true;
            }
            if(ParseOffset + 1 < CurrentLine.size())
            {
                if(std::memcmp(CurrentLine.data()+ParseOffset,"/#",2) == 0)
                {
                    if(Type == i_FormatType::Regular)
                    {
                        break;
                    }
                    if(Type != i_FormatType::SubFormat)
                    {
                        throw std::runtime_error("Missmatched _# /# pair");    
                    }
                    Retriever.DiscardLine();
                    break;   
                }
                if (std::memcmp(CurrentLine.data() + ParseOffset, "/_", 2) == 0)
                {
                    if(Type == i_FormatType::Regular)
                    {
                        break;
                    }
                    if (Type != i_FormatType::TopSubFormat)
                    {
                        throw std::runtime_error("Missmatched #_ /_ pair");    
                    }
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
                    //#_ break for regular # sections
                    if(Type == i_FormatType::Regular)
                    {
                        break;   
                    }
                    ParseChildFormat = true;
                }
            }
            if (ParseChildFormat)
            {
                AttributeList NewAttributes;
                FormatElementComponent NewFormat = p_ParseFormatElement(Retriever,&NewAttributes);
                if (CurrentAttributes.IsEmpty() == false)
                {
                    NewFormat.SetAttributes(CurrentAttributes);
                    CurrentAttributes.Clear();
                }
                TopFormat.Contents.push_back(std::move(NewFormat));
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
                if (Type != i_FormatType::Null)
                {
                    TopFormat.Contents.push_back(FormatElementComponent(p_ParseDirective(Retriever)));
                    CurrentElementCount += 1;
                    continue;
                }
                else 
                {
                    ReturnValue = FormatElementComponent(p_ParseDirective(Retriever));
                    break;
                }
            }
            std::unique_ptr<BlockElement> NewBlockElement = p_ParseBlockElement(Retriever);
            if(CurrentAttributes.IsEmpty() == false)
            {
                NewBlockElement->Attributes = CurrentAttributes; 
                CurrentAttributes.Clear();
            }
            if(Type != i_FormatType::Null)
            {
                TopFormat.Contents.push_back(FormatElementComponent(std::move(NewBlockElement)));
                CurrentElementCount += 1; 
            }
            else
            {
                ReturnValue = FormatElementComponent(std::move(NewBlockElement));
                break;
            }
        }
        *OutAttributes = CurrentAttributes;

        if(Type != i_FormatType::Null)
        {
            return(TopFormat);    
        }

        else
        {
            return(ReturnValue);    
        }
        return(ReturnValue);   
    }
    std::vector<FormatElementComponent> DocumentParsingContext::p_ParseFormatElements(LineRetriever& Retriever)
    {
        std::vector<FormatElementComponent> ReturnValue;
        AttributeList CurrentAttributes;
        while(!Retriever.Finished())
        {
            AttributeList NewAttributes;
            ReturnValue.push_back(p_ParseFormatElement(Retriever,&NewAttributes));
            //ASSUMPTION the only situation where no rule matches is when the remaining data is whitespace
            if (ReturnValue.back().GetType() == FormatComponentType::Null)
            {
                ReturnValue.pop_back();
                break;
            }
            if(CurrentAttributes.IsEmpty() == false)
            {
                ReturnValue.back().SetAttributes(CurrentAttributes);
                CurrentAttributes.Clear();   
            }
            if(NewAttributes.IsEmpty() == false)
            {
                CurrentAttributes = std::move(NewAttributes);  
            } 
        } 
        return(ReturnValue);   
    }
    void DocumentParsingContext::ReferenceExtractor::EnterFormat(FormatElement const& Format)
    {
        Targets.insert(Format.Name); 
    }
    void DocumentParsingContext::ReferenceExtractor::Visit(UnresolvedReference const& Ref)
    {
        References.insert(Ref.ReferenceString); 
    }

    
    //BEGIN ReferenceTargetsResolver 
    ReferenceTargetsResolver::LabelNode ReferenceTargetsResolver::p_ExtractLabelNode(FormatElement const& ElementToConvert)
    {
        ReferenceTargetsResolver::LabelNode ReturnValue;
        ReturnValue.Name = ElementToConvert.Name;
        ReturnValue.DefinitionRange = ElementToConvert.DefinitionRange;
        for(auto const& Child : ElementToConvert.Contents)
        {
            if(Child.GetType() == FormatComponentType::Format)
            {
                ReturnValue.Children.push_back(p_ExtractLabelNode(Child.GetFormatData()));
            }   
        }
        std::sort(ReturnValue.Children.begin(),ReturnValue.Children.end());
        return(ReturnValue);
    }
    std::vector<ReferenceTargetsResolver::LabelNode> ReferenceTargetsResolver::p_GetDocumentLabelNodes(std::vector<FormatElementComponent>  const& SourceToConvert)
    {
        std::vector<ReferenceTargetsResolver::LabelNode> ReturnValue;
        for(auto const& Element : SourceToConvert)
        {
            if(Element.GetType() == FormatComponentType::Format)
            {
                ReturnValue.push_back(p_ExtractLabelNode(Element.GetFormatData()));
            }
        }
        return(ReturnValue);
    }
    ReferenceTargetsResolver::FlatLabelNode ReferenceTargetsResolver::p_UpdateFlatNodeList(std::vector<ReferenceTargetsResolver::FlatLabelNode>& OutNodes,ReferenceTargetsResolver::LabelNode const& NodeToFlatten)
    {
        FlatLabelNode ReturnValue;
        ReturnValue.Name = NodeToFlatten.Name;
        ReturnValue.DefinitionRange = NodeToFlatten.DefinitionRange;
        ReturnValue.ChildrenBegin = OutNodes.size();
        ReturnValue.ChildrenEnd = ReturnValue.ChildrenBegin+NodeToFlatten.Children.size();
        OutNodes.resize(OutNodes.size()+NodeToFlatten.Children.size());
        LabelNodeIndex CurrentChildIndex = ReturnValue.ChildrenBegin;
        for(auto const& Child : NodeToFlatten.Children)
        {
            FlatLabelNode NewNode = p_UpdateFlatNodeList(OutNodes,Child);
            OutNodes[CurrentChildIndex] = NewNode;
            CurrentChildIndex++;
        }
        return(ReturnValue);
    }
    std::vector<ReferenceTargetsResolver::FlatLabelNode> ReferenceTargetsResolver::p_GetFlatLabelNodes(std::vector<LabelNode> NodeToConvert)
    {
        std::vector<ReferenceTargetsResolver::FlatLabelNode> ReturnValue;
        std::sort(NodeToConvert.begin(),NodeToConvert.end(),[](LabelNode const& lhs, LabelNode const& rhs) -> bool {return(lhs.Name < rhs.Name);});
        LabelNode TopNode;
        TopNode.Children = std::move(NodeToConvert);
        ReturnValue.resize(1);
        FlatLabelNode FlatTopNode = p_UpdateFlatNodeList(ReturnValue, TopNode);;
        ReturnValue[0] = std::move(FlatTopNode);
        return(ReturnValue);
    }
    ReferenceTargetsResolver::FlatLabelNode const*  ReferenceTargetsResolver::p_ContainsLabel(ReferenceTargetsResolver::LabelNodeIndex NodeIndex,std::vector<std::string> const& Labels,int LabelIndex) const
    {
        FlatLabelNode const*  ReturnValue = nullptr;       
        if(m_Nodes.size() == 0)
        {
            return ReturnValue;   
        }
        assert(NodeIndex < m_Nodes.size());
        FlatLabelNode const& NodeToInspect = m_Nodes[NodeIndex];
        std::string const& NameToSearch = Labels[LabelIndex];
        LabelNodeIndex FirstMatching = std::lower_bound(m_Nodes.data()+NodeToInspect.ChildrenBegin,m_Nodes.data()+NodeToInspect.ChildrenEnd,NameToSearch,
                [](FlatLabelNode const& lhs,std::string const& rhs) -> bool {return(lhs.Name < rhs);})-m_Nodes.data();
        bool MatchSucceeded = true;
        if(FirstMatching == NodeToInspect.ChildrenEnd || m_Nodes[FirstMatching].Name != NameToSearch)
        {
            MatchSucceeded = false;
        }
        assert(Labels.size() != 0);
        if(LabelIndex == Labels.size() -1 && MatchSucceeded)
        {
            return &m_Nodes[FirstMatching];
        }
        else
        {
            if(!MatchSucceeded)
            {
                //search all children with current index offset
                for(int i = NodeToInspect.ChildrenBegin; i < NodeToInspect.ChildrenEnd;i++)
                {
                    auto ChildResult = p_ContainsLabel(i,Labels,LabelIndex);
                    if(ChildResult)
                    {
                        return ChildResult;   
                    }
                }
            }
            else
            {
                //multiple children can have the same name
                while(FirstMatching < NodeToInspect.ChildrenEnd && m_Nodes[FirstMatching].Name == NameToSearch)
                {
                    auto HasSubmatch = p_ContainsLabel(FirstMatching,Labels,LabelIndex+1);
                    if(HasSubmatch)
                    {
                        return HasSubmatch;
                    }
                    FirstMatching++;
                }
            }
        }
        return ReturnValue;
    }
    std::vector<std::string> ReferenceTargetsResolver::GetAllLabelNames() const
    {
        std::vector<std::string> ReturnValue;
        if(m_Nodes.size() == 0)
        {
            return ReturnValue;
        }
        std::unordered_set<std::string> VisitedNames;
        for(size_t i = 1; i < m_Nodes.size();i++)
        {
            if(VisitedNames.find(m_Nodes[i].Name) == VisitedNames.end())
            {
                VisitedNames.insert(m_Nodes[i].Name);
                ReturnValue.push_back(m_Nodes[i].Name);
            }
        }
        return ReturnValue;
    }
    MBLSP::Range ReferenceTargetsResolver::GetLabelRange(std::vector<std::string> const& Labels) const
    {
        MBLSP::Range ReturnValue;
        auto LabelPointer = p_ContainsLabel(0,Labels,0);
        if(LabelPointer != nullptr)
        {
            ReturnValue = LabelPointer->DefinitionRange;
        }
        else
        {
            throw std::runtime_error("Label not found");
        }
        return ReturnValue;
    }
    bool ReferenceTargetsResolver::ContainsLabels(std::vector<std::string> const& Labels) const
    {
        if(Labels.size() == 0 || m_Nodes.size() == 0)
        {
            return(false);   
        }
        return(p_ContainsLabel(0,Labels,0) != nullptr);  
    }
    ReferenceTargetsResolver::ReferenceTargetsResolver()
    {
           
    }
    ReferenceTargetsResolver::ReferenceTargetsResolver(std::vector<FormatElementComponent> const& FormatsToInspect)
    {
        m_Nodes = p_GetFlatLabelNodes(p_GetDocumentLabelNodes(FormatsToInspect));
    }

    MBParsing::JSONObject ReferenceTargetsResolver::p_ToJSON(std::vector<ReferenceTargetsResolver::FlatLabelNode> const& Nodes, size_t Index)
    {
        MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
        FlatLabelNode const& CurrentNode = Nodes[Index];
        ReturnValue["Name"] = CurrentNode.Name;
        std::vector<MBParsing::JSONObject> Children;
        Children.reserve(CurrentNode.ChildrenEnd - CurrentNode.ChildrenBegin);
        for(auto i = CurrentNode.ChildrenBegin; i < CurrentNode.ChildrenEnd;i++)
        {
            Children.emplace_back(p_ToJSON(Nodes,i));
        }
        ReturnValue["Children"] = std::move(Children);
        return ReturnValue;
    }
    MBParsing::JSONObject ReferenceTargetsResolver::ToJSON(ReferenceTargetsResolver const& ObjectToConvert)
    {
        if (ObjectToConvert.m_Nodes.size() == 0)
        {
            return std::vector<MBParsing::JSONObject>();
        }
        return p_ToJSON(ObjectToConvert.m_Nodes,0)["Children"];
    }
    ReferenceTargetsResolver::LabelNode ReferenceTargetsResolver::p_FromJSON(MBParsing::JSONObject const& ObjectToParse)
    {
        LabelNode CurrentNode;
        CurrentNode.Name = ObjectToParse["Name"].GetStringData();
        for(auto const& Child : ObjectToParse["Children"].GetArrayData())
        {
            CurrentNode.Children.push_back(p_FromJSON(Child));
        }
        return CurrentNode;
    }
    void ReferenceTargetsResolver::FromJSON(ReferenceTargetsResolver& Result,MBParsing::JSONObject const& ObjectToParse)
    {
        std::vector<LabelNode> Labels;
        for(auto const& Child : ObjectToParse.GetArrayData())
        {
            Labels.emplace_back(p_FromJSON(Child));
        }
        Result.m_Nodes = p_GetFlatLabelNodes(Labels);
    }
    //END ReferenceTargetsResolver 


    void DocumentParsingContext::p_UpdateReferences(DocumentSource& SourceToModify)
    {
        DocumentTraverser Traverser;
        ReferenceExtractor Extractor;
        Traverser.Traverse(SourceToModify,Extractor);
        //multiple traversal Inefficient, but it is what it is 
        //SourceToModify.References = std::move(Extractor.References);
        SourceToModify.ReferenceTargets = ReferenceTargetsResolver(SourceToModify.Contents);
        auto VisitFunc = [&](Directive const& Directive)
        {
            if(Directive.DirectiveName == "title" && Directive.Arguments.PositionalArgumentsCount() > 0)
            {
                if(SourceToModify.Title == "")
                {
                    SourceToModify.Title = Directive.Arguments[0];
                }
            }
        };
        auto Visitor = LambdaVisitor(&VisitFunc);
        Traverser.Traverse(SourceToModify,Visitor);
    }
    DocumentSource DocumentParsingContext::ParseSource(MBUtility::MBOctetInputStream& InputStream,std::string FileName,MBError& OutError)
    {
        DocumentSource ReturnValue;
        ReturnValue.Name = std::move(FileName);
        LineRetriever Retriever(&InputStream);
        //Preprocessing step that is skipped here
        try
        {
            ReturnValue.Contents = p_ParseFormatElements(Retriever); 
            p_UpdateReferences(ReturnValue);
        }  
        catch(std::exception const& e)
        {
            ReturnValue = DocumentSource(); 
            OutError = false;
            OutError.ErrorMessage = "Error parsing document: "+std::string(e.what());
        }
        return(ReturnValue);
    }
    DocumentSource DocumentParsingContext::ParseSource(std::filesystem::path const& InputFile,MBError& OutError)
    {
        std::ifstream FileStream(InputFile.c_str());
        if (FileStream.is_open() == false)
        {
            OutError = false;
            OutError.ErrorMessage = "Can't open file";
            return(DocumentSource());
        }
        MBUtility::MBFileInputStream InputStream(&FileStream);
        DocumentSource ReturnValue = ParseSource(InputStream, MBUnicode::PathToUTF8(InputFile.filename()), OutError);
        ReturnValue.Path = InputFile;
        ReturnValue.Timestamp = uint64_t( std::filesystem::directory_entry(ReturnValue.Path).last_write_time().time_since_epoch().count());
        return(ReturnValue);
    }
    DocumentSource DocumentParsingContext::ParseSource(const char* Data,size_t DataSize,std::string FileName,MBError& OutError)
    {
        MBUtility::MBBufferInputStream BufferStream(Data,DataSize);
        return(ParseSource(BufferStream,std::move(FileName),OutError));
    }
    //END DocumentSource

    //BEGIN DocumentBuild
    
    void DocumentBuild::p_CreateBuildFromDirectory(DocumentBuild& OutBuild,std::filesystem::path const& DirPath)
    {
        std::filesystem::directory_iterator DirIterator(DirPath);
        for(auto const& Entry : DirIterator)
        {
            if(Entry.is_regular_file())
            {
                if(Entry.path().extension() == ".mbd")
                {
                    OutBuild.DirectoryFiles.push_back(Entry.path());
                }
            }
            else if(Entry.is_directory())
            {
                DocumentBuild NewBuild;
                p_CreateBuildFromDirectory(NewBuild,Entry.path());
                OutBuild.SubDirectories.push_back(std::make_pair(MBUnicode::PathToUTF8(Entry.path().filename()),std::move(NewBuild)));
            }
        }
        std::sort(OutBuild.DirectoryFiles.begin(),OutBuild.DirectoryFiles.end());
        std::sort(OutBuild.SubDirectories.begin(),OutBuild.SubDirectories.end(),[](auto const& Lhs,auto const& Rhs)
                {
                    return(Lhs.first < Rhs.first);  
                });
    }
    void DocumentBuild::p_ParseDocumentBuildDirectory(DocumentBuild& OutBuild,MBParsing::JSONObject const& DirectoryObject,std::filesystem::path const& BuildDirectory,MBError& OutError)
    {
        //the inclusion of this option is mutually  exclusive with the others
        if(DirectoryObject.HasAttribute("MountDir"))
        {
            if(DirectoryObject.HasAttribute("Files") || DirectoryObject.HasAttribute("SubDirectories"))
            {
                throw std::runtime_error("The inclusion of 'MountDir' is mutally exlusive with both 'Files' and 'SubDirectories'");
            }
            MBParsing::JSONObject const& MountDir = DirectoryObject["MountDir"];
            if(MountDir.GetType() != MBParsing::JSONObjectType::String)
            {
                throw std::runtime_error("'MountDir' Attribute has to be a string");
            }
            std::filesystem::path PathToInspect = BuildDirectory/MountDir.GetStringData();
            if(!std::filesystem::is_directory(PathToInspect))
            {
                throw std::runtime_error("The directory "+MBUnicode::PathToUTF8(PathToInspect)+" isn't a directory, which is required by 'MountDir'");
            }
            p_CreateBuildFromDirectory(OutBuild,PathToInspect);
            return;
        }
        if(DirectoryObject.HasAttribute("Files"))
        {
            for(MBParsing::JSONObject const& Object : DirectoryObject["Files"].GetArrayData())
            {
                std::string CurrentString = Object.GetStringData();
                if (CurrentString.size() == 0)
                {
                    OutError = false;
                    OutError.ErrorMessage = "Empty source name not allowed";
                    return;
                }
                if (CurrentString[0] == '/')
                {
                    OutError = false;
                    OutError.ErrorMessage = "Invalid source name \"" + CurrentString + "\": Only relative paths allowed";
                    return;
                }
                OutBuild.DirectoryFiles.push_back(BuildDirectory/CurrentString);
                if (!OutError)
                {
                    return;
                }
            }
        }
        if(DirectoryObject.HasAttribute("SubDirectories"))
        {
            MBParsing::JSONObject const& SubDirectories = DirectoryObject["SubDirectories"];
            for(auto const& SubDirSpecifier : SubDirectories.GetMapData())
            {
                MBParsing::JSONObject const& SubDirData = SubDirSpecifier.second;
                DocumentBuild NewSubDir;
                if(SubDirData.GetType() == MBParsing::JSONObjectType::String)
                {
                    NewSubDir = ParseDocumentBuild(BuildDirectory/SubDirData.GetStringData(),OutError);
                    if(!OutError)
                    {
                        return;   
                    }
                }
                else
                {
                    p_ParseDocumentBuildDirectory(NewSubDir,SubDirData,BuildDirectory,OutError);  
                }
                if(!OutError)
                {
                    return;   
                }
                OutBuild.SubDirectories.push_back(std::pair<std::string,DocumentBuild>(SubDirSpecifier.first,std::move(NewSubDir)));
            }
        }
    }
    size_t DocumentBuild::GetTotalFiles() const
    {
        size_t ReturnValue = 0;    
        ReturnValue += DirectoryFiles.size();
        for(auto const& SubDir : SubDirectories)
        {
            ReturnValue += SubDir.second.GetTotalFiles(); 
        }
        return(ReturnValue);
    }
    DocumentBuild DocumentBuild::ParseDocumentBuild(MBUtility::MBOctetInputStream& InputStream,std::filesystem::path const& BuildDirectory,MBError& OutError)
    {
        DocumentBuild ReturnValue;
        std::string TotalFileData;
        while(true)
        {
            constexpr size_t ReadChunkSize = 4096;
            uint8_t ReadBuffer[ReadChunkSize];
            size_t ReadBytes = InputStream.Read(ReadBuffer,ReadChunkSize);
            TotalFileData.insert(TotalFileData.end(),ReadBuffer,ReadBuffer+ReadBytes);
            if(ReadBytes < ReadChunkSize)
            {
                break;
            }
        }
        MBParsing::JSONObject JSONData = MBParsing::ParseJSONObject(TotalFileData,0,nullptr,&OutError);
        if(!OutError)
        {
            return(ReturnValue);
        }
        try
        {
            if(JSONData.HasAttribute("Name") && JSONData["Name"].GetType() == MBParsing::JSONObjectType::String)
            {
                ReturnValue.Name = JSONData["Name"].GetStringData();
            }
            MBParsing::JSONObject const& TopDirectory = JSONData["TopDirectory"];
            p_ParseDocumentBuildDirectory(ReturnValue,TopDirectory,BuildDirectory,OutError);
        }
        catch(std::exception const& e)
        {
            OutError = false;
            OutError.ErrorMessage = "Error parsing MBDocBuild: "+std::string(e.what());
        }
        return(ReturnValue); 
    }

    DocumentBuild DocumentBuild::ParseDocumentBuild(std::filesystem::path const& FilePath,MBError& OutError)
    {
        DocumentBuild ReturnValue;
        if(!std::filesystem::exists(FilePath) || !std::filesystem::is_regular_file(FilePath))
        {
            OutError = false;
            OutError.ErrorMessage = "Filepath doesn't exist or isn't a file";
        }
        else
        {
            MBUtility::MBFileInputStream FileInput(std::ifstream(FilePath.c_str()));
            ReturnValue = ParseDocumentBuild(FileInput,FilePath.parent_path(),OutError);
        }
        return(ReturnValue);
    }
    //END DocumentBuild
    
    //BEGIN DocumentPath
    DocumentPath DocumentPath::GetRelativePath(DocumentPath const& TargetPath,DocumentPath const& CurrentPath)
    {
        DocumentPath ReturnValue;
        if (TargetPath[0] != "/" || CurrentPath[0] != "/")
        {

        }
        size_t CommonDirectoryPrefix = 0;
        for (size_t i = 0; i < std::min(TargetPath.ComponentCount(), CurrentPath.ComponentCount());i++)
        {
            if (TargetPath[i] == CurrentPath[i])
            {
                CommonDirectoryPrefix += 1;
            }
            else
            {
                break;
            }
        }
        //Assumes that the last part if CurrentPath is a file name
        for (int i = 0; i < int(CurrentPath.ComponentCount()) - 1 - int(CommonDirectoryPrefix); i++)
        {
            ReturnValue.AddDirectory("..");
        }
        for (int i = CommonDirectoryPrefix; i < TargetPath.ComponentCount(); i++)
        {
            ReturnValue.AddDirectory(TargetPath[i]);
        }
        ReturnValue.SetPartIdentifier(TargetPath.GetPartIdentifier());
        return(ReturnValue);        
    }
    bool DocumentPath::IsSubPath(DocumentPath const& PathToCompare) const
    {
        bool ReturnValue = true;

        return(ReturnValue); 
    }
    std::string DocumentPath::GetString() const
    {
        std::string ReturnValue;
        for (std::string const& Name : m_PathComponents)
        {
            ReturnValue += Name;
            if (Name != "/")
            {
                ReturnValue += "/";
            }
        }
        if (ReturnValue.size() > 0 && ReturnValue.size() != 1)
        {
            ReturnValue.resize(ReturnValue.size() - 1);
        }
        return(ReturnValue); 
    }
    size_t DocumentPath::ComponentCount() const
    {
        return(m_PathComponents.size());
    }

    std::string DocumentPath::operator[](size_t ComponentIndex) const
    {
        return(m_PathComponents[ComponentIndex]); 
    }
    bool DocumentPath::operator==(DocumentPath const& PathToCompare) const
    {
        return(m_PartIdentifier == PathToCompare.m_PartIdentifier && m_PathComponents == PathToCompare.m_PathComponents);
    }
    bool DocumentPath::operator!=(DocumentPath const& PathToCompare) const
    {
        return(!(*this == PathToCompare));
    }
    bool DocumentPath::operator<(DocumentPath const& OtherPath) const
    {
        bool ReturnValue = false;
        bool IsEqual = true;
        if (m_PathComponents.size() == 1 && OtherPath.ComponentCount() > 1) 
        {
            return(true);
        }
        if (m_PathComponents.size() > 1 && OtherPath.ComponentCount() == 1) 
        {
            return(false);
        }
        //all components are equal only in the case where the s
        for(size_t i = 0; i < std::min(m_PathComponents.size(),OtherPath.m_PathComponents.size());i++)
        {
            if(m_PathComponents[i] < OtherPath.m_PathComponents[i])
            {
                ReturnValue = true;
                IsEqual = false;
                break;  
            }
            else
            {
                ReturnValue = false;
                IsEqual = false;
                break;  
            } 
        }
        if (IsEqual)
        {
            return(ComponentCount() < OtherPath.ComponentCount());
        }
        return(ReturnValue);
    }
    DocumentPath DocumentPath::ParsePath(std::string const& PathToParse,MBError& OutError)
    {
        DocumentPath ReturnValue;
        std::vector<std::string> Components = MBUtility::Split(PathToParse,"/");
        //a bit hacky, but initial / gets cut of from previous split
        if (PathToParse.size() > 0 && PathToParse[0] == '/') 
        {
            Components.insert(Components.begin(), "/");
        }
        for(std::string& Component : Components)
        {
            if(Component == "")
            {
                continue;   
            }
            ReturnValue.AddDirectory(std::move(Component));   
        }
        return(ReturnValue);
    }
    void DocumentPath::AddDirectory(std::string StringToAdd)
    {
        m_PathComponents.push_back(std::move(StringToAdd));        
    }
    void DocumentPath::PopDirectory()
    {
        if(m_PathComponents.size() > 0)
        {
            m_PathComponents.resize(m_PathComponents.size()-1);   
        }
    }
    bool DocumentPath::Empty() const
    {
        return(m_PathComponents.size() == 0 && m_PartIdentifier.size() == 0);   
    }
    void DocumentPath::SetPartIdentifier(std::vector<std::string> PartSpecifier)
    {
        m_PartIdentifier = std::move(PartSpecifier);
    }
    std::vector<std::string> const& DocumentPath::GetPartIdentifier() const
    {
        return(m_PartIdentifier);
    }
    //END DocumentPath

    //BEGIN DocumentReference
    std::vector<std::string> DocumentReference::ParseHomeSpecifier(std::string_view Data, size_t InParseOffset,size_t DataSize,size_t& OutParseOffset,MBError& OutError)
    {
        std::vector<std::string> ReturnValue;
        //only enter this function given that Data[ParseOffset] == '~'
        size_t ParseOffset = InParseOffset + 1;
        std::string CurrentDirectoryId = "";
        while(ParseOffset < Data.size())
        {
            if(Data[ParseOffset] == '/')
            {
                ParseOffset += 1;
                break;   
            }
            else if(Data[ParseOffset] == '{')
            {
                OutError = false;   
                OutError.ErrorMessage = "Invalid character '{' in home specifier";
                break;
            }
            else if(Data[ParseOffset] == '.')
            {
                ReturnValue.push_back(CurrentDirectoryId);
                CurrentDirectoryId = "";
                ParseOffset += 1;
            }
            else
            {
                CurrentDirectoryId += Data[ParseOffset];
                ParseOffset += 1;
            }
        }
        ReturnValue.push_back(CurrentDirectoryId);
        OutParseOffset = ParseOffset;
        return ReturnValue;
    }
    DocumentReference DocumentReference::ParseReference(std::string StringToParse,MBError& OutError)
    {
        DocumentReference ReturnValue;
        size_t LastSlash = StringToParse.find_last_of('/');
        size_t LastBracket = std::min(StringToParse.find_last_of('{'),StringToParse.find_last_of('}'));
        //ASSUMPTION: hashtag cannot be part of normal filename
        size_t FirstHashTag = StringToParse.find('#');
        if(FirstHashTag != StringToParse.npos)
        {
            if((FirstHashTag < LastSlash && LastSlash != StringToParse.npos) || (FirstHashTag < LastBracket && LastBracket != StringToParse.npos))
            {
                OutError = false;
                OutError.ErrorMessage = "Path specifier is not allowed after the part specifier"; 
                return(ReturnValue);
            } 
            //TODO, make new MBUtility::Split, the current one is mega ass
            ReturnValue.PartSpecifier = MBUtility::Split(StringToParse.substr(FirstHashTag+1),"#");
            StringToParse.resize(FirstHashTag);
        }
        size_t ParseOffset = 0;
        MBParsing::SkipWhitespace(StringToParse,ParseOffset,&ParseOffset);

        if(ParseOffset < StringToParse.size() && StringToParse[ParseOffset] == '~')
        {
            ReturnValue.HomeSpecifier = ParseHomeSpecifier(StringToParse,ParseOffset,StringToParse.size(),ParseOffset,OutError);
            if(!OutError)
            {
                return ReturnValue;   
            }
        }
        else if(ParseOffset < StringToParse.size() && StringToParse[ParseOffset] == '/')
        {
            PathSpecifier NewSpecifier;
            NewSpecifier.PathNames.push_back("/");
            ReturnValue.PathSpecifiers.push_back(NewSpecifier);
            ParseOffset += 1;
        }
        PathSpecifier CurrentSpecifier;
        while(ParseOffset < StringToParse.size())
        {
            size_t NextDelimiter = std::min(StringToParse.find('/',ParseOffset),StringToParse.find('{',ParseOffset));
            std::string FSName = StringToParse.substr(ParseOffset,NextDelimiter-ParseOffset);

            //PathSpecifier NewSpecifier;
            if(FSName != "")
            {
                CurrentSpecifier.PathNames.push_back(StringToParse.substr(ParseOffset,NextDelimiter-ParseOffset));
            }
            if(NextDelimiter == StringToParse.npos)
            {
                break;   
            }
            ParseOffset = NextDelimiter;
            if(StringToParse[ParseOffset] == '/')
            {
                ParseOffset+=1;   
            }
            else
            {
                ParseOffset += 1;
                if(CurrentSpecifier.PathNames.size() > 0)
                {
                    if(ReturnValue.PathSpecifiers.size() > 0)
                    {
                        OutError = false;
                        OutError = "There can only be 1 absolute path specifier";   
                        return(ReturnValue);
                    }
                    ReturnValue.PathSpecifiers.push_back(std::move(CurrentSpecifier));     
                    CurrentSpecifier = PathSpecifier();
                }
                size_t AnyRootDelimiter = StringToParse.find('}',ParseOffset);     
                if(AnyRootDelimiter == StringToParse.npos)
                {
                    OutError = false;
                    OutError.ErrorMessage = "No delimiting } found when parsing path specifier";   
                    return(ReturnValue);
                }
                std::string SpecifierData = StringToParse.substr(ParseOffset,AnyRootDelimiter-ParseOffset);  
                if(SpecifierData.find('{') != SpecifierData.npos || SpecifierData.find('}') != SpecifierData.npos)
                {
                    OutError = false;
                    OutError.ErrorMessage = "Nested AnyRoot specifiers not allowed";   
                    return(ReturnValue);
                }
                PathSpecifier AnyRootSpecifier;
                AnyRootSpecifier.AnyRoot = true;  
                std::vector<std::string> PathNames = MBUtility::Split(SpecifierData,"/");
                for(std::string& Path : PathNames)
                {
                    if(Path == "")
                    {
                        continue;   
                    }
                    AnyRootSpecifier.PathNames.push_back(std::move(Path));
                }
                if(AnyRootSpecifier.PathNames.size() == 0)
                {
                    OutError = true;
                    OutError.ErrorMessage = "Can't specify empty AnyRoot specifier";   
                    return(ReturnValue);
                }
                ReturnValue.PathSpecifiers.push_back(std::move(AnyRootSpecifier));
                ParseOffset = AnyRootDelimiter+1;
            }
        }
        if (CurrentSpecifier.PathNames.size() > 0)
        {
            ReturnValue.PathSpecifiers.push_back(std::move(CurrentSpecifier));
        }
        return(ReturnValue); 
    }
    //END DocumentReference

    //BEGIN DocumentFilesystemIterator
    DocumentFilesystemIterator::DocumentFilesystemIterator(size_t DirectoryRoot)
    {
        m_CurrentDirectoryIndex = DirectoryRoot; 
        m_OptionalDirectoryRoot = DirectoryRoot;
        //Calculate depth
    }
    IndexType DocumentFilesystemIterator::GetCurrentDirectoryIndex() const
    {
        return(m_CurrentDirectoryIndex);   
    }
    IndexType DocumentFilesystemIterator::GetCurrentFileIndex() const
    {
        return(m_DirectoryFilePosition); 
    }
    void DocumentFilesystemIterator::CalculateDepth()
    {
        DocumentDirectoryInfo const* CurrentInfo = &m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex];
        while (CurrentInfo->ParentDirectoryIndex != -1)
        {
            m_CurrentDepth += 1;
            CurrentInfo = &m_AssociatedFilesystem->m_DirectoryInfos[CurrentInfo->ParentDirectoryIndex];
        }
    }
    void DocumentFilesystemIterator::UseUserOrder()
    {
        m_UseUserOrder = true; 
    }
    void DocumentFilesystemIterator::Increment()
    {
        DocumentDirectoryInfo const& CurrentInfo = m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex];
        if (m_DirectoryFilePosition == -1)
        {
            m_CurrentDepth += 1;
        }
        if(!m_UseUserOrder)
        {
            m_DirectoryFilePosition += 1;
        }
        else
        {
            if(m_DirectoryFilePosition == -1)
            {
                m_DirectoryFilePosition = CurrentInfo.FirstFileIndex;   
            } 
            else
            {
                m_DirectoryFilePosition = m_AssociatedFilesystem->m_TotalSources[CurrentInfo.FileIndexBegin+m_DirectoryFilePosition].NextFile;    
            }
            if(m_DirectoryFilePosition == -1)
            {
                m_DirectoryFilePosition = (CurrentInfo.FileIndexEnd - CurrentInfo.FileIndexBegin);
            }
        }
        assert(m_DirectoryFilePosition <= CurrentInfo.FileIndexEnd-CurrentInfo.FileIndexBegin);
        if(m_DirectoryFilePosition == (CurrentInfo.FileIndexEnd - CurrentInfo.FileIndexBegin))
        {
            m_DirectoryFilePosition = -1;
            if(CurrentInfo.DirectoryIndexEnd - CurrentInfo.DirectoryIndexBegin != 0)
            {
                if(!m_UseUserOrder)
                {
                    m_CurrentDirectoryIndex = CurrentInfo.DirectoryIndexBegin;
                }
                else
                {
                    m_CurrentDirectoryIndex = CurrentInfo.DirectoryIndexBegin + CurrentInfo.FirstSubDirIndex;
                }
            }
            else
            {
                size_t CurrentDirectoryIndex = m_CurrentDirectoryIndex;
                m_CurrentDirectoryIndex = -1;
                //Being -1 implies that the end is reached
                //ASSUMPTION the root node MUST have -1 as parent
                while(true)
                {
                    m_CurrentDepth -= 1;
                    //Finished
                    if(CurrentDirectoryIndex == m_OptionalDirectoryRoot)
                    {
                        break;   
                    }
                    size_t PreviousIndex = CurrentDirectoryIndex;
                    CurrentDirectoryIndex = m_AssociatedFilesystem->m_DirectoryInfos[PreviousIndex].ParentDirectoryIndex; 
                    //Finished
                    if(CurrentDirectoryIndex == -1)
                    {
                        break;
                    }
                    DocumentDirectoryInfo const& CurrentParentInfo = m_AssociatedFilesystem->m_DirectoryInfos[CurrentDirectoryIndex];
                    if(!m_UseUserOrder)
                    {
                        if(PreviousIndex + 1 < CurrentParentInfo.DirectoryIndexEnd)
                        {
                            m_CurrentDirectoryIndex = PreviousIndex+1;   
                            break;
                        }
                    }
                    else
                    {
                        DocumentDirectoryInfo const& PreviousDirectoryInfo = m_AssociatedFilesystem->m_DirectoryInfos[PreviousIndex];
                        if(PreviousDirectoryInfo.NextDirectory != -1)
                        {
                            m_CurrentDirectoryIndex = CurrentParentInfo.DirectoryIndexBegin+PreviousDirectoryInfo.NextDirectory;
                            break;
                        }
                    }
                }    
            }
        }
    }
    void DocumentFilesystemIterator::NextDirectory()
    {
        DocumentDirectoryInfo const& DirectoryInfo = m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex];
        m_DirectoryFilePosition = (DirectoryInfo.FileIndexEnd-DirectoryInfo.FileIndexBegin)-1;
        Increment();
    }
    void DocumentFilesystemIterator::NextFile()
    {
        while(!HasEnded() && EntryIsDirectory())
        {
            Increment();  
        } 
    }
    bool DocumentFilesystemIterator::EntryIsDirectory() const
    {
        return(m_DirectoryFilePosition == -1);       
    }
    IndexType DocumentFilesystemIterator::GetFileIndex() const
    {
        return m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex].DirectoryIndexBegin+m_DirectoryFilePosition;
    }
    std::string DocumentFilesystemIterator::GetEntryName() const
    {
        if(EntryIsDirectory())
        {
            return(m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex].Name);  
        } 
        return(m_AssociatedFilesystem->m_TotalSources[m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex].DirectoryIndexBegin+m_DirectoryFilePosition].Document.Name);
    }
    size_t DocumentFilesystemIterator::CurrentDepth() const
    {
        return(m_CurrentDepth);
    }
    bool DocumentFilesystemIterator::HasEnded() const
    {
        return(m_DirectoryFilePosition == -1 && m_CurrentDirectoryIndex == -1);       
    }
    DocumentSource const& DocumentFilesystemIterator::GetDocumentInfo() const
    {
        if(EntryIsDirectory())
        {
            throw std::runtime_error("Can't access document info for directory");
        }       
        if(HasEnded())
        {
            throw std::runtime_error("Iterator has ended");
        }
        return(m_AssociatedFilesystem->m_TotalSources[m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex].FileIndexBegin+m_DirectoryFilePosition].Document); 
    }

    DocumentPath DocumentFilesystemIterator::GetCurrentPath() const
    {
        size_t CurrentDirectoryIndex = m_CurrentDirectoryIndex;
        //Naive solution
        std::vector<std::string> Directories;
        if(m_DirectoryFilePosition != -1)
        {
            Directories.push_back(m_AssociatedFilesystem->m_TotalSources[m_AssociatedFilesystem->m_DirectoryInfos[CurrentDirectoryIndex].FileIndexBegin+m_DirectoryFilePosition].Document.Name);
        }
        while(CurrentDirectoryIndex != -1)
        {
            Directories.push_back(m_AssociatedFilesystem->m_DirectoryInfos[CurrentDirectoryIndex].Name);
            CurrentDirectoryIndex = m_AssociatedFilesystem->m_DirectoryInfos[CurrentDirectoryIndex].ParentDirectoryIndex;
        }
        std::reverse(Directories.begin(),Directories.end());
        DocumentPath ReturnValue;
        for(std::string& Path : Directories)
        {
            ReturnValue.AddDirectory(std::move(Path));
        }
        return(ReturnValue);
    }

    //void DocumentFilesystemIterator::SkipDirectoryFiles()
    //{
    //    m_DirectoryFilePosition = m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex].FileIndexEnd-1;         
    //    Increment();
    //}
    //void DocumentFilesystemIterator::ExitDirectory()
    //{
    //     
    //}
    //void DocumentFilesystemIterator::SkipDirectory()
    //{
    //       
    //}
    DocumentFilesystemIterator& DocumentFilesystemIterator::operator++()
    {
        Increment(); 
        return(*this);
    }
    DocumentFilesystemIterator& DocumentFilesystemIterator::operator++(int)
    {
        Increment(); 
        return(*this);
    }
    //END DocumentFilesystemIterator

    //BEGIN DocumentPathFileIterator
    DocumentPathFileIterator::DocumentPathFileIterator(std::vector<DocumentPath> const& Data,int Depth,size_t Offset,size_t End)
    {
        m_Data = &Data;
        m_Depth = Depth;
        m_DataOffset = Offset;
        m_DataEnd = End;
        std::string CurrentDirectory = "";
        for(size_t i = Offset;i < End;i++)
        {
            if(Data[i].ComponentCount() > Depth+1 && Data[i][Depth] != CurrentDirectory)
            {
                m_DirectoryBegins.push_back(i);
                CurrentDirectory = Data[i][Depth];
            }
        } 
    }
    size_t DocumentPathFileIterator::DirectoryCount() const
    {
        return(m_DirectoryBegins.size()); 
    }
    std::string DocumentPathFileIterator::GetDirectoryName() const
    {
        if((*m_Data)[m_FileOffset].ComponentCount() > m_Depth+1)
        {
            return((*m_Data)[m_FileOffset][m_Depth]);
        }
        return("");
    }
    DocumentPath const& DocumentPathFileIterator::CurrentFilePath() const
    {
        return((*m_Data)[m_FileOffset]);
    }
    size_t DocumentPathFileIterator::GetCurrentOffset() const
    {
        return(m_FileOffset-m_DataOffset); 
    }
    size_t DocumentPathFileIterator::GetDirectoryEnd() const
    {
        if(m_DirectoryOffset == m_DirectoryBegins.size()-1)
        {
            return(m_DataEnd); 
        }
        return(m_DirectoryBegins[m_DirectoryOffset+1]);
    }
    size_t DocumentPathFileIterator::GetDirectoryBegin(size_t DirectoryIndex) const
    {
        return(m_DirectoryBegins[DirectoryIndex]);
    }
    size_t DocumentPathFileIterator::GetDirectoryEnd(size_t DirectoryIndex) const
    {
        if (DirectoryIndex == m_DirectoryBegins.size() - 1)
        {
            return(m_DataEnd);
        }
        return(m_DirectoryBegins[DirectoryIndex + 1]);
    }
    bool DocumentPathFileIterator::HasEnded() const
    {
        return(m_FileOffset == m_DataEnd); 
    }
    void DocumentPathFileIterator::NextDirectory()
    {
        if(m_DirectoryBegins.size() == 0 || m_DirectoryOffset == m_DirectoryBegins.size() -1)
        {
            m_FileOffset = m_DataEnd;
            return; 
        }
        m_DirectoryOffset+=1;
        m_FileOffset = m_DirectoryBegins[m_DirectoryOffset];
    }
   
    //END DocumentPathFileIterator
    void DocumentTraverser::Visit(BlockElement const& BlockToVisit)
    {
        m_AssociatedVisitor->EnterBlock(BlockToVisit); 
        BlockToVisit.Accept(*this); 
        m_AssociatedVisitor->LeaveBlock(BlockToVisit);
    }
    void DocumentTraverser::Visit(BlockElement& BlockToVisit)
    {
        m_AssociatedVisitor->EnterBlock(BlockToVisit); 
        BlockToVisit.Accept(*this); 
        m_AssociatedVisitor->LeaveBlock(BlockToVisit);
    }
    void DocumentTraverser::Visit(Directive const& DirectiveToVisit)
    {
        m_AssociatedVisitor->Visit(DirectiveToVisit);
    }
    void DocumentTraverser::Visit(Directive& DirectiveToVisit)
    {
        m_AssociatedVisitor->Visit(DirectiveToVisit);
    }
    void DocumentTraverser::Visit(FormatElement const& FormatToVisit)
    {
        m_AssociatedVisitor->EnterFormat(FormatToVisit);
        for(auto const& Element : FormatToVisit.Contents)
        {
            Element.Accept(*this);
        }
        m_AssociatedVisitor->LeaveFormat(FormatToVisit); 
    }
    void DocumentTraverser::Visit(FormatElement& FormatToVisit)
    {
        m_AssociatedVisitor->EnterFormat(FormatToVisit);
        for(auto& Element : FormatToVisit.Contents)
        {
            Element.Accept(*this);
        }
        m_AssociatedVisitor->LeaveFormat(FormatToVisit); 
    }

    void DocumentTraverser::Visit(Paragraph const& VisitedParagraph)
    {
        m_AssociatedVisitor->Visit(VisitedParagraph);
        for(auto const& TextPart : VisitedParagraph.TextElements)
        {
            //if(TextPart.IsType<DocReference>())
            //{
            //    m_AssociatedVisitor->ResolveReference(const_cast<TextElement&>(TextPart));
            //}
            m_AssociatedVisitor->EnterText(TextPart.GetBase());
            TextPart.Accept(*this);
            m_AssociatedVisitor->LeaveText(TextPart.GetBase());
        }
    }
    void DocumentTraverser::Visit(Paragraph& VisitedParagraph)
    {
        m_AssociatedVisitor->Visit(VisitedParagraph);
        for(auto& TextPart : VisitedParagraph.TextElements)
        {
            if(TextPart.IsType<DocReference>())
            {
                m_AssociatedVisitor->ResolveReference(TextPart);
            }
            m_AssociatedVisitor->EnterText(TextPart.GetBase());
            TextPart.Accept(*this);
            m_AssociatedVisitor->LeaveText(TextPart.GetBase());
        }
    }
    void DocumentTraverser::Visit(MediaInclude const& VisitedMedia) 
    {
        m_AssociatedVisitor->Visit(VisitedMedia);
    }
    void DocumentTraverser::Visit(MediaInclude& VisitedMedia) 
    {
        m_AssociatedVisitor->Visit(VisitedMedia);
    }
    void DocumentTraverser::Visit(CodeBlock const& VisitedBlock) 
    { 
        m_AssociatedVisitor->Visit(VisitedBlock);
    }
    void DocumentTraverser::Visit(CodeBlock& VisitedBlock) 
    { 
        m_AssociatedVisitor->Visit(VisitedBlock);
    }
    void DocumentTraverser::Visit(Table const& Table)
    {
        m_AssociatedVisitor->Visit(Table);
        for(auto& Row : Table)
        {
            for(auto& Cell : Row)
            {
                m_AssociatedVisitor->EnterBlock(Cell);
                Visit(Cell);
                m_AssociatedVisitor->LeaveBlock(Cell);
            }   
        }
    }
    void DocumentTraverser::Visit(Table& Table)
    {
        m_AssociatedVisitor->Visit(Table);
        for(auto& Row : Table)
        {
            for(auto& Cell : Row)
            {
                m_AssociatedVisitor->EnterBlock(Cell);
                Visit(Cell);
                m_AssociatedVisitor->LeaveBlock(Cell);
            }   
        }
    }
    void DocumentTraverser::Visit(List const& ListToVisit)
    {
        m_AssociatedVisitor->Visit(ListToVisit);
        for(auto const& Content : ListToVisit.Content)
        {
            m_AssociatedVisitor->EnterBlock(Content.Text);
            this->Visit(Content.Text);
            m_AssociatedVisitor->LeaveBlock(Content.Text);
            //
            if(Content.SubList != nullptr)
            {
                m_AssociatedVisitor->EnterBlock(*Content.SubList);
                this->Visit(*Content.SubList);
                m_AssociatedVisitor->LeaveBlock(*Content.SubList);
            }
        }
    }
    void DocumentTraverser::Visit(List& ListToVisit)
    {
        m_AssociatedVisitor->Visit(ListToVisit);
        for(auto& Content : ListToVisit.Content)
        {
            m_AssociatedVisitor->EnterBlock(Content.Text);
            this->Visit(Content.Text);
            m_AssociatedVisitor->LeaveBlock(Content.Text);
            //
            if(Content.SubList != nullptr)
            {
                m_AssociatedVisitor->EnterBlock(*Content.SubList);
                this->Visit(*Content.SubList);
                m_AssociatedVisitor->LeaveBlock(*Content.SubList);
            }
        }
    }

    void DocumentTraverser::Visit(RegularText const& VisitedText)
    {
        m_AssociatedVisitor->Visit(VisitedText);
    }
    void DocumentTraverser::Visit(RegularText& VisitedText)
    {
        m_AssociatedVisitor->Visit(VisitedText);
    }
    void DocumentTraverser::Visit(DocReference const& VisitedText) 
    {
        VisitedText.Accept(*this);
    }
    void DocumentTraverser::Visit(DocReference& VisitedText) 
    {
        VisitedText.Accept(*this);
    }

    void DocumentTraverser::Visit(FileReference const& FileRef) 
    {
        m_AssociatedVisitor->Visit(FileRef);
    }
    void DocumentTraverser::Visit(FileReference& FileRef) 
    {
        m_AssociatedVisitor->Visit(FileRef);
    }
    void DocumentTraverser::Visit(URLReference const& URLRef) 
    {
        m_AssociatedVisitor->Visit(URLRef);
    }
    void DocumentTraverser::Visit(URLReference& URLRef) 
    {
        m_AssociatedVisitor->Visit(URLRef);
    }
    void DocumentTraverser::Visit(ResourceReference& URLRef) 
    {
        m_AssociatedVisitor->Visit(URLRef);
    }
    void DocumentTraverser::Visit(ResourceReference const& URLRef) 
    {
        m_AssociatedVisitor->Visit(URLRef);
    }
    void DocumentTraverser::Visit(UnresolvedReference const& UnresolvedRef) 
    {
        m_AssociatedVisitor->Visit(UnresolvedRef);
    }
    void DocumentTraverser::Visit(UnresolvedReference& UnresolvedRef) 
    {
        m_AssociatedVisitor->Visit(UnresolvedRef);
    }
    void DocumentTraverser::Traverse(DocumentSource const& SourceToTraverse,DocumentVisitor& Vistor)
    {
        m_AssociatedVisitor = &Vistor; 
        for(auto const& Format : SourceToTraverse.Contents)
        {
            Format.Accept(*this);
        }
    }
    void DocumentTraverser::Traverse(DocumentSource& SourceToTraverse,DocumentVisitor& Vistor)
    {
        m_AssociatedVisitor = &Vistor; 
        for(auto& Format : SourceToTraverse.Contents)
        {
            Format.Accept(*this);
        }
    }
    //BEGIN DocumentFilesystem
    DocumentFilesystem::FSSearchResult DocumentFilesystem::p_ResolveDirectorySpecifier(IndexType RootDirectoryIndex,DocumentReference const& ReferenceToResolve,IndexType SpecifierOffset) const
    {
        FSSearchResult ReturnValue;        
        DocumentFilesystemIterator Iterator(RootDirectoryIndex); 
        Iterator.m_AssociatedFilesystem = this;
        while(!Iterator.HasEnded())
        {
            FSSearchResult CurrentResult = p_ResolveAbsoluteDirectory(Iterator.GetCurrentDirectoryIndex(),ReferenceToResolve.PathSpecifiers[SpecifierOffset]);
            if(CurrentResult.Type != DocumentFSType::Null)
            {
                if(SpecifierOffset == ReferenceToResolve.PathSpecifiers.size()-1)
                {
                    ReturnValue = CurrentResult;
                    break;  
                } 
                if(CurrentResult.Type == DocumentFSType::Directory)
                {
                    CurrentResult = p_ResolveDirectorySpecifier(CurrentResult.Index,ReferenceToResolve,SpecifierOffset+1);
                    if(CurrentResult.Type != DocumentFSType::Null)
                    {
                        ReturnValue = CurrentResult;   
                        break;
                    }
                }
            }
            Iterator.NextDirectory();
        }
        return(ReturnValue);
    }
    bool h_DirectoryComp(DocumentDirectoryInfo const& LeftInfo, std::string const& RightInfo) 
    {
        return(LeftInfo.Name < RightInfo);
    }
    bool h_FileComp(FilesystemDocumentInfo const& LeftInfo, std::string const& RightInfo)
    {
        return(LeftInfo.Document.Name < RightInfo);
    }
    IndexType DocumentFilesystem::p_DirectorySubdirectoryIndex(IndexType DirectoryIndex,std::string const& SubdirectoryName) const
    {
        IndexType ReturnValue = 0; 
        if(SubdirectoryName == "..")
        {
            return(m_DirectoryInfos[DirectoryIndex].ParentDirectoryIndex);
        }
        DocumentDirectoryInfo const& DirectoryInfo = m_DirectoryInfos[DirectoryIndex];
        if(DirectoryInfo.DirectoryIndexEnd - DirectoryInfo.DirectoryIndexBegin== 0)
        {
            return(-1);   
        }
        IndexType Index = std::lower_bound(m_DirectoryInfos.begin()+DirectoryInfo.DirectoryIndexBegin,m_DirectoryInfos.begin()+DirectoryInfo.DirectoryIndexEnd,SubdirectoryName,h_DirectoryComp)-m_DirectoryInfos.begin();
        if(Index == DirectoryInfo.DirectoryIndexEnd)
        {
            ReturnValue = -1;  
        } 
        else
        {
            if (m_DirectoryInfos[Index].Name != SubdirectoryName)
            {
                ReturnValue = -1;
            }
            else
            {
                ReturnValue = Index;
            }
        }
        return(ReturnValue);
    }
    IndexType DocumentFilesystem::p_DirectoryFileIndex(IndexType DirectoryIndex,std::string const& FileName) const
    {
        IndexType ReturnValue = 0; 
        DocumentDirectoryInfo const& DirectoryInfo = m_DirectoryInfos[DirectoryIndex];
        if(DirectoryInfo.FileIndexEnd - DirectoryInfo.FileIndexBegin == 0)
        {
            return(-1);   
        }
        IndexType Index = std::lower_bound(m_TotalSources.begin()+DirectoryInfo.FileIndexBegin,m_TotalSources.begin()+DirectoryInfo.FileIndexEnd,FileName, h_FileComp)-m_TotalSources.begin();
        if(Index == DirectoryInfo.FileIndexEnd)
        {
            ReturnValue = -1;  
        } 
        else
        {
            if (m_TotalSources[Index].Document.Name != FileName)
            {
                ReturnValue = -1;
            }
            else
            {
                ReturnValue = Index;
            }
        }
        return(ReturnValue);
    }
    DocumentFilesystem::FSSearchResult DocumentFilesystem::p_ResolveAbsoluteDirectory(IndexType RootDirectoryIndex,PathSpecifier const& Path) const
    {
        //All special cases stems from the fact that the special / directory isnt the child of any other directory
        FSSearchResult ReturnValue;        
        IndexType LastDirectoryIndex = RootDirectoryIndex;
        int Offset = 0;
        if (Path.PathNames.front() == "/")
        {
            Offset = 1;
            LastDirectoryIndex = 0;
        }
        for(int i = Offset; i < int(Path.PathNames.size())-1;i++)
        {
            IndexType SubdirectoryIndex = p_DirectorySubdirectoryIndex(LastDirectoryIndex,Path.PathNames[i]);
            if(SubdirectoryIndex == -1)
            {
                ReturnValue.Type = DocumentFSType::Null;  
                return(ReturnValue);
            } 
            LastDirectoryIndex = SubdirectoryIndex;
        }
        if (Offset >= int(Path.PathNames.size()))
        {
            ReturnValue.Index = LastDirectoryIndex;
            ReturnValue.Type = DocumentFSType::Directory;
            return(ReturnValue);
        }
        IndexType FileIndex = p_DirectoryFileIndex(LastDirectoryIndex,Path.PathNames.back());
        if(FileIndex != -1)
        {
            ReturnValue.Type = DocumentFSType::File;   
            ReturnValue.Index = FileIndex;
        }
        else
        {
            IndexType DirectoryIndex = p_DirectorySubdirectoryIndex(LastDirectoryIndex,Path.PathNames.back());    
            if(DirectoryIndex != -1)
            {
                ReturnValue.Type = DocumentFSType::Directory;   
                ReturnValue.Index = DirectoryIndex;
            }
        }
        return(ReturnValue);
           
    }
    DocumentFilesystem::FSSearchResult DocumentFilesystem::p_ResolvePartSpecifier(FSSearchResult CurrentResult,std::vector<std::string> const& PartSpecifier) const
    {
        FSSearchResult ReturnValue;        
        assert(CurrentResult.Type == DocumentFSType::File || CurrentResult.Type == DocumentFSType::Directory);
        if(CurrentResult.Type == DocumentFSType::File)
        {
            if(m_TotalSources[CurrentResult.Index].Document.ReferenceTargets.ContainsLabels(PartSpecifier))
            {
                ReturnValue = CurrentResult;     
            }   
        }
        if(CurrentResult.Type == DocumentFSType::Directory)
        {
            DocumentFilesystemIterator Iterator(CurrentResult.Index);
            Iterator.m_AssociatedFilesystem = this;
            while(!Iterator.HasEnded())
            {
                if (!Iterator.EntryIsDirectory())
                {
                    if (Iterator.GetDocumentInfo().ReferenceTargets.ContainsLabels(PartSpecifier))
                    {
                        ReturnValue.Type = DocumentFSType::File;
                        ReturnValue.Index = Iterator.GetCurrentFileIndex();
                        break;
                    }
                }
                Iterator.Increment();
            }
        }
        return(ReturnValue);
    }
    IndexType DocumentFilesystem::p_GetSatisfyingHomeSubdir(IndexType HomeIndex, std::vector<std::string> const& HomeSpecifier,int Offset) const
    {
        IndexType ReturnValue = -1;
        auto const& CurrentPart = HomeSpecifier[Offset];
        if(CurrentPart == "")
        {
            return HomeIndex;   
        }
        for(auto const& ChildIndex : m_HomeIntervalls[HomeIndex].Children)
        {
            auto const& SubDir = m_HomeIntervalls[ChildIndex];
            if(SubDir.Name == CurrentPart)
            {
                if(Offset + 1 == HomeSpecifier.size())
                {
                    return ChildIndex;
                }
                else 
                {
                    IndexType SubDirSatisfying = p_GetSatisfyingHomeSubdir(ChildIndex,HomeSpecifier,Offset+1);
                    if(SubDirSatisfying != -1)
                    {
                        return SubDirSatisfying;
                    }
                }
            }
        }
        return ReturnValue;
    }
    DocumentFilesystem::FSSearchResult DocumentFilesystem::p_ResolveHomeSpecifier(IndexType FileIndex,std::vector<std::string> const& HomeSpecifier) const
    {
        FSSearchResult ReturnValue;
        if(m_HomeIntervalls.size() == 0)
        {
            return ReturnValue;   
        }
        auto FirstHomeIt = std::lower_bound(m_HomeIntervalls.begin(),m_HomeIntervalls.end(),FileIndex,
                [&](HomeIntervall const& Lhs,IndexType Index)
                {
                    auto const& CurrentDir = m_DirectoryInfos[Lhs.DirIndex];
                    return CurrentDir.FileIndexBegin <= Index;
                });
        if((FirstHomeIt - m_HomeIntervalls.begin()) == 0)
        {
            return ReturnValue;   
        }
        IndexType CurrentHomeIndex = (FirstHomeIt - m_HomeIntervalls.begin())-1;
        if(!(m_DirectoryInfos[m_HomeIntervalls[CurrentHomeIndex].DirIndex].FileIndexBegin <= FileIndex && FileIndex < m_DirectoryInfos[m_HomeIntervalls[CurrentHomeIndex].DirIndex].FileIndexEnd))
        {
            return ReturnValue;
        }
        while(CurrentHomeIndex != -1)
        {
            IndexType SatisfyingSubdir = p_GetSatisfyingHomeSubdir(CurrentHomeIndex,HomeSpecifier,0);
            if(SatisfyingSubdir != -1)
            {
                ReturnValue.Index = m_HomeIntervalls[SatisfyingSubdir].DirIndex;
                ReturnValue.Type = DocumentFSType::Directory;
                return ReturnValue;
            }
            else
            {
                CurrentHomeIndex = m_HomeIntervalls[CurrentHomeIndex].ParentIndex;
            }
        }
        return ReturnValue;
    }
    bool h_ContainFileComp(DocumentDirectoryInfo const& DirectoryToCheck, size_t FileToContain)
    { 
        if (DirectoryToCheck.FileIndexBegin == DirectoryToCheck.FileIndexEnd)
        {
            return(DirectoryToCheck.FileIndexBegin <= FileToContain);
        }
        if(DirectoryToCheck.FileIndexEnd <= FileToContain)
        {
            return(true);
        }
        if (DirectoryToCheck.FileIndexBegin > FileToContain)
        {
            return(false);
        }
        //We want the first element *not less* than value
        //if (DirectoryToCheck.FileIndexBegin <= FileToContain &&  FileToContain < DirectoryToCheck.FileIndexEnd)
        //{
        //    return(false);
        //}
        return(false);
    }
    DocumentPath DocumentFilesystem::p_GetFileIndexPath(IndexType FileIndex) const
    {
        DocumentPath ReturnValue;
        IndexType DirectoryIndex = std::lower_bound(m_DirectoryInfos.begin(), m_DirectoryInfos.end(), FileIndex, h_ContainFileComp) - m_DirectoryInfos.begin();
        if (DirectoryIndex == m_DirectoryInfos.size())
        {
            return(ReturnValue);
        }
        std::vector<std::string> Components;
        while (DirectoryIndex != -1)
        {
            Components.push_back(m_DirectoryInfos[DirectoryIndex].Name);
            DirectoryIndex = m_DirectoryInfos[DirectoryIndex].ParentDirectoryIndex;
        }
        std::reverse(Components.begin(), Components.end());
        for (std::string& Component : Components)
        {
            ReturnValue.AddDirectory(std::move(Component));
        }
        ReturnValue.AddDirectory(m_TotalSources[FileIndex].Document.Name);
        return(ReturnValue);
    }
    IndexType DocumentFilesystem::p_GetFileDirectoryIndex(DocumentPath const& PathToSearch) const
    {
        IndexType ReturnValue = 0;
        int Offset = 0;
        if (PathToSearch[0] == "/")
        {
            Offset = 1;
        }
        for (int i = Offset; i < int(PathToSearch.ComponentCount()) - 1; i++)
        {
            ReturnValue = p_DirectorySubdirectoryIndex(ReturnValue, PathToSearch[i]);
            if (ReturnValue == -1)
            {
                break;
            }
        }
        return(ReturnValue);
    }
    IndexType DocumentFilesystem::p_GetFileIndex(DocumentPath const& PathToSearch) const
    {
        IndexType ReturnValue = 0;
        for (size_t i = 0; i < PathToSearch.ComponentCount(); i++)
        {
            if (PathToSearch[i] == "/")
            {
                continue;
            }
            if (i + 1 < PathToSearch.ComponentCount())
            {
                ReturnValue = p_DirectorySubdirectoryIndex(ReturnValue, PathToSearch[i]);
                if (ReturnValue == -1)
                {
                    break;
                }
            }
            else 
            {
                ReturnValue = p_DirectoryFileIndex(ReturnValue, PathToSearch[i]);
            }
        }
        return(ReturnValue);
    }
     
    DocumentFilesystem::FSSearchResult DocumentFilesystem::p_ResolvePath(DocumentPath const& FilePath,DocumentReference const& ReferenceIdentifier,bool* OutResult) const
    {
        FSSearchResult SearchRoot;
        SearchRoot.Type = DocumentFSType::Directory;
        SearchRoot.Index = 0;//Root directory
        IndexType SpecifierOffset = 0;

        if(ReferenceIdentifier.HomeSpecifier.size() != 0)
        {
            auto FileIndex = p_GetFileIndex(FilePath);
            SearchRoot = p_ResolveHomeSpecifier(FileIndex,ReferenceIdentifier.HomeSpecifier);
            if(SearchRoot.Type == DocumentFSType::Null)
            {
                *OutResult = false;   
                return SearchRoot;
            }
        }
        if (ReferenceIdentifier.PathSpecifiers.size() > 0)
        {
            if (ReferenceIdentifier.PathSpecifiers[0].AnyRoot == false)
            {
                SpecifierOffset = 1;
                if (ReferenceIdentifier.PathSpecifiers[0].PathNames.front() != "/" && ReferenceIdentifier.HomeSpecifier.size() == 0)
                {
                    //The current directory needs to be resolved 
                    SearchRoot.Index = p_GetFileDirectoryIndex(FilePath);
                    if (SearchRoot.Index == -1)
                    {
                        *OutResult = false;
                        return(SearchRoot);
                    }
                }
                SearchRoot = p_ResolveAbsoluteDirectory(SearchRoot.Index, ReferenceIdentifier.PathSpecifiers[0]);
                if (SearchRoot.Type == DocumentFSType::Null)
                {
                    *OutResult = false;
                    return(SearchRoot);
                }
            }
            else
            {
                SearchRoot.Index = p_GetFileDirectoryIndex(FilePath);
                if (SearchRoot.Index == -1)
                {
                    *OutResult = false;
                    return(SearchRoot);
                }
            }
            if (SpecifierOffset == 0 || ReferenceIdentifier.PathSpecifiers.size() > 1)
            {
                if (SearchRoot.Type == DocumentFSType::File)
                {
                    *OutResult = false;
                    return(SearchRoot);
                }
                SearchRoot = p_ResolveDirectorySpecifier(SearchRoot.Index, ReferenceIdentifier, SpecifierOffset);
            }
            if (SearchRoot.Type == DocumentFSType::Null)
            {
                *OutResult = false;
                return(SearchRoot);
            }
        }
        else
        {
            SearchRoot.Type = DocumentFSType::File;
            SearchRoot.Index = p_GetFileIndex(FilePath);
            if (SearchRoot.Index == -1)
            {
                *OutResult = false;
                return(SearchRoot);
            }
        }
        if(ReferenceIdentifier.PartSpecifier.size() != 0)
        {
            SearchRoot = p_ResolvePartSpecifier(SearchRoot,ReferenceIdentifier.PartSpecifier);
        }
        if(SearchRoot.Type == DocumentFSType::Null)
        {
            *OutResult = false;   
            return(SearchRoot);
        }
        return SearchRoot;
    }
    DocumentPath DocumentFilesystem::p_ResolveReference(DocumentPath const& FilePath,DocumentReference const& ReferenceIdentifier,bool* OutResult) const
    {
        bool Result = true;
        DocumentPath ReturnValue;
        auto SearchRoot = p_ResolvePath(FilePath,ReferenceIdentifier,&Result);
        if(SearchRoot.Index == -1 || SearchRoot.Type == DocumentFSType::Null)
        {
            *OutResult = false;   
            return ReturnValue;
        }
        if (SearchRoot.Type == DocumentFSType::Directory)
        {
            IndexType FileIndex = p_DirectoryFileIndex(SearchRoot.Index, "index.mbd");
            if (FileIndex != -1)
            {
                SearchRoot.Index = FileIndex;
                SearchRoot.Type = DocumentFSType::File;
            }
            else
            {
                *OutResult = false;
                return(ReturnValue);
            }
        }
        ReturnValue = p_GetFileIndexPath(SearchRoot.Index); 
        if (ReferenceIdentifier.PartSpecifier.size() != 0)
        {
            ReturnValue.SetPartIdentifier(ReferenceIdentifier.PartSpecifier);
        }
        assert(ReturnValue.ComponentCount() > 0);
        *OutResult = Result;
        return(ReturnValue);
    }
    DocumentFilesystemIterator DocumentFilesystem::begin() const
    {
        DocumentFilesystemIterator ReturnValue(0);
        ReturnValue.m_AssociatedFilesystem = this;
        ReturnValue.CurrentDepth();
        ReturnValue.UseUserOrder();
        return(ReturnValue);
    }
    bool DocumentFilesystem::DocumentExists(DocumentPath const& DocumentPath) const
    {
        IndexType OutIndex = 0;
        return DocumentExists(DocumentPath,OutIndex);
    }
    bool DocumentFilesystem::DocumentExists(DocumentPath const& DocumentPath,IndexType& OutIndex) const
    {
        bool ReturnValue = true;
        size_t DirIndex = 0;
        size_t PartIndex = 0;
        while(PartIndex < DocumentPath.ComponentCount())
        {
            DocumentDirectoryInfo const& Dir = m_DirectoryInfos[DirIndex];
            auto DirIt = std::lower_bound(m_DirectoryInfos.begin()+Dir.DirectoryIndexBegin,m_DirectoryInfos.begin()+Dir.DirectoryIndexEnd,
                    DocumentPath[PartIndex],
                    [](DocumentDirectoryInfo const& SubDir,std::string const& Name){return SubDir.Name < Name;});
            auto FileIt = std::lower_bound(m_TotalSources.begin()+Dir.FileIndexBegin,m_TotalSources.begin()+Dir.FileIndexEnd,
                    DocumentPath[PartIndex],
                    [](FilesystemDocumentInfo const& File,std::string const& Name){return File.Document.Name < Name;});
            if(DirIt != m_DirectoryInfos.begin()+Dir.DirectoryIndexEnd)
            {
                DirIndex = DirIt - (m_DirectoryInfos.begin());
            }
            else
            {
                if(PartIndex + 1 == DocumentPath.ComponentCount() && FileIt != m_TotalSources.begin()+Dir.FileIndexEnd)
                {
                    return true;
                }
            }
            PartIndex++;
        }
        return ReturnValue;
    }
    DocumentPath DocumentFilesystem::ResolveReference(DocumentPath const& Path,std::string const& PathIdentifier,MBError& OutResult) const
    {
        //TEMP
        MBError ParseReferenceError = true;
        DocumentPath ReturnValue;
        DocumentReference ReferenceToUse = DocumentReference::ParseReference(PathIdentifier,ParseReferenceError);
        if(!ParseReferenceError)
        {
            OutResult = ParseReferenceError;
            return(ReturnValue);
        }
        bool Result = true;
        ReturnValue = p_ResolveReference(Path,ReferenceToUse,&Result);
        if (!Result)
        {
            OutResult = false;
            OutResult.ErrorMessage = "Failed to resolve reference";
        }
        return(ReturnValue);
    }
    //DocumentDirectoryInfo DocumentFilesystem::p_UpdateFilesystemOverFiles(DocumentBuild const& CurrentBuild,size_t FileIndexBegin,std::vector<DocumentPath> const& Files,size_t DirectoryIndex,int //Depth,size_t DirectoryBegin,size_t DirectoryEnd)
    //{
    //    DocumentDirectoryInfo ReturnValue; 
    //    DocumentPathFileIterator FileIterator(Files,Depth,DirectoryBegin,DirectoryEnd);
    //    FileIterator.NextDirectory();
    //
    //    ReturnValue.FileIndexBegin = FileIndexBegin;
    //    ReturnValue.FileIndexEnd = FileIndexBegin +FileIterator.GetCurrentOffset();
    //    ReturnValue.DirectoryIndexBegin = m_DirectoryInfos.size();
    //    ReturnValue.DirectoryIndexEnd = m_DirectoryInfos.size()+FileIterator.DirectoryCount();
    //    for(size_t i = 0; i < FileIterator.GetCurrentOffset();i++)
    //    {
    //        DocumentParsingContext Parser;
    //        MBError ParseError = true;
    //        DocumentSource NewSource = Parser.ParseSource(CurrentBuild.BuildRootDirectory/Files[DirectoryBegin+i].GetString(),ParseError);
    //        if(!ParseError)
    //        {
    //            throw new std::runtime_error("Error parsing file: "+ParseError.ErrorMessage);
    //        }
    //        m_TotalSources[FileIndexBegin + i] = std::move(NewSource);
    //    }
    //    m_DirectoryInfos.resize(m_DirectoryInfos.size() + FileIterator.DirectoryCount());
    //    size_t TotalFiles = 0;
    //    size_t SubDirectoriesFileBegin = m_TotalSources.size();
    //
    //    size_t CurrentDirectoryOffset = 0;
    //    size_t CurrentFileOffset = 0;
    //    while(FileIterator.HasEnded() == false)
    //    {
    //        DocumentDirectoryInfo NewDirectoryInfo = p_UpdateFilesystemOverFiles(CurrentBuild, SubDirectoriesFileBegin+CurrentFileOffset,Files,ReturnValue.DirectoryIndexBegin+CurrentDirectoryOffset,Depth//+1,DirectoryBegin+FileIterator.GetCurrentOffset(),DirectoryBegin+FileIterator.GetDirectoryEnd());
    //
    //        DocumentPathFileIterator TempIterator = DocumentPathFileIterator(Files, Depth + 1, DirectoryBegin + FileIterator.GetCurrentOffset(), DirectoryBegin + FileIterator.GetDirectoryEnd());
    //        TempIterator.NextDirectory();
    //        CurrentFileOffset += TempIterator.GetCurrentOffset();
    //
    //        NewDirectoryInfo.ParentDirectoryIndex = DirectoryIndex;
    //        m_DirectoryInfos[ReturnValue.DirectoryIndexBegin+CurrentDirectoryOffset] = std::move(NewDirectoryInfo);
    //        FileIterator.NextDirectory();
    //    }
    //    return(ReturnValue);
    //}
    //DocumentDirectoryInfo DocumentFilesystem::p_UpdateFilesystemOverBuild(DocumentBuild const& BuildToAppend,size_t FileIndexBegin,size_t DirectoryIndex,std::string DirectoryName,MBError& OutError)
    //{
    //    MBError Result = true;
    //    std::vector<DocumentPath> const& BuildFiles = BuildToAppend.BuildFiles;
    //    BuildDirectory DirectoryContents = p_ParseBuildDirectory(BuildToAppend.BuildFiles);
    //    DocumentDirectoryInfo NewInfo;
    //    //size_t NewDirectoryIndex = m_DirectoryInfos.size();
    //    size_t FirstDirectoryIndex = m_DirectoryInfos.size();
    //    size_t SubDirectoriesCount = DirectoryContents.SubDirectories.size() + BuildToAppend.SubBuilds.size();
    //    NewInfo.DirectoryIndexBegin = m_DirectoryInfos.size();
    //    NewInfo.DirectoryIndexEnd = m_DirectoryInfos.size()+SubDirectoriesCount;
    //    NewInfo.FileIndexBegin = FileIndexBegin;
    //    NewInfo.FileIndexEnd = FileIndexBegin +DirectoryContents.FileNames.size();
    //    NewInfo.Name = std::move(DirectoryName); 
    //    //NewInfo.ParentDirectoryIndex = ParentIndex; Set by the caller
    //    //Adding the new files
    //    for(size_t i = 0; i < DirectoryContents.FileNames.size();i++)
    //    {
    //        DocumentParsingContext Parser;
    //        DocumentSource NewSource = Parser.ParseSource(BuildToAppend.BuildRootDirectory/ DirectoryContents.FileNames[i],OutError);
    //        if(!OutError)
    //        {
    //            return(NewInfo);   
    //        }
    //        m_TotalSources[FileIndexBegin+i] =std::move(NewSource);
    //    }
    //    m_DirectoryInfos.resize(m_DirectoryInfos.size()+SubDirectoriesCount);
    //    //total files
    //    size_t TotalSubdirectoryFiles = 0;
    //    for (auto const& Build : BuildToAppend.SubBuilds)
    //    {
    //        TotalSubdirectoryFiles += DirectoryContents.SubDirectories.size();
    //    }
    //    for (size_t i = 0; i < FilesIterator.DirectoryCount(); i++)
    //    {
    //        DocumentPathFileIterator TempIterator = DocumentPathFileIterator(BuildFiles, 1, FilesIterator.GetDirectoryBegin(i), FilesIterator.GetDirectoryEnd(i));
    //        TempIterator.NextDirectory();
    //        TotalSubdirectoryFiles += TempIterator.GetCurrentOffset();
    //    }
    //    size_t SubdirectoryFileBegin = m_TotalSources.size();
    //    m_TotalSources.resize(m_TotalSources.size() + TotalSubdirectoryFiles);
    //
    //    auto SubBuildIterator = BuildToAppend.SubBuilds.begin();
    //    auto SubBuildEnd = BuildToAppend.SubBuilds.end();
    //    size_t CurrentDirectoryIndex = 0;
    //    size_t CurrentFileOffset = 0;
    //    while(FilesIterator.HasEnded() == false  && SubBuildIterator != SubBuildEnd)
    //    {
    //        if(FilesIterator.GetDirectoryName() < SubBuildIterator->first)
    //        {
    //            DocumentDirectoryInfo NewDirectoryInfo = p_UpdateFilesystemOverFiles(BuildToAppend, SubdirectoryFileBegin+CurrentFileOffset,BuildFiles,FirstDirectoryIndex//+CurrentDirectoryIndex,0,FilesIterator.GetCurrentOffset(),FilesIterator.GetDirectoryEnd());
    //
    //            DocumentPathFileIterator TempIterator = DocumentPathFileIterator(BuildFiles, 1, FilesIterator.GetCurrentOffset(), FilesIterator.GetDirectoryEnd());
    //            TempIterator.NextDirectory();
    //            CurrentFileOffset += TempIterator.GetCurrentOffset();
    //
    //            NewDirectoryInfo.ParentDirectoryIndex = DirectoryIndex;
    //            m_DirectoryInfos[FirstDirectoryIndex+CurrentDirectoryIndex] = std::move(NewDirectoryInfo);
    //            FilesIterator.NextDirectory();
    //        }
    //        else if(FilesIterator.GetDirectoryName() > SubBuildIterator->first)
    //        {
    //            DocumentDirectoryInfo NewDirectoryInfo = p_UpdateFilesystemOverBuild(SubBuildIterator->second, SubdirectoryFileBegin+CurrentFileOffset,FirstDirectoryIndex//+CurrentDirectoryIndex,SubBuildIterator->first,OutError);
    //            NewDirectoryInfo.ParentDirectoryIndex = DirectoryIndex;
    //            m_DirectoryInfos[FirstDirectoryIndex+CurrentDirectoryIndex] = std::move(NewDirectoryInfo);
    //            CurrentFileOffset += SubBuildIterator->second.BuildFiles.size();
    //            SubBuildIterator++;
    //        }
    //        else if(FilesIterator.GetDirectoryName() == SubBuildIterator->first)
    //        {
    //            OutError = false;
    //            OutError.ErrorMessage = "Directory and sub build mount name can't be the same";   
    //            return(NewInfo);
    //        }
    //        CurrentDirectoryIndex++;
    //    }
    //    while (!FilesIterator.HasEnded())
    //    {
    //        DocumentDirectoryInfo NewDirectoryInfo = p_UpdateFilesystemOverFiles(BuildToAppend, SubdirectoryFileBegin + CurrentFileOffset, BuildFiles, FirstDirectoryIndex + CurrentDirectoryIndex, 1, //FilesIterator.GetCurrentOffset(), FilesIterator.GetDirectoryEnd());
    //        NewDirectoryInfo.ParentDirectoryIndex = DirectoryIndex;
    //        NewDirectoryInfo.Name = FilesIterator.GetDirectoryName();
    //        m_DirectoryInfos[FirstDirectoryIndex + CurrentDirectoryIndex] = std::move(NewDirectoryInfo);
    //
    //        DocumentPathFileIterator TempIterator = DocumentPathFileIterator(BuildFiles, 1, FilesIterator.GetCurrentOffset(), FilesIterator.GetDirectoryEnd());
    //        TempIterator.NextDirectory();
    //        CurrentFileOffset += TempIterator.GetCurrentOffset();
    //
    //        CurrentDirectoryIndex++;
    //        FilesIterator.NextDirectory();
    //    }
    //    while (SubBuildIterator != SubBuildEnd)
    //    {
    //        DocumentDirectoryInfo NewDirectoryInfo = p_UpdateFilesystemOverBuild(SubBuildIterator->second, SubdirectoryFileBegin + CurrentFileOffset, FirstDirectoryIndex + CurrentDirectoryIndex, //SubBuildIterator->first, OutError);
    //        NewDirectoryInfo.ParentDirectoryIndex = DirectoryIndex;
    //        m_DirectoryInfos[FirstDirectoryIndex + CurrentDirectoryIndex] = std::move(NewDirectoryInfo);
    //
    //        DocumentPathFileIterator TempIterator = DocumentPathFileIterator(SubBuildIterator->second.BuildFiles, 0, 0, SubBuildIterator->second.BuildFiles.size());
    //        TempIterator.NextDirectory();
    //        CurrentFileOffset += TempIterator.GetCurrentOffset();
    //
    //        SubBuildIterator++;
    //        CurrentDirectoryIndex++;
    //    }
    //    assert(CurrentFileOffset == TotalSubdirectoryFiles);
    //    OutError = Result;
    //    return(NewInfo);
    //}
    //bool h_PairStringOrder(std::pair<std::string, DocumentFilesystem::BuildDirectory> const& Left, std::pair<std::string, DocumentFilesystem::BuildDirectory> Right)
    //{
    //    return(Left.first < Right.first);
    //}

    //DocumentFilesystem::BuildDirectory h_ParseBuildDirectory(std::vector<DocumentPath> const& Files,std::filesystem::path const& DirectoryPath, int Depth, size_t Begin, size_t End)
    //{
    //    DocumentFilesystem::BuildDirectory ReturnValue;
    //    ReturnValue.DirectoryPath = DirectoryPath;
    //    DocumentPathFileIterator PathIterator(Files, Depth, Begin, End);
    //    
    //    PathIterator.NextDirectory();
    //    
    //    for (size_t i = 0; i < PathIterator.GetCurrentOffset(); i++)
    //    {
    //        ReturnValue.FileNames.push_back(Files[Begin + i][Files[Begin + i].ComponentCount() - 1]);
    //    }
    //    while (!PathIterator.HasEnded())
    //    {
    //        std::string CurrentDirectoryName = PathIterator.GetDirectoryName();
    //        DocumentFilesystem::BuildDirectory NewDirectory = h_ParseBuildDirectory(Files,DirectoryPath/CurrentDirectoryName, Depth + 1, Begin + PathIterator.GetCurrentOffset(),Begin+PathIterator.GetDirectoryEnd());
    //        ReturnValue.SubDirectories.push_back(std::pair<std::string, DocumentFilesystem::BuildDirectory>(CurrentDirectoryName, std::move(NewDirectory)));
    //        PathIterator.NextDirectory();
    //    }

    //    return(ReturnValue);
    //}

    //DocumentFilesystem::BuildDirectory DocumentFilesystem::p_ParseBuildDirectory(DocumentBuild const& BuildToParse)
    //{
    //    BuildDirectory ReturnValue;
    //    ReturnValue.DirectoryPath = BuildToParse.BuildRootDirectory;
    //    DocumentPathFileIterator PathIterator(BuildToParse.BuildFiles,0,0,BuildToParse.BuildFiles.size());
    //    PathIterator.NextDirectory();
    //    for (size_t i = 0; i < PathIterator.GetCurrentOffset(); i++)
    //    {
    //        ReturnValue.FileNames.push_back(BuildToParse.BuildFiles[i][BuildToParse.BuildFiles[i].ComponentCount() - 1]);
    //    }
    //    while (PathIterator.HasEnded() == false)
    //    {
    //        std::string DirectoryName = PathIterator.GetDirectoryName();
    //        DocumentFilesystem::BuildDirectory NewDirectory = h_ParseBuildDirectory(BuildToParse.BuildFiles,BuildToParse.BuildRootDirectory/DirectoryName, 1, PathIterator.GetCurrentOffset(), PathIterator.GetDirectoryEnd());
    //        ReturnValue.SubDirectories.push_back(std::pair<std::string, DocumentFilesystem::BuildDirectory>(std::move(DirectoryName), std::move(NewDirectory)));
    //        PathIterator.NextDirectory();
    //    }
    //    for (auto const& SubBuild : BuildToParse.SubBuilds)
    //    {
    //        ReturnValue.SubDirectories.push_back(std::pair<std::string, BuildDirectory>(SubBuild.first, p_ParseBuildDirectory(SubBuild.second)));
    //    }
    //    std::sort(ReturnValue.SubDirectories.begin(), ReturnValue.SubDirectories.end(), h_PairStringOrder);
    //    return(ReturnValue);
    //}
    std::vector<size_t> h_GetOriginalMap(std::vector<size_t> const& VectorToMap)
    {
        std::vector<size_t> ReturnValue = std::vector<size_t>(VectorToMap.size(),0);
        for(size_t i = 0; i < VectorToMap.size();i++)
        {
            ReturnValue[VectorToMap[i]] = i;    
        }
        return(ReturnValue);
    }
    size_t DocumentFilesystem::p_GetDirectoryFiles(DocumentBuild const& Directory)
    {
        size_t TotalSubFiles = 0;
        for (auto const& SubDirectory : Directory.SubDirectories) 
        {
            TotalSubFiles += SubDirectory.second.DirectoryFiles.size();
        }
        return TotalSubFiles;
    }
    DocumentDirectoryInfo DocumentFilesystem::p_UpdateOverDirectory(DocumentBuild const& Directory, IndexType FileIndexBegin, IndexType DirectoryIndex,bool ParseSource)
    {
        DocumentDirectoryInfo ReturnValue;
        IndexType NewDirectoryIndexBegin = m_DirectoryInfos.size();
        ReturnValue.DirectoryIndexBegin = NewDirectoryIndexBegin;
        ReturnValue.DirectoryIndexEnd = NewDirectoryIndexBegin+Directory.SubDirectories.size();
        ReturnValue.FileIndexBegin = FileIndexBegin;
        ReturnValue.FileIndexEnd = FileIndexBegin+Directory.DirectoryFiles.size();
        
        MBError ParseError = true;
        IndexType FileIndex = 0;
        std::vector<size_t> SortedFileIndexes = std::vector<size_t>(Directory.DirectoryFiles.size(),0);
        std::iota(SortedFileIndexes.begin(),SortedFileIndexes.end(),0);
        std::sort(SortedFileIndexes.begin(),SortedFileIndexes.end(),[&](size_t Left,size_t Right)
                {
                    return(Directory.DirectoryFiles[Left].filename() < Directory.DirectoryFiles[Right].filename());     
                });
        //This part assumes that the names of the files are sorted
        //for (std::filesystem::path const& File : Directory.DirectoryFiles)
        for (size_t i = 0; i < SortedFileIndexes.size();i++)
        {
            size_t SortedIndex = SortedFileIndexes[i];   
            std::filesystem::path const& File = Directory.DirectoryFiles[SortedIndex];
            DocumentParsingContext Parser;
            DocumentSource NewFile;
            if(ParseSource)
            {
                NewFile = Parser.ParseSource(File, ParseError);
            }
            if (!ParseError)
            {
                throw std::runtime_error("Error parsing file "+MBUnicode::PathToUTF8(File) +": " + ParseError.ErrorMessage);
            }
            //library bug? seems like std::move(NewFile) produces a file with empty path on msvc, weird
            m_TotalSources[FileIndexBegin + FileIndex].Document = std::move(NewFile);
            m_TotalSources[FileIndexBegin + FileIndex].Document.Path = File;
            m_TotalSources[FileIndexBegin + FileIndex].Document.Name = MBUnicode::PathToUTF8(File.filename());
            m_TotalSources[FileIndexBegin + FileIndex].Document.Timestamp = 
                std::filesystem::directory_entry(File).last_write_time().time_since_epoch().count();
            FileIndex++;
        }
        auto OriginalFileMap = h_GetOriginalMap(SortedFileIndexes);
        for(size_t i = 0; i < OriginalFileMap.size();i++)
        {
            if( i == 0)
            {
                ReturnValue.FirstFileIndex = OriginalFileMap[i];
            }
            else
            {
                m_TotalSources[FileIndexBegin+OriginalFileMap[i-1]].NextFile = OriginalFileMap[i];
            }
        }
        size_t TotalSubFiles = p_GetDirectoryFiles(Directory);

        IndexType NewFileIndexBegin = m_TotalSources.size();
        m_TotalSources.resize(TotalSubFiles + m_TotalSources.size());
        m_DirectoryInfos.resize(m_DirectoryInfos.size() + Directory.SubDirectories.size());
        IndexType DirectoryOffset = 0;
        IndexType FileOffset = 0;


        std::vector<size_t> SortedDirectoryIndexes = std::vector<size_t>(Directory.SubDirectories.size(),0);
        std::iota(SortedDirectoryIndexes.begin(),SortedDirectoryIndexes.end(),0);
        std::sort(SortedDirectoryIndexes.begin(),SortedDirectoryIndexes.end(),[&](size_t Left,size_t Right)
                {
                    return(Directory.SubDirectories[Left].first < Directory.SubDirectories[Right].first);     
                });
        IndexType HomeIndex = -1;
        if(Directory.Name != "")
        {
            HomeIndex = m_HomeIntervalls.size();   
            m_HomeIntervalls.emplace_back();
            m_HomeIntervalls.back().Name = Directory.Name;
            m_HomeIntervalls.back().DirIndex = DirectoryIndex;
        }
        for(size_t i = 0; i < SortedDirectoryIndexes.size();i++)
        {
            IndexType PrevHomeSize = m_HomeIntervalls.size();
            size_t SortedDirectoryIndex = SortedDirectoryIndexes[i];    
            auto const& SubDirectory = Directory.SubDirectories[SortedDirectoryIndex];
            m_DirectoryInfos[NewDirectoryIndexBegin + DirectoryOffset] = p_UpdateOverDirectory(SubDirectory.second, NewFileIndexBegin + FileOffset, NewDirectoryIndexBegin + DirectoryOffset,ParseSource);
            m_DirectoryInfos[NewDirectoryIndexBegin + DirectoryOffset].Name = SubDirectory.first;
            m_DirectoryInfos[NewDirectoryIndexBegin + DirectoryOffset].ParentDirectoryIndex = DirectoryIndex;
            if(PrevHomeSize < m_HomeIntervalls.size() && HomeIndex != -1)
            {
                m_HomeIntervalls[PrevHomeSize].ParentIndex = HomeIndex;
                m_HomeIntervalls[HomeIndex].Children.push_back(PrevHomeSize);
            }
            DirectoryOffset++;
            FileOffset += SubDirectory.second.DirectoryFiles.size();
        }
        auto OriginalDirectoryMap = h_GetOriginalMap(SortedDirectoryIndexes);
        for(size_t i = 0; i < SortedDirectoryIndexes.size();i++)
        {
            if( i == 0)
            {
                ReturnValue.FirstSubDirIndex = OriginalDirectoryMap[i];
            }
            else
            {
                m_DirectoryInfos[NewDirectoryIndexBegin+OriginalDirectoryMap[i-1]].NextDirectory = OriginalDirectoryMap[i];
            }
        }
        return(ReturnValue);
    }
    bool h_DirectoryOrder(DocumentDirectoryInfo const& Left, DocumentDirectoryInfo const& Right)
    {
        if (Left.FileIndexBegin < Right.FileIndexBegin || Left.FileIndexEnd < Right.FileIndexEnd)
        {
            return(true);
        }
        return(false);
    }
    void DocumentFilesystem::__PrintDirectoryStructure(DocumentDirectoryInfo const& CurrentDir,int Depth) const
    {
        std::cout << std::string(4 * Depth, ' ')<<CurrentDir.Name<<"\n";
        for (size_t i = CurrentDir.FileIndexBegin; i < CurrentDir.FileIndexEnd;i++)
        {
            std::cout << std::string(4 * (Depth + 1), ' ') << m_TotalSources[i].Document.Name << std::endl;
        }
        for (size_t i = CurrentDir.DirectoryIndexBegin; i < CurrentDir.DirectoryIndexEnd; i++)
        {
            __PrintDirectoryStructure(m_DirectoryInfos[i], Depth + 1);
        }

    }
    void DocumentFilesystem::__PrintDirectoryStructure() const
    {
        DocumentDirectoryInfo const& CurrentInfo = m_DirectoryInfos[0];
        __PrintDirectoryStructure(CurrentInfo, 0);
    }
    void DocumentFilesystem::p_CreateDirectoryStructure(DocumentFilesystem& OutFilesystem,DocumentBuild const& BuildToParse,bool ParseSources)
    {
        OutFilesystem.m_DirectoryInfos.resize(1);
        OutFilesystem.m_HomeIntervalls.resize(1);
        OutFilesystem.m_HomeIntervalls.front().DirIndex = 0;
        OutFilesystem.m_HomeIntervalls.front().ParentIndex = -1;
        OutFilesystem.m_HomeIntervalls.front().Name = "/";
        //BuildDirectory Directory = p_ParseBuildDirectory(BuildToParse);
        OutFilesystem.m_TotalSources.reserve(BuildToParse.GetTotalFiles());
        OutFilesystem.m_TotalSources.resize(OutFilesystem.m_TotalSources.size()+BuildToParse.DirectoryFiles.size());
        DocumentDirectoryInfo TopInfo = OutFilesystem.p_UpdateOverDirectory(BuildToParse, 0, 0,ParseSources);
        if(OutFilesystem.m_HomeIntervalls.size() > 1)
        {
            OutFilesystem.m_HomeIntervalls.front().Children.push_back(1);   
        }
        OutFilesystem.m_DirectoryInfos[0] = TopInfo;
        OutFilesystem.m_DirectoryInfos[0].Name = "/";
        assert(std::is_sorted(OutFilesystem.m_DirectoryInfos.begin(), OutFilesystem.m_DirectoryInfos.end(), h_DirectoryOrder));
    }
    void DocumentFilesystem::p_PostProcessSources(DocumentFilesystem& OutFilesystem, LSPInfo const& LSPConf, ProcessedColorConfiguration const& ColorConf,std::vector<IndexType> const& SourcesToUpdate)
    {
        if(SourcesToUpdate.size() == 0)
        {
            return;   
        }
        OutFilesystem.p_ColorizeLSP(ColorConf,LSPConf,SourcesToUpdate);
        OutFilesystem.p_ResolveReferences(SourcesToUpdate);
    }
    MBError DocumentFilesystem::CreateDocumentFilesystem(DocumentBuild const& BuildToParse,LSPInfo const& LSPConf,ProcessedColorConfiguration const& 
                ColorConf,DocumentFilesystem& OutFilesystem,bool ParseSource,bool Colorize)
    {
        MBError ReturnValue = true;
        try 
        {
            DocumentFilesystem Result;
            p_CreateDirectoryStructure(Result,BuildToParse,ParseSource);
            if(Colorize)
            {
                Result.p_ColorizeLSP(ColorConf,LSPConf,{});
            }
            Result.p_ResolveReferences({}); 
            if (ReturnValue)
            {
                OutFilesystem = std::move(Result);
            }

        }
        catch (std::exception const& e) 
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = e.what();
        }
        return(ReturnValue);  
    }
    void DocumentFilesystem::p_UpdateDirectoryFiles(DocumentFilesystem& NewFilesystem,DocumentFilesystem& OldFilesystem,DocumentDirectoryInfo const& NewDir,DocumentDirectoryInfo const& OldDir,std::vector<IndexType>& UpdatedFiles,std::vector<bool>& NewFileUpdated)
    {
        IndexType NewFileIndex = NewDir.FileIndexBegin;
        IndexType OldFileIndex = OldDir.FileIndexBegin;
        while(NewFileIndex < NewDir.FileIndexEnd && OldFileIndex < OldDir.FileIndexEnd)
        {
            FilesystemDocumentInfo& NewFile = NewFilesystem.m_TotalSources[NewFileIndex];
            FilesystemDocumentInfo& OldFile = OldFilesystem.m_TotalSources[OldFileIndex];
            if(NewFile.Document.Name < OldFile.Document.Name)
            {
                UpdatedFiles.push_back(NewFileIndex);
                NewFileIndex++;
            }
            else if(OldFile.Document.Name < NewFile.Document.Name)
            {
                OldFileIndex++;
            }
            else if(NewFile.Document.Name == OldFile.Document.Name)
            {
                bool FileUpdated = false;
                if(NewFile.Document.Path != OldFile.Document.Path || NewFile.Document.Timestamp > OldFile.Document.Timestamp)
                {
                    FileUpdated = true;
                }
                else
                {
                    //check wheter all references of the old file exists, if it has a reference part also 
                    //ensure that the refernced file hasnt been updated.
                    for(auto const& ExtRef : OldFile.Document.ExternalReferences)
                    {
                        IndexType FileIndex = 0;
                        if(!NewFilesystem.DocumentExists(ExtRef,FileIndex))
                        {
                            FileUpdated = true;
                        }
                        else
                        {
                            //check existance, WIP
                        }
                    }
                }
                if(!FileUpdated)
                {
                    std::swap(NewFile.Document,OldFile.Document);
                }
                else
                {
                    UpdatedFiles.push_back(NewFileIndex);   
                }
                OldFileIndex++;
                NewFileIndex++;
            }
        }
        while(NewFileIndex != NewDir.FileIndexEnd)
        {
            UpdatedFiles.push_back(NewFileIndex);
            NewFileIndex++;
        }
    }
    void DocumentFilesystem::p_UpdateOverDirectory(DocumentFilesystem& NewFilesystem,DocumentFilesystem& OldFilesystem,IndexType NewDirIndex,IndexType OldDirIndex,std::vector<IndexType>& UpdatedFiles,std::vector<bool>& NewFileUpdated)
    {
        DocumentDirectoryInfo const& NewDir = NewFilesystem.m_DirectoryInfos[NewDirIndex];
        DocumentDirectoryInfo const& OldDir = OldFilesystem.m_DirectoryInfos[OldDirIndex];
        p_UpdateDirectoryFiles(NewFilesystem,OldFilesystem,NewDir,OldDir,UpdatedFiles,NewFileUpdated);
        IndexType NewSubDirIndex = NewDir.DirectoryIndexBegin;
        IndexType OldSubDirIndex = OldDir.DirectoryIndexBegin;
        while(NewSubDirIndex < NewDir.DirectoryIndexEnd && OldSubDirIndex < OldDir.DirectoryIndexEnd)
        {
            DocumentDirectoryInfo const& NewSubDir = NewFilesystem.m_DirectoryInfos[NewSubDirIndex];
            DocumentDirectoryInfo const& OldSubDir = OldFilesystem.m_DirectoryInfos[OldDirIndex];
            if(NewSubDir.Name < OldSubDir.Name)
            {
                //all files are to be considered new
                if(NewSubDir.DirectoryIndexBegin != NewSubDir.DirectoryIndexEnd)
                {
                    IndexType FileBegin = NewFilesystem.m_DirectoryInfos[NewSubDir.DirectoryIndexBegin].FileIndexBegin;
                    IndexType FileEnd = NewFilesystem.m_DirectoryInfos[NewSubDir.DirectoryIndexEnd-1].FileIndexEnd;
                    for(auto i = FileBegin; i < FileEnd;i++)
                    {
                        UpdatedFiles.push_back(i);
                        NewFileUpdated[i] = true;
                    }
                }
                NewSubDirIndex++;
            }
            else if(OldSubDir.Name < NewSubDir.Name)
            {
                OldSubDirIndex++;
            }
            else
            {
                p_UpdateOverDirectory(NewFilesystem,OldFilesystem,NewSubDirIndex,OldSubDirIndex,UpdatedFiles,NewFileUpdated);
                NewSubDirIndex++;
                OldSubDirIndex++;
            }
        }
        while(NewSubDirIndex < NewDir.DirectoryIndexEnd)
        {

            NewSubDirIndex++;
        }
    }
    MBError DocumentFilesystem::CreateDocumentFilesystem(DocumentBuild const& BuildToParse,LSPInfo const& LSPConf,ProcessedColorConfiguration const& ColorConf,DocumentFilesystem& OutBuild,DocumentFilesystem& PreviousBuild,std::vector<IndexType>& OutOutdatedFiles)
    {
        MBError ReturnValue = true;
        try 
        {
            std::vector<IndexType> OutdatedFiles;
            std::vector NewFileUpdated = std::vector(OutBuild.m_TotalSources.size(),false);
            p_UpdateOverDirectory(OutBuild,PreviousBuild,0,0,OutdatedFiles,NewFileUpdated);
            MBError ParseError = true;
            for(auto const& Index : OutdatedFiles)
            {
                DocumentParsingContext Parser;
                OutBuild.m_TotalSources[Index].Document = Parser.ParseSource(OutBuild.m_TotalSources[Index].Document.Path, ParseError);
                if(!ParseError)
                {
                    return ParseError;   
                }
            }
            p_PostProcessSources(OutBuild,LSPConf,ColorConf,OutdatedFiles);
            std::swap(OutOutdatedFiles,OutdatedFiles);
        }
        catch (std::exception const& e) 
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = e.what();
        }
        return(ReturnValue);  
    }
    DocumentFilesystem DocumentFilesystem::CreateDefaultFilesystem()
    {
        DocumentFilesystem ReturnValue;
        auto& TopDir = ReturnValue.m_DirectoryInfos.emplace_back();
        TopDir.DirectoryIndexBegin = 1;
        TopDir.DirectoryIndexEnd = 1;
        TopDir.FileIndexBegin = 0;
        TopDir.FileIndexEnd = 0;
        TopDir.FirstFileIndex = 0;
        TopDir.FirstSubDirIndex = 0;
        TopDir.Name = "/";
        return ReturnValue;
    }
    MBParsing::JSONObject ToJSON(DocumentSource const& Source)
    {
        MBParsing::JSONObject Result(MBParsing::JSONObjectType::Aggregate);
        Result["Path"] = MBUnicode::PathToUTF8(Source.Path);
        Result["Name"] = Source.Name;
        Result["Title"] = Source.Title;
        Result["Timestamp"] = intmax_t(Source.Timestamp);
        std::vector<MBParsing::JSONObject> ExternalReferences;
        for(auto const& Reference : Source.ExternalReferences)
        {
            ExternalReferences.emplace_back(Reference.GetString());
        }
        Result["ExternalReferences"] = std::move(ExternalReferences);
        Result["ReferenceTargets"] = ReferenceTargetsResolver::ToJSON(Source.ReferenceTargets);
        return Result;
    }
    MBParsing::JSONObject ToJSON(DocumentDirectoryInfo const& Source)
    {
        MBParsing::JSONObject Result(MBParsing::JSONObjectType::Aggregate);
        Result["Name"] = MBParsing::ToJSON(Source.Name);
        Result["ParentDirectoryIndex"] = MBParsing::ToJSON(Source.ParentDirectoryIndex);
        Result["FileIndexBegin"] = MBParsing::ToJSON(Source.FileIndexBegin);
        Result["FileIndexEnd"] = MBParsing::ToJSON(Source.FileIndexEnd);
        Result["DirectoryIndexBegin"] = MBParsing::ToJSON(Source.DirectoryIndexBegin);
        Result["DirectoryIndexEnd"] = MBParsing::ToJSON(Source.DirectoryIndexEnd);
        Result["NextDirectory"] = MBParsing::ToJSON(Source.NextDirectory);
        Result["FirstFileIndex"] = MBParsing::ToJSON(Source.FirstFileIndex);
        Result["FirstSubDirIndex"] = MBParsing::ToJSON(Source.FirstSubDirIndex);
        return Result;
    }
    MBParsing::JSONObject ToJSON(DocumentFilesystem const& Filesystem)
    {
        MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
        ReturnValue["ResourceMap"] = DocumentFilesystem::ResourceMap::ToJSON(Filesystem.m_ResourceMap);
        ReturnValue["Files"] = MBParsing::ToJSON(Filesystem.m_TotalSources);
        ReturnValue["Directories"] = MBParsing::ToJSON(Filesystem.m_DirectoryInfos);
        ReturnValue["HomeIntervalls"] = MBParsing::ToJSON(Filesystem.m_HomeIntervalls);
        return ReturnValue;
    }
    void FromJSON(DocumentSource& Source, MBParsing::JSONObject const& ObjectToParse)
    {
        std::string PathString;
        MBParsing::FromJSON(PathString,ObjectToParse["Path"]);
        Source.Path = PathString;
        MBParsing::FromJSON(Source.Name,ObjectToParse["Name"]);
        MBParsing::FromJSON(Source.Timestamp,ObjectToParse["Timestamp"]);
        MBParsing::FromJSON(Source.Title,ObjectToParse["Title"]);
        std::vector<std::string> ReferenceStrings;
        MBParsing::FromJSON(ReferenceStrings,ObjectToParse["ExternalReferences"]);
        for(auto const& Reference : ReferenceStrings)
        {
            MBError Result = true;
            Source.ExternalReferences.push_back(DocumentPath::ParsePath(Reference,Result));
            if(!Result)
            {
                throw std::runtime_error("Error parsing reference: "+Result.ErrorMessage);   
            }
        }
        ReferenceTargetsResolver::FromJSON(Source.ReferenceTargets,ObjectToParse["ReferenceTargets"]);
    }
    void FromJSON(DocumentDirectoryInfo& Directory, MBParsing::JSONObject const& ObjectToParse)
    {
        MBParsing::FromJSON(Directory.Name,ObjectToParse["Name"]);
        MBParsing::FromJSON(Directory.ParentDirectoryIndex,ObjectToParse["ParentDirectoryIndex"]);
        MBParsing::FromJSON(Directory.FileIndexBegin,ObjectToParse["FileIndexBegin"]);
        MBParsing::FromJSON(Directory.FileIndexEnd,ObjectToParse["FileIndexEnd"]);
        MBParsing::FromJSON(Directory.DirectoryIndexBegin,ObjectToParse["DirectoryIndexBegin"]);
        MBParsing::FromJSON(Directory.DirectoryIndexEnd,ObjectToParse["DirectoryIndexEnd"]);
        MBParsing::FromJSON(Directory.NextDirectory,ObjectToParse["NextDirectory"]);
        MBParsing::FromJSON(Directory.FirstFileIndex,ObjectToParse["FirstFileIndex"]);
        MBParsing::FromJSON(Directory.FirstSubDirIndex,ObjectToParse["FirstSubDirIndex"]);
    }
    void FromJSON(DocumentFilesystem& Filesystem, MBParsing::JSONObject const& ObjectToParse)
    {
        MBParsing::FromJSON(Filesystem.m_DirectoryInfos,ObjectToParse["Directories"]);
        DocumentFilesystem::ResourceMap::FromJSON(Filesystem.m_ResourceMap,ObjectToParse["ResourceMap"]);
        MBParsing::FromJSON(Filesystem.m_TotalSources,ObjectToParse["Files"]);
        MBParsing::FromJSON(Filesystem.m_HomeIntervalls,ObjectToParse["HomeIntervalls"]);
        if(!Filesystem.p_VerifyParsedFilesystem())
        {
            throw std::runtime_error("Error reading BuildInfo.json: invalid serialized state");
        }
    }
    bool DocumentFilesystem::p_VerifyParsedFilesystem()
    {
        bool ReturnValue = true;
        if(m_DirectoryInfos.size() < 0)
        {
            return false;   
        }
        if(m_DirectoryInfos[0].ParentDirectoryIndex != -1)
        {
            return false;   
        }
        DocumentDirectoryInfo const* PreviousDirectory = nullptr;
        //Verify directories
        for(int i = 0; i < m_DirectoryInfos.size();i++)
        {
            auto const& CurrentDirInfo = m_DirectoryInfos[i];
            if(!(CurrentDirInfo.FileIndexBegin <= CurrentDirInfo.FileIndexEnd) || CurrentDirInfo.FileIndexBegin < 0 || CurrentDirInfo.FileIndexEnd < 0 || CurrentDirInfo.FileIndexEnd > m_TotalSources.size())
            {
                return false;
            }
            if(!(CurrentDirInfo.DirectoryIndexBegin <= CurrentDirInfo.DirectoryIndexEnd) || CurrentDirInfo.DirectoryIndexBegin < 0 || CurrentDirInfo.DirectoryIndexEnd < 0 || CurrentDirInfo.DirectoryIndexEnd > m_DirectoryInfos.size())
            {
                return false;
            }
            if(! (CurrentDirInfo.DirectoryIndexBegin >= i))
            {
                return false;
            }
            if( (CurrentDirInfo.FirstFileIndex != -1 && CurrentDirInfo.FirstFileIndex < 0) || (CurrentDirInfo.FirstFileIndex >= (CurrentDirInfo.FileIndexEnd-CurrentDirInfo.FileIndexBegin)))
            {
                return false;   
            }
            if( (CurrentDirInfo.FirstSubDirIndex != -1 && CurrentDirInfo.FirstSubDirIndex < 0) || (CurrentDirInfo.FirstSubDirIndex >= (CurrentDirInfo.DirectoryIndexEnd-CurrentDirInfo.DirectoryIndexBegin)))
            {
                return false;   
            }
            if( (CurrentDirInfo.ParentDirectoryIndex != -1 && CurrentDirInfo.ParentDirectoryIndex < 0) || CurrentDirInfo.ParentDirectoryIndex >= i || 
                    (CurrentDirInfo.ParentDirectoryIndex >= 0 && CurrentDirInfo.ParentDirectoryIndex >= m_DirectoryInfos.size()))
            {
                return false;
            }
            //TODO should also check for cycles in NextDir...
            if(CurrentDirInfo.ParentDirectoryIndex != -1)
            {
                auto const& ParentDir = m_DirectoryInfos[CurrentDirInfo.ParentDirectoryIndex];
                if( !(ParentDir.DirectoryIndexBegin <= i && i < ParentDir.DirectoryIndexEnd))
                {
                    return false;   
                }
            }
            if(PreviousDirectory != nullptr)
            {
                auto const& PreviousDirectoryInfo = *PreviousDirectory;
                if(PreviousDirectoryInfo.DirectoryIndexEnd > CurrentDirInfo.DirectoryIndexBegin || PreviousDirectoryInfo.FileIndexEnd > CurrentDirInfo.FileIndexEnd)
                {
                    return false;   
                }
            }
            PreviousDirectory = &CurrentDirInfo;
        }
        //Verify home intervalls
        HomeIntervall const* PreviousHome = nullptr;
        for(int i = 0; i < m_HomeIntervalls.size();i++)
        {
            auto const& CurrentInterval = m_HomeIntervalls[i];
            if(CurrentInterval.DirIndex < 0 || CurrentInterval.DirIndex >= m_DirectoryInfos.size())
            {
                return false;   
            }
            for(auto const& ChildIndex : CurrentInterval.Children)
            {
                if(ChildIndex <= CurrentInterval.DirIndex || ChildIndex < 0 || ChildIndex >= m_HomeIntervalls.size())
                {
                    return false;   
                }
            }
            if( (CurrentInterval.ParentIndex != -1 && CurrentInterval.ParentIndex < 0) || CurrentInterval.ParentIndex >= i)
            {
                return false;   
            }
            if(CurrentInterval.ParentIndex != -1)
            {
                auto const& ParentInfo = m_HomeIntervalls[CurrentInterval.ParentIndex];
                bool ContainsThis = false;
                for(auto const& Child : ParentInfo.Children)
                {
                    if(Child == i)
                    {
                        ContainsThis = true;
                        break;   
                    }
                }
                if(!ContainsThis)
                {
                    return false;   
                }
            }
        }
        
        return ReturnValue;
    }

    DocumentFilesystem::DocumentFilesystemReferenceResolver::DocumentFilesystemReferenceResolver(DocumentFilesystem* AssociatedFilesystem,DocumentPath CurrentPath,std::filesystem::path DocumentPath,ResourceMap* ResourceMappinwg)
    {
        m_AssociatedFilesystem = AssociatedFilesystem;
        m_DocumentPath = DocumentPath;
        m_CurrentPath = CurrentPath;
        m_ResourceMapping = ResourceMappinwg;
    }
    void DocumentFilesystem::DocumentFilesystemReferenceResolver::Visit(UnresolvedReference& Ref)
    {
        MBError Result = true;
        DocumentPath NewPath = m_AssociatedFilesystem->ResolveReference(m_CurrentPath,Ref.ReferenceString,Result);
        m_ShouldUpdate = false;
        if(Result)
        {
            FileReference NewRef;
            NewRef.Color = Ref.Color;
            NewRef.Modifiers = Ref.Modifiers;
            NewRef.VisibleText = Ref.VisibleText;
            NewRef.Path = NewPath;
            NewRef.Begin = Ref.Begin;
            NewRef.End = Ref.End;
            NewRef.ReferenceBegin = Ref.ReferenceBegin;
            NewRef.ReferenceString = Ref.ReferenceString;
            m_Result = TextElement(std::unique_ptr<DocReference>(new FileReference(std::move(NewRef))));
            m_ShouldUpdate = true;
        }
        else
        {
            if(std::filesystem::is_regular_file(m_DocumentPath.parent_path()/Ref.ReferenceString))
            {
                ResourceReference NewRef;
                NewRef.Color = Ref.Color;
                NewRef.Modifiers = Ref.Modifiers;
                NewRef.VisibleText = Ref.VisibleText;
                NewRef.Begin = Ref.Begin;
                NewRef.End = Ref.End;
                NewRef.ReferenceBegin = Ref.ReferenceBegin;
                NewRef.ResourceCanonicalPath = MBUnicode::PathToUTF8(std::filesystem::canonical(m_DocumentPath.parent_path()/Ref.ReferenceString));
                NewRef.ID = m_ResourceMapping->GetResourceID(NewRef.ResourceCanonicalPath);
                m_Result = TextElement(std::unique_ptr<DocReference>(new ResourceReference(std::move(NewRef))));
                m_ShouldUpdate = true;
            }
        }
    }
    void DocumentFilesystem::DocumentFilesystemReferenceResolver::ResolveReference(TextElement& ReferenceToResolve)
    {
        m_ShouldUpdate = false;
        DocReference& Ref = ReferenceToResolve.GetType<DocReference>();
        Ref.Accept(*this);
        if(m_ShouldUpdate)
        {
           ReferenceToResolve = std::move(m_Result); 
        } 
    }
    void DocumentFilesystem::DocumentFilesystemReferenceResolver::Visit(CodeBlock& CodeBlock)
    {
        if(CodeBlock.HighlightedText)
        {
            for(auto& Row : CodeBlock.HighlightedText.Value())
            {
                for(auto& Element : Row)
                {
                    if(Element.IsType<DocReference>())
                    {
                        ResolveReference(Element);
                    }
                }
            }
        }
    }
    void DocumentFilesystem::DocumentFilesystemReferenceResolver::Visit(MediaInclude& MediaInclude) 
    {
        std::filesystem::path RelativePath = m_DocumentPath.parent_path()/MediaInclude.MediaPath;
        if(std::filesystem::exists(RelativePath))
        {
            MediaInclude.MediaPath = MBUnicode::PathToUTF8(std::filesystem::canonical(RelativePath));
        }
        else
        {
            MediaInclude.MediaPath = MBUnicode::PathToUTF8(RelativePath);
        }
        MediaInclude.ID = m_ResourceMapping->GetResourceID(MediaInclude.MediaPath);
    }
    void DocumentFilesystem::p_ResolveReferences(std::vector<IndexType> const& ModifiedSources)
    {
        DocumentFilesystemIterator Iterator(0); 
        Iterator.m_AssociatedFilesystem = this;
        DocumentTraverser Traverser; 
        std::unordered_set<bool> ModifiedMap = std::unordered_set<bool>(ModifiedSources.begin(),ModifiedSources.end());
        while(Iterator.HasEnded() == false)
        {
            if(!Iterator.EntryIsDirectory())
            {
                DocumentPath CurrentPath = Iterator.GetCurrentPath();
                //yikes
                DocumentSource& CurrentDoc = const_cast<DocumentSource&>(Iterator.GetDocumentInfo());
                if(ModifiedSources.size() == 0 || ModifiedMap.find(Iterator.GetCurrentFileIndex())  != ModifiedMap.end())
                {
                    DocumentFilesystemReferenceResolver Resolver(this,CurrentPath,CurrentDoc.Path,&m_ResourceMap);
                    Traverser.Traverse(CurrentDoc,Resolver);
                }
            }
            Iterator++; 
        }
    }
    //mega slow, should improve
    IndexType DocumentFilesystem::p_InsertNewDirectories(IndexType ParentDir,int DirOffset,DocumentPath const& Documentpath)
    {
        //TODO take into considerations home intervalls
        //returns directory index, and file index
        //IndexType ReturnValue = m_DirectoryInfos[ParentDir].;
        auto& DirectoryParent = m_DirectoryInfos[ParentDir];
        auto NewDirPosition = std::lower_bound(m_DirectoryInfos.begin()+DirectoryParent.DirectoryIndexBegin,m_DirectoryInfos.begin()+DirectoryParent.DirectoryIndexEnd,Documentpath[DirOffset]);
        IndexType NewDirIndex = NewDirPosition-m_DirectoryInfos.begin();
        IndexType FirstChildDirPosition = 0;
        IndexType NewFilePosition = 0;
        if(NewDirIndex != DirectoryParent.DirectoryIndexEnd)
        {
            FirstChildDirPosition = m_DirectoryInfos[NewDirIndex].DirectoryIndexBegin;
            NewFilePosition = m_DirectoryInfos[NewDirIndex].FileIndexBegin;
        }
        else
        {
            //+1 because we now take this position
            FirstChildDirPosition = DirectoryParent.DirectoryIndexEnd+1;
            NewFilePosition = m_TotalSources.size();
        }
        for(int i = DirectoryParent.DirectoryIndexBegin; i < DirectoryParent.DirectoryIndexEnd;i++)
        {
            auto& Dir = m_DirectoryInfos[i];
            if(Dir.NextDirectory == -1)
            {
                Dir.NextDirectory = NewDirIndex-DirectoryParent.DirectoryIndexBegin;
                break;
            }
        }
        if(DirectoryParent.FirstSubDirIndex >= NewDirIndex && (DirectoryParent.DirectoryIndexEnd-DirectoryParent.DirectoryIndexBegin) != 0)
        {
            DirectoryParent.FirstSubDirIndex += 1;
        }

        DirectoryParent.DirectoryIndexEnd += 1;
        for(int i = NewDirIndex; i < m_DirectoryInfos.size();i++)
        {
            m_DirectoryInfos[i].DirectoryIndexBegin += 1;
            m_DirectoryInfos[i].DirectoryIndexEnd += 1;
            if(m_DirectoryInfos[i].ParentDirectoryIndex >= NewDirIndex && m_DirectoryInfos[i].ParentDirectoryIndex != -1)
            {
                m_DirectoryInfos[i].ParentDirectoryIndex += 1;
            }
        }
        DocumentDirectoryInfo FirstNewDir;
        FirstNewDir.ParentDirectoryIndex = ParentDir;
        FirstNewDir.DirectoryIndexBegin = FirstChildDirPosition;
        FirstNewDir.DirectoryIndexEnd = FirstChildDirPosition+ (Documentpath.ComponentCount()-DirOffset-2);
        FirstNewDir.FirstSubDirIndex = 0;
        FirstNewDir.FirstFileIndex = -1;
        FirstNewDir.FileIndexBegin = NewDirIndex != m_DirectoryInfos.size() ? m_TotalSources.size() : m_DirectoryInfos[NewDirIndex].FirstFileIndex;
        FirstNewDir.FileIndexEnd = FirstNewDir.FileIndexBegin;
        FirstNewDir.Name = Documentpath[DirOffset];
        //TODO update properly
        IndexType DirIndex = NewDirPosition-m_DirectoryInfos.begin();
        FirstNewDir.NextDirectory = -1;


        m_DirectoryInfos.insert(NewDirPosition,FirstNewDir);
        if(DirOffset+1 < Documentpath.ComponentCount()-1)
        {
            return p_InsertNewDirectories(DirIndex,DirOffset+1,Documentpath);
        }
        else
        {
            return NewDirIndex;
        }
    }
    IndexType DocumentFilesystem::InsertFile(DocumentPath const& Documentpath,DocumentSource const& NewSource)
    {
        IndexType ReturnValue = 0;

        int FirstNewDirectoryIndex = -1;
        IndexType ReplacedDirectoryParent = -1;
        IndexType CurrentDirIndex = 0;
        for(int i = 0; i < Documentpath.ComponentCount() - 1;i++)
        {
            auto const& CurrentDir = m_DirectoryInfos[CurrentDirIndex];
            auto SubDirIt = std::lower_bound(
                    m_DirectoryInfos.begin()+CurrentDir.DirectoryIndexBegin,m_DirectoryInfos.begin()+CurrentDir.DirectoryIndexEnd,Documentpath[i]);
            if(SubDirIt == m_DirectoryInfos.begin()+CurrentDir.DirectoryIndexEnd || SubDirIt->Name != Documentpath[i])
            {
                //new directories needs to be inserted
                FirstNewDirectoryIndex = i;
                break;
            }
            else
            {
                CurrentDirIndex = SubDirIt - m_DirectoryInfos.begin();
            }
        }
        IndexType FileDirectoryIndex = CurrentDirIndex;
        if(FirstNewDirectoryIndex != -1)
        {

            FileDirectoryIndex = p_InsertNewDirectories(FirstNewDirectoryIndex,FirstNewDirectoryIndex,Documentpath);
        }

        auto& FileDir = m_DirectoryInfos[FileDirectoryIndex];
        auto FilePosition = std::lower_bound(m_TotalSources.begin()+FileDir.FileIndexBegin,m_TotalSources.begin()+FileDir.FileIndexEnd,Documentpath[Documentpath.ComponentCount()-1]);
        ReturnValue = FilePosition-m_TotalSources.begin();
        for(int i = FileDir.FileIndexBegin; i < FileDir.FileIndexEnd && i < m_TotalSources.size();i++)
        {
            auto& File = m_TotalSources[i];
            if(File.NextFile == -1)
            {
                File.NextFile = ReturnValue-FileDir.FileIndexBegin;   
                break;
            }
        }
        if(FileDir.FirstFileIndex == -1)
        {
            FileDir.FirstFileIndex = 0;
        }
        else if(FileDir.FirstFileIndex >= ReturnValue)
        {
            FileDir.FirstFileIndex += 1;
        }
        FileDir.FileIndexEnd += 1;
        auto NewFile = m_TotalSources.insert(FilePosition,FilesystemDocumentInfo());
        NewFile->Document.ExternalReferences = NewSource.ExternalReferences;
        NewFile->Document.Name = NewSource.Name;
        NewFile->Document.Path = NewSource.Path;
        NewFile->Document.ReferenceTargets = NewSource.ReferenceTargets;
        NewFile->Document.Timestamp = NewSource.Timestamp;
        NewFile->Document.Title = NewSource.Title;
        
        int ExtraDirectoriesCount = FirstNewDirectoryIndex == -1 ? 0 : FirstNewDirectoryIndex+1;
        for(int i = FileDirectoryIndex+1; i < m_DirectoryInfos.size();i++)
        {
            auto& CurrentDir = m_DirectoryInfos[i];
            CurrentDir.FileIndexBegin += 1;
            CurrentDir.FileIndexEnd += 1;
        }
        
        return ReturnValue;
    }
    void DocumentFilesystem::ResolveReferences(DocumentPath const& DocumentPath,DocumentSource& SourceToUpdate)
    {
        DocumentFilesystemReferenceResolver Resolver(this,DocumentPath,SourceToUpdate.Path,&m_ResourceMap);
        DocumentTraverser Traverser;
        Traverser.Traverse(SourceToUpdate,Resolver);
    }
    std::vector<std::string> DocumentFilesystem::GetCompletions(IndexType FileID,std::string const& ReferenceString)
    {
        std::vector<std::string> ReturnValue;
        auto FilePath = p_GetFileIndexPath(FileID);
        MBError ParseResult(true);
        DocumentReference OriginalRef = DocumentReference::ParseReference(ReferenceString,ParseResult);
        if(!ParseResult)
        {
            return ReturnValue;   
        }
        bool ResolveResult = true;
        if(OriginalRef.PartSpecifier.size() != 0)
        {
            FSSearchResult ResolvedPath;
            OriginalRef.PartSpecifier.clear();
            if(OriginalRef.PathSpecifiers.size() != 0 || OriginalRef.HomeSpecifier.size() != 0)
            {
                ResolvedPath =  p_ResolvePath(FilePath,OriginalRef,&ResolveResult);
            }
            else
            {
                ResolvedPath.Index = FileID;
                ResolvedPath.Type = DocumentFSType::File;
            }
            if(ResolvedPath.Index != -1 && ResolvedPath.Type == DocumentFSType::File)
            {
                auto const& Source = m_TotalSources[ResolvedPath.Index];
                ReturnValue = Source.Document.ReferenceTargets.GetAllLabelNames();
            }
        }
        else
        {
            IndexType ResultDirectoryIndex = -1;
            if(OriginalRef.PathSpecifiers.size() <= 1)
            {
                ResultDirectoryIndex = p_GetFileDirectoryIndex(FilePath);
            }
            else
            {
                if(OriginalRef.PathSpecifiers.back().PathNames.size() > 1)
                {
                    OriginalRef.PathSpecifiers.back().PathNames.pop_back();   
                }
                else
                {
                    OriginalRef.PathSpecifiers.pop_back();   
                }
                auto ResolvedPath = p_ResolvePath(FilePath,OriginalRef,&ResolveResult);
                if(!ResolveResult || ResolvedPath.Index == -1 || ResolvedPath.Type == DocumentFSType::Null)
                {
                    return ReturnValue;   
                }
                if(ResolvedPath.Type == DocumentFSType::Directory)
                {
                    //ResultDirectoryIndex = m_DirectoryInfos[ResolvedPath.Index].ParentDirectoryIndex;
                    ResultDirectoryIndex = ResolvedPath.Index;
                }
                else
                {
                    //parent resolves to file, child cannot exist
                }

            }
            if(ResultDirectoryIndex != -1)
            {
                auto const& Dir = m_DirectoryInfos[ResultDirectoryIndex];
                for(int i = Dir.DirectoryIndexBegin; i < Dir.DirectoryIndexEnd;i++)
                {
                    ReturnValue.push_back(m_DirectoryInfos[i].Name);
                }
                for(int i = Dir.FileIndexBegin; i < Dir.FileIndexEnd;i++)
                {
                    ReturnValue.push_back(m_TotalSources[i].Document.Name);
                }
            }

        }
        return ReturnValue;
    }
    IndexType DocumentFilesystem::GetPathFile(DocumentPath const& DocumentPath) const
    {
        return p_GetFileIndex(DocumentPath);
    }
    DocumentSource const& DocumentFilesystem::GetFile(IndexType ID) const
    {
        if(ID < 0 || ID >= m_TotalSources.size())
        {
            throw std::runtime_error("Invalid ID: "+std::to_string(ID));   
        }
        return m_TotalSources[ID].Document;
    }
    std::vector<std::pair<std::string,IndexType>> DocumentFilesystem::GetAllDocuments() const
    {
        std::vector<std::pair<std::string,IndexType>> ReturnValue;
        for(int i = 0; i < m_TotalSources.size();i++)
        {
            auto const& CurrentSource = m_TotalSources[i];
            auto& NewValue = ReturnValue.emplace_back();
            NewValue.first = MBUnicode::PathToUTF8(std::filesystem::canonical(CurrentSource.Document.Path));
            NewValue.second = i;
        }
        std::sort(ReturnValue.begin(),ReturnValue.end());
        return ReturnValue;
    }
    DocumentPath DocumentFilesystem::GetDocumentPath(IndexType FileID) const
    {
        if(FileID < 0 || FileID >= m_TotalSources.size())
        {
            throw std::runtime_error("Invalid file id: "+std::to_string(FileID));   
        }
        return p_GetFileIndexPath(FileID);
    }
    ResolvedCodeText DocumentFilesystem::p_CombineColorings(std::vector<Coloring> const& RegexColoring,std::vector<Coloring> const& LSPColoring,
            std::string const& OriginalContent,LineIndex const& Index,ProcessedColorInfo const& ColorInfo,LSPReferenceResolver* OptionalLSPResolver)
    {
        ResolvedCodeText ReturnValue;
        //for(auto const& Coloring : ColoringsToCombine)
        //{
        //    assert(std::is_sorted(Coloring.begin(), Coloring.end()));
        //}
        assert(std::is_sorted(RegexColoring.begin(),RegexColoring.end()));
        assert(std::is_sorted(LSPColoring.begin(),LSPColoring.end()));
        
        std::vector<Coloring> PrunedRegexColoring = RemoveDuplicates(LSPColoring,RegexColoring);
        std::vector<Coloring> TotalColorings;
        TotalColorings.resize(PrunedRegexColoring.size()+LSPColoring.size());
        std::merge(PrunedRegexColoring.begin(),PrunedRegexColoring.end(),LSPColoring.begin(),LSPColoring.end(),TotalColorings.begin());
        TotalColorings = RemoveDuplicates(TotalColorings); 
        assert(std::is_sorted(TotalColorings.begin(),TotalColorings.end()));
        std::vector<TextElement> CurrentRow;
        size_t ParseOffset = 0; 
        int ColoringOffset = 0;
        int LineOffset = 1;

        int FunctionIndex = -2;
        int ClassIndex = -2;
        auto FunctionIndexIt = ColorInfo.ColoringNameToIndex.find("function");
        auto ClassIndexIt = ColorInfo.ColoringNameToIndex.find("class");
        if(FunctionIndexIt != ColorInfo.ColoringNameToIndex.end())
        {
            FunctionIndex = FunctionIndexIt->second;   
        }
        if(ClassIndexIt != ColorInfo.ColoringNameToIndex.end())
        {
            ClassIndex = ClassIndexIt->second;   
        }
        while(ParseOffset < OriginalContent.size())
        {
            if(LineOffset < Index.LineCount() && Index[LineOffset] <= ParseOffset)
            {
                if(ParseOffset == Index[LineOffset])
                {
                    ParseOffset++; 
                }
                ReturnValue.m_Rows.push_back(std::move(CurrentRow));   
                CurrentRow.clear();
                LineOffset++;
            }
            else if(ColoringOffset < TotalColorings.size())
            {
                if(LineOffset < Index.LineCount() && Index[LineOffset] < TotalColorings[ColoringOffset].ByteOffset)
                {
                    RegularText NewElement;
                    NewElement.Text = std::string(OriginalContent.data()+ParseOffset,OriginalContent.data()+Index[LineOffset]);
                    NewElement.Color = ColorInfo.DefaultColor;
                    CurrentRow.push_back(std::move(NewElement));
                    ParseOffset = Index[LineOffset]+1;
                }
                else
                {
                    if(TotalColorings[ColoringOffset].ByteOffset != ParseOffset)
                    {
                        RegularText NewElement;
                        NewElement.Text = std::string(OriginalContent.data()+ParseOffset,OriginalContent.data()+TotalColorings[ColoringOffset].ByteOffset);
                        NewElement.Color = ColorInfo.DefaultColor;
                        CurrentRow.push_back(std::move(NewElement));
                    }
                    std::string VisibleText = std::string(OriginalContent.data()+TotalColorings[ColoringOffset].ByteOffset,
                            OriginalContent.data()+TotalColorings[ColoringOffset].ByteOffset+TotalColorings[ColoringOffset].Length);
 
                    ColorTypeIndex ColorType = TotalColorings[ColoringOffset].Color;
                    if(OptionalLSPResolver != nullptr && (ColorType == FunctionIndex || ColorType == ClassIndex) && OptionalLSPResolver->ResolvesReferences())
                    {
                        TextElement NewElement = OptionalLSPResolver->CreateReference(LineOffset-1,
                                TotalColorings[ColoringOffset].ByteOffset-(Index[LineOffset-1]+1),VisibleText,
                                ColorType == FunctionIndex ? CodeReferenceType::Function : CodeReferenceType::Class);
                        NewElement.GetBase().Color = ColorInfo.ColorMap[ColorType];
                        //if(NewElement.IsType<DocReference>())
                        //{
                        //    std::cout<<NewElement.GetType<DocReference>().VisibleText<<std::endl;   
                        //}
                        CurrentRow.push_back(std::move(NewElement));
                    }
                    else
                    {
                        RegularText NewElement;
                        NewElement.Text = VisibleText;
                        NewElement.Color = ColorInfo.ColorMap[ColorType];
                        CurrentRow.push_back(std::move(NewElement));
                    }
                    ParseOffset = TotalColorings[ColoringOffset].ByteOffset + TotalColorings[ColoringOffset].Length;
                    ColoringOffset++;
                }
            }
            else
            {
                if(LineOffset < Index.LineCount())
                {
                    RegularText NewElement;
                    NewElement.Color = ColorInfo.DefaultColor;
                    NewElement.Text = std::string(OriginalContent.data()+ParseOffset,OriginalContent.data()+Index[LineOffset]);
                    CurrentRow.push_back(std::move(NewElement));
                    ParseOffset = Index[LineOffset]+1;
                }
                else
                {
                    RegularText NewElement;
                    NewElement.Text = std::string(OriginalContent.data()+ParseOffset,OriginalContent.data()+OriginalContent.size());
                    NewElement.Color = ColorInfo.DefaultColor;
                    ParseOffset = OriginalContent.size();
                    CurrentRow.push_back(std::move(NewElement));
                }
            }
        }
        if(CurrentRow.size() != 0)
        {
            ReturnValue.m_Rows.push_back(std::move(CurrentRow));   
        }
        return(ReturnValue);
    }
    std::vector<Coloring> DocumentFilesystem::p_GetLSPColoring(MBLSP::LSP_Client& ClientToUse,std::string const& URI,std::string const& TextContent,ProcessedColorInfo const& ColorInfo
            ,LineIndex const& Index)
    //void DocumentFilesystem::p_ColorizeCodeBlock(MBLSP::LSP_Client& ClientToUse,CodeBlock& BlockToColorize)
    {
        std::vector<Coloring> ReturnValue;

        MBLSP::SemanticToken_Request TokenRequest;
        TokenRequest.params.textDocument.uri = URI;
        MBLSP::SemanticToken_Response Tokens = ClientToUse.SendRequest(TokenRequest);
        try
        {
            if(Tokens.result)
            {
                std::string const& Text = TextContent;
                std::vector<int> const& TokenData = Tokens.result->data;
                if(TokenData.size() % 5 != 0)
                {
                    throw std::runtime_error("Invalid token count, must be divisible by 5");
                }
                int CurrentLineIndex = 0;
                int CurrentTokensOffset = 0;
                int TokenOffset = 0;
                int LatestTokenEnd = 0;

                while(TokenOffset < TokenData.size())
                {
                    int Line = TokenData[TokenOffset];
                    int Offset = TokenData[TokenOffset+1];
                    int Length = TokenData[TokenOffset+2];
                    int Type = TokenData[TokenOffset+3];
                    if(Line < 0 || Offset < 0 || Length < 0 || Type < 0)
                    {
                        throw std::runtime_error("Only positive integers are allowed in semantic tokens");   
                    }
                    if(Line > 0)
                    {
                        CurrentTokensOffset = 0;   
                    }
                    CurrentLineIndex += Line;
                    CurrentTokensOffset += Offset;

                    int CurrentTokenBegin = Index[CurrentLineIndex]+CurrentTokensOffset;
                    if (CurrentLineIndex > 0)
                    {
                        CurrentTokenBegin += 1;
                    }
                    int CurrentTokenEnd = CurrentTokenBegin + Length;
                   
                    Coloring TextToAdd;   
                    TextToAdd.Length = Length;
                    //TextToAdd.Color = TextColor(0, 0, 255);
                    auto TypeIt = ColorInfo.ColoringNameToIndex.find(ClientToUse.GetSemanticTokenInfo().TokenIndexToName(Type));
                    if(TypeIt != ColorInfo.ColoringNameToIndex.end())
                    {
                        TextToAdd.Color = TypeIt->second;
                    }
                    TextToAdd.ByteOffset = CurrentTokenBegin;
                    ReturnValue.push_back(TextToAdd);
                    LatestTokenEnd = CurrentTokenEnd;
                    TokenOffset += 5;
                }
            }
        } 
        catch(std::exception const& e)
        {

        }
        return(ReturnValue);
    }
    std::vector<Coloring> DocumentFilesystem::p_GetRegexColorings(std::vector<Coloring> const& PreviousColorings,
            ProcessedRegexColoring const& RegexesToUse,
            std::vector<TextColor> const& ColorMap,
            std::string const& DocumentContent)
    {
        std::vector<Coloring> ReturnValue;
        size_t ParseOffset = 0;
        size_t PreviousColorIndex = 0;
        while(ParseOffset < DocumentContent.size())
        {
            size_t TextToInspectEnd = DocumentContent.size();
            if(PreviousColorIndex < PreviousColorings.size())
            {
                if(ParseOffset < PreviousColorings[PreviousColorIndex].ByteOffset)
                {
                    TextToInspectEnd = PreviousColorings[PreviousColorIndex].ByteOffset;
                }
                else if(ParseOffset >= PreviousColorings[PreviousColorIndex].ByteOffset && 
                        ParseOffset < PreviousColorings[PreviousColorIndex].ByteOffset+PreviousColorings[PreviousColorIndex].Length)
                {
                    //in previous coloring, skip
                    ParseOffset = PreviousColorings[PreviousColorIndex].ByteOffset+PreviousColorings[PreviousColorIndex].Length;
                    PreviousColorIndex++;
                    continue;
                }
                else
                {
                    PreviousColorIndex++;
                    continue;
                }
            }
            for(auto const& RegexCategory : RegexesToUse.Regexes)
            {
                ColorTypeIndex CurrentColor = RegexCategory.first;
                for(auto const& Regex : RegexCategory.second)
                {
                    std::sregex_iterator Begin = std::sregex_iterator(DocumentContent.begin()+ParseOffset,
                            DocumentContent.begin()+TextToInspectEnd, Regex);
                    std::sregex_iterator End;
                    while(Begin != End)
                    {
                        assert(Begin->size() == 1);
                        Coloring NewColoring;
                        NewColoring.ByteOffset = ParseOffset+Begin->position();
                        NewColoring.Color = CurrentColor;
                        NewColoring.Length = Begin->length();
                        ReturnValue.push_back(NewColoring);
                        ++Begin;
                    }
                }
            }
            ParseOffset = TextToInspectEnd;
        }
        //maybe uneccessary
        std::sort(ReturnValue.begin(),ReturnValue.end());
        return(ReturnValue);
    }
    void DocumentFilesystem::LSPReferenceResolver::SetLSP(MBLSP::LSP_Client* AssociatedLSP)
    {
        m_AssociatedLSP = AssociatedLSP;
    }
    void DocumentFilesystem::LSPReferenceResolver::SetDocumentURI(std::string const& NewURI)
    {
        //DidOpenTextDocument_Notification OpenNotification;
        //OpenNotification.params.textDocument.text = DocumentData;
        //OpenNotification.params.textDocument.uri = MBLSP::URLEncodePath(std::filesystem::current_path()/"asdasdasdasd.cpp");
        //m_CurrentDocument = OpenNotification.params.textDocument.uri;
        //m_AssociatedLSP->SendNotification(OpenNotification);
        m_CurrentDocument = NewURI;
    }
    bool DocumentFilesystem::LSPReferenceResolver::ResolvesReferences()
    {
        return(m_AssociatedLSP->GetServerCapabilites().definitionProvider.IsInitalized() && m_AssociatedLSP->GetServerCapabilites().definitionProvider.Value());
    }
    TextElement DocumentFilesystem::LSPReferenceResolver::CreateReference(int Line,int Offset,std::string const& VisibleText,
            CodeReferenceType ReferenceType)
    {
        TextElement ReturnValue;
        MBLSP::GotoDefinition_Request GotoDefinitionRequest;
        GotoDefinitionRequest.params.textDocument.uri = m_CurrentDocument;
        GotoDefinitionRequest.params.position.line = Line;
        GotoDefinitionRequest.params.position.character = Offset;
        MBLSP::GotoDefinition_Response Response = m_AssociatedLSP->SendRequest(GotoDefinitionRequest);
        //Multiple locations doesn't make alot of sense, and may be indicative of for example virtual functions
        if(Response.result && Response.result->size() == 1)
        {
            auto const& Result = Response.result.Value(); 
            UnresolvedReference Reference;
            Reference.VisibleText = VisibleText;
            Reference.ReferenceString = "../../{Code/";
            if(ReferenceType == CodeReferenceType::Class)
            {
                Reference.ReferenceString += "Classes";
            }
            else if(ReferenceType == CodeReferenceType::Function)
            {
                Reference.ReferenceString += "Functions";
            }
            Reference.ReferenceString += "}/"+VisibleText+".mbd";

            MBLSP::Location const& LocationToCheck = Result[0];
            std::filesystem::path AbsolutePath = MBLSP::URLDecodePath(LocationToCheck.uri); 
            //Kinda hacky, assumes the existance of mbpm...
            const char* MBPM_PACKET_PATH = std::getenv("MBPM_PACKETS_INSTALL_DIRECTORY");
            if(MBPM_PACKET_PATH == nullptr)
            {
                throw std::runtime_error("Cant resolve CodeBlock definitions without MBPM_PACKETS_INSTALL_DIRECTORY set, "
                       "assumes that CodeBlocks uses mbpm packetmanager");
            }
            std::filesystem::path PacketPath = MBPM_PACKET_PATH;
            std::string RelativePath = MBUnicode::PathToUTF8(std::filesystem::relative(AbsolutePath,PacketPath));
            std::replace(RelativePath.begin(), RelativePath.end(), '\\', '/');
            if(RelativePath.size() >= 2 && !(RelativePath[0] == '.' && RelativePath[1] == '.'))
            {
                size_t DirectoryEnd = RelativePath.find('/');
                if(DirectoryEnd == RelativePath.npos)
                {
                    DirectoryEnd = RelativePath.size();
                }
                Reference.ReferenceString = "/{";
                Reference.ReferenceString.insert(Reference.ReferenceString.end(),RelativePath.begin(),RelativePath.begin()+DirectoryEnd);
                Reference.ReferenceString += "}/{Code/";
                if(ReferenceType == CodeReferenceType::Class)
                {
                    Reference.ReferenceString += "Classes";
                }
                else if(ReferenceType == CodeReferenceType::Function)
                {
                    Reference.ReferenceString += "Functions";
                }
                Reference.ReferenceString += "}/"+VisibleText+".mbd";
            }
            ReturnValue = std::move(Reference);
            //Reference.ReferenceString = 
        }
        else
        {
            RegularText NewElement;    
            NewElement.Text = VisibleText;
            ReturnValue = std::move(NewElement);
        }
        return(ReturnValue);
    }
    void DocumentFilesystem::p_ColorizeLSP(ProcessedColorConfiguration const& ColorConig,LSPInfo const& LSPConfig,std::vector<IndexType> const& ModifiedSources)
    {
        //how to create these is not completelty obvious
        std::unordered_map<std::string,std::unique_ptr<MBLSP::LSP_Client>> InitialziedLSPs;
       
        //DEBUG AF, hardcodec LSP
        //MBSystem::BiDirectionalSubProcess SubProcess("clangd",{});
        //InitialziedLSPs["clangd"] = std::make_unique<MBLSP::LSP_Client>(
        //        std::make_unique<MBUtility::NonOwningIndeterminateInputStream>(&SubProcess),
        //        std::make_unique<MBUtility::NonOwningOutputStream>(&SubProcess));
        //InitialziedLSPs["clangd"]->InitializeServer(Init);
        LSPReferenceResolver ReferenceResolver;
        //for(auto& Entry : m_TotalSources)
        for(int i = 0; i < m_TotalSources.size();i++)
        {
            if(ModifiedSources.size() > 0 && i >= ModifiedSources.size())
            {
                break;   
            }
            auto& Entry = ModifiedSources.size() == 0 ? m_TotalSources[i] : m_TotalSources[ModifiedSources[i]];
            std::filesystem::path DocumentPath = Entry.Document.Path;
            auto Lambda = [&](CodeBlock& BlockToModify) -> void
                    {
                        auto HandlesIt = ColorConig.LanguageConfigs.find(BlockToModify.CodeType);
                        std::string DocumentURI;
                        if(HandlesIt != ColorConig.LanguageConfigs.end())
                        {
                            LineIndex Index(BlockToModify.Content);
                            std::vector<Coloring> LSPColorings;
                            bool ResolveReferences = false;
                            MBLSP::LSP_Client* AssociatedLSP = nullptr;
                            if(HandlesIt->second.LSP != "")
                            {
                                std::unique_ptr<MBLSP::LSP_Client>& LSP = InitialziedLSPs[HandlesIt->second.LSP];
                                if(LSP == nullptr)
                                {
                                    auto LSPConfIt = LSPConfig.Servers.find(HandlesIt->second.LSP);
                                    if(LSPConfIt != LSPConfig.Servers.end())
                                    {
                                        LSP = StartLSPServer(LSPConfIt->second);
                                    }
                                }
                                if(LSP != nullptr)
                                {
                                    if(BlockToModify.CodeType == "cpp")
                                    {
                                        DocumentURI = MBLSP::URLEncodePath(DocumentPath.replace_extension(".cpp"));
                                        //Hack to add support for MBDocGen
                                        std::string PrefixToSearch = "/Code/Sources/";
                                        auto MBDocGenPrefixPosition = DocumentURI.find(PrefixToSearch); 
                                        if(MBDocGenPrefixPosition != DocumentURI.npos)
                                        {
                                            std::string NewURI = DocumentURI.substr(0,MBDocGenPrefixPosition);
                                            //removes that last directory, allowing all versions of doc/docs/Docs etc etc
                                            auto LastSlash = NewURI.find_last_of('/');
                                            if(LastSlash != NewURI.npos)
                                            {
                                                NewURI = NewURI.substr(0,LastSlash);
                                                NewURI += "/";
                                                NewURI += DocumentURI.substr(MBDocGenPrefixPosition+PrefixToSearch.size());
                                                DocumentURI = NewURI;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        DocumentURI = MBLSP::URLEncodePath(DocumentPath);
                                    }

                                    ReferenceResolver.SetLSP(LSP.get());
                                    ReferenceResolver.SetDocumentURI(DocumentURI); 

                                    MBLSP::DidOpenTextDocument_Notification OpenNotification;
                                    OpenNotification.params.textDocument.text = BlockToModify.Content;
                                    OpenNotification.params.textDocument.uri = DocumentURI;
                                    LSP->SendNotification(OpenNotification);


                                    ResolveReferences = true;
                                    LSPColorings = p_GetLSPColoring(*LSP,DocumentURI,BlockToModify.Content,ColorConig.ColorInfo,Index);
                                    AssociatedLSP = LSP.get();
                                }
                            }
                            std::vector<Coloring> RegexColorings = p_GetRegexColorings(LSPColorings,HandlesIt->second.RegexColoring,
                                    ColorConig.ColorInfo.ColorMap,BlockToModify.Content);
                            //std::vector<std::vector<Coloring>> TotalColorings = {std::move(LSPColorings),std::move(RegexColorings)};
                            BlockToModify.HighlightedText = p_CombineColorings(RegexColorings,LSPColorings,BlockToModify.Content,
                                    Index, ColorConig.ColorInfo,ResolveReferences ? &ReferenceResolver : nullptr);
                            if(AssociatedLSP)
                            {
                                MBLSP::DidCloseTextDocument_Notification CloseNotification;
                                CloseNotification.params.textDocument.uri = DocumentURI;
                                AssociatedLSP->SendNotification(CloseNotification);
                            }
                        }

                    };
            auto CodeBlockModifier = LambdaVisitor<decltype(Lambda)>(&Lambda);
            DocumentTraverser Traverser;
            Traverser.Traverse(Entry.Document,CodeBlockModifier);
        }
        for(auto const& Server : InitialziedLSPs)
        {
            if (Server.second)
            {
                Server.second->QuitServer(); 
            }
        }
    }
    //END DocumentFilesystem
}
