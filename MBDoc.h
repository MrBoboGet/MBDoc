#pragma once
#include "MBDoc.h"
#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <MBUtility/MBInterfaces.h>
#include <MBUtility/MBErrorHandling.h>
#include <unordered_set>
#include <variant>

#include "MBCLI/MBCLI.h"
namespace MBDoc
{

    // Syntax completely line based
    
    
    //@Identifier <- Implicit visible text ,@[VisibleText](LinkIdentifier)

    //Format elements can contain other format elements, and contain block elements, which can contatin text elements, but no intermixin
    
    
        
    //Implicit whitespace normalization, extra spaces and newlines aren't preserved, which means that a sequence of text elements are not ambigous   

    
    enum class ReferenceType
    {
        Null,
        URL,
        MBDocIdentifier   
    };
    struct MBDocIdentifier
    {
        std::string LinkText;//Temp     
    };
    
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
    struct TextColor
    {
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
    struct TextElement
    {
        TextElementType Type;
        TextModifier Modifiers;
        TextColor Color;
    };


    struct DocReference : TextElement
    {
        DocReference()
        {
            Type = TextElementType::Reference;
        }
        std::string VisibleText;
        std::string Identifier;
    };
    struct RegularText : TextElement
    {
        RegularText()
        {
            Type = TextElementType::Regular;
        }
        std::string Text;   
    };
    struct StringLiteral : TextElement
    {
        std::string Text;   
    };

    struct CustomTextElement : TextElement
    {
        std::string ElementName;
        std::vector<std::pair<std::string,std::string>> Attributes; 
        std::unique_ptr<TextElement> Text;
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
        Table,
        CodeBlock,
    };
    struct BlockElement
    {
        BlockElementType Type = BlockElementType::Null;
        AttributeList Attributes;
    };

    struct Paragraph : BlockElement
    {
        Paragraph() 
        {
            Type = BlockElementType::Paragraph; 
        };
        std::vector<std::unique_ptr<TextElement>> TextElements;//Can be sentences, text, inline references etc
    };
    struct CodeBlock : BlockElement 
    {
        CodeBlock() 
        {
            Type = BlockElementType::CodeBlock;
        }
        std::string CodeType;
        std::string RawText;
    };
    struct Note : BlockElement
    {
        std::unique_ptr<TextElement> Text; 
    };
    struct Image : BlockElement
    {
        std::string ImageResource;     
    };

    struct CustomBlockElement : BlockElement
    {
        std::string ElementName;
        std::vector<std::pair<std::string,std::string>> Attributes;
        std::vector<std::unique_ptr<TextElement>> Content; 
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
    enum class FormatElementType
    {
        Null,
        Section,
        Default,
        Anchor
    };

    enum class FormatComponentType
    {
        Null,
        Format,
        Directive,
        Block,
    };
    
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
        FormatElementComponent& operator=(FormatElementComponent const&) = delete;
        
        FormatElementComponent(std::unique_ptr<BlockElement> BlockData);
        FormatElementComponent(Directive DirectiveData);
        FormatElementComponent(FormatElement DirectieData);
        
        FormatComponentType GetType() const;
        BlockElement& GetBlockData();
        BlockElement const& GetBlockData() const;
        Directive& GetDirectiveData();
        Directive const& GetDirectiveData() const;
        FormatElement& GetFormatData();
        FormatElement const& GetFormatData() const;
        ~FormatElementComponent();
    };
    struct FormatElement
    {
        FormatElement(FormatElement &&) = default;
        FormatElement(FormatElement const&) = delete;
        FormatElement() {};
        FormatElementType Type;
        std::string Name;
        AttributeList  Attributes;
        std::vector<FormatElementComponent> Contents;    
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
        DocumentSource(DocumentSource&&) = default;
        DocumentSource& operator=(DocumentSource OtherSource)
        {
            std::swap(Name, OtherSource.Name);
            std::swap(References, OtherSource.References);
            std::swap(ReferenceTargets, OtherSource.ReferenceTargets);
            std::swap(Contents, OtherSource.Contents);
            return(*this);
        }
        DocumentSource() {};

        std::string Name;
        std::unordered_set<std::string> ReferenceTargets;
        std::unordered_set<std::string> References;
        std::vector<FormatElement> Contents;
    };

    class LineRetriever
    {
    private:
        MBUtility::MBOctetInputStream* m_InputStream = nullptr;
        std::string m_CurrentLine;
        std::string m_Buffer;
        size_t m_BufferOffset = 0;
        bool m_Finished = false;
        bool m_StreamFinished = false;
        bool m_LineBuffered = false;
    public:
        LineRetriever(MBUtility::MBOctetInputStream* InputStream);
        bool Finished();
        bool GetLine(std::string& OutLine);
        void DiscardLine();
        std::string& PeekLine();

    };

