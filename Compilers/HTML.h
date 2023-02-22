#pragma once
#include "../MBDoc.h"


namespace MBDoc
{
    class HTMLReferenceSolver : public ReferenceVisitor
    {
    private:
        std::string m_VisitResultProperties;
        std::string m_VisitResultText;
        DocumentFilesystem const* m_AssociatedBuild = nullptr;
        DocumentPath m_CurrentPath;
        bool m_Colorize = true;
    public:
        void Visit(URLReference const& Ref) override;
        void Visit(FileReference const& Ref) override;
        void Visit(UnresolvedReference const& Ref) override;
        std::string GetReferenceString(DocReference const& ReferenceIdentifier,bool Colorize = true);
        std::string GetDocumentPathURL(DocumentPath const& PathToConvert);
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
        void p_WriteDirectory(MBUtility::MBOctetOutputStream& OutStream,HTMLReferenceSolver& ReferenceSolver,Directory const& DirectoryToWrite) const; 
        void p_WriteFile(MBUtility::MBOctetOutputStream& OutStream, HTMLReferenceSolver& ReferenceSolver,File const& DirectoryToWrite) const;
        //void p_WriteTopDirectory(
        void p_ToggleOpen(DocumentPath const& PathToToggle);

        void __PrintDirectoryStructure(Directory const& DirectoryToPrint,int Depth) const;
    public:
        HTMLNavigationCreator() {};
        HTMLNavigationCreator(DocumentFilesystem const& FilesystemToInspect);
        void SetOpen(DocumentPath NewPath);
        void SetCurrentPath(DocumentPath CurrentPath);
        void WriteTableDiv(MBUtility::MBOctetOutputStream& OutStream, HTMLReferenceSolver& ReferenceSolver) const;

        void __PrintDirectoryStructure() const;
    };

    //Share common data, pointless to move every time
    class HTMLCompiler : public DocumentVisitor, public DocumentCompiler
    {
    private:
        
        int m_ExportedElementsCount = 0;
        DocumentPath p_GetUniquePath(std::string const& Extension);

        std::unordered_map<std::string, DocumentPath> m_MovedResources = {};
        CommonCompilationOptions m_CommonOptions;
        HTMLReferenceSolver m_ReferenceSolver;        
        HTMLNavigationCreator m_NavigationCreator;


        int m_FormatDepth = 0;
        std::vector<std::string> m_CurrentPartIdentifier;


        std::vector<int> m_FormatCounts;
        std::filesystem::path m_SourcePath;
        std::string p_GetNumberLabel(int FormatDepth);
        std::unique_ptr<MBUtility::MBOctetOutputStream> m_OutStream;
        
        //really ghetto, it is what is is
        bool m_InCodeBlock = false;
        bool m_InTable = false;
        int m_TableWidth = 0;
        int m_CurrentColumnCount = 0;
    public:
        HTMLCompiler();
        
        void EnterText(TextElement_Base const& ElementToEnter) override; 
        void LeaveText(TextElement_Base const& ElementToEnter) override; 

        void EnterFormat(FormatElement const& ElementToEnter) override; 
        void LeaveFormat(FormatElement const& ElementToEnter) override; 

        void LeaveBlock(BlockElement const& BlockToLeave) override;
        void EnterBlock(BlockElement const& BlockToEnter) override;

        void Visit(CodeBlock const& BlockToVisit) override;
        void Visit(MediaInclude const& BlockToVisit) override;
        void Visit(Paragraph const& BlockToVisit) override;

        void Visit(URLReference const& BlockToVisit) override;
        void Visit(FileReference const& BlockToVisit) override;
        void Visit(UnresolvedReference const& BlockToVisit) override;
        void Visit(DocReference const& BlockToVisit) override;

        void Visit(RegularText const& BlockToVisit) override;

        void Visit(Directive const& BlockToVisit) override;
        //void Compile(DocumentFilesystem const& BuildToCompile,CommonCompilationOptions const& Options) override; 
        void AddOptions(CommonCompilationOptions const& Options) override;
        void PeekDocumentFilesystem(DocumentFilesystem const& FilesystemToCompile) override;
        void CompileDocument(DocumentPath const& Path,DocumentSource const& Document) override;
    };
}
