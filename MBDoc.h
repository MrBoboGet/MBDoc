#pragma once
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <MBUtility/MBInterfaces.h>
#include <MBUtility/MBErrorHandling.h>
#include <MBUtility/MBInterfaces.h>
#include <unordered_set>
#include <set>
#include <variant>
#include <assert.h>

#include "MBCLI/MBCLI.h"
#include "MBDoc.h"

#include <MBUtility/Iterator.h>

#include <MBLSP/MBLSP.h>

#include "ColorConf.h"
#include "LSP.h"
#include "MBLSP/LSP_Structs.h"

#include "MBParsing/JSON.h"


#include <MBLSP/MBLSP.h>

#include <MBUtility/Optional.h>
#include <MBUtility/LineRetriever.h>

#include "Coloring.h"
#include "LSPUtils.h"

namespace MBDoc
{

    class DocumentPath
    {
    private:
        bool m_IsAbsolute = false;
        std::vector<std::string> m_PathComponents;
        std::vector<std::string> m_PartIdentifier;
    public:
        //Requires that both files are relative
        static DocumentPath GetRelativePath(DocumentPath const& TargetObject, DocumentPath const& CurrentObject);
        bool IsSubPath(DocumentPath const& PathToCompare) const;
        std::string GetString() const;
        size_t ComponentCount() const;

        std::string operator[](size_t ComponentIndex) const;
        bool operator<(DocumentPath const& OtherPath) const;

        bool operator==(DocumentPath const& PathToCompare) const;
        bool operator!=(DocumentPath const& PathToCompare) const;

        static DocumentPath ParsePath(std::string const& PathToParse, MBError& OutError);

        void AddDirectory(std::string StringToAdd);
        void PopDirectory();
        void SetPartIdentifier(std::vector<std::string> PartSpecifier);
        std::vector<std::string> const& GetPartIdentifier() const;

        bool Empty() const;
    };
    // Syntax completely line based
    
    
    //@Identifier <- Implicit visible text ,@[VisibleText](LinkIdentifier)

    //Format elements can contain other format elements, and contain block elements, which can contatin text elements, but no intermixin
    
    
        
    //Implicit whitespace normalization, extra spaces and newlines aren't preserved, which means that a sequence of text elements are not ambigous   

    
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
    //sligthly hacky, but probably fine as these elements should only be read by compilers

    enum class TextElementType
    {
        Null,
        Regular,
        Literal,     
        Reference,
        Custom,
    };
    class TextVisitor;
    struct TextElement_Base
    {
        TextModifier Modifiers;
        TextColor Color;
    };

    struct RegularText : TextElement_Base
    {
        std::string Text;   
    };

    class ReferenceVisitor;
    struct DocReference : public TextElement_Base
    {
        //all references can be overriden with visible text, but may 
        //not neccesarilly be neccessary for references
        std::string VisibleText;
        MBLSP::Position Begin;
        //sentinel for use in LSP when determining of there was a parser error
        MBLSP::Position End = MBLSP::Position{-1,-1};
        virtual void Accept(ReferenceVisitor& Visitor) const = 0;
        virtual void Accept(ReferenceVisitor& Visitor) = 0;
        virtual std::unique_ptr<DocReference> Copy() = 0;
        virtual ~DocReference()
        {
               
        }
        template<typename T>
        bool IsType() const
        {
            //TODO make so this interface is not necessary...
            if(dynamic_cast<T const*>(this) != nullptr)
            {
                return true;
            }
            return false;
        }
        template<typename T>
        T const& GetType() const
        {
            //TODO make so this interface is not necessary...
            auto Pointer = dynamic_cast<T const*>(this);
            if(Pointer != nullptr)
            {
                return *Pointer;
            }
            throw std::runtime_error("Invalid type cast when using DocReference");
        }
    };

    struct FileReference : public DocReference
    {
        DocumentPath Path;
        virtual void Accept(ReferenceVisitor& Visitor) const;
        virtual void Accept(ReferenceVisitor& Visitor);
        std::unique_ptr<DocReference> Copy()
        {
            return std::make_unique<FileReference>(*this);
        }
        ~FileReference()
        {
               
        }
    };
    struct ResourceReference : public DocReference
    {
        std::string ResourceCanonicalPath;
        int ID = -1;
        virtual void Accept(ReferenceVisitor& Visitor) const;
        virtual void Accept(ReferenceVisitor& Visitor);
        std::unique_ptr<DocReference> Copy()
        {
            return std::make_unique<ResourceReference>(*this);
        }
        ~ResourceReference()
        {
               
        }
    };
    struct URLReference : public DocReference
    {
        std::string URL; 
        virtual void Accept(ReferenceVisitor& Visitor) const;
        virtual void Accept(ReferenceVisitor& Visitor);
        std::unique_ptr<DocReference> Copy()
        {
            return std::make_unique<URLReference>(*this);
        }
        ~URLReference()
        {
               
        }
    };
    struct UnresolvedReference : public DocReference
    {
        std::string ReferenceString;         
        virtual void Accept(ReferenceVisitor& Visitor) const;
        virtual void Accept(ReferenceVisitor& Visitor);
        std::unique_ptr<DocReference> Copy()
        {
            return std::make_unique<UnresolvedReference>(*this);
        }
        ~UnresolvedReference()
        {
               
        }
    };
    
    class ReferenceVisitor
    {
    public:
        virtual void Visit(FileReference const& FileRef){};
        virtual void Visit(URLReference const& URLRef){};
        virtual void Visit(UnresolvedReference const& UnresolvedRef){};
        virtual void Visit(ResourceReference const& UnresolvedRef){};

        virtual void Visit(FileReference& FileRef){};
        virtual void Visit(URLReference& URLRef){};
        virtual void Visit(UnresolvedReference& UnresolvedRef){};
        virtual void Visit(ResourceReference& UnresolvedRef){};


        virtual ~ReferenceVisitor() {};
    };

    struct TextElement
    {

    private:
        std::variant<RegularText,std::unique_ptr<DocReference>> m_Content;
    public:
        TextElement(TextElement&& ElemToMove) noexcept
        {
            m_Content = std::move(ElemToMove.m_Content);    
        }
        TextElement(TextElement const& OtherElement)
        {
            if(std::holds_alternative<RegularText>(OtherElement.m_Content))
            {
                m_Content = std::get<RegularText>(OtherElement.m_Content);
            }
            else
            {
                m_Content = std::get<std::unique_ptr<DocReference>>(OtherElement.m_Content)->Copy();
            }
        }
        template<typename T>
        TextElement(T ValueToStore)
        {
            if constexpr(std::is_base_of<DocReference,T>::value)
            {
                m_Content = std::unique_ptr<DocReference>(new T(std::move(ValueToStore)));
            }
            else
            {
                static_assert(!(sizeof(T)+1),"Element to assign cannot possibly be a TextElement");
            }
        }
        TextElement(RegularText Text)
        {
            m_Content = std::move(Text);   
        }
        TextElement(std::unique_ptr<DocReference> Ref)
        {
            m_Content = std::move(Ref);   
        }
        //hack purely because gcc is non-conformant and doesn't allow 
        //specializations in class scope, if i understand the issue correctly
        //https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282
        template<typename T>
        bool IsType() const
        {
            if constexpr(std::is_same<T,DocReference>::value)
            {
                return(std::holds_alternative<std::unique_ptr<DocReference>>(m_Content)); 
            }
            else
            {
                return(std::holds_alternative<T>(m_Content)); 
            }
        }

