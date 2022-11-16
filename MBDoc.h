#pragma once
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <MBUtility/MBInterfaces.h>
#include <MBUtility/MBErrorHandling.h>
#include <MBUtility/MBInterfaces.h>
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
    class TextVisitor;
    struct TextElement
    {
        TextElementType Type;
        TextModifier Modifiers;
        TextColor Color;
        void Accept(TextVisitor& Visitor) const;
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

    class TextVisitor
    {
    public:
        virtual void Visit(RegularText const& VisitedText) = 0;
        virtual void Visit(DocReference const& VisitedText) = 0;
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
    };

    struct Paragraph : BlockElement
    {
        Paragraph() 
        {
            Type = BlockElementType::Paragraph; 
        };
        std::vector<std::unique_ptr<TextElement>> TextElements;//Can be sentences, text, inline references etc
    };
    struct MediaInclude : BlockElement
    {
        MediaInclude() 
        {
            Type = BlockElementType::MediaInclude; 
        };
        std::string MediaPath;
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

    class BlockVisitor
    {
    public:
        virtual void Visit(Paragraph const& VisitedParagraph) = 0;
        virtual void Visit(MediaInclude const& VisitedMedia) = 0;
        virtual void Visit(CodeBlock const& CodeBlock) = 0;
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

        void Accept(FormatVisitor& Visitor) const;
    };
    struct FormatElement
    {
        FormatElement(FormatElement &&) = default;
        FormatElement(FormatElement const&) = delete;
        FormatElement() {};
        FormatElementType Type = FormatElementType::Null;
        std::string Name;
        AttributeList  Attributes;
        std::vector<FormatElementComponent> Contents;    
    };


    class FormatVisitor
    {
    public: 
        virtual void Visit(BlockElement const& BlockToVisit) = 0;
        virtual void Visit(Directive const& DirectiveToVisit) = 0;
        virtual void Visit(FormatElement const& FormatToVisit) = 0;
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
        std::vector<FormatElement> Contents;
    };
    typedef MBUtility::LineRetriever LineRetriever;
    //class LineRetriever
    //{
    //private:
    //    MBUtility::MBOctetInputStream* m_InputStream = nullptr;
    //    std::string m_CurrentLine;
    //    std::string m_Buffer;
    //    size_t m_BufferOffset = 0;
    //    bool m_Finished = false;
    //    bool m_StreamFinished = false;
    //    bool m_LineBuffered = false;
    //public:
    //    LineRetriever(MBUtility::MBOctetInputStream* InputStream);
    //    bool Finished();
    //    bool GetLine(std::string& OutLine);
    //    void DiscardLine();
    //    std::string& PeekLine();

    //};

    class DocumentParsingContext
    {
    private:
        DocReference p_ParseReference(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset);
        std::vector<std::unique_ptr<TextElement>> p_ParseTextElements(void const* Data,size_t DataSize,size_t ParseOffset,size_t* OutParseOffset);
        std::unique_ptr<BlockElement> p_ParseCodeBlock(LineRetriever& Retriever);
        std::unique_ptr<BlockElement> p_ParseMediaInclude(LineRetriever& Retriever);
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

        bool operator==(DocumentPath const& PathToCompare) const;
        bool operator!=(DocumentPath const& PathToCompare) const;

        static DocumentPath ParsePath(std::string const& PathToParse, MBError& OutError);

        void AddDirectory(std::string StringToAdd);
        void PopDirectory();
        void SetPartIdentifier(std::string PartSpecifier);
        std::string const& GetPartIdentifier() const;

        bool Empty() const;
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

        std::unordered_map<std::string,DocumentPath> m_CachedPathsConversions;

        std::vector<DocumentDirectoryInfo> m_DirectoryInfos;   
        std::vector<FilesystemDocumentInfo> m_TotalSources;

        struct FSSearchResult
        {
            DocumentFSType Type = DocumentFSType::Null; 
            IndexType Index = -1;
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
    public: 
        DocumentFilesystemIterator begin() const;
        DocumentPath ResolveReference(DocumentPath const& DocumentPath,std::string const& PathIdentifier,MBError& OutResult) const;
        static MBError CreateDocumentFilesystem(DocumentBuild const& BuildToParse,DocumentFilesystem* OutBuild);

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
        //virtual void Compile(DocumentFilesystem const& FilesystemToCompile,CommonCompilationOptions const& Options) = 0;
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

    class MarkdownCompiler
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
        //void Compile(DocumentFilesystem const& BuildToCompile,CommonCompilationOptions const& Options) override; 
    };
    class HTMLReferenceSolver
    {
    private:
        DocumentFilesystem const* m_AssociatedBuild = nullptr;
        DocumentPath m_CurrentPath;
    public:
        std::string GetReferenceString(DocReference const& ReferenceIdentifier) const;
        std::string GetDocumentPathURL(DocumentPath const& PathToConvert) const;
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
    class HTMLNavigationCreator
    {
        struct File
        {
            std::string Name; 
            DocumentPath Path;
        };
        struct Directory
        {
            std::string Name;
            DocumentPath Path;
            std::vector<File> Files;
            std::vector<Directory> SubDirectories;
            bool Open = false;
        };
        DocumentPath m_CurrentPath;
        DocumentPath m_PreviousOpen;
        Directory m_TopDirectory;
        
        static Directory p_CreateDirectory(DocumentFilesystemIterator& FileIterator); 
        void p_WriteDirectory(MBUtility::MBOctetOutputStream& OutStream,HTMLReferenceSolver const& ReferenceSolver,Directory const& DirectoryToWrite) const; 
        void p_WriteFile(MBUtility::MBOctetOutputStream& OutStream, HTMLReferenceSolver const& ReferenceSolver,File const& DirectoryToWrite) const;
        //void p_WriteTopDirectory(
        void p_ToggleOpen(DocumentPath const& PathToToggle);

        void __PrintDirectoryStructure(Directory const& DirectoryToPrint,int Depth) const;
    public:
        HTMLNavigationCreator() {};
        HTMLNavigationCreator(DocumentFilesystem const& FilesystemToInspect);
        void SetOpen(DocumentPath NewPath);
        void SetCurrentPath(DocumentPath CurrentPath);
        void WriteTableDiv(MBUtility::MBOctetOutputStream& OutStream, HTMLReferenceSolver const& ReferenceSolver) const;

        void __PrintDirectoryStructure() const;
    };

    //Share common data, pointless to move every time
    class HTMLCompiler : public DocumentCompiler, public BlockVisitor, public TextVisitor, public FormatVisitor
    {
    private:
        
        int m_ExportedElementsCount = 0;
        DocumentPath p_GetUniquePath(std::string const& Extension);

        std::unordered_map<std::string, DocumentPath> m_MovedResources = {};
        CommonCompilationOptions m_CommonOptions;
        HTMLReferenceSolver m_ReferenceSolver;        
        HTMLNavigationCreator m_NavigationCreator;
        int m_FormatDepth = 0;
        std::vector<int> m_FormatCounts;
        std::filesystem::path m_SourcePath;
        std::string p_GetNumberLabel(int FormatDepth);
        std::unique_ptr<MBUtility::MBOctetOutputStream> m_OutStream;
        
        void p_CompileText(std::vector<std::unique_ptr<TextElement>> const& ElementsToCompile,HTMLReferenceSolver const& HTMLReferenceSolverReferenceSolver,MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileBlock(BlockElement const* BlockToCompile,std::filesystem::path const& SourcePath, HTMLReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileDirective(Directive const& DirectiveToCompile, HTMLReferenceSolver const& ReferenceSolver, MBUtility::MBOctetOutputStream& OutStream);
        void p_CompileFormat(FormatElement const& SourceToCompile,std::filesystem::path const& SourcePath, HTMLReferenceSolver const& ReferenceSolver,MBUtility::MBOctetOutputStream& OutStream,int Depth
            ,std::string const& NamePrefix);
        void p_CompileSource(std::string const& OutPath,DocumentSource const& SourceToCompile, HTMLReferenceSolver const&  ReferenceSolver,HTMLNavigationCreator const& NavigationCreator);

         
        void p_EnterText(TextElement const& ElementToEnter); 
        void p_LeaveText(TextElement const& ElementToEnter); 
    public:
        HTMLCompiler();
        
        void Visit(CodeBlock const& BlockToVisit) override;
        void Visit(MediaInclude const& BlockToVisit) override;
        void Visit(Paragraph const& BlockToVisit) override;

        void Visit(DocReference const& BlockToVisit) override;
        void Visit(RegularText const& BlockToVisit) override;

        void Visit(BlockElement const& BlockToVisit) override;
        void Visit(FormatElement const& BlockToVisit) override;
        void Visit(Directive const& BlockToVisit) override;
        //void Compile(DocumentFilesystem const& BuildToCompile,CommonCompilationOptions const& Options) override; 
        void AddOptions(CommonCompilationOptions const& Options) override;
        void PeekDocumentFilesystem(DocumentFilesystem const& FilesystemToCompile) override;
        void CompileDocument(DocumentPath const& Path,DocumentSource const& Document) override;
    };
}
