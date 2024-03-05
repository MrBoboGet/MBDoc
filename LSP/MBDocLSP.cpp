#include "MBDocLSP.h"
#include <MBUtility/MBFiles.h>

#include "../LSPUtils.h"
#include <MBLSP/TextChanges.h>

#include <MBLSP/Capabilities.h>
namespace MBDoc
{

    bool MBDocLSP::LoadedServer::FileLoaded(std::string const& FilePath)
    {
        return m_OpenedFiles.find(FilePath) != m_OpenedFiles.end();
    }
    void MBDocLSP::LoadedServer::LoadFile(std::string const& FilePath,std::vector<CodeBlock> const& CodeBlocks)
    {
        std::string TotalData;
        m_OpenedFiles.insert(FilePath);
        int PreviousLine = 0;
        for(auto const& Block : CodeBlocks)
        {
            TotalData += std::string( (Block.LineBegin - PreviousLine)+1,'\n');
            TotalData += Block.Content;
            TotalData += "\n";
            PreviousLine = Block.LineEnd;
            assert(PreviousLine >= 0);
        }
        MBLSP::DidOpenTextDocument_Notification OpenNotification;
        OpenNotification.params.textDocument.text = TotalData;
        OpenNotification.params.textDocument.uri = MBLSP::URLEncodePath(FilePath);
        m_AssociatedServer->SendNotification(OpenNotification);
    }
    bool MBDocLSP::LoadedServer::p_LineInBlocks(int LineIndex,std::vector<CodeBlock> const& Blocks)
    {
        bool ReturnValue = false;
        for(auto const& Block : Blocks)
        {
            if(Block.LineBegin < LineIndex && LineIndex < Block.LineEnd)
            {
                return true;   
            }
        }
        return ReturnValue;
    }
    void MBDocLSP::LoadedServer::UpdateContent(std::string const& FilePath,std::vector<CodeBlock> const& OldBlock,std::vector<MBLSP::TextChange> const& Changes)
    {
        if(!FileLoaded(FilePath))
        {
            LoadFile(FilePath,OldBlock);
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
                RemoveCodeblockChange.params.textDocument.uri = MBLSP::URLEncodePath(FilePath);
                MBLSP::TextDocumentContentChangeEvent Change;
                Change.text = std::string(CodeBlock.LineEnd - CodeBlock.LineBegin,'\n');
                Change.range = MBLSP::Range();
                Change.range->start = MBLSP::Position{CodeBlock.LineBegin,0};
                Change.range->end = MBLSP::Position{CodeBlock.LineEnd,0};
                m_AssociatedServer->SendNotification(RemoveCodeblockChange);
            }
        }
        //make changes so that lines outside of boxes are empty, with correct line offsets
        std::vector<MBLSP::TextChange> NewChanges;
        for(auto const& Change : Changes)
        {
            if(p_LineInBlocks(Change.LinePosition,OldBlock))
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
        UpdateNotification.params.textDocument.uri = MBLSP::URLEncodePath(FilePath);
        UpdateNotification.params.contentChanges = MBLSP::LineChangesToChangeEvents(NewChanges);
    }
    void MBDocLSP::LoadedServer::UpdateNewBlocks(std::string const& FilePath,std::vector<CodeBlock> const& NewBlocks,std::vector<MBLSP::TextChange> const& Changes)
    {
        MBLSP::ChangeIndex ChangesResult(Changes);
        for(auto const& CodeBlock : NewBlocks)
        {
            auto BeginChange = ChangesResult.GetChangeType(CodeBlock.LineBegin);
            auto EndChange = ChangesResult.GetChangeType(CodeBlock.LineEnd);
            if(BeginChange != MBLSP::LineChangeType::Null || EndChange != MBLSP::LineChangeType::Null)
            {
                MBLSP::DidChangeTextDocument_Notification RemoveCodeblockChange;
                RemoveCodeblockChange.params.textDocument.uri = MBLSP::URLEncodePath(FilePath);
                MBLSP::TextDocumentContentChangeEvent Change;
                Change.text = CodeBlock.Content;
                Change.range = MBLSP::Range();
                Change.range->start = MBLSP::Position{CodeBlock.LineBegin+1,0};
                Change.range->end = MBLSP::Position{CodeBlock.LineEnd,0};
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
        if(m_UserLSPInfo.Servers.find(LanguageName) != m_UserLSPInfo.Servers.end())
        {
            auto& NewInfo = m_LoadedServers[LanguageName];
            NewInfo = LoadedServer(StartLSPServer(m_UserLSPInfo.Servers[LanguageName]));
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
    void MBDocLSP::p_InitializeFile(std::string const& FilePath,std::string const& Content)
    {
        auto CanonicalPath = std::filesystem::canonical(FilePath);
        std::string CanonicalString = MBUnicode::PathToUTF8(CanonicalPath);
        LoadedFile& NewFile = m_LoadedFiles.emplace(std::make_pair(CanonicalString,LoadedFile()) ).first->second;
        NewFile.ContainingBuild = p_FindFirstContainingBuild(CanonicalPath);

        DocumentParsingContext Parser;
        MBError Result = true;
        NewFile.Source = Parser.ParseSource(Content.data(),Content.size(),MBUnicode::PathToUTF8(CanonicalPath.filename()),Result);
        if(!Result)
        {
            NewFile.ParseError = true;   
        }
        //extract codeblocks
        NewFile.LangaugeCodeblocks = p_ExtractCodeblocks(NewFile.Source);
        //initialize server, and get opening content
        for(auto const& Language : NewFile.LangaugeCodeblocks)
        {
            p_InitializeLSP(Language.first);
            auto It = m_LoadedServers.find(Language.first);
            if(It != m_LoadedServers.end())
            {
                It->second.LoadFile(MBUnicode::PathToUTF8(CanonicalPath),Language.second);
            }
        }
    }
    void MBDocLSP::p_UpdateFile(std::string const& FilePath,std::string const& Content,std::vector<MBLSP::TextChange> const& Changes)
    {
        auto CanonicalPath = std::filesystem::canonical(FilePath);
        std::string CanonicalString = MBUnicode::PathToUTF8(CanonicalPath);
        auto FileIt = m_LoadedFiles.find(CanonicalString);
        if(FileIt == m_LoadedFiles.end())
        {
            return;   
        }
        auto& CurrentFile = FileIt->second;

        MBLSP::ChangeIndex ChangesResult(Changes);
        MBLSP::DidChangeTextDocument_Notification GenericChanges;
        GenericChanges.params.textDocument.uri = MBLSP::URLEncodePath(CanonicalPath);
        GenericChanges.params.contentChanges = MBLSP::LineChangesToChangeEvents(Changes);
        for(auto const& Language : CurrentFile.LangaugeCodeblocks)
        {
            p_InitializeLSP(Language.first);
            if(auto ServerIt = m_LoadedServers.find(Language.first); ServerIt != m_LoadedServers.end())
            {
                ServerIt->second.UpdateContent(CanonicalString,Language.second,Changes);
            }
        }
        
        DocumentParsingContext Parser;
        MBError Result = true;
        DocumentSource NewSource;
        NewSource = Parser.ParseSource(Content.data(),Content.size(),MBUnicode::PathToUTF8(CanonicalPath.filename()),Result);
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
                    LangIt->second.UpdateNewBlocks(CanonicalString,Language.second,Changes);
                }
            }
        }
    }
    void MBDocLSP::OpenedDocument(std::string const& URI,std::string const& Content)
    {
        p_InitializeFile(MBUnicode::PathToUTF8(MBLSP::URLDecodePath(URI)),Content);
    }
    void MBDocLSP::DocumentChanged(std::string const& URI,std::string const& NewContent, std::vector<MBLSP::TextChange> const& Changes)
    {
        p_UpdateFile(MBUnicode::PathToUTF8(MBLSP::URLDecodePath(URI)),NewContent,Changes);
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
            if(ServerCapabilities.semanticTokensProvider.IsInitalized() && ServerCapabilities.semanticTokensProvider->legend.tokenTypes.size() > CurrentTokenID)
            {
                auto const& ServerMap = ServerCapabilities.semanticTokensProvider->legend.tokenTypes;
                ReturnValue[ReturnValue.size()-2] = m_TokenToIndex[ ServerMap[CurrentTokenID]];
            }
            OffsetInfo.CurrentIndex += 5;
        }
        return ReturnValue;
    }
    std::vector<int> MBDocLSP::p_CalculateDocumentTokens(std::string const& DocumentPath,LoadedFile& File)
    {
        std::vector<int> ReturnValue;
        if(File.ParseError != true)
        {
            MBLSP::SemanticToken_Request Request;
            Request.params.textDocument.uri  = MBLSP::URLEncodePath(DocumentPath);
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
                    Results.emplace_back(std::make_pair(Future.first,std::move(Response.result.Value().data)));
                    if(Results.back().second.size() % 5 != 0)
                    {
                        throw std::runtime_error("Server sent semantic tokens count not divisible by 5");
                    }
                }
            }
            //merge results, taking into account that different languages might be interwoven
            ReturnValue = p_MergeTokens(Results);
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
        for(int i = 0; i < ReturnValue.result->capabilities.semanticTokensProvider->legend.tokenTypes.size();i++)
        {
            m_TokenToIndex[ReturnValue.result->capabilities.semanticTokensProvider->legend.tokenTypes[i]] = i;
        }
        return ReturnValue;
    }
    MBLSP::GotoDefinition_Response MBDocLSP::HandleRequest(MBLSP::GotoDefinition_Request const& Request)
    {
        std::string CanonicalPath = MBUnicode::PathToUTF8(std::filesystem::canonical(Request.params.textDocument.uri));
        MBLSP::GotoDefinition_Response Response;
        auto FielIt = m_LoadedFiles.find(CanonicalPath);
        return Response;
    }
    MBLSP::SemanticToken_Response MBDocLSP::HandleRequest(MBLSP::SemanticToken_Request const& Request)
    {
        std::string Path = MBUnicode::PathToUTF8(std::filesystem::canonical(MBLSP::URLDecodePath(Request.params.textDocument.uri)));
        MBLSP::SemanticToken_Response Response;
        MBLSP::SemanticTokens Result = MBLSP::SemanticTokens();
        auto FileIt = m_LoadedFiles.find(Path);
        if(FileIt != m_LoadedFiles.end())
        {
            Result.data = p_CalculateDocumentTokens(Path,FileIt->second);
        }
        Response.result = std::move(Result);
        return Response;
    }
    MBLSP::SemanticTokensRange_Response MBDocLSP::HandleRequest(MBLSP::SemanticTokensRange_Request const& Request)
    {
        std::string Path = MBUnicode::PathToUTF8(std::filesystem::canonical(MBLSP::URLDecodePath(Request.params.textDocument.uri)));
        MBLSP::SemanticTokensRange_Response Response;
        MBLSP::SemanticTokens Result = MBLSP::SemanticTokens();
        auto FileIt = m_LoadedFiles.find(Path);
        if(FileIt != m_LoadedFiles.end())
        {
            std::vector<int> TotalData = p_CalculateDocumentTokens(Path,FileIt->second);
            Result.data = MBLSP::GetTokenRange(TotalData,Request.params.range);
        }
        Response.result = std::move(Result);
        return Response;
    }
    MBLSP::Completion_Response MBDocLSP::HandleRequest(MBLSP::Completion_Request const& Request)
    {
        std::string Path = MBUnicode::PathToUTF8(std::filesystem::canonical(Request.params.textDocument.uri));
        auto FileIt = m_LoadedFiles.find(Path);

        MBLSP::Completion_Response Response;
        Response.result = MBLSP::Completion_Result();

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
}