        template<typename T>
        T const& GetType() const
        {
            if constexpr(std::is_same<T,DocReference>::value)
            {
                return(*std::get<std::unique_ptr<DocReference>>(m_Content));    
            }
            else
            {
                return(std::get<T>(m_Content));    
            }
        }

        template<typename T>
        T& GetType() 
        {
            if constexpr(std::is_same<T,DocReference>::value)
            {
                return(*std::get<std::unique_ptr<DocReference>>(m_Content));    
            }
            else
            {
                return(std::get<T>(m_Content));    
            }
        }
        template<typename T>
        TextElement& operator=(T ObjectToSteal)
        {
            if constexpr(std::is_base_of<DocReference,T>::value)
            {
                m_Content = std::unique_ptr<DocReference>(new T(std::move(ObjectToSteal)));
                return(*this);
            }
            else
            {
                m_Content = std::move(ObjectToSteal);
                return(*this);
            }
        }
        TextElement& operator=(TextElement ObjectToSteal)
        {
            m_Content = std::move(ObjectToSteal.m_Content);
            return(*this);
        }
        TextElement_Base const& GetBase() const
        {
            if(IsType<DocReference>())
            {
                return(GetType<DocReference>());
            } 
            else if(IsType<RegularText>())
            {
                return(GetType<RegularText>());
            }
            assert(false && "GetBase doesn't cover all cases");
            throw std::runtime_error("Invalid element stored in TextElement");
        }
        TextElement_Base& GetBase() 
        {
            if(IsType<DocReference>())
            {
                return(GetType<DocReference>());
            } 
            else if(IsType<RegularText>())
            {
                return(GetType<RegularText>());
            }
            assert(false && "GetBase doesn't cover all cases");
            throw std::runtime_error("Invalid element stored in TextElement");
        }
        void Accept(TextVisitor& Visitor) const;
        void Accept(TextVisitor& Visitor);
        TextElement() {};
    };
    



    class TextVisitor
    {
    public:
        virtual void Visit(RegularText const& VisitedText){};
        virtual void Visit(DocReference const& VisitedText) {};

        virtual void Visit(RegularText& VisitedText){};
        virtual void Visit(DocReference& VisitedText) {};

        virtual ~TextVisitor() {};
    };
    //Text elements
    class AttributeList
    {
    private:
        std::unordered_set<std::string> m_Attributes;
    public:
        bool IsEmpty() const;
        void Clear();
        void AddAttribute(std::string const& AttributeName);
        bool HasAttribute(std::string const& AttributeToCheck) const;
    };
    //Block elements
    enum class BlockElementType
    {
        Null,
        Paragraph,
        MediaInclude,
        Table,
        CodeBlock,
        List,
    };
    class BlockVisitor;

    struct Paragraph;
    struct MediaInclude;
    struct CodeBlock;
    struct Table;
    struct List;
    struct BlockElement
    {
    protected:
        BlockElementType Type = BlockElementType::Null;
    public:
        AttributeList Attributes;
        void Accept(BlockVisitor& Visitor) const;
        void Accept(BlockVisitor& Visitor);
        template<typename T>
        bool IsType() const
        {
            if constexpr(std::is_same<Paragraph,T>::value)
            {
                return(Type == BlockElementType::Paragraph);
            }    
            else if constexpr(std::is_same<CodeBlock,T>::value)
            {
                return(Type == BlockElementType::CodeBlock);
            }
            else if constexpr(std::is_same<MediaInclude,T>::value)
            {
                return(Type == BlockElementType::MediaInclude);
            }
            else if constexpr(std::is_same<Table,T>::value)
            {
                return(Type == BlockElementType::Table);
            }
            else if constexpr(std::is_same<List,T>::value)
            {
                return(Type == BlockElementType::List);
            }
            else if constexpr(true)
            {
                //hack, apparently the static_assert needs to be dependant on the template 
                //argument in order to not always dissallow compilation 
                //https://stackoverflow.com/questions/38304847/constexpr-if-and-static-assert
                static_assert(!(sizeof(T)+1),"BlockElement cannot possibly be of supplied type");
            }
        }
        template<typename T>
        T& GetType();
        template<typename T>
        T const& GetType() const;
        virtual ~BlockElement(){};
    };
    struct Paragraph : public BlockElement
    {
        Paragraph() 
        {
            Type = BlockElementType::Paragraph; 
        };
        Paragraph(std::vector<TextElement> Content)
        {
            Type = BlockElementType::Paragraph;
            TextElements = std::move(Content);
        }
        Paragraph(Paragraph&& ParToMove) noexcept
        {
            Attributes = std::move(ParToMove.Attributes);
            TextElements = std::move(ParToMove.TextElements);  
            Type = ParToMove.Type;
        }
        Paragraph& operator=(Paragraph ParagraphToCopy)
        {
            Type = BlockElementType::Paragraph;
            std::swap(TextElements,ParagraphToCopy.TextElements);
            return(*this);
        }
        std::vector<TextElement> TextElements;//Can be sentences, text, inline references etc
    };
    struct Table : public BlockElement
    {
    private:
        int m_Width = 0;
        int m_Height = 0;
        std::vector<Paragraph> m_Contents;
        std::vector<std::string> m_ColumnNames;
    public:
        Table()
        {
            Type = BlockElementType::Table;
        }
        class TableRowIterator_Const : public MBUtility::Iterator_Base<TableRowIterator_Const,
            MBUtility::RangeIterable<Paragraph const*>>
        {
        protected:
            Table const* m_AssociatedTable = nullptr;
            int m_RowOffset = 0;
            MBUtility::RangeIterable<Paragraph const*> m_Iterator;
        public:      
            TableRowIterator_Const(){};
            TableRowIterator_Const(Table const* AssociatedTable,int RowOffset)
            {
                m_AssociatedTable = AssociatedTable;
                m_RowOffset = RowOffset;
            }
            void Increment()
            {
                m_RowOffset++;   
            }
            MBUtility::RangeIterable<Paragraph const*>& GetRef()
            {
                auto Begin = &(*m_AssociatedTable)(m_RowOffset,0);
                m_Iterator = MBUtility::RangeIterable<Paragraph const*>(Begin,Begin+m_AssociatedTable->m_Width); 
                return(m_Iterator);
            }
            bool IsEqual(TableRowIterator_Const const& RHS) const
            {
                return(m_RowOffset == RHS.m_RowOffset);
            }
        };
        class TableRowIterator :  public MBUtility::Iterator_Base<TableRowIterator,
            MBUtility::RangeIterable<Paragraph*>>
        {
            Table* m_AssociatedTable = nullptr;
            int m_RowOffset = 0;
            MBUtility::RangeIterable<Paragraph*> m_Iterator;
        public: 
            TableRowIterator(){};
            TableRowIterator(Table* AssociatedTable,int RowOffset)
            {
                m_AssociatedTable = AssociatedTable;
                m_RowOffset = RowOffset;
            }
            void Increment()
            {
                m_RowOffset++;   
            }
            MBUtility::RangeIterable<Paragraph*>& GetRef()
            {
                auto Begin = &(*m_AssociatedTable)(m_RowOffset,0);
                m_Iterator = MBUtility::RangeIterable<Paragraph*>(Begin,Begin+m_AssociatedTable->m_Width);
                return(m_Iterator);
            }
            bool IsEqual(TableRowIterator const& RHS) const
            {
                return(m_RowOffset == RHS.m_RowOffset);
            }

        };
        TableRowIterator begin()
        {
            return(TableRowIterator(this,0));   
        }
        TableRowIterator end()
        {
            return(TableRowIterator(this,m_Height));   
        }
        TableRowIterator_Const begin() const
        {
            return(TableRowIterator_Const(this,0));   
        }
        TableRowIterator_Const end() const
        {
            return(TableRowIterator_Const(this,m_Height));   
        }
        //Throws exception if Content.size()%Width != 0
        Table(std::vector<std::string> ColumnNames /*may be zero to indicate no names are given*/, int Width,std::vector<std::vector<TextElement>>
                Content)
        {
            Type = BlockElementType::Table;
            if(Content.size() % Width != 0)
            {
                throw std::runtime_error("Invalid table size");  
            } 
            m_ColumnNames = std::move(ColumnNames);
            m_Width = Width;
            m_Height = Content.size()/Width;
            for(int i = 0; i < Content.size();i++)
            {
                Paragraph NewParagraph; 
                NewParagraph.TextElements = std::move(Content[i]);
                m_Contents.push_back(std::move(NewParagraph));
            }
        }
        bool HasColumnNames() const {return m_ColumnNames.size() != 0;};
        std::vector<std::string> const& GetColumnNames() 
        {
            if(!HasColumnNames()) 
                throw std::runtime_error("Table has no column names");
            return m_ColumnNames;
        };
        int ColumnCount() const {return m_Width;};
        int RowCount() const{return m_Height;};
        Paragraph& operator()(int Row,int Column){return(m_Contents[Row*m_Width+Column]);}
        Paragraph const& operator()(int Row,int Column) const{return(m_Contents[Row*m_Width+Column]);}
    };
    struct List : public BlockElement
    {
        List()
        {
            Type = BlockElementType::List;   
        }
        //TODO, Improve, kind of arbitrary and ugly
        struct ListContent
        {
            Paragraph Text;
            std::unique_ptr<List> SubList;
        };
        void AddParagraph(Paragraph ParagraphToAdd)
        {
            ListContent NewElement;
            NewElement.Text = std::move(ParagraphToAdd);
            Content.push_back(std::move(NewElement));
        }
        std::vector<ListContent> Content;
    };
    struct MediaInclude : public BlockElement
    {
        MediaInclude() 
        {
            Type = BlockElementType::MediaInclude; 
        };
        std::string MediaPath;
        int ID = -1;
    };










