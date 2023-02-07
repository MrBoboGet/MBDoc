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

#include <regex>
namespace MBDoc
{
    TextColor h_ParseTextColor(std::string const& TextString)
    {
        TextColor ReturnValue;    
        if(TextString.size() != 6)
        {
            throw std::runtime_error("Error parsing color: unrecognized color \""+TextString+"\"");
        }
        bool Result = true;
        char Data[3];
        for(int i = 0; i < 3;i++)
        {
            Data[i] = MBUtility::HexValueToByte(TextString[i*2],TextString[i*2+1],&Result);
            if(!Result)
            {
                throw std::runtime_error("Error parsing color: unrecognized color \""+TextString+"\"");
            }
        }
        ReturnValue = TextColor(Data[0],Data[1],Data[2]);
        return(ReturnValue);
    }
    ProcessedColorConfiguration ProcessColorConfig(ColorConfiguration const& Config)
    {
        ProcessedColorConfiguration ReturnValue;
        ReturnValue.ColorInfo.DefaultColor = h_ParseTextColor(Config.Coloring.DefaultColor); 
        ReturnValue.ColorInfo.ColoringNameToIndex["Default"] = 0;
        ReturnValue.ColorInfo.ColorMap.push_back(ReturnValue.ColorInfo.DefaultColor);
        for(auto const& LangConf : Config.LanguageColorings)
        {
            ProcessedLanguageColorConfig NewConf;
            NewConf.LSP = LangConf.second.LSP;
            for(auto const& RegexList : LangConf.second.Regexes)
            {
                ColorTypeIndex& RegexColorType = ReturnValue.ColorInfo.ColoringNameToIndex[RegexList.first]; 
                if(RegexColorType == 0)
                {
                    RegexColorType = ReturnValue.ColorInfo.ColorMap.size();
                    ReturnValue.ColorInfo.ColorMap.push_back(TextColor());
                }
                auto& Regexes = NewConf.RegexColoring.Regexes[RegexColorType];
                for(auto const& Regex : RegexList.second)
                {
                    Regexes.push_back(std::regex(Regex,std::regex_constants::ECMAScript|std::regex_constants::nosubs));
                }
            }
        }
        for(auto const& Color : Config.Coloring.ColorMap)
        {
            ColorTypeIndex& Index = ReturnValue.ColorInfo.ColoringNameToIndex[Color.first];
            if(Index == 0)
            {
                Index = ReturnValue.ColorInfo.ColorMap.size();   
                ReturnValue.ColorInfo.ColorMap.push_back(TextColor());
            }
            ReturnValue.ColorInfo.ColorMap[Index] = h_ParseTextColor(Color.second);
        }
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
        if(Type == BlockElementType::CodeBlock)
        {
            Visitor.Visit(static_cast<CodeBlock const&>(*this));
        }       
        else if(Type == BlockElementType::MediaInclude)
        {
            Visitor.Visit(static_cast<MediaInclude const&>(*this));
        }
        else if(Type == BlockElementType::Paragraph)
        {
            Visitor.Visit(static_cast<Paragraph const&>(*this));
        }
    }
    void BlockElement::Accept(BlockVisitor& Visitor) 
    {
        if(Type == BlockElementType::CodeBlock)
        {
            Visitor.Visit(static_cast<CodeBlock&>(*this));
        }       
        else if(Type == BlockElementType::MediaInclude)
        {
            Visitor.Visit(static_cast<MediaInclude&>(*this));
        }
        else if(Type == BlockElementType::Paragraph)
        {
            Visitor.Visit(static_cast<Paragraph&>(*this));
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
    TextElement DocumentParsingContext::p_ParseReference(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset)
    {
        TextElement ReturnValue;
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
            size_t TextEnd = std::find(CharData+ParseOffset+2,CharData+DataSize,']')-(CharData);
            if(TextEnd == DataSize)
            {
                throw std::runtime_error("Unexpected end of visible text in reference");   
            }
            VisibleText = std::string(CharData+ParseOffset+2,CharData+TextEnd);
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
            ReferenceString = std::string(CharData+ParseOffset+1,CharData+ReferenceIDEnd);
            ParseOffset = ReferenceIDEnd + 1;
        }
        else
        {
            if (CharData[ParseOffset + 1] != '(') 
            {
                size_t DelimiterPosition = MBParsing::GetNextDelimiterPosition({ '\t',' ','\n',',','.' }, Data, DataSize, ParseOffset, nullptr);
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
                ReferenceString = std::string(CharData + ParseOffset + 2, CharData + IdentifierEnd);
                ParseOffset = IdentifierEnd + 1;
            }
        }
        if(ReferenceString.find("://") == ReferenceString.npos)
        {
             UnresolvedReference NewRef;
             NewRef.ReferenceString = ReferenceString;
             NewRef.VisibleText = VisibleText;
             ReturnValue = TextElement(std::unique_ptr<DocReference>(new UnresolvedReference(std::move(NewRef))));
        }
        else
        {
             URLReference NewRef;
             NewRef.VisibleText = VisibleText;
             NewRef.URL = ReferenceString;
             ReturnValue = TextElement(std::unique_ptr<DocReference>(new URLReference(std::move(NewRef))));
               
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
    std::vector<TextElement> DocumentParsingContext::p_ParseTextElements(void const* Data, size_t DataSize, size_t ParseOffset, size_t* OutParseOffset)
    {
        std::vector<TextElement> ReturnValue;
        //ASSUMPTION no # or block delimiter present, guaranteed text element data 
        const char* CharData = (const char*)Data;
        TextColor CurrentTextColor;
        TextModifier CurrentTextModifier = TextModifier(0);
        while (ParseOffset < DataSize)
        {
            size_t FindTextModifierDelimiterOffset = ParseOffset;
            size_t NextReferenceDeclaration = DataSize;
            size_t NextTextModifier = DataSize;
            size_t NextModifier = DataSize;
            while (FindTextModifierDelimiterOffset < DataSize)
            {
                NextReferenceDeclaration = std::find(CharData + FindTextModifierDelimiterOffset, CharData + DataSize, '@') - (CharData);
                NextTextModifier = std::find(CharData + FindTextModifierDelimiterOffset, CharData + DataSize, '*') - (CharData);
                //NextTextModifier = std::min(NextTextModifier, size_t(std::find(CharData + FindTextModifierDelimiterOffset, CharData + DataSize, '_') - (CharData)));

                NextModifier = std::min(NextTextModifier, NextReferenceDeclaration);
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
                NewText.Text = std::string(CharData + ParseOffset, NextModifier - ParseOffset);
                //Unescape text

                //TODO fix efficiently, jank
                ReturnValue.push_back(RegularText(std::move(NewText)));
                ParseOffset = NextModifier;
            }
            if (NextTextModifier >= DataSize && NextReferenceDeclaration >= DataSize)
            {
                break;
            }
            if (NextReferenceDeclaration < NextTextModifier)
            {
                TextElement NewReference = p_ParseReference(Data, DataSize, ParseOffset, &ParseOffset);
                NewReference.GetBase().Color = CurrentTextColor;
                NewReference.GetBase().Modifiers = CurrentTextModifier;
                ReturnValue.push_back(std::move(NewReference));
            }
            if (NextTextModifier < NextReferenceDeclaration)
            {
                TextState NewState = h_ParseTextState(Data, DataSize, NextTextModifier, &ParseOffset);
                CurrentTextColor = NewState.Color;
                CurrentTextModifier = TextModifier(uint64_t(CurrentTextModifier) ^ uint64_t(NewState.Modifiers));
            }
        }
        return(ReturnValue);
    }
    std::unique_ptr<BlockElement> DocumentParsingContext::p_ParseMediaInclude(LineRetriever& Retriever)
    {
        std::unique_ptr<BlockElement> ReturnValue = std::make_unique<MediaInclude>();
        MediaInclude& NewElement = (MediaInclude&)*ReturnValue;
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
        NewElement.MediaPath = std::string(IncludeString.begin()+ParseOffset+1,EndIt); 
        return(ReturnValue);
    }
    std::unique_ptr<BlockElement> DocumentParsingContext::p_ParseCodeBlock(LineRetriever& Retriever)
    {
        std::unique_ptr<BlockElement> ReturnValue = std::make_unique<CodeBlock>();
        CodeBlock& NewCodeBlock = static_cast<CodeBlock&>(*ReturnValue.get());
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
        NewCodeBlock.Content = std::string();
        while (!Retriever.Finished())
        {
            std::string NewLine;
            Retriever.GetLine(NewLine);
            MBParsing::SkipWhitespace(NewLine, 0, &ParseOffset);
            if (ParseOffset + 2 < NewLine.size() && std::memcmp(NewLine.data()+ParseOffset, "```", 3) == 0)
            {
                break;
            }
            std::get<std::string>(NewCodeBlock.Content) += NewLine;
            std::get<std::string>(NewCodeBlock.Content).insert(std::get<std::string>(NewCodeBlock.Content).end(), '\n');
        }
        //there is always 1 extra newline
        std::get<std::string>(NewCodeBlock.Content).resize(std::get<std::string>(NewCodeBlock.Content).size() - 1);
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
    ArgumentList DocumentParsingContext::p_ParseArgumentList(void const* Data, size_t DataSize,size_t InOffset)
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
    void DocumentParsingContext::p_UpdateReferences(DocumentSource& SourceToModify)
    {
        DocumentTraverser Traverser;
        ReferenceExtractor Extractor;
        Traverser.Traverse(SourceToModify,Extractor);
        SourceToModify.References = std::move(Extractor.References);
        SourceToModify.ReferenceTargets = Extractor.Targets;
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
        return(ReturnValue);
    }
    DocumentSource DocumentParsingContext::ParseSource(const void* Data,size_t DataSize,std::string FileName,MBError& OutError)
    {
        MBUtility::MBBufferInputStream BufferStream(Data,DataSize);
        return(ParseSource(BufferStream,std::move(FileName),OutError));
    }
    //END DocumentSource

    //BEGIN DocumentBuild
    
    void DocumentBuild::p_ParseDocumentBuildDirectory(DocumentBuild& OutBuild,MBParsing::JSONObject const& DirectoryObject,std::filesystem::path const& BuildDirectory,MBError& OutError)
    {
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
    void DocumentPath::SetPartIdentifier(std::string PartSpecifier)
    {
        m_PartIdentifier = std::move(PartSpecifier);
    }
    std::string const& DocumentPath::GetPartIdentifier() const
    {
        return(m_PartIdentifier);
    }
    //END DocumentPath

    //BEGIN DocumentReference
    DocumentReference DocumentReference::ParseReference(std::string StringToParse,MBError& OutError)
    {
        DocumentReference ReturnValue;
        size_t LastSlash = StringToParse.find_last_of('/');
        size_t LastBracket = std::min(StringToParse.find_last_of('{'),StringToParse.find_last_of('}'));
        size_t LastHashtag = StringToParse.find_last_of('#');
        if(LastHashtag != StringToParse.npos)
        {
            if((LastHashtag < LastSlash && LastSlash != StringToParse.npos) || (LastHashtag < LastBracket && LastBracket != StringToParse.npos))
            {
                OutError = false;
                OutError.ErrorMessage = "Path specifier is not allowed after the part specifier"; 
                return(ReturnValue);
            } 
            ReturnValue.PartSpecifier = StringToParse.substr(LastHashtag+1);
            StringToParse.resize(LastHashtag);
        }
        size_t ParseOffset = 0;
        MBParsing::SkipWhitespace(StringToParse,ParseOffset,&ParseOffset);
        if(ParseOffset < StringToParse.size() && StringToParse[ParseOffset] == '/')
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
        for(auto& Element : FormatToVisit.Contents)
        {
            Element.Accept(*this);
        }
        m_AssociatedVisitor->LeaveFormat(FormatToVisit); 
    }
    void DocumentTraverser::Visit(FormatElement& FormatToVisit)
    {
        m_AssociatedVisitor->EnterFormat(FormatToVisit);
        for(auto const& Element : FormatToVisit.Contents)
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
    DocumentFilesystem::FSSearchResult DocumentFilesystem::p_ResolvePartSpecifier(FSSearchResult CurrentResult,std::string const& PartSpecifier) const
    {
        FSSearchResult ReturnValue;        
        assert(CurrentResult.Type == DocumentFSType::File || CurrentResult.Type == DocumentFSType::Directory);
        if(CurrentResult.Type == DocumentFSType::File)
        {
            if(m_TotalSources[CurrentResult.Index].Document.ReferenceTargets.find(PartSpecifier) != m_TotalSources[CurrentResult.Index].Document.ReferenceTargets.end())
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
                    if (Iterator.GetDocumentInfo().ReferenceTargets.find(PartSpecifier) != Iterator.GetDocumentInfo().ReferenceTargets.end())
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
     
    DocumentPath DocumentFilesystem::p_ResolveReference(DocumentPath const& FilePath,DocumentReference const& ReferenceIdentifier,bool* OutResult) const
    {
        bool Result = true;
        DocumentPath ReturnValue;
        FSSearchResult SearchRoot;
        SearchRoot.Type = DocumentFSType::Directory;
        SearchRoot.Index = 0;//Root directory
        IndexType SpecifierOffset = 0;
        if (ReferenceIdentifier.PathSpecifiers.size() > 0)
        {
            if (ReferenceIdentifier.PathSpecifiers[0].AnyRoot == false)
            {
                SpecifierOffset = 1;
                if (ReferenceIdentifier.PathSpecifiers[0].PathNames.front() != "/")
                {
                    //The current directory needs to be resolved 
                    SearchRoot.Index = p_GetFileDirectoryIndex(FilePath);
                    if (SearchRoot.Index == -1)
                    {
                        *OutResult = false;
                        return(ReturnValue);
                    }
                }
                SearchRoot = p_ResolveAbsoluteDirectory(SearchRoot.Index, ReferenceIdentifier.PathSpecifiers[0]);
                if (SearchRoot.Type == DocumentFSType::Null)
                {
                    *OutResult = false;
                    return(ReturnValue);
                }
            }
            else
            {
                SearchRoot.Index = p_GetFileDirectoryIndex(FilePath);
                if (SearchRoot.Index == -1)
                {
                    *OutResult = false;
                    return(ReturnValue);
                }
            }
            if (SpecifierOffset == 0 || ReferenceIdentifier.PathSpecifiers.size() > 1)
            {
                if (SearchRoot.Type == DocumentFSType::File)
                {
                    *OutResult = false;
                    return(ReturnValue);
                }
                SearchRoot = p_ResolveDirectorySpecifier(SearchRoot.Index, ReferenceIdentifier, SpecifierOffset);
            }
            if (SearchRoot.Type == DocumentFSType::Null)
            {
                *OutResult = false;
                return(ReturnValue);
            }
        }
        else
        {
            SearchRoot.Type = DocumentFSType::File;
            SearchRoot.Index = p_GetFileIndex(FilePath);
            if (SearchRoot.Index == -1)
            {
                *OutResult = false;
                return(ReturnValue);
            }
        }
        if(ReferenceIdentifier.PartSpecifier != "")
        {
            SearchRoot = p_ResolvePartSpecifier(SearchRoot,ReferenceIdentifier.PartSpecifier);
        }
        if(SearchRoot.Type == DocumentFSType::Null)
        {
            *OutResult = false;   
            return(ReturnValue);
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
        if (ReferenceIdentifier.PartSpecifier != "")
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
    DocumentDirectoryInfo DocumentFilesystem::p_UpdateOverDirectory(DocumentBuild const& Directory, IndexType FileIndexBegin, IndexType DirectoryIndex)
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
            DocumentSource NewFile = Parser.ParseSource(File, ParseError);
            if (!ParseError)
            {
                throw std::runtime_error("Error parsing file "+MBUnicode::PathToUTF8(File) +": " + ParseError.ErrorMessage);
            }
            //LIBRARY BUG? seems like std::move(NewFile) produces a file with empty path, weird
            m_TotalSources[FileIndexBegin + FileIndex].Document = std::move(NewFile);
            m_TotalSources[FileIndexBegin + FileIndex].Document.Path = File;
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
        size_t TotalSubFiles = 0;
        size_t TotalDirectories = Directory.SubDirectories.size();
        for (auto const& SubDirectory : Directory.SubDirectories) 
        {
            TotalSubFiles += SubDirectory.second.DirectoryFiles.size();
        }
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
        //for (auto const& SubDirectory : Directory.SubDirectories)
        for(size_t i = 0; i < SortedDirectoryIndexes.size();i++)
        {
            size_t SortedDirectoryIndex = SortedDirectoryIndexes[i];    
            auto const& SubDirectory = Directory.SubDirectories[SortedDirectoryIndex];
            m_DirectoryInfos[NewDirectoryIndexBegin + DirectoryOffset] = p_UpdateOverDirectory(SubDirectory.second, NewFileIndexBegin + FileOffset, NewDirectoryIndexBegin + DirectoryOffset);
            m_DirectoryInfos[NewDirectoryIndexBegin + DirectoryOffset].Name = SubDirectory.first;
            m_DirectoryInfos[NewDirectoryIndexBegin + DirectoryOffset].ParentDirectoryIndex = DirectoryIndex;
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
    MBError DocumentFilesystem::CreateDocumentFilesystem(DocumentBuild const& BuildToParse,LSPInfo const& LSPConf,ProcessedColorConfiguration const& 
                ColorConf,DocumentFilesystem& OutFilesystem)
    {
        MBError ReturnValue = true;
        try 
        {
            DocumentFilesystem Result;
            Result.m_DirectoryInfos.resize(1);
            //BuildDirectory Directory = p_ParseBuildDirectory(BuildToParse);
            Result.m_TotalSources.resize(BuildToParse.GetTotalFiles());
            DocumentDirectoryInfo TopInfo = Result.p_UpdateOverDirectory(BuildToParse, 0, 0);
            Result.m_DirectoryInfos[0] = TopInfo;
            Result.m_DirectoryInfos[0].Name = "/";
            assert(std::is_sorted(Result.m_DirectoryInfos.begin(), Result.m_DirectoryInfos.end(), h_DirectoryOrder));
            
            
            Result.p_ResolveReferences(); 
            Result.p_ColorizeLSP(ColorConf,LSPConf);
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
    DocumentFilesystem::DocumentFilesystemReferenceResolver::DocumentFilesystemReferenceResolver(DocumentFilesystem* AssociatedFilesystem,DocumentPath CurrentPath)
    {
        m_AssociatedFilesystem = AssociatedFilesystem;
        m_CurrentPath = CurrentPath;
    }
    void DocumentFilesystem::DocumentFilesystemReferenceResolver::Visit(UnresolvedReference const& Ref)
    {
        MBError Result = true;
        DocumentPath NewPath = m_AssociatedFilesystem->ResolveReference(m_CurrentPath,Ref.ReferenceString,Result);
        if(Result)
        {
            FileReference NewRef;
            NewRef.Color = Ref.Color;
            NewRef.Modifiers = Ref.Modifiers;
            NewRef.VisibleText = Ref.VisibleText;
            NewRef.Path = NewPath;
            m_Result = TextElement(std::unique_ptr<DocReference>(new FileReference(std::move(NewRef))));
        }
        m_ShouldUpdate = Result;
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
    void DocumentFilesystem::p_ResolveReferences()
    {
        DocumentFilesystemIterator Iterator(0); 
        Iterator.m_AssociatedFilesystem = this;
        DocumentTraverser Traverser; 
        while(Iterator.HasEnded() == false)
        {
            if(!Iterator.EntryIsDirectory())
            {
                DocumentPath CurrentPath = Iterator.GetCurrentPath();
                //yikes
                DocumentSource& CurrentDoc = const_cast<DocumentSource&>(Iterator.GetDocumentInfo());
                
                DocumentFilesystemReferenceResolver Resolver(this,CurrentPath);
                Traverser.Traverse(CurrentDoc,Resolver);
            }
            Iterator++; 
        }
    }

    class i_LineIndex
    {
        std::vector<int> m_LineToOffset; 
    public:
        i_LineIndex(std::string const& DocumentData)
        {
            m_LineToOffset.push_back(0);
            for(int i = 0; i < DocumentData.size();i++)
            {
                if(DocumentData[i] == '\n')
                {
                    m_LineToOffset .push_back(i);
                }  
            } 
        }     
        int LineCount() const
        {
            return(m_LineToOffset.size());
        }
        int operator[](int Index)
        {
            if(Index > m_LineToOffset.size())
            {
                throw std::runtime_error("Invalid line index");
            }   
            return(m_LineToOffset[Index]);
        }
    };

    void DocumentFilesystem::p_ColorizeCodeBlock(MBLSP::LSP_Client& ClientToUse,CodeBlock& BlockToColorize)
    {
        std::vector<std::vector<TextElement>> Result;
        DidOpenTextDocument_Notification OpenNotification;
        OpenNotification.params.textDocument.text = std::get<std::string>(BlockToColorize.Content);
        OpenNotification.params.textDocument.uri = MBLSP::URLEncodePath(std::filesystem::current_path()/"asdasdasdasd.cpp");
        ClientToUse.SendNotification(OpenNotification);

        SemanticToken_Request TokenRequest;
        TokenRequest.params.textDocument.uri = OpenNotification.params.textDocument.uri;
        SemanticToken_Response Tokens = ClientToUse.SendRequest(TokenRequest);
        try
        {
            if(Tokens.result)
            {
                std::string const& Text = OpenNotification.params.textDocument.text;
                i_LineIndex Index(OpenNotification.params.textDocument.text);
                std::vector<int> const& TokenData = Tokens.result->data;
                if(TokenData.size() % 5 != 0)
                {
                    throw std::runtime_error("Invalid token count, must be divisible by 5");
                }
                int CurrentLineIndex = 0;
                int CurrentTokensOffset = 0;
                int TokenOffset = 0;
                
                int LatestTokenEnd = 0;
                std::vector<TextElement> CurrentLine;
                while(CurrentTokensOffset < TokenData.size())
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

                    int CurrentTokenBegin = Index[CurrentLineIndex]+1+CurrentTokensOffset;
                    int CurrentTokenEnd = CurrentTokenBegin + Length;
                   
                    std::string StringToAdd; 
                    while(LatestTokenEnd < CurrentTokenBegin)
                    {
                        if(Text[LatestTokenEnd] != '\n')
                        {
                            StringToAdd += Text[LatestTokenEnd];  
                        } 
                        else
                        {
                            if(StringToAdd.size() > 0)
                            {
                                RegularText TextToAdd;   
                                TextToAdd.Text = std::move(StringToAdd);
                                StringToAdd.clear();
                                CurrentLine.push_back(std::move(TextToAdd));
                                Result.push_back(std::move(CurrentLine));
                                CurrentLine.clear();
                            } 
                        }
                        LatestTokenEnd++;
                    }
                    if(StringToAdd.size() > 0)
                    {
                        RegularText TextToAdd;   
                        TextToAdd.Text = std::move(StringToAdd);
                        StringToAdd.clear();
                        CurrentLine.push_back(std::move(TextToAdd));
                    } 
                    
                    RegularText TextToAdd;   
                    TextToAdd.Text = std::string(Text.begin()+CurrentTokenBegin,Text.begin()+CurrentTokenEnd);
                    TextToAdd.Color = TextColor(0, 0, 255);
                    CurrentLine.push_back(std::move(TextToAdd));
                    LatestTokenEnd = CurrentTokenEnd;
                    TokenOffset += 5;
                }
                std::string StringToAdd;
                while (LatestTokenEnd < Text.size())
                {
                    if (Text[LatestTokenEnd] != '\n')
                    {
                        StringToAdd += Text[LatestTokenEnd];
                    }
                    else
                    {
                        if (StringToAdd.size() > 0)
                        {
                            RegularText TextToAdd;
                            TextToAdd.Text = std::move(StringToAdd);
                            StringToAdd.clear();
                            CurrentLine.push_back(std::move(TextToAdd));
                            Result.push_back(std::move(CurrentLine));
                            CurrentLine.clear();
                        }
                    }
                    LatestTokenEnd++;
                }
                if (StringToAdd.size() > 0)
                {
                    RegularText TextToAdd;
                    TextToAdd.Text = std::move(StringToAdd);
                    StringToAdd.clear();
                    CurrentLine.push_back(std::move(TextToAdd));
                }
                if(CurrentLine.size() > 0)
                {
                    Result.push_back(std::move(CurrentLine));   
                }
            }
        } 
        catch(std::exception const& e)
        {

        }
        DidCloseTextDocument_Notification CloseNotification;
        CloseNotification.params.textDocument.uri = OpenNotification.params.textDocument.uri;
        ClientToUse.SendNotification(CloseNotification);

        BlockToColorize.Content = ResolvedCodeText(std::move(Result));
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
                        ParseOffset < PreviousColorings[PreviousColorIndex].ByteOffset+PreviousColorings[PreviousColorIndex].Content.size())
                {
                    //in previous coloring, skip
                    ParseOffset = PreviousColorings[PreviousColorIndex].ByteOffset+PreviousColorings[PreviousColorIndex].Content.size();
                    PreviousColorIndex++;
                    continue;
                }
                else
                {
                    PreviousColorIndex++;
                    continue;
                }
            }
            while(ParseOffset < TextToInspectEnd)
            {
                for(auto const& RegexCategory : RegexesToUse.Regexes)
                {
                    TextColor CurrentColor = ColorMap[RegexCategory.first];
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
                            NewColoring.Content = Begin->str();
                            ReturnValue.push_back(NewColoring);
                        }
                    }
                }
            }
            ParseOffset = TextToInspectEnd;
        }
        return(ReturnValue);
    }
    void DocumentFilesystem::p_ColorizeLSP(ProcessedColorConfiguration const& ColorConig,LSPInfo const& LSPConfig)
    {
        //how to create these is not completelty obvious
        std::unordered_map<std::string,std::unique_ptr<MBLSP::LSP_Client>> InitialziedLSPs;
       
        //DEBUG AF, hardcodec LSP
        //MBSystem::BiDirectionalSubProcess SubProcess("clangd",{});
        InitializeRequest Init;
        Init.params.rootUri = MBLSP::URLEncodePath(std::filesystem::current_path());
        //InitialziedLSPs["clangd"] = std::make_unique<MBLSP::LSP_Client>(
        //        std::make_unique<MBUtility::NonOwningIndeterminateInputStream>(&SubProcess),
        //        std::make_unique<MBUtility::NonOwningOutputStream>(&SubProcess));
        //InitialziedLSPs["clangd"]->InitializeServer(Init);
        for(auto& Entry : m_TotalSources)
        {
            auto Lambda = [&](CodeBlock& BlockToModify) -> void
                    {
                        auto HandlesIt = ColorConig.LanguageConfigs.find(BlockToModify.CodeType);
                        if(HandlesIt != ColorConig.LanguageConfigs.end())
                        {
                            std::vector<Coloring> LSPColorings;
                            if(HandlesIt->second.LSP != "")
                            {
                                std::unique_ptr<MBLSP::LSP_Client>& LSP = InitialziedLSPs[HandlesIt->second.LSP];
                                if(LSP == nullptr)
                                {
                                    auto LSPConfIt = LSPConfig.Servers.find(HandlesIt->second.LSP);
                                    if(LSPConfIt != LSPConfig.Servers.end())
                                    {
                                        LSP = p_InitializeLSP(LSPConfIt->second,Init);
                                    }
                                }
                                if(LSP != nullptr)
                                {
                                    LSPColorings = p_GetLSPColoring(*LSP,std::get<std::string>(BlockToModify.Content));
                                }
                            }
                            std::vector<Coloring> RegexColorings = p_GetRegexColorings(LSPColorings,HandlesIt->second.RegexColoring,
                                    ColorConig.ColorInfo.ColorMap,std::get<std::string>(BlockToModify.Content));
                            std::vector<std::vector<Coloring>> TotalColorings = {std::move(LSPColorings),std::move(RegexColorings)};
                            BlockToModify.Content = p_CombineColorings(TotalColorings,std::get<std::string>(BlockToModify.Content));
                        }

                    };
            auto CodeBlockModifier = LambdaVisitor<decltype(Lambda)>(&Lambda);
            DocumentTraverser Traverser;
            Traverser.Traverse(Entry.Document,CodeBlockModifier);
        }
        for(auto const& Server : InitialziedLSPs)
        {
            Server.second->QuitServer(); 
        }
    }
    //END DocumentFilesystem
}
