#include "MBDocLSP.h"
#include <MBUtility/MBFiles.h>

#include "../LSPUtils.h"
#include <MBLSP/TextChanges.h>

#include <MBLSP/Capabilities.h>
namespace MBDoc
{
    //DiagnosticsStore
    std::string MBDocLSP::DiagnosticsStore::NormalizeURI(std::string const& URI)
    {
       return MBUnicode::PathToUTF8(std::filesystem::canonical(MBLSP::URLDecodePath(URI)));
    }
    void MBDocLSP::DiagnosticsStore::StoreDiagnostics(std::string const& URI,std::string const& Language,std::vector<MBLSP::Diagnostic> const& Diagnostics)
    {
        std::lock_guard Lock(m_InternalsMutex);
        m_StoredDiagnostics[NormalizeURI(URI)][Language] = Diagnostics;
    }
    std::vector<MBLSP::Diagnostic> MBDocLSP::DiagnosticsStore::GetDiagnostics(std::string const& URI)
    {
        std::vector<MBLSP::Diagnostic> ReturnValue;
        for(auto const& Language : m_StoredDiagnostics[NormalizeURI(URI)])
        {
            ReturnValue.insert(ReturnValue.end(),Language.second.begin(),Language.second.end());
        }
        return ReturnValue;
    }
    // 
    bool MBDocLSP::LoadedServer::FileLoaded(std::string const& URI)
    {
        return m_OpenedFiles.find(URI) != m_OpenedFiles.end();
    }
    std::string const& MBDocLSP::LoadedServer::GetFileContent(std::string const& URI)
    {
        auto FileIt = m_OpenedFiles.find(URI);
        if(FileIt == m_OpenedFiles.end())
        {
            throw std::runtime_error(URI+" not opened");
        }
        return FileIt->second;
    }
    void MBDocLSP::LoadedServer::LoadFile(std::string const& URI,int TotalLines,std::vector<CodeBlock> const& CodeBlocks)
    {
        std::string TotalData;
        if(m_OpenedFiles.find(URI) != m_OpenedFiles.end())
        {
            MBLSP::DidCloseTextDocument_Notification CloseNotification;
            CloseNotification.params.textDocument.uri = URI;
            m_AssociatedServer->SendNotification(CloseNotification);
        }
        //else
        //{
        //    m_OpenedFiles.insert(URI);
        //}
        int PreviousLine = 0;
        for(auto const& Block : CodeBlocks)
        {
            TotalData += std::string( (Block.LineBegin - PreviousLine)+1,'\n');
            TotalData += Block.Content;
            TotalData += "\n";
            PreviousLine = Block.LineEnd+1;
            assert(PreviousLine >= 0);
        }
        TotalData += std::string( (TotalLines-PreviousLine)+1,'\n');
        m_OpenedFiles[URI] = TotalData;
        MBLSP::DidOpenTextDocument_Notification OpenNotification;
        OpenNotification.params.textDocument.text = TotalData;
        OpenNotification.params.textDocument.uri = URI;
        m_AssociatedServer->SendNotification(OpenNotification);
    }
    bool MBDocLSP::LoadedServer::p_LineInBlocks(MBLSP::TextChange const& Change,std::vector<CodeBlock> const& Blocks)
    {
        bool ReturnValue = false;
        for(auto const& Block : Blocks)
        {
            if(Block.LineBegin < Change.LinePosition && Change.LinePosition < Block.LineEnd || 
                    (std::holds_alternative<MBLSP::TextChange_AddedLine>(Change.Change) && Change.LinePosition == Block.LineEnd))
            {
                return true;   
            }
        }
        return ReturnValue;
    }
    void MBDocLSP::LoadedServer::UpdateContent(std::string const& URI,int TotalLines,std::vector<CodeBlock> const& OldBlock,std::vector<MBLSP::TextChange> const& Changes)
    {
        if(!FileLoaded(URI))
        {
            LoadFile(URI,TotalLines,OldBlock);
            return;
        }
        MBLSP::ChangeIndex ChangesResult(Changes);

        for(auto const& CodeBlock : OldBlock)
        {
            auto BeginChange = ChangesResult.GetChangeType(CodeBlock.LineBegin);
            auto EndChange = ChangesResult.GetChangeType(CodeBlock.LineEnd);
            if(BeginChange != MBLSP::LineChangeType::Null || EndChange != MBLSP::LineChangeType::Null)
            {
                MBLSP::DidChangeTextDocument_Notification RemoveCodeblockChange;
                RemoveCodeblockChange.params.textDocument.uri = URI;
                MBLSP::TextDocumentContentChangeEvent& Change = RemoveCodeblockChange.params.contentChanges.emplace_back();
                Change.text = std::string( (CodeBlock.LineEnd - CodeBlock.LineBegin)-1,'\n');
                Change.range = MBLSP::Range();
                Change.range->start = MBLSP::Position{CodeBlock.LineBegin+1,0};
                Change.range->end = MBLSP::Position{CodeBlock.LineEnd,0};
                m_OpenedFiles[URI] = MBLSP::ApplyEdits(m_OpenedFiles[URI],RemoveCodeblockChange.params.contentChanges).Text;
                m_AssociatedServer->SendNotification(RemoveCodeblockChange);
            }
        }
        //make changes so that lines outside of boxes are empty, with correct line offsets
        std::vector<MBLSP::TextChange> NewChanges;
        for(auto const& Change : Changes)
        {
            if(p_LineInBlocks(Change,OldBlock))
            {
                NewChanges.push_back(Change);
            }
            else
            {
                if(std::holds_alternative<MBLSP::TextChange_TextEdit>(Change.Change))
                {
                    auto NewChange = Change;
                    std::get<MBLSP::TextChange_TextEdit>(NewChange.Change).LineContent = "";
                    NewChanges.push_back(NewChange);
                }
                else if(std::holds_alternative<MBLSP::TextChange_AddedLine>(Change.Change))
                {
                    auto NewChange = Change;
                    std::get<MBLSP::TextChange_AddedLine>(NewChange.Change).LineContent = "";
                    NewChanges.push_back(NewChange);
                }
                else
                {
                    NewChanges.push_back(Change);
                }
            }
        }
        MBLSP::DidChangeTextDocument_Notification UpdateNotification;
        UpdateNotification.params.textDocument.uri = URI;
        UpdateNotification.params.contentChanges = MBLSP::LineChangesToChangeEvents(NewChanges);
        m_OpenedFiles[URI] = MBLSP::ApplyEdits(m_OpenedFiles[URI],UpdateNotification.params.contentChanges).Text;
        m_AssociatedServer->SendNotification(UpdateNotification);
    }
    void MBDocLSP::LoadedServer::UpdateNewBlocks(std::string const& URI,int TotalLines,std::vector<CodeBlock> const& NewBlocks,std::vector<MBLSP::TextChange> const& Changes)
    {
        if(!FileLoaded(URI))
        {
            LoadFile(URI,TotalLines,NewBlocks);
            return;
        }
        MBLSP::ChangeIndex ChangesResult(Changes);
        for(auto const& CodeBlock : NewBlocks)
        {
            auto BeginChange = ChangesResult.GetChangeType(CodeBlock.LineBegin);
            auto EndChange = ChangesResult.GetChangeType(CodeBlock.LineEnd);
            if(BeginChange != MBLSP::LineChangeType::Null || EndChange != MBLSP::LineChangeType::Null)
            {
                MBLSP::DidChangeTextDocument_Notification RemoveCodeblockChange;
                RemoveCodeblockChange.params.textDocument.uri = URI;
                MBLSP::TextDocumentContentChangeEvent& Change = RemoveCodeblockChange.params.contentChanges.emplace_back();
                Change.text = CodeBlock.Content;
                Change.range = MBLSP::Range();
                Change.range->start = MBLSP::Position{CodeBlock.LineBegin+1,0};
                Change.range->end = MBLSP::Position{CodeBlock.LineEnd,0};

                m_OpenedFiles[URI] = MBLSP::ApplyEdits(m_OpenedFiles[URI],RemoveCodeblockChange.params.contentChanges).Text;
                m_AssociatedServer->SendNotification(RemoveCodeblockChange);
            }
        }
    }
    
       
    MBDocLSP::LoadedBuild const& MBDocLSP::p_GetBuild(std::string const& CanonicalPath)
    {
        LoadedBuild NewBuild;
        m_LoadedBuilds[CanonicalPath] = std::move(NewBuild);
        return m_LoadedBuilds[CanonicalPath];
    }
    std::string MBDocLSP::p_FindFirstContainingBuild(std::filesystem::path const& Filepath)
    {
        std::string ReturnValue;
        std::filesystem::path CurrentPath = Filepath.parent_path();
        while(!CurrentPath.empty())
        {
            try
            {
                if(std::filesystem::exists(CurrentPath/"MBDocBuild.json"))
                {
                    std::string CanonicalBuildPath = MBUnicode::PathToUTF8(std::filesystem::canonical(CurrentPath/"MBDocBuild.json"));
                    auto const& Build = p_GetBuild(CanonicalBuildPath);
                    if(Build.ParseError != true && Build.FileExists(MBUnicode::PathToUTF8(Filepath)))
                    {
                        return CanonicalBuildPath;
                    }
                }
            }
            catch(...)
            {
                   
            }
            auto NewPath = CurrentPath.parent_path();
            if(NewPath == CurrentPath)
            {
                break;   
            }
            CurrentPath = NewPath;
        }
        return ReturnValue;
    }
    void MBDocLSP::p_InitializeLSP(std::string const& LanguageName)
    {
        if(m_LoadedServers.find(LanguageName) != m_LoadedServers.end())
        {
            return;
        }
        auto ConfigIt = m_UserColorInfo.LanguageConfigs.find(LanguageName);
        if(ConfigIt != m_UserColorInfo.LanguageConfigs.end())
        {
            if(ConfigIt->second.LSP != "")
            {
                if(m_UserLSPInfo.Servers.find(ConfigIt->second.LSP) != m_UserLSPInfo.Servers.end())
                {
                    auto const& Server = m_UserLSPInfo.Servers[ConfigIt->second.LSP];
                    auto& NewInfo = m_LoadedServers[LanguageName];
                    MBLSP::InitializeRequest InitReq;
                    InitReq = m_ParentRequest;
                    if(Server.initializationOptions)
                    {
                        InitReq.params.initializationOptions = Server.initializationOptions.Value();   
                    }
                    if(m_ParentRequest.params.capabilities.completion)
                    {
                        InitReq.params.capabilities.completion = m_ParentRequest.params.capabilities.completion;
                    }
                    NewInfo = LoadedServer(StartLSPServer(m_UserLSPInfo.Servers[ConfigIt->second.LSP],InitReq));
                    NewInfo.GetClient().AddRawNotificationHandler( [&,Lang=LanguageName](MBParsing::JSONObject Notification)
                            {
                                if(Notification.HasAttribute("method") && Notification.GetAttribute("method").GetStringData() == "textDocument/publishDiagnostics")
                                {
                                    //Should probably normalise all URI's...
                                    MBLSP::PublishDiagnostics_Notification SentNotification;
                                    SentNotification.FillObject(Notification);
                                    m_DiagnosticsStore.StoreDiagnostics(SentNotification.params.uri,Lang,std::move(SentNotification.params.diagnostics));
                                    p_SendDiagnosticsRequest(SentNotification.params.uri);
                                }
                            });
                }
            }
        }
    }
    std::unordered_map<std::string,std::vector<CodeBlock>> MBDocLSP::p_ExtractCodeblocks(DocumentSource const& SourceToInspect)
    {
        std::unordered_map<std::string,std::vector<CodeBlock>> ReturnValue;
        auto CodeBlockTraverser = [&](CodeBlock const& NewBlock)
        {
            ReturnValue[NewBlock.CodeType].push_back(NewBlock);
        };
        LambdaVisitor Visitor = LambdaVisitor(&CodeBlockTraverser);
        DocumentTraverser Traverser;
        Traverser.Traverse(SourceToInspect,Visitor);
        return ReturnValue;
    }
    void MBDocLSP::p_InitializeFile(std::string const& URI,std::string const& Content)
    {
        auto Path = MBLSP::URLDecodePath(URI);
        //std::string CanonicalString = MBUnicode::PathToUTF8(CanonicalPath);
        LoadedFile& NewFile = m_LoadedFiles.emplace(std::make_pair(URI,LoadedFile()) ).first->second;
        NewFile.ContainingBuild = p_FindFirstContainingBuild(Path);
        NewFile.Content = Content;



        DocumentParsingContext Parser;
        MBError Result = true;
        NewFile.Source = Parser.ParseSource(Content.data(),Content.size(),MBUnicode::PathToUTF8(Path.filename()),Result);
        if(!Result)
        {
            NewFile.ParseError = true;   
        }
        m_DiagnosticsStore.StoreDiagnostics(URI,"",p_GetDiagnostics(NewFile.Source));
        p_SendDiagnosticsRequest(URI);
        //extract codeblocks
        NewFile.LangaugeCodeblocks = p_ExtractCodeblocks(NewFile.Source);
        //initialize server, and get opening content
        int TotalLines = std::count(Content.begin(),Content.end(),'\n');
        for(auto const& Language : NewFile.LangaugeCodeblocks)
        {
            p_InitializeLSP(Language.first);
            auto It = m_LoadedServers.find(Language.first);
            if(It != m_LoadedServers.end())
            {
                It->second.LoadFile(URI,TotalLines,Language.second);
            }
        }
    }
    void MBDocLSP::p_UpdateFile(std::string const& URI,std::string const& Content,std::vector<MBLSP::TextChange> const& Changes)
    {
        auto Path = MBLSP::URLDecodePath(URI);
        auto FileIt = m_LoadedFiles.find(URI);
        if(FileIt == m_LoadedFiles.end())
        {
            return;   
        }
        auto& CurrentFile = FileIt->second;

        CurrentFile.Content = Content;
        CurrentFile.Tokens = MBLSP::UpdateSemanticTokens(CurrentFile.Tokens,Changes);

        MBLSP::ChangeIndex ChangesResult(Changes);
        MBLSP::DidChangeTextDocument_Notification GenericChanges;
        GenericChanges.params.textDocument.uri = URI;
        GenericChanges.params.contentChanges = MBLSP::LineChangesToChangeEvents(Changes);


        int TotalLines = std::count(Content.begin(),Content.end(),'\n') + 1;

        for(auto const& Language : CurrentFile.LangaugeCodeblocks)
        {
            p_InitializeLSP(Language.first);
            if(auto ServerIt = m_LoadedServers.find(Language.first); ServerIt != m_LoadedServers.end())
            {
                ServerIt->second.UpdateContent(URI,TotalLines,Language.second,Changes);
            }
        }
        
        DocumentParsingContext Parser;
        MBError Result = true;
        DocumentSource NewSource;
        NewSource = Parser.ParseSource(Content.data(),Content.size(),MBUnicode::PathToUTF8(Path.filename()),Result);
        m_DiagnosticsStore.StoreDiagnostics(URI,"",p_GetDiagnostics(NewSource));
        p_SendDiagnosticsRequest(URI);
        if(!Result)
        {
            FileIt->second.ParseError = true;
        }
        else
        {
            CurrentFile.LangaugeCodeblocks = p_ExtractCodeblocks(NewSource);
            CurrentFile.Source = std::move(NewSource);
            for(auto const& Language : CurrentFile.LangaugeCodeblocks)
            {
                p_InitializeLSP(Language.first);    
                if(auto LangIt = m_LoadedServers.find(Language.first); LangIt != m_LoadedServers.end())
                {
                    LangIt->second.UpdateNewBlocks(URI,TotalLines,Language.second,Changes);
                }
            }
        }
    }
    std::vector<MBLSP::Diagnostic> MBDocLSP::p_GetDiagnostics(DocumentSource const& LoadedSource)
    {
        std::vector<MBLSP::Diagnostic> ReturnValue;
        auto RefVisitor = [&](DocReference const& Ref)
        {
            if(Ref.IsType<UnresolvedReference>())
            {
                auto& NewDiagnostic = ReturnValue.emplace_back();
                NewDiagnostic.message = "Unresolved reference";
                NewDiagnostic.range.start = Ref.Begin;
                NewDiagnostic.range.end = Ref.End;
            }
        };
        LambdaVisitor Visitor(&RefVisitor);
        DocumentTraverser Traverser;
        Traverser.Traverse(LoadedSource,Visitor);
        return ReturnValue;
    }
    void MBDocLSP::p_SendDiagnosticsRequest(std::string const& URI)
    {
        MBLSP::PublishDiagnostics_Notification DiagnosticNotification;
        DiagnosticNotification.params.uri = URI;
        DiagnosticNotification.params.diagnostics = m_DiagnosticsStore.GetDiagnostics(URI);
        m_Handler->SendNotification(DiagnosticNotification);
    }
    void MBDocLSP::OpenedDocument(std::string const& URI,std::string const& Content)
    {
        p_InitializeFile(URI,Content);
    }
    void MBDocLSP::DocumentChanged(std::string const& URI,std::string const& NewContent, std::vector<MBLSP::TextChange> const& Changes)
    {
        p_UpdateFile(URI,NewContent,Changes);
    }