    class DocumentParsingContext;
    class DocumentFilesystem;
    class ResolvedCodeText
    {
    private:
        std::vector<std::vector<TextElement>> m_Rows;
    protected:
        friend DocumentParsingContext;
        friend DocumentFilesystem;
        ResolvedCodeText(std::vector<std::vector<TextElement>> Content)
        {
            m_Rows = std::move(Content);   
        }
    public:     
        ResolvedCodeText()
        {
               
        }
        //do not depend on the returned class, other than to use as an iterator
        class RowIterator : public MBUtility::Iterator_Base<RowIterator,const std::vector<TextElement>>
        {
        private:
            std::vector<std::vector<TextElement>>::const_iterator m_CurrentValue;
            std::vector<std::vector<TextElement>>::const_iterator m_End;
        public:
            RowIterator(std::vector<std::vector<TextElement>>::const_iterator Begin,std::vector<std::vector<TextElement>>::const_iterator  End)
            {
                m_CurrentValue = std::move(Begin);    
                m_End = std::move(End);
            }
            void Increment()
            {
                ++m_CurrentValue;
            }
            std::vector<TextElement> const& GetRef()
            {
                return(*m_CurrentValue);    
            }
            bool IsEqual(RowIterator const& RHS) const
            {
                return(m_CurrentValue == RHS.m_CurrentValue);
            }
        };
        class RowIterator_Mut : public MBUtility::Iterator_Base<RowIterator_Mut,std::vector<TextElement>>
        {
        private:
            std::vector<std::vector<TextElement>>::iterator m_CurrentValue;
            std::vector<std::vector<TextElement>>::iterator m_End;
        public:
            RowIterator_Mut(std::vector<std::vector<TextElement>>::iterator Begin,std::vector<std::vector<TextElement>>::iterator End)
            {
                m_CurrentValue = std::move(Begin);    
                m_End = std::move(End);
            }
            void Increment()
            {
                ++m_CurrentValue;
            }
            std::vector<TextElement>& GetRef()
            {
                return(*m_CurrentValue);    
            }
            bool IsEqual(RowIterator_Mut const& RHS) const
            {
                return(m_CurrentValue == RHS.m_CurrentValue);
            }
        };
        RowIterator_Mut begin()
        {
            return(RowIterator_Mut(m_Rows.begin(),m_Rows.end()));    
        }
        RowIterator_Mut end() 
        {
            return(RowIterator_Mut(m_Rows.end(),m_Rows.end()));    
        }
        RowIterator begin() const
        {
            return(RowIterator(m_Rows.begin(),m_Rows.end()));    
        }
        RowIterator end() const
        {
            return(RowIterator(m_Rows.end(),m_Rows.end()));    
        }
    };

    struct CodeBlock : BlockElement 
    {
        CodeBlock() 
        {
            Type = BlockElementType::CodeBlock;
        }
        int LineBegin = 0;
        int LineEnd = 0;
        std::string CodeType;
        std::string Content;
        MBUtility::Optional<ResolvedCodeText> HighlightedText;
    };
    template<typename T>
    T& BlockElement::GetType()
    {
        if constexpr(std::is_same<Paragraph,T>::value)
        {
            return(static_cast<Paragraph&>(*this));
        }    
        else if constexpr(std::is_same<CodeBlock,T>::value)
        {
            return(static_cast<CodeBlock&>(*this));
        }
        else if constexpr(std::is_same<MediaInclude,T>::value)
        {
            return(static_cast<MediaInclude&>(*this));
        }
        else if constexpr(std::is_same<Table,T>::value)
        {
            return(static_cast<Table&>(*this));
        }
        else if constexpr(std::is_same<List,T>::value)
        {
            return(static_cast<List&>(*this));
        }
        else
        {
            static_assert(!(sizeof(T)+1),"BlockElement cannot possibly be of supplied type");
        }
    }
    template<typename T>
    T const& BlockElement::GetType() const
    {
        if constexpr(std::is_same<Paragraph,T>::value)
        {
            return(static_cast<Paragraph const&>(*this));
        }    
        else if constexpr(std::is_same<CodeBlock,T>::value)
        {
            return(static_cast<CodeBlock const&>(*this));
        }
        else if constexpr(std::is_same<MediaInclude,T>::value)
        {
            return(static_cast<MediaInclude const&>(*this));
        }
        else if constexpr(std::is_same<Table,T>::value)
        {
            return(static_cast<Table const&>(*this));
        }
        else if constexpr(std::is_same<List,T>::value)
        {
            return(static_cast<List const&>(*this));
        }
        else
        {
            static_assert(!(sizeof(T)+1),"BlockElement cannot possibly be of supplied type");
        }

    }

