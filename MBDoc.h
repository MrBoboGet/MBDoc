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
#include <variant>
#include <assert.h>

#include "MBCLI/MBCLI.h"
#include "MBDoc.h"

#include <MBUtility/Iterator.h>
namespace MBDoc
{

    class DocumentPath
    {
    private:
        bool m_IsAbsolute = false;
        std::vector<std::string> m_PathComponents;
        std::string m_PartIdentifier;
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
        void SetPartIdentifier(std::string PartSpecifier);
        std::string const& GetPartIdentifier() const;

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
    struct TextColor
    {

    private:
        bool m_Default = true;
    public:
        TextColor(){};
        TextColor(uint8_t NR,uint8_t  NG,uint8_t NB)
        {
            R = NR;   
            G = NG;   
            B = NB;   
            m_Default = false;
        }
        bool IsDefault()
        {
            return(m_Default);   
        }
        uint8_t R = 0;   
        uint8_t G = 0;   
        uint8_t B = 0;   
    };

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
        virtual void Accept(ReferenceVisitor& Visitor) const = 0;
        virtual ~DocReference()
        {
               
        }
    };

    struct FileReference : public DocReference
    {
        DocumentPath Path;
        virtual void Accept(ReferenceVisitor& Visitor) const;
        ~FileReference()
        {
               
        }
    };
    struct URLReference : public DocReference
    {
        std::string URL; 
        virtual void Accept(ReferenceVisitor& Visitor) const;
        ~URLReference()
        {
               
        }
    };
    struct UnresolvedReference : public DocReference
    {
        std::string ReferenceString;         
        virtual void Accept(ReferenceVisitor& Visitor) const;
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

        virtual ~ReferenceVisitor() {};
    };

    struct TextElement
    {

    private:
        std::variant<RegularText,std::unique_ptr<DocReference>> m_Content;
    public:
        template<typename T>
        TextElement(T ValueToStore)
        {
            m_Content = std::move(ValueToStore);
        }
        template<typename T>
        bool IsType() const
        {
            return(std::holds_alternative<T>(m_Content)); 
        }
        template<>
        bool IsType<DocReference>() const
        {
            return(std::holds_alternative<std::unique_ptr<DocReference>>(m_Content)); 
        }

        template<typename T>
        T const& GetType() const
        {
            return(std::get<T>(m_Content));    
        }
        template<>
        DocReference const& GetType() const
        {
            return(*std::get<std::unique_ptr<DocReference>>(m_Content));    
        }

        template<typename T>
        T& GetType() 
        {
            return(std::get<T>(m_Content));    
        }
        template<>
        DocReference& GetType() 
        {
            return(*std::get<std::unique_ptr<DocReference>>(m_Content));    
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
        TextElement() {};
    };
    



    class TextVisitor
    {
    public:
        virtual void Visit(RegularText const& VisitedText){};
        virtual void Visit(DocReference const& VisitedText) {};


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
    };
    class BlockVisitor;
    struct BlockElement
    {
        BlockElementType Type = BlockElementType::Null;
        AttributeList Attributes;
        void Accept(BlockVisitor& Visitor) const;

        virtual ~BlockElement(){};
    };

    struct Paragraph : BlockElement
    {
        Paragraph() 
        {
            Type = BlockElementType::Paragraph; 
        };
        std::vector<TextElement> TextElements;//Can be sentences, text, inline references etc
    };
    struct MediaInclude : BlockElement
    {
        MediaInclude() 
        {
            Type = BlockElementType::MediaInclude; 
        };
        std::string MediaPath;
    };
    class DocumentParsingContext;
    class ResolvedCodeText
    {
    private:
        std::vector<std::vector<TextElement>> m_Rows;
    protected:
        friend DocumentParsingContext;
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
        std::string CodeType;
        std::variant<std::string,ResolvedCodeText> Content;
    };