    std::vector<int> MBDocLSP::p_MergeTokens(std::vector<std::pair<std::string, std::vector<int>>> const& TotalTokens)
    {
        std::vector<int> ReturnValue;
        struct TokensOffsetInfo
        {
            int CurrentIndex = 0;
            //only line needed, as tkens must come from different blocks
            int Line = 0;
        };
        std::vector<TokensOffsetInfo> CurrentOffsets = std::vector(TotalTokens.size(),TokensOffsetInfo());
        while(true)
        {
            int NextContainerIndex = -1;
            constexpr int max = std::numeric_limits<int>::max();
            int LowestLineNumber = max;
            for(int i = 0; i < CurrentOffsets.size();i++)
            {
                auto const& OffsetInfo = CurrentOffsets[i];
                if(OffsetInfo.Line < LowestLineNumber && OffsetInfo.CurrentIndex < TotalTokens[i].second.size())
                {
                    NextContainerIndex = i;
                    LowestLineNumber = OffsetInfo.Line;
                }
            }
            if(NextContainerIndex == -1)
            {
                break;   
            }
            auto& OffsetInfo = CurrentOffsets[NextContainerIndex];
            auto const& CurrentContainer = TotalTokens[NextContainerIndex].second;
            OffsetInfo.Line += OffsetInfo.Line;

            auto& Client = m_LoadedServers[TotalTokens[NextContainerIndex].first].GetClient();
            auto const& ServerCapabilities = Client.GetServerCapabilites();

            ReturnValue.insert(ReturnValue.end(),CurrentContainer.begin()+ OffsetInfo.CurrentIndex,
                     CurrentContainer.begin()+ OffsetInfo.CurrentIndex+5);
            int CurrentTokenID = ReturnValue[ReturnValue.size()-2];
            //if(ServerCapabilities.semanticTokensProvider.IsInitalized() && ServerCapabilities.semanticTokensProvider->legend.tokenTypes.size() > CurrentTokenID)
            //{
            //    auto const& ServerMap = ServerCapabilities.semanticTokensProvider->legend.tokenTypes;
            //    ReturnValue[ReturnValue.size()-2] = m_TokenToIndex[ ServerMap[CurrentTokenID]];
            //}
            OffsetInfo.CurrentIndex += 5;
        }
        return ReturnValue;
    }
    void MBDocLSP::p_NormalizeTypes(std::vector<int>& LSPTokens,std::vector<std::string> const& LSPTypes)
    {
        int Offset = 0;
        while(Offset + 3 < LSPTokens.size())
        {
            int& CurrentType = LSPTokens[Offset+3];
            if(CurrentType < LSPTypes.size())
            {
                CurrentType = m_TokenToIndex[LSPTypes[LSPTokens[Offset+3]]];
            }
            Offset += 5;
        }
    }
    std::vector<int> MBDocLSP::p_CalculateDocumentTokens(std::string const& URI,LoadedFile const& File)
    {
        std::vector<int> ReturnValue;
        if(File.ParseError != true)
        {
            MBLSP::SemanticToken_Request Request;
            Request.params.textDocument.uri  = URI;
            std::vector<std::pair<std::string,std::future<MBLSP::SemanticToken_Response>>> Futures;
            std::vector<std::pair<std::string, std::vector<int>>> Results;
            for(auto const& Language : File.LangaugeCodeblocks)
            {
                auto LanguageClient = m_LoadedServers.find(Language.first);
                if(LanguageClient != m_LoadedServers.end())
                {
                    Futures.push_back(std::make_pair(Language.first, LanguageClient->second.GetClient().GetRequestFuture(Request)));
                }
            }
            int PreviousLineEnd = 0;
            for(auto& Future : Futures)
            {
                auto Response = std::move(Future.second.get());
                if(Response.result.IsInitalized())
                {
                    std::vector<int> Tokens = std::move(Response.result.Value().data);
                    auto& Server = m_LoadedServers[Future.first];
                    auto& Client = Server.GetClient();
                    auto const& CurrentCapabilites = Client.GetServerCapabilites();
                    if(CurrentCapabilites.semanticTokensProvider.IsInitalized())
                    {
                        p_NormalizeTypes(Tokens,CurrentCapabilites.semanticTokensProvider->legend.tokenTypes);
                    }
                    auto ColorIt = m_UserColorInfo.LanguageConfigs.find(Future.first);
                    if(ColorIt != m_UserColorInfo.LanguageConfigs.end())
                    {
                        std::string const& Contents = Server.GetFileContent(URI);
                        MBLSP::LineIndex Index(Contents);
                        auto const& Regexes = ColorIt->second.RegexColoring;
                        auto LSPColorings = ConvertColoring(Tokens,Index);
                        auto RegexColorings = GetRegexColorings(LSPColorings,Regexes,Contents);
                        auto CombinedColors = CombineColorings(LSPColorings,RegexColorings);
                        Tokens = ConvertColoring(CombinedColors,Index);
                    }

                    Results.emplace_back(std::make_pair(Future.first,std::move(Tokens)));
                    if(Results.back().second.size() % 5 != 0)
                    {
                        throw std::runtime_error("Server sent semantic tokens count not divisible by 5");
                    }
                }
                else if(Response.error.IsInitalized())
                {
                    auto const& Debug = Response.error.Value();
                    int hej = 1;
                }
            }
            //merge results, taking into account that different languages might be interwoven
            ReturnValue = p_MergeTokens(Results);
        }
        else
        {
            ReturnValue = File.Tokens;   
        }
        return ReturnValue;
    }
    std::vector<std::string> MBDocLSP::p_PossibleResponses(DocumentFilesystem const& AssociatedFilesystem,std::string const& ReferenceString,
            DocumentPath const& AssociatedFile)
    {
        return {};
    }
    MBLSP::Initialize_Response MBDocLSP::HandleRequest(MBLSP::InitializeRequest const& Request)
    {
        MBLSP::Initialize_Response ReturnValue;
        ReturnValue.result = MBLSP::Initialize_Result();
        ReturnValue.result->capabilities = MBLSP::GetDefaultServerCapabilities(*this);

        m_ParentRequest = Request;
        std::unordered_set<std::string> AddedColors;
        std::vector<std::string> ColorMap;
        std::vector<std::pair<std::string,ColorTypeIndex>> ConfigColors;
        for(auto const& Pair : m_UserColorInfo.ColorInfo.ColoringNameToIndex)
        {
            ConfigColors.push_back(Pair);
        }
        std::sort(ConfigColors.begin(),ConfigColors.end(), [](auto const& lhs,auto const& rhs){return lhs.second < rhs.second; });
        for(auto const& Pair : ConfigColors)
        {
            ColorMap.push_back(Pair.first);
            AddedColors.insert(Pair.first);
        }

        for(auto const& Color : ReturnValue.result->capabilities.semanticTokensProvider->legend.tokenTypes)
        {
            if(AddedColors.find(Color) == AddedColors.end())
            {
                AddedColors.insert(Color);
                ColorMap.emplace_back(Color);
            }
        }
        ReturnValue.result->capabilities.semanticTokensProvider->legend.tokenTypes = std::move(ColorMap);
        for(int i = 0; i < ReturnValue.result->capabilities.semanticTokensProvider->legend.tokenTypes.size();i++)
        {
            m_TokenToIndex[ReturnValue.result->capabilities.semanticTokensProvider->legend.tokenTypes[i]] = i;
        }
        return ReturnValue;
    }
    MBLSP::GotoDefinition_Response MBDocLSP::HandleRequest(MBLSP::GotoDefinition_Request const& Request)
    {
        MBLSP::GotoDefinition_Response Response;
        auto FileIt = m_LoadedFiles.find(Request.params.textDocument.uri);

        if(FileIt == m_LoadedFiles.end())
        {
            return Response;
        }
        auto const& CurrentFile = FileIt->second;
        MBParsing::JSONObject RawResponse;
        if(p_DelegatePositionRequest(Request.params.position,CurrentFile,Request,Response))
        {
            //m_Handler->UseRawResponse(std::move(RawResponse));
            return Response;
        }
        return Response;
    }
    MBLSP::SemanticToken_Response MBDocLSP::HandleRequest(MBLSP::SemanticToken_Request const& Request)
    {
        std::string URI = Request.params.textDocument.uri;
        MBLSP::SemanticToken_Response Response;
        MBLSP::SemanticTokens Result = MBLSP::SemanticTokens();
        auto FileIt = m_LoadedFiles.find(URI);
        if(FileIt != m_LoadedFiles.end())
        {
            Result.data = p_CalculateDocumentTokens(URI,FileIt->second);
            FileIt->second.Tokens = Result.data;
        }
        Response.result = std::move(Result);
        return Response;
    }
    MBLSP::SemanticTokensRange_Response MBDocLSP::HandleRequest(MBLSP::SemanticTokensRange_Request const& Request)
    {
        std::string URI = Request.params.textDocument.uri;
        MBLSP::SemanticTokensRange_Response Response;
        MBLSP::SemanticTokens Result = MBLSP::SemanticTokens();
        auto FileIt = m_LoadedFiles.find(URI);
        if(FileIt != m_LoadedFiles.end())
        {
            std::vector<int> TotalData = p_CalculateDocumentTokens(URI,FileIt->second);
            FileIt->second.Tokens = TotalData;
            Result.data = MBLSP::GetTokenRange(TotalData,Request.params.range);
        }
        Response.result = std::move(Result);
        return Response;
    }
    //MBParsing::JSONObject MBDocLSP::HandleGenericRequest(MBParsing::JSONObject const& Request)
    //{
    //    MBParsing::JSONObject ReturnValue;
    //    if(Request.HasAttribute("textDocument") && Request["textDocument"].HasAttribute("uri") && Request.HasAttribute("position"))
    //    {
    //        std::string URI = Request["textDocument"]["uri"].GetStringData();
    //        MBLSP::Position TargetPos;
    //        TargetPos.Parse(Request["position"]);
    //        auto FileIt = m_LoadedFiles.find(URI);
    //        if(FileIt != m_LoadedFiles.end())
    //        {
    //            auto const& File = FileIt->second;
    //            if(p_DelegatePositionRequest(TargetPos,File,Request,ReturnValue))
    //            {
    //                return ReturnValue;
    //            }
    //            else
    //            {
    //                throw std::runtime_error("invalid position");   
    //            }
    //        }
    //        else
    //        {
    //            throw std::runtime_error("file not loaded");   
    //        }
    //    }
    //    else
    //    {
    //        throw std::runtime_error("Request not implemented");
    //    }