    class BlockVisitor
    {
    public:
        virtual void Visit(Paragraph const& VisitedParagraph) {};
        virtual void Visit(Table const& VisitedParagraph) {};
        virtual void Visit(MediaInclude const& VisitedMedia) {};
        virtual void Visit(CodeBlock const& CodeBlock) {};
        virtual void Visit(List const& CodeBlock) {};

        virtual void Visit(Paragraph& VisitedParagraph) {};
        virtual void Visit(Table& VisitedParagraph) {};
        virtual void Visit(MediaInclude& VisitedMedia) {};
        virtual void Visit(CodeBlock& CodeBlock) {};
        virtual void Visit(List& CodeBlock) {};
        virtual ~BlockVisitor() {};
    };
    //Block elements
   
    //Directives
    class ArgumentList
    {
    private:
        std::vector<std::string> m_PositionalArguments;
        std::unordered_map<std::string, std::string> m_NamedArguments;
    public:
        std::string const& operator[](size_t Index) const;
        std::string const& operator[](std::string const& AttributeName) const;
        bool HasArgument(std::string const& AttributeName) const;
        size_t PositionalArgumentsCount() const;

        void AddArgument(std::pair<std::string, std::string> NewAttribute);
        void AddArgument(std::string ArgumentToAdd);
    };
    struct Directive
    {
        std::string DirectiveName;
        ArgumentList Arguments;
    };
    //

    //Format elements 
    //Starts with #, default new section name
    //optionally ends with /#

    enum class FormatComponentType
    {
        Null,
        Format,
        Directive,
        Block,
    };
    class FormatVisitor; 
    class FormatElement;
    class FormatElementComponent
    {
    private:
        FormatComponentType m_Type = FormatComponentType::Null;
        void* m_Data = nullptr;
    public:
        FormatElementComponent() {};

        FormatElementComponent(FormatElementComponent&& ElementToSteal) noexcept;

        FormatElementComponent(FormatElementComponent const&) = delete;
        
        FormatElementComponent(std::unique_ptr<BlockElement> BlockData);
        FormatElementComponent(Directive DirectiveData);
        FormatElementComponent(FormatElement DirectieData);

        FormatElementComponent& operator=(FormatElementComponent FormatToCopy);

        void SetAttributes(AttributeList NewAttributes);
        
        FormatComponentType GetType() const;
        BlockElement& GetBlockData();
        BlockElement const& GetBlockData() const;
        Directive& GetDirectiveData();
        Directive const& GetDirectiveData() const;
        FormatElement& GetFormatData();
        FormatElement const& GetFormatData() const;
        ~FormatElementComponent();

        void Accept(FormatVisitor& Visitor) const;
        void Accept(FormatVisitor& Visitor);
    };
    struct FormatElement
    {
        FormatElement(FormatElement &&) = default;
        FormatElement(FormatElement const&) = delete;
        FormatElement() {};
        std::string Name;
        AttributeList  Attributes;
        std::vector<FormatElementComponent> Contents;    
    };
    
    class DirectiveVisitor
    {
    public:
        virtual void Visit(Directive const& DirectiveToVisit){};
        virtual void Visit(Directive& DirectiveToVisit){};
    };

    class FormatVisitor : public DirectiveVisitor
    {
    public: 
        using DirectiveVisitor::Visit;
        virtual void Visit(BlockElement const& BlockToVisit) {};
        virtual void Visit(FormatElement const& FormatToVisit){};
        virtual void Visit(BlockElement& BlockToVisit) {};
        virtual void Visit(FormatElement& FormatToVisit){};
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
    //
    class DocumentSource;
    class ReferenceTargetsResolver
    {
        struct LabelNode
        {
            std::string Name;      
            std::vector<LabelNode> Children;
            bool operator<(LabelNode const& rhs) 
            {
                return(Name < rhs.Name);
            }
        };
        typedef int LabelNodeIndex;
        struct FlatLabelNode
        {
            std::string Name;
            LabelNodeIndex ChildrenBegin = 0; 
            LabelNodeIndex ChildrenEnd = 0; 
        };
        std::vector<FlatLabelNode> m_Nodes;
        
        static MBParsing::JSONObject p_ToJSON(std::vector<FlatLabelNode> const&, size_t Index);
        static LabelNode p_FromJSON(MBParsing::JSONObject const& ObjectToParse);
        static LabelNode p_ExtractLabelNode(FormatElement const& ElementToConvert);;
        static std::vector<LabelNode> p_GetDocumentLabelNodes(std::vector<FormatElementComponent>  const& SourceToConvert);
        static FlatLabelNode p_UpdateFlatNodeList(std::vector<FlatLabelNode>& OutNodes,LabelNode const& NodeToFlatten);
        static std::vector<FlatLabelNode> p_GetFlatLabelNodes(std::vector<LabelNode> NodeToConvert);

        bool p_ContainsLabel(LabelNodeIndex NodeIndex,std::vector<std::string> const& Labels,int LabelIndex) const;
    public:      
        static MBParsing::JSONObject ToJSON(ReferenceTargetsResolver const& ObjectToConvert);
        static void FromJSON(ReferenceTargetsResolver& Result,MBParsing::JSONObject const& ObjectToParse);
        bool ContainsLabels(std::vector<std::string> const& Labels) const;
        ReferenceTargetsResolver();
        ReferenceTargetsResolver(std::vector<FormatElementComponent> const& FormatsToInspect);
    };
    
    class DocumentSource
    {
    public:
        DocumentSource(DocumentSource const&) = delete;
        DocumentSource(DocumentSource&&) noexcept = default;
        DocumentSource& operator=(DocumentSource&& OtherSource) = default;
        DocumentSource& operator=(DocumentSource const& OtherSource) = delete;
        DocumentSource() {};


        //"meta data", stored on disk when doing incremental builds
        std::filesystem::path Path;
        std::string Name;
        uint64_t Timestamp = 0;
        ReferenceTargetsResolver ReferenceTargets;
        //doesnt include refernces internal to the file, such as @[here](#foo). 
        //used for implementing 
        std::vector<DocumentPath> ExternalReferences;
        std::string Title;
        