    class DocumentParsingContext
    {
    private:
        DocReference p_ParseReference(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset);
        std::vector<std::unique_ptr<TextElement>> p_ParseTextElements(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset);
        std::unique_ptr<BlockElement> p_ParseCodeBlock(LineRetriever& Retriever);
        std::unique_ptr<BlockElement> p_ParseBlockElement(LineRetriever& Retriever);
        AttributeList p_ParseAttributeList(LineRetriever& Retriever);

        ArgumentList p_ParseArgumentList(void const* Data,size_t DataSize,size_t InOffset);
        Directive p_ParseDirective(LineRetriever& Retriever);
        //Incredibly ugly, but the alternative for the current syntax is to include 2 lines look ahead
        FormatElement p_ParseFormatElement(LineRetriever& Retriever,AttributeList* OutCurrentList);
        std::vector<FormatElement> p_ParseFormatElements(LineRetriever& Retriever);
        
        static void p_UpdateReferences(DocumentSource& SourceToModify);
    public:
        DocumentSource ParseSource(MBUtility::MBOctetInputStream& InputStream,std::string FileName,MBError& OutError);
        DocumentSource ParseSource(std::filesystem::path const& InputFile,MBError& OutError);
        DocumentSource ParseSource(const void* Data,size_t DataSize,std::string FileName,MBError& OutError);
    };
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

        static DocumentPath ParsePath(std::string const& PathToParse, MBError& OutError);

        void AddDirectory(std::string StringToAdd);
        void PopDirectory();
        void SetPartIdentifier(std::string PartSpecifier);
        std::string const& GetPartIdentifier() const;
    };

    class DocumentBuild
    {
    public:
        std::filesystem::path BuildRootDirectory;
        std::vector<DocumentPath> BuildFiles = {};
        //The first part is the "MountPoint", the directory prefix. Currently only supports a single directory name

        //Guranteeed sorting
        std::vector<std::pair<std::string,DocumentBuild>> SubBuilds;
        
        //The relative directory is a part of the identity
        //[[SemanticallyAuthoritative]]
        