    //    return ReturnValue;
    //}
    MBLSP::Completion_Response MBDocLSP::HandleRequest(MBLSP::Completion_Request const& Request)
    {
        std::string URI = Request.params.textDocument.uri;
        auto FileIt = m_LoadedFiles.find(URI);
        MBLSP::Completion_Response Response;
        Response.result = MBLSP::Completion_Result();
        if(FileIt == m_LoadedFiles.end())
        {
            return Response;   
        }
        //auto Test = m_LoadedServers["cpp"].GetClient().SendRequest(MBLSP::Completion_Request());
        //return Response;
        auto const& CurrentFile = FileIt->second;
        //auto NewRequest = Request;
        //std::string RequestToSendString = NewRequest.GetJSON().ToPrettyString();
        //std::string RawRequestStrign = m_Handler->GetRawServerMessage().ToPrettyString();
        //NewRequest.params.textDocument.uri = MBLSP::URLEncodePath(MBLSP::URLDecodePath(Request.params.textDocument.uri));
        if(p_DelegatePositionRequest(Request.params.position,CurrentFile,Request,Response))
        {
            //m_Handler->UseRawResponse(std::move(RawResponse));
            return Response;
        }

        DocumentPath AssociatedPath;
        DocumentSource const& AssociatedDocument = DocumentSource();
        DocumentFilesystem const& AssociatedFilesystem = DocumentFilesystem();
        auto ReferenceTraverser = [&](DocReference const& NewRef)
        {
            //TODO make so this interface is not necessary...
            if(MBLSP::Contains(NewRef.Begin,NewRef.End,Request.params.position))
            {
                std::string ReferenceString;
                if(NewRef.IsType<FileReference>())
                {
                    ReferenceString = NewRef.GetType<FileReference>().Path.GetString();
                }
                else if(NewRef.IsType<UnresolvedReference>())
                {
                    ReferenceString = NewRef.GetType<UnresolvedReference>().ReferenceString;
                }
                auto PossibleValues = p_PossibleResponses(AssociatedFilesystem,ReferenceString,AssociatedPath);
                for(auto& Value : PossibleValues)
                {
                    MBLSP::CompletionItem& NewItem = Response.result->items.emplace_back();
                    NewItem.label = std::move(Value);
                }
            }
        };
        LambdaVisitor Visitor = LambdaVisitor(&ReferenceTraverser);
        DocumentTraverser Traverser;
        Traverser.Traverse(AssociatedDocument,Visitor);
        return Response;
    }
    MBLSP::CompletionItemResolve_Response MBDocLSP::HandleRequest(MBLSP::CompletionItemResolve_Request const& Request) 
    {
        MBLSP::CompletionItemResolve_Response ReturnValue;
        ReturnValue.result = Request.params;
        if(m_LastAnsweredServer == "Default")
        {
            return ReturnValue;
        }
        auto ServerIt = m_LoadedServers.find(m_LastAnsweredServer);
        if(ServerIt != m_LoadedServers.end())
        {
            auto const& Capabilities = ServerIt->second.GetClient().GetServerCapabilites();
            if(Capabilities.completionProvider && Capabilities.completionProvider && Capabilities.completionProvider->resolveProvider && 
                    Capabilities.completionProvider->resolveProvider.Value())
            {
                ReturnValue = ServerIt->second.GetClient().SendRequest(Request);
            }
        }
        return ReturnValue;
    }
}
