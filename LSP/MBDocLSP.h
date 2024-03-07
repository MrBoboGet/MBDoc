#include "../MBDoc.h"
#include <MBLSP/MBLSP.h>
#include <MBLSP/SemanticTokens.h>
#include "../LSPUtils.h"
namespace MBDoc
{
    class MBDocLSP : public MBLSP::LSP_Server
    {
        struct LoadedBuild
        {
            bool ParseError = false;
            //std::unordered_set<std::string> AssociatedFiles;
            DocumentFilesystem Filesystem;
            std::vector<std::pair<std::string,IndexType>> Files;

            void UpdateFileMetadata(std::string const& CanonicalPath,DocumentSource const& AssociatedSource)
            {
                   
            }
            bool FileExists(std::string const& CanonicalPath) const
            {
                bool ReturnValue = false;
                auto It = std::lower_bound(Files.begin(),Files.end(),CanonicalPath,[](std::pair<std::string,IndexType> const& lhs,std::string const& rhs)
                        {
                            return lhs.first < rhs;
                        });
                if(It != Files.end() && It->first == CanonicalPath)
                {
                    ReturnValue = true;
                }
                return ReturnValue;
            }
        };
        struct LoadedFile
        {
            std::string ContainingBuild;
            DocumentSource Source;
            bool ParseError = false;
            std::vector<int> Tokens;
            std::unordered_map<std::string,std::vector<CodeBlock>> LangaugeCodeblocks;
        };
        struct LoadedServer
        {
        private:
            std::unique_ptr<MBLSP::LSP_Client> m_AssociatedServer;
            std::unordered_set<std::string> m_OpenedFiles;

            bool p_LineInBlocks(int LineIndex,std::vector<CodeBlock> const& Blocks);
        public:
            LoadedServer() = default;
            LoadedServer(LoadedServer&&) noexcept = default;
            LoadedServer& operator=(LoadedServer&&) noexcept = default;
            LoadedServer(std::unique_ptr<MBLSP::LSP_Client> Client) : m_AssociatedServer(std::move(Client)){};

            bool FileLoaded(std::string const& FilePath);
            void LoadFile(std::string const& FilePath,int TotalLines,std::vector<CodeBlock> const& CodeBlocks);
            void UpdateContent(std::string const& FilePath,int TotalLines,std::vector<CodeBlock> const& OldBlock,std::vector<MBLSP::TextChange> const& Changes);
            void UpdateNewBlocks(std::string const& FilePath,int TotalLines,std::vector<CodeBlock> const& NewBlocks,std::vector<MBLSP::TextChange> const& Changes);

            MBLSP::LSP_Client& GetClient() { return *m_AssociatedServer;};
        };

        MBLSP::LSP_ServerHandler* m_Handler = nullptr;

        LSPInfo m_UserLSPInfo;
        std::unordered_map<std::string,int> m_TokenToIndex;
        
        std::unordered_map<std::string,LoadedBuild> m_LoadedBuilds;
        std::unordered_map<std::string,LoadedFile> m_LoadedFiles;
        std::unordered_map<std::string,LoadedServer> m_LoadedServers;



        std::string p_FindFirstContainingBuild(std::filesystem::path const& Filepath);

        LoadedBuild const& p_GetBuild(std::string const& CanonicalPath);
        void p_InitializeLSP(std::string const& LanguageName);
        void p_InitializeFile(std::string const& FilePath,std::string const& Content);
        void p_UpdateFile(std::string const& FilePath,std::string const& Content,std::vector<MBLSP::TextChange> const& Changes);

        std::unordered_map<std::string,std::vector<CodeBlock>> p_ExtractCodeblocks(DocumentSource const& SourceToInspect);

        std::vector<int> p_CalculateDocumentTokens(std::string const& DocumentPath,LoadedFile& File);
        std::vector<int> p_MergeTokens(std::vector<std::pair<std::string,std::vector<int>>> const& TotalTokens);
        std::vector<std::string> p_PossibleResponses(DocumentFilesystem const& AssociatedFilesystem,std::string const& ReferenceString,
                DocumentPath const& AssociatedFile);
    public:
        MBDocLSP()
        {
            m_UserLSPInfo = LoadLSPConfig();   
        }
        
        void SetHandler(MBLSP::LSP_ServerHandler* NewHandler){m_Handler = NewHandler;};
        virtual void ClosedDocument(std::string const& URI) {}
        virtual void DocumentChanged(std::string const& URI,std::string const& NewContent){ throw MBLSP::LSP_Not_Implemented();};//should not happen
                                                                                                                                 //
                                                                                                                                 //
        virtual void OpenedDocument(std::string const& URI,std::string const& Content);
        virtual void DocumentChanged(std::string const& URI,std::string const& NewContent, std::vector<MBLSP::TextChange> const& Changes);

        virtual MBLSP::Initialize_Response HandleRequest(MBLSP::InitializeRequest const& Request); 
        virtual MBLSP::GotoDefinition_Response HandleRequest(MBLSP::GotoDefinition_Request const& Request);
        virtual MBLSP::SemanticToken_Response HandleRequest(MBLSP::SemanticToken_Request const& Request);
        virtual MBLSP::SemanticTokensRange_Response HandleRequest(MBLSP::SemanticTokensRange_Request const& Request);
        virtual MBLSP::Completion_Response HandleRequest(MBLSP::Completion_Request const& Request);
    };
}