    class BlockVisitor
    {
    public:
        virtual void Visit(Paragraph const& VisitedParagraph) {};
        virtual void Visit(MediaInclude const& VisitedMedia) {};
        virtual void Visit(CodeBlock const& CodeBlock) {};

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
    };

    class FormatVisitor : public DirectiveVisitor
    {
    public: 
        using DirectiveVisitor::Visit;
        virtual void Visit(BlockElement const& BlockToVisit) {};
        virtual void Visit(FormatElement const& FormatToVisit){};
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
    class DocumentSource
    {
    public:
        DocumentSource(DocumentSource const&) = delete;
        DocumentSource(DocumentSource&&) noexcept = default;
        DocumentSource& operator=(DocumentSource OtherSource)
        {
            std::swap(Name, OtherSource.Name);
            std::swap(References, OtherSource.References);
            std::swap(ReferenceTargets, OtherSource.ReferenceTargets);
            std::swap(Contents, OtherSource.Contents);
            return(*this);
        }
        DocumentSource() {};

        std::filesystem::path Path;
        std::string Name;
        std::unordered_set<std::string> ReferenceTargets;
        std::unordered_set<std::string> References;
        //TODO replace with FormatElementComponent
        std::vector<FormatElementComponent> Contents;
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
   

    class DocumentTraverser : DocumentVisitor, FormatVisitor
    {
    private:

        void Visit(BlockElement const& BlockToVisit) override;
        void Visit(Directive const& DirectiveToVisit)override;
        void Visit(FormatElement const& FormatToVisit)override;

        void Visit(Paragraph const& VisitedParagraph) override;
        void Visit(MediaInclude const& VisitedMedia) override;
        void Visit(CodeBlock const& CodeBlock) override;

        void Visit(RegularText const& VisitedText)override;
        void Visit(DocReference const& VisitedText) override;

        void Visit(FileReference const& FileRef) override;
        void Visit(URLReference const& URLRef) override;
        void Visit(UnresolvedReference const& UnresolvedRef) override;
        
        DocumentVisitor* m_AssociatedVisitor = nullptr;
    public:     
        void Traverse(DocumentSource const& SourceToTraverse,DocumentVisitor& Vistor);
    };



















    typedef MBUtility::LineRetriever LineRetriever;

    class DocumentParsingContext
    {
    private:
        TextElement p_ParseReference(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset);
        std::vector<TextElement> p_ParseTextElements(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset);
        std::unique_ptr<BlockElement> p_ParseCodeBlock(LineRetriever& Retriever);
        std::unique_ptr<BlockElement> p_ParseMediaInclude(LineRetriever& Retriever);
        std::unique_ptr<BlockElement> p_ParseBlockElement(LineRetriever& Retriever);
        AttributeList p_ParseAttributeList(LineRetriever& Retriever);

        ArgumentList p_ParseArgumentList(void const* Data,size_t DataSize,size_t InOffset);
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
        DocumentSource ParseSource(const void* Data,size_t DataSize,std::string FileName,MBError& OutError);
    };

    class DocumentBuild
    {
    private:
        
        static void p_ParseDocumentBuildDirectory(DocumentBuild& OutBuild,MBParsing::JSONObject const& DirectoryObject,std::filesystem::path const& BuildDirectory,MBError& OutError);
    public:
        //std::filesystem::path BuildRootDirectory;
        //std::vector<DocumentPath> BuildFiles = {};
        //The first part is the "MountPoint", the directory prefix. Currently only supports a single directory name
        
        std::vector<std::filesystem::path> DirectoryFiles;
        //Guranteeed sorting
        std::vector<std::pair<std::string,DocumentBuild>> SubDirectories;
        
        size_t GetTotalFiles() const;    
        //The relative directory is a part of the identity
        //[[SemanticallyAuthoritative]]
        

