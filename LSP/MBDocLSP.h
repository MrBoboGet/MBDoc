#include "../MBDoc.h"
#include <MBLSP/MBLSP.h>
#include <MBLSP/SemanticTokens.h>
#include "../LSPUtils.h"
namespace MBDoc
{
    class MBDocLSP : public MBLSP::LSP_Server
    {
        class LoadedBuild
        {
            bool m_ParseError = false;
            //std::unordered_set<std::string> AssociatedFiles;
            DocumentFilesystem m_Filesystem = DocumentFilesystem::CreateDefaultFilesystem();
            std::vector<std::pair<std::string,IndexType>> m_Files;
            std::filesystem::path m_BuildDirectory;

            bool p_PathIsContained(std::filesystem::path const& PathToCompare) const
            {
                bool ReturnValue = true;
                //default build contains all other files
                if(m_BuildDirectory.empty())
                {
                    return true;
                }
                auto FirstMismatch = std::mismatch(m_BuildDirectory.begin(),m_BuildDirectory.end(),PathToCompare.begin(),PathToCompare.end());
                return FirstMismatch.first == m_BuildDirectory.end();
            }
        public:
            LoadedBuild()
            {
                   
            }
            LoadedBuild(std::filesystem::path const& BuildToreadPath)
            {
                MBError Result(true);
                m_BuildDirectory = BuildToreadPath.parent_path();
                MBParsing::JSONObject BuildObject = MBParsing::ParseJSONObject(BuildToreadPath,&Result);
                if(Result)
                {
                    DocumentBuild AssociatedBuild = DocumentBuild::ParseDocumentBuild(BuildToreadPath,Result);
                    if(Result)
                    {
                        Result = DocumentFilesystem::CreateDocumentFilesystem(AssociatedBuild,
                                LSPInfo(),
                                ProcessedColorConfiguration(),
                                m_Filesystem,true,false);
                        m_Files = m_Filesystem.GetAllDocuments();
                    }
                }
                if(!Result)
                {
                    m_ParseError = true;
                }
            }
            //called on create / load, either updates metadata for files, or add's the file to the filesystem
            void UpdateFileMetadata(std::string const& CanonicalPath,DocumentSource const& AssociatedSource)
            {
                auto FileIt = std::lower_bound(m_Files.begin(),m_Files.end(),CanonicalPath,[](std::pair<std::string,IndexType> const& lhs,std::string const& rhs)
                        {
                            return lhs.first < rhs;
                        });
                if(FileIt != m_Files.end() && FileIt->first == CanonicalPath)
                {
                    //update metadata
                }
                else
                {
                    //path doesnt exist, insert path
                    std::filesystem::path RelativePath;
                    if(!m_BuildDirectory.empty())
                    {
                        RelativePath = std::filesystem::relative(CanonicalPath,m_BuildDirectory);
                    }
                    else
                    {
                        RelativePath =std::filesystem::path(CanonicalPath).filename();   
                    }
                    DocumentPath NewPath;
                    for(auto const& Part : RelativePath)
                    {
                        NewPath.AddDirectory(MBUnicode::PathToUTF8(Part));
                    }
                    auto NewIndex = m_Filesystem.InsertFile(NewPath,AssociatedSource);
                    for(auto& File : m_Files)
                    {
                        if(File.second >= NewIndex)
                        {
                            File.second += 1;
                        }
                    }
                    auto NewFileIt = std::lower_bound(m_Files.begin(),m_Files.end(),NewIndex,[](std::pair<std::string,IndexType> const& lhs,IndexType rhs)
                        {
                            return lhs.second < rhs;
                        });
                    m_Files.insert(NewFileIt,{CanonicalPath,NewIndex});
                }
            }

            std::filesystem::path GetDocumentPath(DocumentPath const& Path)
            {
                std::filesystem::path ReturnValue;  
                IndexType FileIndex = m_Filesystem.GetPathFile(Path);
                if(FileIndex != -1)
                {
                    ReturnValue = m_Filesystem.GetFile(FileIndex).Path;
                }

                return ReturnValue;
            } 