        //actual content, may not be filled when doing incremental builds
        std::vector<FormatElementComponent> Contents;
        friend MBParsing::JSONObject ToJSON(DocumentSource const& Source);
        friend void FromJSON(DocumentSource& Source, MBParsing::JSONObject const& ObjectToParse);

        friend bool operator<(DocumentSource const& Lhs,std::string const& Rhs)
        {
            return Lhs.Name < Rhs;
        }
    };
    class Document
    {
        std::variant<DocumentSource,ResourceReference> m_Data;
    public:

        Document(){};
        template<typename T>
        Document(T&& Data)
        {
            m_Data = std::move(Data);   
        }

        template<typename T>
        bool IsType() const
        {
            return std::holds_alternative<T>(m_Data);
        }
        template<typename T>
        T& GetType()
        {
            return std::get<T>(m_Data);   
        }
        template<typename T>
        T const& GetType() const
        {
            return std::get<T>(m_Data);   
        }
    };

    class ReferenceResolver
    {
    public: 
        virtual void ResolveReference(TextElement& ReferenceToResolve) {};

        virtual ~ReferenceResolver(){};
    };


    class DocumentVisitor : public BlockVisitor, public TextVisitor, public ReferenceVisitor,public DirectiveVisitor,public ReferenceResolver
    {
    public:
        using BlockVisitor::Visit;
        using TextVisitor::Visit;
        using ReferenceVisitor::Visit;
        using DirectiveVisitor::Visit;
        virtual void EnterFormat(FormatElement const& FormatToEnter) {};
        virtual void LeaveFormat(FormatElement const& FormatToLeave) {};

        virtual void EnterBlock(BlockElement const& BlockToEnter) {};
        virtual void LeaveBlock(BlockElement const& BlockToLeave) {};

        virtual void EnterText(TextElement_Base const& TextToEnter) {};
        virtual void LeaveText(TextElement_Base const& TextToLeave) {};

        virtual ~DocumentVisitor(){};
    };
   
    //kinda bloaty, maybe should use tempalte operator() all the way... 
    template<typename T>
    class LambdaVisitor : public DocumentVisitor
    {
    private:
        T& m_Ref;

        template<typename V>
        void p_Visit(V const& ThingToVisit)
        {
            if constexpr(std::is_invocable<T,V const&>::value)
            {
                m_Ref(ThingToVisit);  
            }       
        }
        template<typename V>
        void p_Visit(V& ThingToVisit)
        {
            if constexpr(std::is_invocable<T,V&>::value)
            {
                m_Ref(ThingToVisit);  
            }       
        }
    public:

        LambdaVisitor(T* Ref)
            : m_Ref(*Ref)
        {
        }



        
        void Visit(Directive const& DirectiveToVisit)override
        {
            p_Visit(DirectiveToVisit);
        }
        void Visit(Paragraph const& VisitedParagraph) override
        {
            p_Visit(VisitedParagraph);
        }
        void Visit(MediaInclude const& VisitedMedia) override
        {
            p_Visit(VisitedMedia);
        }
        void Visit(CodeBlock const& Block) override
        {
            p_Visit(Block);
        }
        void Visit(RegularText const& VisitedText)override
        {
            p_Visit(VisitedText);
        }
        void Visit(DocReference const& VisitedText) override
        {
            p_Visit(VisitedText);
        }
        void Visit(FileReference const& FileRef) override
        {
            p_Visit(FileRef);
        }
        void Visit(ResourceReference const& FileRef) override
        {
            p_Visit(FileRef);
        }
        void Visit(URLReference const& URLRef) override
        {
            p_Visit(URLRef);
        }
        void Visit(UnresolvedReference const& UnresolvedRef) override
        {
            p_Visit(UnresolvedRef);
        }
        void Visit(Directive& DirectiveToVisit)override
        {
            p_Visit(DirectiveToVisit);
        }

        void Visit(List& VisitedList) override
        {
            p_Visit(VisitedList);
        }
        void Visit(List const& VisitedList) override
        {
            p_Visit(VisitedList);
        }
        void Visit(Table& VisitedTable) override
        {
            p_Visit(VisitedTable);
        }
        void Visit(Table const& VisitedTable) override
        {
            p_Visit(VisitedTable);
        }
        void Visit(Paragraph& VisitedParagraph) override
        {
            p_Visit(VisitedParagraph);
        }
        void Visit(MediaInclude& VisitedMedia) override
        {
            p_Visit(VisitedMedia);
        }
        void Visit(CodeBlock& Block) override
        {
            p_Visit(Block);
        }
        void Visit(RegularText& VisitedText)override
        {
            p_Visit(VisitedText);
        }
        void Visit(DocReference& VisitedText) override
        {
            p_Visit(VisitedText);
        }
        void Visit(FileReference& FileRef) override
        {
            p_Visit(FileRef);
        }
        void Visit(ResourceReference& FileRef) override
        {
            p_Visit(FileRef);
        }
        void Visit(URLReference & URLRef) override
        {
            p_Visit(URLRef);
        }
        void Visit(UnresolvedReference& UnresolvedRef) override
        {
            p_Visit(UnresolvedRef);
        }
    };

    class DocumentTraverser : DocumentVisitor, FormatVisitor
    {
    private:

        void Visit(BlockElement const& BlockToVisit) override;
        void Visit(BlockElement& BlockToVisit) override;

        void Visit(Directive const& DirectiveToVisit)override;
        void Visit(Directive& DirectiveToVisit)override;

        void Visit(FormatElement const& FormatToVisit)override;
        void Visit(FormatElement& FormatToVisit)override;

        void Visit(Paragraph const& VisitedParagraph) override;
        void Visit(Paragraph& VisitedParagraph) override;

        void Visit(MediaInclude const& VisitedMedia) override;
        void Visit(MediaInclude& VisitedMedia) override;
        void Visit(CodeBlock const& CodeBlock) override;
        void Visit(CodeBlock& CodeBlock) override;
        void Visit(Table const& CodeBlock) override;
        void Visit(Table& CodeBlock) override;
        void Visit(List const& ListToVisit) override;
        void Visit(List& ListToVisit) override;

        void Visit(RegularText& VisitedText)override;
        void Visit(RegularText const& VisitedText)override;
        void Visit(DocReference& VisitedText) override;
        void Visit(DocReference const& VisitedText) override;

        void Visit(FileReference& FileRef) override;
        void Visit(FileReference const& FileRef) override;
        void Visit(URLReference& URLRef) override;
        void Visit(URLReference const& URLRef) override;
        void Visit(ResourceReference& URLRef) override;
        void Visit(ResourceReference const& URLRef) override;
        void Visit(UnresolvedReference& UnresolvedRef) override;
        void Visit(UnresolvedReference const& UnresolvedRef) override;
        
        DocumentVisitor* m_AssociatedVisitor = nullptr;
    public:     
        void Traverse(DocumentSource const& SourceToTraverse,DocumentVisitor& Visitor);
        void Traverse(DocumentSource& SourceToTraverse,DocumentVisitor& Visitor);
    };



















    //typedef MBUtility::LineRetriever LineRetriever;
    