        static DocumentBuild ParseDocumentBuild(MBUtility::MBOctetInputStream& InputStream,std::filesystem::path const& BuildDirectory,MBError& OutError);
        static DocumentBuild ParseDocumentBuild(std::filesystem::path const& FilePath,MBError& OutError);
    };
    typedef std::int_least32_t IndexType;
    struct PathSpecifier
    {
        bool AnyRoot = false;
        std::vector<std::string> PathNames;
    };
    struct DocumentReference
    {
        std::vector<PathSpecifier> PathSpecifiers;
        std::string PartSpecifier; 
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
    };
    struct FilesystemDocumentInfo
    {
        DocumentSource Document;
        IndexType NextFile = -1;
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
        friend class DocumentFilesystemReferenceResolver;

        std::unordered_map<std::string,DocumentPath> m_CachedPathsConversions;

        std::vector<DocumentDirectoryInfo> m_DirectoryInfos;   
        std::vector<FilesystemDocumentInfo> m_TotalSources;

        struct FSSearchResult
        {
            DocumentFSType Type = DocumentFSType::Null; 
            IndexType Index = -1;
        };
        
        class DocumentFilesystemReferenceResolver : public MBDoc::DocumentVisitor
        {
        private:
            DocumentFilesystem* m_AssociatedFilesystem = nullptr;
            DocumentPath m_CurrentPath;
            TextElement m_Result;
            bool m_ShouldUpdate = false;
        public:
            void Visit(UnresolvedReference const& Ref) override;
            DocumentFilesystemReferenceResolver(DocumentFilesystem* AssociatedFilesystem,DocumentPath CurrentPath);
            void ResolveReference(TextElement& ReferenceToResolve) override;
        };
        
        IndexType p_DirectorySubdirectoryIndex(IndexType DirectoryIndex,std::string const& SubdirectoryName) const;
        IndexType p_DirectoryFileIndex(IndexType DirectoryIndex,std::string const& FileName) const;
        //Resolving
        FSSearchResult p_ResolveDirectorySpecifier(IndexType RootDirectoryIndex,DocumentReference const& ReferenceToResolve,IndexType SpecifierOffset) const;
        FSSearchResult p_ResolveAbsoluteDirectory(IndexType RootDirectoryIndex,PathSpecifier const& Path) const;
        FSSearchResult p_ResolvePartSpecifier(FSSearchResult CurrentResult,std::string const& PartSpecifier) const;
        
        DocumentPath p_GetFileIndexPath(IndexType FileIndex) const;
        IndexType p_GetFileDirectoryIndex(DocumentPath const& PathToSearch) const;
        IndexType p_GetFileIndex(DocumentPath const& PathToSearch) const;
         
        DocumentPath p_ResolveReference(DocumentPath const& CurrentPath,DocumentReference const& ReferenceIdentifier,bool* OutResult) const;

        //DocumentDirectoryInfo p_UpdateFilesystemOverFiles(DocumentBuild const& CurrentBuild,size_t FileIndexBegin,size_t DirectoryIndex,BuildDirectory const& DirectoryToCompile);
        //DocumentDirectoryInfo p_UpdateFilesystemOverBuild(DocumentBuild const& BuildToAppend,size_t FileIndexBegin,size_t DirectoryIndex,std::string DirectoryName,MBError& OutError);

        //static BuildDirectory p_ParseBuildDirectory(DocumentBuild const&  BuildToParse);
        DocumentDirectoryInfo p_UpdateOverDirectory(DocumentBuild const& Directory,IndexType FileIndexBegin,IndexType DirectoryIndex);

        void __PrintDirectoryStructure(DocumentDirectoryInfo const& CurrentDir,int Depth) const;


        void p_ResolveReferences();
        void p_ColorizeLSP();
    public: 
        DocumentFilesystemIterator begin() const;
        DocumentPath ResolveReference(DocumentPath const& DocumentPath,std::string const& PathIdentifier,MBError& OutResult) const;
        [[nodiscard]] 
        static MBError CreateDocumentFilesystem(DocumentBuild const& BuildToParse,DocumentFilesystem& OutBuild);

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
