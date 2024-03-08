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
            MBLSP::LineIndex Index;
            std::string Content;
            std::unordered_map<std::string,std::vector<CodeBlock>> LangaugeCodeblocks;
        };
        struct LoadedServer
        {
        private:
            std::unique_ptr<MBLSP::LSP_Client> m_AssociatedServer;
            std::unordered_set<std::string> m_OpenedFiles;

            bool p_LineInBlocks(MBLSP::TextChange const& Change,std::vector<CodeBlock> const& Blocks);
        public:
            LoadedServer() = default;
            LoadedServer(LoadedServer&&) noexcept = default;
            LoadedServer& operator=(LoadedServer&&) noexcept = default;
            LoadedServer(std::unique_ptr<MBLSP::LSP_Client> Client) : m_AssociatedServer(std::move(Client)){};

            bool FileLoaded(std::string const& URI);
            void LoadFile(std::string const& URI,int TotalLines,std::vector<CodeBlock> const& CodeBlocks);
            void UpdateContent(std::string const& URI,int TotalLines,std::vector<CodeBlock> const& OldBlock,std::vector<MBLSP::TextChange> const& Changes);
            void UpdateNewBlocks(std::string const& URI,int TotalLines,std::vector<CodeBlock> const& NewBlocks,std::vector<MBLSP::TextChange> const& Changes);

            MBLSP::LSP_Client& GetClient() { return *m_AssociatedServer;};
        };

        class DiagnosticsStore
        {
            std::mutex m_InternalsMutex;
            std::unordered_map<std::string,std::unordered_map<std::string,std::vector<MBLSP::Diagnostic>>> m_StoredDiagnostics;

            static std::string NormalizeURI(std::string const& URI);
        public:
            void StoreDiagnostics(std::string const& URI,std::string const& Language,std::vector<MBLSP::Diagnostic> const& Diagnostics);
            std::vector<MBLSP::Diagnostic> GetDiagnostics(std::string const& URI);
        };

        MBLSP::LSP_ServerHandler* m_Handler = nullptr;

        DiagnosticsStore m_DiagnosticsStore;
        LSPInfo m_UserLSPInfo;
        ProcessedColorConfiguration m_UserColorInfo;
        std::unordered_map<std::string,int> m_TokenToIndex;

        //could exists problems with sending everything indiscriminently...
        MBParsing::JSONObject m_ParentRequest;
        
        std::unordered_map<std::string,LoadedBuild> m_LoadedBuilds;
        std::unordered_map<std::string,LoadedFile> m_LoadedFiles;
        std::unordered_map<std::string,LoadedServer> m_LoadedServers;



        std::string p_FindFirstContainingBuild(std::filesystem::path const& Filepath);

        LoadedBuild const& p_GetBuild(std::string const& CanonicalPath);
        void p_InitializeLSP(std::string const& LanguageName);
        void p_InitializeFile(std::string const& URI,std::string const& Content);
        void p_UpdateFile(std::string const& URI,std::string const& Content,std::vector<MBLSP::TextChange> const& Changes);
        std::vector<MBLSP::Diagnostic> p_GetDiagnostics(DocumentSource const& LoadedSource);
        void p_SendDiagnosticsRequest(std::string const& URI);

        std::unordered_map<std::string,std::vector<CodeBlock>> p_ExtractCodeblocks(DocumentSource const& SourceToInspect);

        std::vector<int> p_CalculateDocumentTokens(std::string const& DocumentPath,LoadedFile const& File);
        void p_NormalizeTypes(std::vector<int>& LSPTokens,std::vector<std::string> const& LSPTypes);
        std::vector<int> p_MergeTokens(std::vector<std::pair<std::string,std::vector<int>>> const& TotalTokens);
        std::vector<std::string> p_PossibleResponses(DocumentFilesystem const& AssociatedFilesystem,std::string const& ReferenceString,
                DocumentPath const& AssociatedFile);
       

        template<typename T>
        bool p_DelegatePositionRequest(MBLSP::Position const& TargetPosition,LoadedFile const& File,T const& Request,decltype(MBLSP::GetRequestResponseType<T>())& Response)
        {
            bool ReturnValue = false;
            for(auto const& Language : File.LangaugeCodeblocks)
            {
                auto ServerIt = m_LoadedServers.find(Language.first);
                if(ServerIt == m_LoadedServers.end())
                {
                    continue;   
                }
                for(auto const& Block : Language.second)
                {
                    if(MBLSP::Contains(MBLSP::Position{Block.LineBegin,0},MBLSP::Position{Block.LineEnd,0},TargetPosition))
                    {
                        Response = ServerIt->second.GetClient().SendRequest(Request);
                        //DEBUG
                        if(Response.error.IsInitalized())
                        {
                            auto const& Error = Response.error.Value();
                            int hej = 2;
                        }
                        return true;
                    }
                }
            }
            return ReturnValue;
        }
        bool p_DelegatePositionRequest(MBLSP::Position const& TargetPosition,LoadedFile const& File,MBParsing::JSONObject const& RawRequest,MBParsing::JSONObject& Response)
        {
            bool ReturnValue = false;
            for(auto const& Language : File.LangaugeCodeblocks)
            {
                auto ServerIt = m_LoadedServers.find(Language.first);
                if(ServerIt == m_LoadedServers.end())
                {
                    continue;   
                }
                for(auto const& Block : Language.second)
                {
                    if(MBLSP::Contains(MBLSP::Position{Block.LineBegin,0},MBLSP::Position{Block.LineEnd,0},TargetPosition))
                    {
                        Response = ServerIt->second.GetClient().SendRawRequest(RawRequest);
                        return true;
                    }
                }
            }
            return ReturnValue;
        }
    public:
        MBDocLSP(MBDocLSP&&) noexcept = delete;
        MBDocLSP()
        {
            m_UserLSPInfo = LoadLSPConfig();   
            m_UserColorInfo = LoadColorConfig();   
        }
        
        void SetHandler(MBLSP::LSP_ServerHandler* NewHandler) override{m_Handler = NewHandler;};
        virtual void ClosedDocument(std::string const& URI)  override{}
        virtual void DocumentChanged(std::string const& URI,std::string const& NewContent) override{ throw MBLSP::LSP_Not_Implemented();};//should not happen
                                                                                                                                 //
                                                                                                                                 //
        virtual void OpenedDocument(std::string const& URI,std::string const& Content) override;
        virtual void DocumentChanged(std::string const& URI,std::string const& NewContent, std::vector<MBLSP::TextChange> const& Changes) override;

        virtual MBLSP::Initialize_Response HandleRequest(MBLSP::InitializeRequest const& Request) override; 
        virtual MBLSP::GotoDefinition_Response HandleRequest(MBLSP::GotoDefinition_Request const& Request) override;
        virtual MBLSP::SemanticToken_Response HandleRequest(MBLSP::SemanticToken_Request const& Request) override;
        virtual MBLSP::SemanticTokensRange_Response HandleRequest(MBLSP::SemanticTokensRange_Request const& Request) override;
        virtual MBLSP::Completion_Response HandleRequest(MBLSP::Completion_Request const& Request) override;
        //virtual MBParsing::JSONObject HandleGenericRequest(MBParsing::JSONObject const& Request) override;



    };
}