    class LineRetriever
    {
        MBUtility::LineRetriever m_UnderlyingRetriever;

        void p_IteratorNextNonComment();
    public:   
        LineRetriever(MBUtility::MBOctetInputStream* InputStream);
        bool Finished();
        bool GetLine(std::string& OutLine);
        void DiscardLine();
        int GetLineIndex() const;
        std::string& PeekLine();
    };

    class DocumentParsingContext
    {
    private:
        //helpers
        static Paragraph p_ParseParagraph(int InitialLine,int InitialLineCharacterOffset,std::string const& TotalParagraphData);

        
        static TextElement p_ParseReference(int InitialLine,int InitialLineOffset,char const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset);
        static std::vector<TextElement> p_ParseTextElements(int InitialLine,int InitialCharacterOffset,char const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset);

        std::unique_ptr<BlockElement> p_ParseCodeBlock(LineRetriever& Retriever);
        std::unique_ptr<BlockElement> p_ParseMediaInclude(LineRetriever& Retriever);
        std::unique_ptr<BlockElement> p_ParseTable(ArgumentList const& Arguments,LineRetriever& Retriever);
        std::unique_ptr<List> p_ParseList(ArgumentList const& Arguments,LineRetriever& Retriever);
        std::unique_ptr<BlockElement> p_ParseNamedBlockElement(LineRetriever& Retriever);

        std::unique_ptr<BlockElement> p_ParseBlockElement(LineRetriever& Retriever);
        AttributeList p_ParseAttributeList(LineRetriever& Retriever);

        ArgumentList p_ParseArgumentList(char const* Data,size_t DataSize,size_t InOffset);
        Directive p_ParseDirective(LineRetriever& Retriever);
        //Incredibly ugly, but the alternative for the current syntax is to include 2 lines look ahead
        FormatElementComponent p_ParseFormatElement(LineRetriever& Retriever,AttributeList* OutCurrentList);
        std::vector<FormatElementComponent> p_ParseFormatElements(LineRetriever& Retriever);
        
        class ReferenceExtractor : public MBDoc::DocumentVisitor
        {
        public:
            std::unordered_set<std::string> Targets;
            std::unordered_set<std::string> References;
            void EnterFormat(FormatElement const& Format) override;
            void Visit(UnresolvedReference const& Ref) override;
        };
        static void p_UpdateReferences(DocumentSource& SourceToModify);
    public:
        DocumentSource ParseSource(MBUtility::MBOctetInputStream& InputStream,std::string FileName,MBError& OutError);
        DocumentSource ParseSource(std::filesystem::path const& InputFile,MBError& OutError);
        DocumentSource ParseSource(const char* Data,size_t DataSize,std::string FileName,MBError& OutError);
    };

    class DocumentBuild
    {
    private:
        
        static void p_ParseDocumentBuildDirectory(DocumentBuild& OutBuild,MBParsing::JSONObject const& DirectoryObject,std::filesystem::path const& BuildDirectory,MBError& OutError);
        static void p_CreateBuildFromDirectory(DocumentBuild& OutBuild,std::filesystem::path const& DirPath);
    public:
        //std::filesystem::path BuildRootDirectory;
        //std::vector<DocumentPath> BuildFiles = {};
        //The first part is the "MountPoint", the directory prefix. Currently only supports a single directory name

        //used for home references with ~
        std::string Name; 
        std::vector<std::filesystem::path> DirectoryFiles;
        //Guranteeed sorting
        std::vector<std::pair<std::string,DocumentBuild>> SubDirectories;
        
        size_t GetTotalFiles() const;    
        //The relative directory is a part of the identity
        //[[SemanticallyAuthoritative]]
        

        static DocumentBuild ParseDocumentBuild(MBUtility::MBOctetInputStream& InputStream,std::filesystem::path const& BuildDirectory,MBError& OutError);
        static DocumentBuild ParseDocumentBuild(std::filesystem::path const& FilePath,MBError& OutError);
    };
    //typedef std::int_least32_t IndexType;
    typedef int IndexType;
    struct PathSpecifier
    {
        bool AnyRoot = false;
        std::vector<std::string> PathNames;
    };
    struct DocumentReference
    {
        std::vector<std::string> HomeSpecifier;
        std::vector<PathSpecifier> PathSpecifiers;
        std::vector<std::string> PartSpecifier; 
        static std::vector<std::string> ParseHomeSpecifier(std::string_view Data, size_t ParseOffset,size_t DataSize,size_t& OutOffset,MBError& OutError);
        static DocumentReference ParseReference(std::string StringToParse,MBError& OutError);
    };
    //Usage assumptions: never modified, only created and read from. Optimize for access and assume no modification 
    //
    //In particular, makes so that the iterator only goes through a contigous array of elements
    struct DocumentDirectoryInfo
    {
        std::string Name;       
        IndexType ParentDirectoryIndex = -1;
        IndexType FileIndexBegin = -1;
        IndexType FileIndexEnd = -1;
        IndexType DirectoryIndexBegin = -1;
        IndexType DirectoryIndexEnd = -1;
        //fields required for ordering
        IndexType NextDirectory = -1;
        IndexType FirstFileIndex = -1;
        IndexType FirstSubDirIndex = -1;

        friend bool operator<(DocumentDirectoryInfo const& lhs,std::string const& rhs)
        {
            return lhs.Name < rhs;
        }
        friend MBParsing::JSONObject ToJSON(DocumentDirectoryInfo const& Source);
        friend void FromJSON(DocumentDirectoryInfo& Directory, MBParsing::JSONObject const& ObjectToParse);
    };
    struct FilesystemDocumentInfo
    {
        DocumentSource Document;
        IndexType NextFile = -1;

        friend bool operator<(FilesystemDocumentInfo const& lhs,std::string const& rhs)
        {
            return lhs.Document < rhs;   
        }
        friend MBParsing::JSONObject ToJSON(FilesystemDocumentInfo const& DocumentInfo)
        {
            MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
            ReturnValue["Document"] = ToJSON(DocumentInfo.Document);
            ReturnValue["NextFile"] = MBParsing::ToJSON(intmax_t(DocumentInfo.NextFile));
            return ReturnValue;
        }
        friend void FromJSON(FilesystemDocumentInfo& Result,MBParsing::JSONObject const& ObjectToParse)
        {
            FromJSON(Result.Document,ObjectToParse["Document"]);
            FromJSON(Result.NextFile,ObjectToParse["NextFile"]);
        }
    };
    class DocumentFilesystem;
    class DocumentFilesystemIterator
    {
        friend class DocumentFilesystem;
    private:
        DocumentFilesystem const* m_AssociatedFilesystem = nullptr;
        
        IndexType m_DirectoryFilePosition = -1;
        IndexType m_CurrentDirectoryIndex = -1;
        size_t m_CurrentDepth = 0;