            bool IsValidBuild() const
            {
                return m_ParseError;
            }
            void ResolveReferences(DocumentSource& SourceToModify)
            {
                std::string CanonicalPath = MBUnicode::PathToUTF8(std::filesystem::canonical(SourceToModify.Path));
                auto IndexIt = std::lower_bound(m_Files.begin(),m_Files.end(),CanonicalPath,
                        [](std::pair<std::string,IndexType> const& Lhs,std::string const& Rhs)
                        {
                            return Lhs.first < Rhs;
                        });
                if(IndexIt != m_Files.end() && IndexIt->first == CanonicalPath)
                {
                    DocumentPath SourceDocPath = m_Filesystem.GetDocumentPath(IndexIt->second);
                    m_Filesystem.ResolveReferences(SourceDocPath,SourceToModify);
                }
            }
            bool FileInBuild(std::string const& CanonicalPath) const
            {
                //only using heuristic for this
                return p_PathIsContained(CanonicalPath);

                //bool ReturnValue = false;
                //auto It = std::lower_bound(m_Files.begin(),m_Files.end(),CanonicalPath,[](std::pair<std::string,IndexType> const& lhs,std::string const& rhs)
                //        {
                //            return lhs.first < rhs;
                //        });
                //if(It != m_Files.end() && It->first == CanonicalPath)
                //{
                //    ReturnValue = true;
                //}
                //return ReturnValue;
            }
        };
        struct LoadedFile
        {
            std::string ContainingBuild;
            DocumentSource Source;
            bool ParseError = false;
            std::vector<int> Tokens;
            std::string Content;
            std::unordered_map<std::string,std::vector<CodeBlock>> LangaugeCodeblocks;
        };
        struct LoadedServer
        {
        private:
            std::unique_ptr<MBLSP::LSP_Client> m_AssociatedServer;
            std::unordered_map<std::string,std::string> m_OpenedFiles;

            bool p_LineInBlocks(MBLSP::TextChange const& Change,std::vector<CodeBlock> const& Blocks);
        public:
            LoadedServer() = default;
            LoadedServer(LoadedServer&&) noexcept = default;
            LoadedServer& operator=(LoadedServer&&) noexcept = default;
            LoadedServer(std::unique_ptr<MBLSP::LSP_Client> Client) : m_AssociatedServer(std::move(Client)){};

            bool FileLoaded(std::string const& URI);
            std::string const& GetFileContent(std::string const& URI);
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
            void UpdateDiagnostics(std::string const& URI,MBLSP::ChangeIndex const& ChangeType,MBLSP::OffsetIndex const& OffsetTypes);
            void StoreDiagnostics(std::string const& URI,std::string const& Language,std::vector<MBLSP::Diagnostic> const& Diagnostics);
            std::vector<MBLSP::Diagnostic> GetDiagnostics(std::string const& URI);
        };

        MBLSP::LSP_ServerHandler* m_Handler = nullptr;

        DiagnosticsStore m_DiagnosticsStore;
        LSPInfo m_UserLSPInfo;
        ProcessedColorConfiguration m_UserColorInfo;
        std::unordered_map<std::string,int> m_TokenToIndex;

        std::string m_LastAnsweredServer = "Default";

        MBLSP::InitializeRequest m_ParentRequest;
       
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
                        m_LastAnsweredServer = ServerIt->first;
                        return true;
                    }
                }
            }
            m_LastAnsweredServer = "Default";
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
            //default for files without an associated build
            m_LoadedBuilds[""] = LoadedBuild();
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
        virtual MBLSP::CompletionItemResolve_Response HandleRequest(MBLSP::CompletionItemResolve_Request const& Request) override;
        //virtual MBParsing::JSONObject HandleGenericRequest(MBParsing::JSONObject const& Request) override;



    };
}
