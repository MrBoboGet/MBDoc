#include "MBDoc.h"
#include "MBUtility/MBErrorHandling.h"
#include "MBUtility/MBInterfaces.h"
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
            if (CharData[ParseOffset + 1] != '(') 
            {
                size_t DelimiterPosition = MBParsing::GetNextDelimiterPosition({ '\t',' ','\n',',','.' }, Data, DataSize, ParseOffset, nullptr);
                ReturnValue.Identifier = std::string(CharData + ParseOffset + 1, CharData + DelimiterPosition);
                ParseOffset = DelimiterPosition;
            }
            else 
            {
                size_t IdentifierEnd = std::find(CharData + ParseOffset + 2, CharData + DataSize,')') - CharData;
                if (IdentifierEnd == DataSize)
                {
                    throw std::runtime_error("Unexpected end of reference identifier, missing )");
                }
                ReturnValue.Identifier = std::string(CharData + ParseOffset + 2, CharData + IdentifierEnd);
                ParseOffset = IdentifierEnd + 1;
            }
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
    std::vector<std::unique_ptr<TextElement>> DocumentParsingContext::p_ParseTextElements(void const* Data, size_t DataSize, size_t ParseOffset, size_t* OutParseOffset)
    {
        std::vector<std::unique_ptr<TextElement>> ReturnValue;
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
                NextTextModifier = std::min(NextTextModifier, size_t(std::find(CharData + FindTextModifierDelimiterOffset, CharData + DataSize, '_') - (CharData)));

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
                ReturnValue.push_back(std::unique_ptr<TextElement>(new RegularText(std::move(NewText))));
                ParseOffset = NextModifier;
            }
            if (NextTextModifier >= DataSize && NextReferenceDeclaration >= DataSize)
            {
                break;
            }
            if (NextReferenceDeclaration < NextTextModifier)
            {
                DocReference NewReference = p_ParseReference(Data, DataSize, ParseOffset, &ParseOffset);
                NewReference.Color = CurrentTextColor;
                NewReference.Modifiers = CurrentTextModifier;
                ReturnValue.push_back(std::unique_ptr<TextElement>(new DocReference(std::move(NewReference))));
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
        while (!Retriever.Finished())
        {
            std::string NewLine;
            Retriever.GetLine(NewLine);
            MBParsing::SkipWhitespace(NewLine, 0, &ParseOffset);
            if (ParseOffset + 2 < NewLine.size() && std::memcmp(NewLine.data()+ParseOffset, "```", 3) == 0)
            {
                break;
            }
            NewCodeBlock.RawText += NewLine;
            NewCodeBlock.RawText.insert(NewCodeBlock.RawText.end(), '\n');
        }
        //there is always 1 extra newline
        NewCodeBlock.RawText.resize(NewCodeBlock.RawText.size() - 1);
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
                FormatElement NewFormat = p_ParseFormatElement(Retriever,&NewAttributes);
                if (CurrentAttributes.IsEmpty() == false)
                {
                    NewFormat.Attributes = CurrentAttributes;
                    CurrentAttributes.Clear();
                }
                ReturnValue.Contents.push_back(std::move(NewFormat));
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
                ReturnValue.Contents.push_back(FormatElementComponent(p_ParseDirective(Retriever)));
                CurrentElementCount += 1;
                continue;
            }
            std::unique_ptr<BlockElement> NewBlockElement = p_ParseBlockElement(Retriever);
            if(CurrentAttributes.IsEmpty() == false)
            {
                NewBlockElement->Attributes = CurrentAttributes; 
                CurrentAttributes.Clear();
            }
            ReturnValue.Contents.push_back(FormatElementComponent(std::move(NewBlockElement)));
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
            if (Retriever.Finished() == false && Retriever.PeekLine() == "/_")
            {
                throw std::runtime_error("Missmatched #_ and  /_ pair");
            }
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
    void h_UpdateReferences(DocumentSource& SourceToModify,BlockElement const& BlockComponent)
    {
        if(BlockComponent.Type == BlockElementType::Paragraph)
        {
            Paragraph const& ParagraphComponent = static_cast<Paragraph const&>(BlockComponent);  
            for(std::unique_ptr<TextElement> const& Text : ParagraphComponent.TextElements)
            {
                if(Text->Type == TextElementType::Reference)
                {
                    DocReference const& Reference = static_cast<DocReference const&>(*Text);
                    SourceToModify.References.insert(Reference.Identifier);
                }   
            }
        } 
    }
    void h_UpdateReferences(DocumentSource& SourceToModify,FormatElement const& FormatComponent)
    {
        if (FormatComponent.Name != "")
        {
            SourceToModify.ReferenceTargets.insert(FormatComponent.Name);
        }
        SourceToModify.ReferenceTargets.insert(FormatComponent.Name);
        for(FormatElementComponent const& Element : FormatComponent.Contents)
        {
            if(Element.GetType() == FormatComponentType::Format)
            {
                h_UpdateReferences(SourceToModify,Element.GetFormatData());            
            }
            else if(Element.GetType() == FormatComponentType::Block)
            {
                h_UpdateReferences(SourceToModify,Element.GetBlockData());
            } 
        }
    }
    void DocumentParsingContext::p_UpdateReferences(DocumentSource& SourceToModify)
    {
        for(FormatElement const& Format : SourceToModify.Contents)
        {
            h_UpdateReferences(SourceToModify, Format);
        } 
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
        return(ParseSource(InputStream,MBUnicode::PathToUTF8(InputFile.filename()),OutError));
    }
    DocumentSource DocumentParsingContext::ParseSource(const void* Data,size_t DataSize,std::string FileName,MBError& OutError)
    {
        MBUtility::MBBufferInputStream BufferStream(Data,DataSize);
        return(ParseSource(BufferStream,std::move(FileName),OutError));
    }
    //END DocumentSource

    //BEGIN DocumentBuild
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
            ReturnValue.BuildRootDirectory = BuildDirectory; 
            for(MBParsing::JSONObject const& Object : JSONData["Sources"].GetArrayData())
            {
                std::string CurrentString = Object.GetStringData();
                if (CurrentString.size() == 0)
                {
                    OutError = false;
                    OutError.ErrorMessage = "Empty source name not allowed";
                    return(ReturnValue);
                }
                if (CurrentString[0] == '/')
                {
                    OutError = false;
                    OutError.ErrorMessage = "Invalid source name \"" + CurrentString + "\": Only relative paths allowed";
                    return(ReturnValue);
                }
                ReturnValue.BuildFiles.push_back(DocumentPath::ParsePath(CurrentString,OutError));
                if (!OutError)
                {
                    return(ReturnValue);
                }
            }
            for(auto const& SubBuildSpecifier : JSONData["SubBuilds"].GetMapData())
            {
                DocumentBuild NewSubBuild = ParseDocumentBuild(MBUnicode::PathToUTF8(BuildDirectory)+"/"+SubBuildSpecifier.second.GetStringData(),OutError);
                if(!OutError)
                {
                    return(ReturnValue);   
                }
                ReturnValue.SubBuilds.push_back(std::pair<std::string,DocumentBuild>(SubBuildSpecifier.first,std::move(NewSubBuild)));
            }
        }
        catch(std::exception const& e)
        {
            OutError = false;
            OutError.ErrorMessage = "Error parsing MBDocBuild: "+std::string(e.what());
        }
        std::sort(ReturnValue.BuildFiles.begin(), ReturnValue.BuildFiles.end());
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
    }
    size_t DocumentFilesystemIterator::GetCurrentDirectoryIndex() const
    {
        return(m_CurrentDirectoryIndex);   
    }
    size_t DocumentFilesystemIterator::GetCurrentFileIndex() const
    {
        return(m_DirectoryFilePosition); 
    }
    void DocumentFilesystemIterator::Increment()
    {
        DocumentDirectoryInfo const& CurrentInfo = m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex];
        m_DirectoryFilePosition += 1;
        assert(m_DirectoryFilePosition <= CurrentInfo.FileIndexEnd-CurrentInfo.FileIndexBegin);
        if(m_DirectoryFilePosition == (CurrentInfo.FileIndexEnd - CurrentInfo.FileIndexBegin))
        {
            m_DirectoryFilePosition = -1;
            if(CurrentInfo.DirectoryIndexEnd - CurrentInfo.DirectoryIndexBegin != 0)
            {
                m_CurrentDirectoryIndex = CurrentInfo.DirectoryIndexBegin;
            }
            else
            {
                size_t CurrentDirectoryIndex = m_CurrentDirectoryIndex;
                m_CurrentDirectoryIndex = -1;
                //Being -1 implies that the end is reached
                //ASSUMPTION the root node MUST have -1 as parent
                while(true)
                {
                    if(CurrentDirectoryIndex == m_OptionalDirectoryRoot)
                    {
                        break;   
                    }
                    size_t PreviousIndex = CurrentDirectoryIndex;
                    CurrentDirectoryIndex = m_AssociatedFilesystem->m_DirectoryInfos[PreviousIndex].ParentDirectoryIndex; 
                    if(CurrentDirectoryIndex == -1)
                    {
                        break;
                    }
                    DocumentDirectoryInfo const& CurrentParentInfo = m_AssociatedFilesystem->m_DirectoryInfos[CurrentDirectoryIndex];
                    if(PreviousIndex + 1 < CurrentParentInfo.DirectoryIndexEnd)
                    {
                        m_CurrentDirectoryIndex = PreviousIndex+1;   
                        break;
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
        return(m_AssociatedFilesystem->m_TotalSources[m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex].DirectoryIndexBegin+m_DirectoryFilePosition].Name);
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
        return(m_AssociatedFilesystem->m_TotalSources[m_AssociatedFilesystem->m_DirectoryInfos[m_CurrentDirectoryIndex].FileIndexBegin+m_DirectoryFilePosition]); 
    }

    DocumentPath DocumentFilesystemIterator::GetCurrentPath() const
    {
        size_t CurrentDirectoryIndex = m_CurrentDirectoryIndex;
        //Naive solution
        std::vector<std::string> Directories;
        if(m_DirectoryFilePosition != -1)
        {
            Directories.push_back(m_AssociatedFilesystem->m_TotalSources[m_AssociatedFilesystem->m_DirectoryInfos[CurrentDirectoryIndex].FileIndexBegin+m_DirectoryFilePosition].Name);
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

    //BEGIN DocumentFilesystem
    DocumentFilesystem::FSSearchResult DocumentFilesystem::p_ResolveDirectorySpecifier(size_t RootDirectoryIndex,DocumentReference const& ReferenceToResolve,size_t SpecifierOffset) const
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
    bool h_FileComp(DocumentSource const& LeftInfo, std::string const& RightInfo)
    {
        return(LeftInfo.Name < RightInfo);
    }
    size_t DocumentFilesystem::p_DirectorySubdirectoryIndex(size_t DirectoryIndex,std::string const& SubdirectoryName) const
    {
        size_t ReturnValue = 0; 
        DocumentDirectoryInfo const& DirectoryInfo = m_DirectoryInfos[DirectoryIndex];
        if(DirectoryInfo.DirectoryIndexEnd - DirectoryInfo.DirectoryIndexBegin== 0)
        {
            return(-1);   
        }
        size_t Index = std::lower_bound(m_DirectoryInfos.begin()+DirectoryInfo.DirectoryIndexBegin,m_DirectoryInfos.begin()+DirectoryInfo.DirectoryIndexEnd,SubdirectoryName,h_DirectoryComp)-m_DirectoryInfos.begin();
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
        return(ReturnValue); return(ReturnValue);
    }
    size_t DocumentFilesystem::p_DirectoryFileIndex(size_t DirectoryIndex,std::string const& FileName) const
    {
        size_t ReturnValue = 0; 
        DocumentDirectoryInfo const& DirectoryInfo = m_DirectoryInfos[DirectoryIndex];
        if(DirectoryInfo.FileIndexEnd - DirectoryInfo.FileIndexBegin == 0)
        {
            return(-1);   
        }
        size_t Index = std::lower_bound(m_TotalSources.begin()+DirectoryInfo.FileIndexBegin,m_TotalSources.begin()+DirectoryInfo.FileIndexEnd,FileName, h_FileComp)-m_TotalSources.begin();
        if(Index == DirectoryInfo.FileIndexEnd)
        {
            ReturnValue = -1;  
        } 
        else
        {
            if (m_TotalSources[Index].Name != FileName)
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
    DocumentFilesystem::FSSearchResult DocumentFilesystem::p_ResolveAbsoluteDirectory(size_t RootDirectoryIndex,PathSpecifier const& Path) const
    {
        //All special cases stems from the fact that the special / directory isnt the child of any other directory
        FSSearchResult ReturnValue;        
        size_t LastDirectoryIndex = RootDirectoryIndex;
        int Offset = 0;
        if (Path.PathNames.front() == "/")
        {
            Offset = 1;
            LastDirectoryIndex = 0;
        }
        for(int i = Offset; i < int(Path.PathNames.size())-1;i++)
        {
            size_t SubdirectoryIndex = p_DirectorySubdirectoryIndex(LastDirectoryIndex,Path.PathNames[i]);
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
        size_t FileIndex = p_DirectoryFileIndex(LastDirectoryIndex,Path.PathNames.back());
        if(FileIndex != -1)
        {
            ReturnValue.Type = DocumentFSType::File;   
            ReturnValue.Index = FileIndex;
        }
        else
        {
            size_t DirectoryIndex = p_DirectorySubdirectoryIndex(LastDirectoryIndex,Path.PathNames.back());    
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
            if(m_TotalSources[CurrentResult.Index].ReferenceTargets.find(PartSpecifier) != m_TotalSources[CurrentResult.Index].ReferenceTargets.end())
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
    DocumentPath DocumentFilesystem::p_GetFileIndexPath(size_t FileIndex) const
    {
        DocumentPath ReturnValue;
        size_t DirectoryIndex = std::lower_bound(m_DirectoryInfos.begin(), m_DirectoryInfos.end(), FileIndex, h_ContainFileComp) - m_DirectoryInfos.begin();
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
        ReturnValue.AddDirectory(m_TotalSources[FileIndex].Name);
        return(ReturnValue);
    }
    size_t DocumentFilesystem::p_GetFileDirectoryIndex(DocumentPath const& PathToSearch) const
    {
        size_t ReturnValue = 0;
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
    size_t DocumentFilesystem::p_GetFileIndex(DocumentPath const& PathToSearch) const
    {
        size_t ReturnValue = 0;
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
        size_t SpecifierOffset = 0;
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
            size_t FileIndex = p_DirectoryFileIndex(SearchRoot.Index, "index.mbd");
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
    bool h_PairStringOrder(std::pair<std::string, DocumentFilesystem::BuildDirectory> const& Left, std::pair<std::string, DocumentFilesystem::BuildDirectory> Right)
    {
        return(Left.first < Right.first);
    }

    DocumentFilesystem::BuildDirectory h_ParseBuildDirectory(std::vector<DocumentPath> const& Files,std::filesystem::path const& DirectoryPath, int Depth, size_t Begin, size_t End)
    {
        DocumentFilesystem::BuildDirectory ReturnValue;
        ReturnValue.DirectoryPath = DirectoryPath;
        DocumentPathFileIterator PathIterator(Files, Depth, Begin, End);
        
        PathIterator.NextDirectory();
        
        for (size_t i = 0; i < PathIterator.GetCurrentOffset(); i++)
        {
            ReturnValue.FileNames.push_back(Files[Begin + i][Files[Begin + i].ComponentCount() - 1]);
        }
        while (!PathIterator.HasEnded())
        {
            std::string CurrentDirectoryName = PathIterator.GetDirectoryName();
            DocumentFilesystem::BuildDirectory NewDirectory = h_ParseBuildDirectory(Files,DirectoryPath/CurrentDirectoryName, Depth + 1, Begin + PathIterator.GetCurrentOffset(),Begin+PathIterator.GetDirectoryEnd());
            ReturnValue.SubDirectories.push_back(std::pair<std::string, DocumentFilesystem::BuildDirectory>(CurrentDirectoryName, std::move(NewDirectory)));
            PathIterator.NextDirectory();
        }

        return(ReturnValue);
    }

    DocumentFilesystem::BuildDirectory DocumentFilesystem::p_ParseBuildDirectory(DocumentBuild const& BuildToParse)
    {
        BuildDirectory ReturnValue;
        ReturnValue.DirectoryPath = BuildToParse.BuildRootDirectory;
        DocumentPathFileIterator PathIterator(BuildToParse.BuildFiles,0,0,BuildToParse.BuildFiles.size());
        PathIterator.NextDirectory();
        for (size_t i = 0; i < PathIterator.GetCurrentOffset(); i++)
        {
            ReturnValue.FileNames.push_back(BuildToParse.BuildFiles[i][BuildToParse.BuildFiles[i].ComponentCount() - 1]);
        }
        while (PathIterator.HasEnded() == false)
        {
            std::string DirectoryName = PathIterator.GetDirectoryName();
            DocumentFilesystem::BuildDirectory NewDirectory = h_ParseBuildDirectory(BuildToParse.BuildFiles,BuildToParse.BuildRootDirectory/DirectoryName, 1, PathIterator.GetCurrentOffset(), PathIterator.GetDirectoryEnd());
            ReturnValue.SubDirectories.push_back(std::pair<std::string, DocumentFilesystem::BuildDirectory>(std::move(DirectoryName), std::move(NewDirectory)));
            PathIterator.NextDirectory();
        }
        for (auto const& SubBuild : BuildToParse.SubBuilds)
        {
            ReturnValue.SubDirectories.push_back(std::pair<std::string, BuildDirectory>(SubBuild.first, p_ParseBuildDirectory(SubBuild.second)));
        }
        std::sort(ReturnValue.SubDirectories.begin(), ReturnValue.SubDirectories.end(), h_PairStringOrder);
        return(ReturnValue);
    }
    DocumentDirectoryInfo DocumentFilesystem::p_UpdateOverDirectory(BuildDirectory const& Directory, size_t FileIndexBegin, size_t DirectoryIndex)
    {
        DocumentDirectoryInfo ReturnValue;
        size_t NewDirectoryIndexBegin = m_DirectoryInfos.size();
        ReturnValue.DirectoryIndexBegin = NewDirectoryIndexBegin;
        ReturnValue.DirectoryIndexEnd = NewDirectoryIndexBegin+Directory.SubDirectories.size();
        ReturnValue.FileIndexBegin = FileIndexBegin;
        ReturnValue.FileIndexEnd = FileIndexBegin+Directory.FileNames.size();
        
        MBError ParseError = true;
        size_t FileIndex = 0;
        for (std::string const& File : Directory.FileNames)
        {
            DocumentParsingContext Parser;
            DocumentSource NewFile = Parser.ParseSource(Directory.DirectoryPath / File, ParseError);
            if (!ParseError)
            {
                throw new std::runtime_error("Error parsing file: " + ParseError.ErrorMessage);
            }
            m_TotalSources[FileIndexBegin + FileIndex] = std::move(NewFile);
            FileIndex++;
        }

        size_t TotalSubFiles = 0;
        size_t TotalDirectories = Directory.SubDirectories.size();
        for (auto const& SubDirectory : Directory.SubDirectories) 
        {
            TotalSubFiles += SubDirectory.second.FileNames.size();
        }
        size_t NewFileIndexBegin = m_TotalSources.size();
        m_TotalSources.resize(TotalSubFiles + m_TotalSources.size());
        m_DirectoryInfos.resize(m_DirectoryInfos.size() + Directory.SubDirectories.size());
        size_t DirectoryOffset = 0;
        size_t FileOffset = 0;
        for (auto const& SubDirectory : Directory.SubDirectories)
        {
            m_DirectoryInfos[NewDirectoryIndexBegin + DirectoryOffset] = p_UpdateOverDirectory(SubDirectory.second, NewFileIndexBegin + FileOffset, NewDirectoryIndexBegin + DirectoryOffset);
            m_DirectoryInfos[NewDirectoryIndexBegin + DirectoryOffset].Name = SubDirectory.first;
            m_DirectoryInfos[NewDirectoryIndexBegin + DirectoryOffset].ParentDirectoryIndex = DirectoryIndex;
            DirectoryOffset++;
            FileOffset += SubDirectory.second.FileNames.size();
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
    MBError DocumentFilesystem::CreateDocumentFilesystem(DocumentBuild const& BuildToParse,DocumentFilesystem* OutFilesystem)
    {
        MBError ReturnValue = true;
        try 
        {
            DocumentFilesystem Result;
            Result.m_DirectoryInfos.resize(1);
            BuildDirectory Directory = p_ParseBuildDirectory(BuildToParse);
            Result.m_TotalSources.resize(Directory.FileNames.size());
            DocumentDirectoryInfo TopInfo = Result.p_UpdateOverDirectory(Directory, 0, 0);
            Result.m_DirectoryInfos[0] = TopInfo;
            Result.m_DirectoryInfos[0].Name = "/";
            assert(std::is_sorted(Result.m_DirectoryInfos.begin(), Result.m_DirectoryInfos.end(), h_DirectoryOrder));
            if (ReturnValue)
            {
                *OutFilesystem = std::move(Result);
            }
        }
        catch (std::exception const& e) 
        {
            ReturnValue = false;
            ReturnValue.ErrorMessage = e.what();
        }
        return(ReturnValue);  
    }
    //END DocumentFilesystem

    
    

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
    void MarkdownCompiler::Compile(DocumentFilesystem const& BuildToCompile, CommonCompilationOptions const& Options)
    {
        std::cout << "Not implemented B)" << std::endl;
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
    //
    
    //BEGIN HTTPReferenceSolver
    void HTTPReferenceSolver::SetCurrentPath(DocumentPath CurrentPath)
    {
        m_CurrentPath = std::move(CurrentPath);
    }
    std::string HTTPReferenceSolver::GetReferenceString(DocReference const& ReferenceIdentifier) const
    {
        std::string ReturnValue = "<a href=\"";
        MBError OutResult = true; 
        DocumentPath ReferencePath = m_AssociatedBuild->ResolveReference(m_CurrentPath, ReferenceIdentifier.Identifier, OutResult);
        if (OutResult)
        {
            std::string Path = DocumentPath::GetRelativePath(ReferencePath, m_CurrentPath).GetString();
            ReturnValue += MBUtility::ReplaceAll(Path,".mbd",".html");
            ReturnValue += "\">";
            if (ReferenceIdentifier.VisibleText == "")
            {
                ReturnValue += ReferencePath.GetString();
            }
            else
            {
                ReturnValue += ReferenceIdentifier.VisibleText;
            }
        }
        else
        {
            ReturnValue += "#\" style=\"color: red;\">"+ReferenceIdentifier.VisibleText;
        }
        ReturnValue += "</a>";
        return(ReturnValue);
    }
    void HTTPReferenceSolver::Initialize(DocumentFilesystem const* Build)
    {
        m_AssociatedBuild = Build;
    }
    //END HTTPReferenceSolver


    //BEGIN HTTPCompiler

    void HTTPCompiler::p_CompileText(std::vector<std::unique_ptr<TextElement>> const& ElementsToCompile,HTTPReferenceSolver const& HTTPReferenceSolverReferenceSolver,MBUtility::MBOctetOutputStream& OutStream)
    {
        for(std::unique_ptr<TextElement> const& Text : ElementsToCompile)
        {
            TextModifier Modifiers = Text->Modifiers;
            if ((Modifiers & TextModifier::Bold) != TextModifier::Null)
            {
                OutStream.Write("<b>", 3);
            }
            if ((Modifiers & TextModifier::Italic) != TextModifier::Null)
            {
                OutStream.Write("<i>", 3);
            }
            if(Text->Type == TextElementType::Regular)
            {
                RegularText const& TextToWrite = static_cast<RegularText const&>(*Text);
                OutStream.Write(TextToWrite.Text.data(),TextToWrite.Text.size());
            }
            if (Text->Type == TextElementType::Reference)
            {
                DocReference const& ReferenceToWrite = static_cast<DocReference const&>(*Text);
                std::string StringToWrite = HTTPReferenceSolverReferenceSolver.GetReferenceString(ReferenceToWrite);
                OutStream.Write(StringToWrite.data(), StringToWrite.size());
            }
            if ((Modifiers & TextModifier::Bold) != TextModifier::Null)
            {
                OutStream.Write("</b>", 4);
            }
            if ((Modifiers & TextModifier::Italic) != TextModifier::Null)
            {
                OutStream.Write("</i>", 4);
            }
        }
    }
    void HTTPCompiler::p_CompileBlock(BlockElement const* BlockToCompile, HTTPReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream)
    {
        if(BlockToCompile->Type == BlockElementType::Paragraph)
        {
            if (BlockToCompile->Attributes.HasAttribute("Note"))
            {
                OutStream.Write("<mark>Note: </mark>", 19);
            }
            Paragraph const& ParagraphToWrite = static_cast<Paragraph const&>(*BlockToCompile);    
            p_CompileText(ParagraphToWrite.TextElements,ReferenceSolver,OutStream); 
            OutStream.Write("<br><br>\n\n", 10);
        }
        else if(BlockToCompile->Type == BlockElementType::CodeBlock)
        {
            OutStream.Write("<pre>", 5);
            CodeBlock const& BlockToWrite = static_cast<CodeBlock const&>(*BlockToCompile);
            OutStream.Write(BlockToWrite.RawText.data(), BlockToWrite.RawText.size());
            OutStream.Write("</pre>", 6);
        } 
    }
    void HTTPCompiler::p_CompileDirective(Directive const& DirectiveToCompile, HTTPReferenceSolver const& ReferenceSolver, MBUtility::MBOctetOutputStream& OutStream)
    {
        if (DirectiveToCompile.DirectiveName == "title")
        {
            if (DirectiveToCompile.Arguments.PositionalArgumentsCount() > 0)
            {
                std::string StringToWrite = "<h1 style=\"text-align: center;\">" + DirectiveToCompile.Arguments[0] + "</h1>\n";
                OutStream.Write(StringToWrite.data(), StringToWrite.size());
            }
        }
    }
    void HTTPCompiler::p_CompileFormat(FormatElement const& FormatToCompile, HTTPReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream,int Depth,
        std::string const& NamePrefix)
    {
        std::string HeadingTag = "h"+std::to_string(Depth+2);
        if(FormatToCompile.Type != FormatElementType::Default)
        {
            std::string StringToWrite = "<" + HeadingTag + " id=\"" + FormatToCompile.Name+"\" ";
            if (Depth == 0)
            {
                StringToWrite += "style = \"text-align: center;\"";
            }
            StringToWrite += ">";
            
            StringToWrite += NamePrefix +" "+ FormatToCompile.Name + "</" + HeadingTag + "><br>\n";
            
            OutStream.Write(StringToWrite.data(),StringToWrite.size());
        }
        size_t FormatOffset = 1;
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
                p_CompileFormat(Component.GetFormatData(),ReferenceSolver,OutStream,Depth+1,NamePrefix+"."+std::to_string(FormatOffset));
                FormatOffset++;
            }
        } 
    }
    void HTTPCompiler::p_CompileSource(std::string const& OutPath, DocumentSource const& SourceToCompile, HTTPReferenceSolver const& ReferenceSolver)
    {
        std::string OutFile = OutPath;
        size_t FirstDot = OutFile.find_last_of('.');
        if (FirstDot == OutFile.npos)
        {
            OutFile += ".html";
        }
        else
        {
            OutFile.replace(FirstDot, 5, ".html");
        }
        std::filesystem::path FilePath = OutPath;
        if (!std::filesystem::exists(FilePath.parent_path()))
        {
            std::filesystem::create_directories(FilePath.parent_path());
        }
        std::ofstream OutStream = std::ofstream(OutFile);
        if (!OutStream.is_open())
        {
            throw std::runtime_error("File not open");
        }
        MBUtility::MBFileOutputStream FileStream(&OutStream);
        std::string TopInfo = "<!DOCTYPE html><html><head><style>body{background-color: black; color: #00FF00;}</style></head><body><div style=\"width: 50%;margin: auto\">";
        OutStream.write(TopInfo.data(), TopInfo.size());
        size_t ElementIndex = 1;
        for (FormatElement const& Format : SourceToCompile.Contents)
        {
            p_CompileFormat(Format, ReferenceSolver, FileStream, 0, std::to_string(ElementIndex));
            if (Format.Type != FormatElementType::Default)
            {
                ElementIndex++;
            }
        }
        OutStream.write("</div></body></html>", 20);
        OutStream.flush();
    }
    void HTTPCompiler::Compile(DocumentFilesystem const& BuildToCompile,CommonCompilationOptions const& Options)
    {
        HTTPReferenceSolver ReferenceSolver;
        ReferenceSolver.Initialize(&BuildToCompile);
        DocumentFilesystemIterator FileIterator = BuildToCompile.begin();
        while (!FileIterator.HasEnded())
        {
            if (!FileIterator.EntryIsDirectory())
            {
                DocumentPath CurrentPath = FileIterator.GetCurrentPath();
                DocumentSource const& SourceToCompile = FileIterator.GetDocumentInfo();
                ReferenceSolver.SetCurrentPath(CurrentPath);
                p_CompileSource(Options.OutputDirectory+"/" + CurrentPath.GetString(), SourceToCompile, ReferenceSolver);
            }
            FileIterator++;
        }
    }

    //END HTTPCompiler
}