        IndexType m_OptionalDirectoryRoot = -1;
        bool m_UseUserOrder = false;
    protected:
        DocumentFilesystemIterator(size_t DirectoryRoot);
        IndexType GetCurrentDirectoryIndex() const;
        IndexType GetCurrentFileIndex() const;
        void CalculateDepth();
        void UseUserOrder();
    public:
        //Accessors
        DocumentPath GetCurrentPath() const;
        bool EntryIsDirectory() const;
        IndexType GetFileIndex() const;
        std::string GetEntryName() const;
        DocumentSource const& GetDocumentInfo() const;
        bool HasEnded() const;
        size_t CurrentDepth() const;
        //Modifiers
        void Increment();
        void NextDirectory();
        void NextFile();
        //void ExitDirectory();
        //void SkipDirectory();
        //void SkipDirectoryFiles();
        DocumentFilesystemIterator& operator++();
        DocumentFilesystemIterator& operator++(int);

    };
    enum class DocumentFSType
    {
        File,
        Directory,
        Null
    };


    class DocumentPathFileIterator
    {
    private:
        std::vector<DocumentPath> const* m_Data = nullptr;
        int m_Depth = 0;
        size_t m_DataOffset = 0;
        size_t m_DataEnd = 0;
        
        size_t m_FileOffset = 0;
        size_t m_DirectoryOffset = -1;
        std::vector<size_t> m_DirectoryBegins;
    public:
        DocumentPathFileIterator(std::vector<DocumentPath> const& Data,int Depth,size_t Offset,size_t End);

        size_t DirectoryCount() const;
        std::string GetDirectoryName() const; 
        DocumentPath const& CurrentFilePath() const;
        size_t GetCurrentOffset() const;//return 
        size_t GetDirectoryEnd() const;
        size_t GetDirectoryEnd(size_t DirectoryIndex) const;
        size_t GetDirectoryBegin(size_t DirectoryIndex) const;
        bool HasEnded() const;
        void NextDirectory(); 
    };
    class LineIndex
    {
        std::vector<int> m_LineToOffset; 
        public:
        LineIndex(std::string const& DocumentData)
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
        int operator[](int Index) const
        {
            if(Index > m_LineToOffset.size())
            {
                throw std::runtime_error("Invalid line index");
            }   
            return(m_LineToOffset[Index]);
        }
    };



    class DocumentFilesystem 
    {
    public:
        //struct BuildDirectory
        //{
        //    std::filesystem::path DirectoryPath;
        //    std::vector<std::string> FileNames;
        //    std::vector<std::pair<std::string, BuildDirectory>> SubDirectories;
        //};
    private:
        friend class DocumentFilesystemIterator;
        class ResourceMap
        {
        private:
            int m_CurrentID = 1;
            std::unordered_map<std::string,int> m_ResourceMap;
        public:
            int GetResourceID(std::string const& CanonicalPath)
            {
                int& ReturnValue = m_ResourceMap[CanonicalPath];   
                if(ReturnValue == 0)
                {
                    ReturnValue = m_CurrentID;
                    m_CurrentID++;   
                }
                return ReturnValue;
            }
            int GetResourceUpdateTime(std::string const& CanonicalPath)
            {
                auto It = m_ResourceMap.find(CanonicalPath);
                if(It != m_ResourceMap.end())
                {
                    return 0;   
                }
                else
                {
                    return 0;   
                }
            }
            static MBParsing::JSONObject ToJSON(ResourceMap const& Source)
            {
                MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
                ReturnValue["ResourceMap"] = MBParsing::ToJSON(Source.m_ResourceMap);
                ReturnValue["CurrentID"] = MBParsing::ToJSON(Source.m_CurrentID);
                return ReturnValue;
            }
            static void FromJSON(ResourceMap& Map, MBParsing::JSONObject const& ObjectToParse)
            {
                MBParsing::FromJSON(Map.m_CurrentID,ObjectToParse["CurrentID"]);
                MBParsing::FromJSON(Map.m_ResourceMap,ObjectToParse["ResourceMap"]);
            }
        };
        class DocumentFilesystemReferenceResolver : public MBDoc::DocumentVisitor
        {
        private:
            DocumentFilesystem* m_AssociatedFilesystem = nullptr;
            DocumentPath m_CurrentPath;
            std::filesystem::path m_DocumentPath;
            TextElement m_Result;
            bool m_ShouldUpdate = false;
            ResourceMap* m_ResourceMapping = nullptr;
        public:
            void Visit(UnresolvedReference& Ref) override;
            void Visit(CodeBlock& CodeBlock) override;
            void Visit(MediaInclude& MediaInclude) override;
            //TODO slightly hacky
            DocumentFilesystemReferenceResolver(DocumentFilesystem* AssociatedFilesystem,DocumentPath CurrentPath,std::filesystem::path DocumentPath,ResourceMap* ResourceMapping);
            void ResolveReference(TextElement& ReferenceToResolve) override;
        };
        //only code of specific semantics are resolved, in order to make it 
        //generic
        enum class CodeReferenceType
        {
            Function,
            Class
        };
        class LSPReferenceResolver
        {
        private:      
            std::string m_CurrentDocument;
            MBLSP::LSP_Client* m_AssociatedLSP;
        public:
            void SetLSP(MBLSP::LSP_Client* AssociatedLSP);
            void SetDocumentURI(std::string const& DocumentURI);
            bool ResolvesReferences();
            TextElement CreateReference(int Line,int Offset,std::string const& VisibleText,CodeReferenceType ReferenceType);
        };
        struct HomeIntervall
        {
            std::string Name;
            std::vector<IndexType> Children;
            IndexType DirIndex = 0;
            IndexType ParentIndex = 0;
            friend MBParsing::JSONObject ToJSON(HomeIntervall const& IntervalToConvert)
            {
                MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
                ReturnValue["Name"] = MBParsing::ToJSON(IntervalToConvert.Name);
                ReturnValue["Children"] = MBParsing::ToJSON(IntervalToConvert.Children);
                ReturnValue["DirIndex"] = MBParsing::ToJSON(IntervalToConvert.DirIndex);
                ReturnValue["ParentIndex"] = MBParsing::ToJSON(IntervalToConvert.ParentIndex);
                return ReturnValue;
            }
            friend void FromJSON(HomeIntervall& OutInterval,MBParsing::JSONObject const& ObjectToParse)
            {
                MBParsing::FromJSON(OutInterval.Name,ObjectToParse["Name"]);
                MBParsing::FromJSON(OutInterval.Children,ObjectToParse["Children"]);
                MBParsing::FromJSON(OutInterval.DirIndex,ObjectToParse["DirIndex"]);
                MBParsing::FromJSON(OutInterval.ParentIndex,ObjectToParse["ParentIndex"]);
            }
        };
        struct FSSearchResult
        {
            DocumentFSType Type = DocumentFSType::Null; 
            IndexType Index = -1;
        };

        ResourceMap m_ResourceMap;
        std::vector<DocumentDirectoryInfo> m_DirectoryInfos;   
        std::vector<FilesystemDocumentInfo> m_TotalSources;
        std::vector<HomeIntervall> m_HomeIntervalls;
        