        static DocumentBuild ParseDocumentBuild(MBUtility::MBOctetInputStream& InputStream,std::filesystem::path const& BuildDirectory,MBError& OutError);
        static DocumentBuild ParseDocumentBuild(std::filesystem::path const& FilePath,MBError& OutError);
    };
    
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
        size_t ParentDirectoryIndex = -1;
        size_t FileIndexBegin = -1;
        size_t FileIndexEnd = -1;
        size_t DirectoryIndexBegin = -1;
        size_t DirectoryIndexEnd = -1;
    };
    class DocumentFilesystem;
    class DocumentFilesystemIterator
    {
        friend class DocumentFilesystem;
    private:
        size_t m_OptionalDirectoryRoot = -1;
        size_t m_DirectoryFilePosition = -1;
        size_t m_CurrentDirectoryIndex = -1;
        DocumentFilesystem const* m_AssociatedFilesystem = nullptr;
    protected:
        DocumentFilesystemIterator(size_t DirectoryRoot);
        size_t GetCurrentDirectoryIndex() const;
        size_t GetCurrentFileIndex() const;
    public:
        //Accessors
        DocumentPath GetCurrentPath() const;
        bool EntryIsDirectory() const;
        std::string GetEntryName() const;
        DocumentSource const& GetDocumentInfo() const;
        bool HasEnded() const;
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
    private:
        friend class DocumentFilesystemIterator;
        std::unordered_map<std::string,DocumentPath> m_CachedPathsConversions;
        std::vector<DocumentDirectoryInfo> m_DirectoryInfos;   
        std::vector<DocumentSource> m_TotalSources;
        struct FSSearchResult
        {
            DocumentFSType Type = DocumentFSType::Null; 
            size_t Index = -1;
        };
        
        
        
        size_t p_DirectorySubdirectoryIndex(size_t DirectoryIndex,std::string const& SubdirectoryName) const;
        size_t p_DirectoryFileIndex(size_t DirectoryIndex,std::string const& FileName) const;
        //Resolving
        FSSearchResult p_ResolveDirectorySpecifier(size_t RootDirectoryIndex,DocumentReference const& ReferenceToResolve,size_t SpecifierOffset) const;
        FSSearchResult p_ResolveAbsoluteDirectory(size_t RootDirectoryIndex,PathSpecifier const& Path) const;
        FSSearchResult p_ResolvePartSpecifier(FSSearchResult CurrentResult,std::string const& PartSpecifier) const;
        
        DocumentPath p_GetFileIndexPath(size_t FileIndex) const;
        size_t p_GetFileDirectoryIndex(DocumentPath const& PathToSearch) const;
        size_t p_GetFileIndex(DocumentPath const& PathToSearch) const;
         
        DocumentPath p_ResolveReference(DocumentPath const& CurrentPath,DocumentReference const& ReferenceIdentifier,bool* OutResult) const;

        DocumentDirectoryInfo p_UpdateFilesystemOverFiles(DocumentBuild const& CurrentBuild,size_t FileIndexBegin,std::vector<DocumentPath> const& Files,size_t DirectoryIndex,int Depth,size_t DirectoryBegin,size_t DirectoryEnd);
        DocumentDirectoryInfo p_UpdateFilesystemOverBuild(DocumentBuild const& BuildToAppend,size_t FileIndexBegin,size_t DirectoryIndex,std::string DirectoryName,MBError& OutError);
    public: 
        DocumentFilesystemIterator begin() const;
        DocumentPath ResolveReference(DocumentPath const& DocumentPath,std::string const& PathIdentifier,MBError& OutResult) const;
        static MBError CreateDocumentFilesystem(DocumentBuild const& BuildToParse,DocumentFilesystem* OutBuild);
    };

    struct CommonCompilationOptions
    {
        std::string OutputDirectory;   
        MBCLI::ArgumentListCLIInput CompilerSpecificOptions;
    };

    class DocumentCompiler
    {
    
    public:
        virtual void Compile(DocumentFilesystem const& FilesystemToCompile,CommonCompilationOptions const& Options) = 0;
    };

    

    class MarkdownReferenceSolver 
    {
    private:
        struct Heading 
        {
            std::string Name;
            std::vector<Heading> SubHeaders;
        };
        std::vector<Heading> m_Headings;
        std::unordered_set<std::string> m_HeadingNames;

        void p_WriteToc(Heading const& CurrentHeading,MBUtility::MBOctetOutputStream& OutStream, size_t Depth) const;
        Heading p_CreateHeading(FormatElement const& Format);
    public:
        std::string GetReferenceString(DocReference const& ReferenceIdentifier) const;
        void CreateTOC(MBUtility::MBOctetOutputStream& OutStream) const;
        
        void Initialize(DocumentSource const& Documents);
    };

    class MarkdownCompiler : public DocumentCompiler
    {
    private:
        

        void p_CompileText(std::vector<std::unique_ptr<TextElement>> const& ElementsToCompile,MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileBlock(BlockElement const* BlockToCompile, MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileDirective(Directive const& DirectiveToCompile, MarkdownReferenceSolver const& ReferenceSolver, MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileFormat(FormatElement const& SourceToCompile, MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream,int Depth);
        void p_CompileSource(DocumentSource const& SourceToCompile, MarkdownReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream);

        MarkdownReferenceSolver p_CreateReferenceSolver(DocumentSource const& Source);
    public:
        virtual void Compile(std::vector<DocumentSource> const& Sources);
        void Compile(DocumentFilesystem const& BuildToCompile,CommonCompilationOptions const& Options) override; 
    };
    class HTTPReferenceSolver
    {
    private:
        DocumentFilesystem const* m_AssociatedBuild = nullptr;
        DocumentPath m_CurrentPath;
    public:
        std::string GetReferenceString(DocReference const& ReferenceIdentifier) const;      
        //Holds a reference to the build for the duration
        void SetCurrentPath(DocumentPath CurrentPath);
        void Initialize(DocumentFilesystem const* Build);

    };

    class HTTPTocCreator
    {
    private:

    public:
        void Initialize(DocReference const& Document);   
        void WriteTOC(MBUtility::MBOctetOutputStream& OutStream);
    };
    class HTTPCompiler : public DocumentCompiler
    {
    private:
        void p_CompileText(std::vector<std::unique_ptr<TextElement>> const& ElementsToCompile,HTTPReferenceSolver const& HTTPReferenceSolverReferenceSolver,MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileBlock(BlockElement const* BlockToCompile, HTTPReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileDirective(Directive const& DirectiveToCompile, HTTPReferenceSolver const& ReferenceSolver, MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileFormat(FormatElement const& SourceToCompile, HTTPReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream,int Depth
            ,std::string const& NamePrefix);
        void p_CompileSource(std::string const& OutPath,DocumentSource const& SourceToCompile, HTTPReferenceSolver const&  ReferenceSolver);
    public:
        void Compile(DocumentFilesystem const& BuildToCompile,CommonCompilationOptions const& Options) override; 
    };
}
