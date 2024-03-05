#include "LSPUtils.h"
#include <MBUtility/MBFiles.h>
#include <MBUnicode/MBUnicode.h>
#include <MBSystem/MBSystem.h>
#include <MBSystem/BiDirectionalSubProcess.h>
#include <MBUtility/InterfaceAdaptors.h>
namespace MBDoc
{
    LSPInfo LoadLSPConfig(std::filesystem::path const& PathToParse)
    {
        LSPInfo ReturnValue;
        std::string Data = MBUtility::ReadWholeFile(MBUnicode::PathToUTF8(PathToParse));
        MBError ParseResult = true;
        MBParsing::JSONObject JSONData = MBParsing::ParseJSONObject(Data,0,nullptr,&ParseResult);
        if(!ParseResult)
        {
            throw std::runtime_error("Error parsing LSPConf.json as json file: "+ParseResult.ErrorMessage);
        }
        try
        {
            ReturnValue.FillObject(JSONData);
        }
        catch(std::exception const& e)
        {
            throw std::runtime_error("Error parsing LSPConf.json: "+std::string(e.what())); 
        }
        return(ReturnValue);
    }
    LSPInfo LoadLSPConfig()
    {
        return LoadLSPConfig(MBSystem::GetUserHomeDirectory()/".mbdoc/LSPConfig.json");
    }
    LSPInfo LoadLSPConfigIfExists()
    {
        LSPInfo ReturnValue;
        if(std::filesystem::exists(MBSystem::GetUserHomeDirectory()/".mbdoc/LSPConfig.json"))
        {
            return LoadLSPConfig(MBSystem::GetUserHomeDirectory()/".mbdoc/LSPConfig.json");   
        }
        return ReturnValue;
    }
    std::unique_ptr<MBLSP::LSP_Client> StartLSPServer(LSPServer const& ServerToStart)
    {
        MBLSP::InitializeRequest InitReq;    
        InitReq.params.rootUri = MBLSP::URLEncodePath(std::filesystem::current_path());
        if(ServerToStart.initializationOptions)
        {
            InitReq.params.initializationOptions = ServerToStart.initializationOptions.Value();   
        }
        return(StartLSPServer(ServerToStart,InitReq));
    }
    std::unique_ptr<MBLSP::LSP_Client> StartLSPServer(LSPServer const& ServerToStart,MBLSP::InitializeRequest const& InitReq)
    {
        std::unique_ptr<MBLSP::LSP_Client> ReturnValue;
        std::unique_ptr<MBSystem::BiDirectionalSubProcess> SubProcess = 
            std::make_unique<MBSystem::BiDirectionalSubProcess>(ServerToStart.CommandName,ServerToStart.CommandArguments);
        std::unique_ptr<MBUtility::IndeterminateInputStream> InputStream = std::make_unique<MBUtility::NonOwningIndeterminateInputStream>(SubProcess.get());
        ReturnValue = std::make_unique<MBLSP::LSP_Client>(std::move(InputStream),std::move(SubProcess));
        ReturnValue->InitializeServer(InitReq);
        return(ReturnValue);
    }
}