        IndexType p_DirectorySubdirectoryIndex(IndexType DirectoryIndex,std::string const& SubdirectoryName) const;
        IndexType p_DirectoryFileIndex(IndexType DirectoryIndex,std::string const& FileName) const;
        //Resolving
        FSSearchResult p_ResolveDirectorySpecifier(IndexType RootDirectoryIndex,DocumentReference const& ReferenceToResolve,IndexType SpecifierOffset) const;
        FSSearchResult p_ResolveAbsoluteDirectory(IndexType RootDirectoryIndex,PathSpecifier const& Path) const;
        FSSearchResult p_ResolvePartSpecifier(FSSearchResult CurrentResult,std::vector<std::string> const& PartSpecifier) const;

        IndexType p_GetSatisfyingHomeSubdir(IndexType HomeIndex, std::vector<std::string> const& HomeSpecifier,int Offset) const;
        FSSearchResult p_ResolveHomeSpecifier(IndexType FileIndex,std::vector<std::string> const& HomeSpecifier) const;
        
        DocumentPath p_GetFileIndexPath(IndexType FileIndex) const;
        IndexType p_GetFileDirectoryIndex(DocumentPath const& PathToSearch) const;
        IndexType p_GetFileIndex(DocumentPath const& PathToSearch) const;
         
        DocumentPath p_ResolveReference(DocumentPath const& CurrentPath,DocumentReference const& ReferenceIdentifier,bool* OutResult) const;

        //DocumentDirectoryInfo p_UpdateFilesystemOverFiles(DocumentBuild const& CurrentBuild,size_t FileIndexBegin,size_t DirectoryIndex,BuildDirectory const& DirectoryToCompile);
        //DocumentDirectoryInfo p_UpdateFilesystemOverBuild(DocumentBuild const& BuildToAppend,size_t FileIndexBegin,size_t DirectoryIndex,std::string DirectoryName,MBError& OutError);

        //static BuildDirectory p_ParseBuildDirectory(DocumentBuild const&  BuildToParse);
        DocumentDirectoryInfo p_UpdateOverDirectory(DocumentBuild const& Directory,IndexType FileIndexBegin,IndexType DirectoryIndex,bool ParseSource);
        static void p_CreateDirectoryStructure(DocumentFilesystem& OutFilesystem,DocumentBuild const& BuildToParse,bool ParseSources);
        static void p_PostProcessSources(DocumentFilesystem& OutFilesystem, LSPInfo const& LSPConf, ProcessedColorConfiguration const& ColorConf,std::vector<IndexType> const& SourcesToUpdate = {});
        static void p_UpdateOverDirectory(DocumentFilesystem& NewFilesystem,DocumentFilesystem& OldBuild,IndexType NewDirIndex,IndexType OldDirIndex,std::vector<IndexType>& UpdatedFiles,std::vector<bool>& NewFileUpdated);
        static void p_UpdateDirectoryFiles(DocumentFilesystem& NewFilesystem,DocumentFilesystem& OldBuild,DocumentDirectoryInfo const& NewDir,DocumentDirectoryInfo const& OldDir,std::vector<IndexType>& UpdatedFiles,std::vector<bool>& NewFileUpdated);

        void __PrintDirectoryStructure(DocumentDirectoryInfo const& CurrentDir,int Depth) const;


        void p_ResolveReferences(std::vector<IndexType> const& ModifiedSources);
        

        static std::vector<Coloring> p_GetRegexColorings(std::vector<Coloring> const& PreviousColorings,
                ProcessedRegexColoring const& RegexesToUse,
                std::vector<TextColor> const& ColorMap,
                std::string const& DocumentContent);
        
        static std::vector<Coloring> p_GetLSPColoring(MBLSP::LSP_Client& ClientToUse,std::string const& URI,std::string const& TextContent,ProcessedColorInfo const& ColorInfo,LineIndex const& Index);
        //Kinda ugly, should probably make this a bit more clean but it is what it is
        static ResolvedCodeText p_CombineColorings(std::vector<Coloring> const& RegexColoring,
                std::vector<Coloring> const& LSPColoring,std::string const& 
                OriginalContent,LineIndex const& Index,ProcessedColorInfo const& ColorInfo,LSPReferenceResolver* OptionalResolver);

        void p_ColorizeLSP(ProcessedColorConfiguration const& ColoringConfiguration,LSPInfo const& LSPConfig,std::vector<IndexType> const& ModifiedSources);




        IndexType p_InsertNewDirectories(IndexType ParentDir,int DirOffset,DocumentPath const& Directories); 

        //Incremental builds
        
        //ensures that all indecies are within bounds, that directories content doesn't overlap,
        //and that no cycles exists
        bool p_VerifyParsedFilesystem();
    public: 
        friend MBParsing::JSONObject ToJSON(DocumentFilesystem const& Filesystem);
        friend void FromJSON(DocumentFilesystem& Filesystem, MBParsing::JSONObject const& ObjectToParse);


        std::vector<std::pair<std::string,IndexType>> GetAllDocuments() const;
        DocumentPath GetDocumentPath(IndexType FileID) const;
        DocumentPath ResolveReference(DocumentPath const& DocumentPath,std::string const& PathIdentifier,MBError& OutResult) const;

        //really slow, but needed for LSP...
        IndexType InsertFile(DocumentPath const& Documentpath,DocumentSource const& NewSource);

        DocumentFilesystemIterator begin() const;

        void ResolveReferences(DocumentPath const& DocumentPath,DocumentSource& SourceToUpdate);
        IndexType GetPathFile(DocumentPath const& DocumentPath) const;


        bool DocumentExists(DocumentPath const& DocumentPath) const;
        bool DocumentExists(DocumentPath const& DocumentPath,IndexType& OutIndex) const;

        [[nodiscard]] 
        static MBError CreateDocumentFilesystem(DocumentBuild const& BuildToParse,LSPInfo const& LSPConf,ProcessedColorConfiguration const& ColorConf,DocumentFilesystem& OutBuild,bool ParseSource = true);
        //previous build is modified, transfering it's metadata to the new build 
        static MBError CreateDocumentFilesystem(DocumentBuild const& BuildToParse,LSPInfo const& LSPConf,ProcessedColorConfiguration const& ColorConf,DocumentFilesystem& OutBuild,DocumentFilesystem& PreviousBuild,std::vector<IndexType>& OutdatedFiles);
        static DocumentFilesystem CreateDefaultFilesystem();
        void __PrintDirectoryStructure() const;
    };

    struct CommonCompilationOptions
    {
        std::string OutputDirectory;   
        MBCLI::ArgumentListCLIInput CompilerSpecificOptions;
    };

    class DocumentCompiler
    {
    public:
        virtual void AddOptions(CommonCompilationOptions const& CurrentOptions) = 0;
        virtual void PeekDocumentFilesystem(DocumentFilesystem const& FilesystemToCompile) = 0;
        virtual void CompileDocument(DocumentPath const& DocumentPath,DocumentSource const& DocumentToCompile) = 0;

        virtual ~DocumentCompiler() {};
        //virtual void Compile(DocumentFilesystem const& FilesystemToCompile,CommonCompilationOptions const& Options) = 0;
    };
}
