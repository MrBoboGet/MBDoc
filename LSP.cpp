#include "LSP.h"
namespace MBDoc
{
LSPServer LSPServer::Parse(MBParsing::JSONObject const& ObjectToParse)
{
    LSPServer ReturnValue;
    ReturnValue.FillObject(ObjectToParse);return ReturnValue;
}
void LSPServer::FillObject(MBParsing::JSONObject const& ObjectToParse)
{
    if(ObjectToParse.GetType() != MBParsing::JSONObjectType::Aggregate)
    {
        throw std::runtime_error("Error parsing LSPServer: Object is not of aggregate type");
    }
    if(!ObjectToParse.HasAttribute("CommandName"))
    {
        throw std::runtime_error("Error parsing object of type LSPServer: missing mandatory field CommandName");
    }
    if(ObjectToParse["CommandName"].GetType() != MBParsing::JSONObjectType::String)
    {
        throw std::runtime_error("Error parsing object of type LSPServer: member CommandName isn't of type string");
    }
    CommandName = ObjectToParse["CommandName"].GetStringData();
    if(!ObjectToParse.HasAttribute("CommandArguments"))
    {
        throw std::runtime_error("Error parsing object of type LSPServer: missing mandatory field CommandArguments");
    }
    if(ObjectToParse["CommandArguments"].GetType() != MBParsing::JSONObjectType::Array)
    {
        throw std::runtime_error("Error parsing struct LSPServer: Member CommandArguments is not of array type");
    }
    for(auto const& SubStruct : ObjectToParse["CommandArguments"].GetArrayData())
    {
        if(SubStruct.GetType() != MBParsing::JSONObjectType::String)
        {
            throw std::runtime_error("Error parsing object of type LSPServer: member CommandArguments isn't of type string");
        }
        CommandArguments.push_back(SubStruct.GetStringData());
    }
    if(ObjectToParse.HasAttribute("initializationOptions"))
    {
        initializationOptions = ObjectToParse["initializationOptions"];
        
    }
    
}
MBParsing::JSONObject LSPServer::GetJSON() const
{
    MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
    ReturnValue["CommandName"] = MBParsing::JSONObject(CommandName);
    std::vector<MBParsing::JSONObject> CommandArguments_Array;
    for(auto const& Element : CommandArguments)
    {
        CommandArguments_Array.push_back(MBParsing::JSONObject(Element));
        
    }
    ReturnValue["CommandArguments"] = std::move(CommandArguments_Array);if(initializationOptions)
    {
        ReturnValue["initializationOptions"] = MBParsing::JSONObject(initializationOptions.Value());
        
    }
    return ReturnValue;
}
LSPInfo LSPInfo::Parse(MBParsing::JSONObject const& ObjectToParse)
{
    LSPInfo ReturnValue;
    ReturnValue.FillObject(ObjectToParse);return ReturnValue;
}
void LSPInfo::FillObject(MBParsing::JSONObject const& ObjectToParse)
{
    if(ObjectToParse.GetType() != MBParsing::JSONObjectType::Aggregate)
    {
        throw std::runtime_error("Error parsing LSPInfo: Object is not of aggregate type");
    }
    if(!ObjectToParse.HasAttribute("Servers"))
    {
        throw std::runtime_error("Error parsing object of type LSPInfo: missing mandatory field Servers");
    }
    {
        auto& i_MapMember = Servers;
        for(auto const& i_Pair : ObjectToParse["Servers"].GetMapData())
        {
            i_MapMember[i_Pair.first] = LSPServer::Parse(i_Pair.second);
        }
    }
    
}
MBParsing::JSONObject LSPInfo::GetJSON() const
{
    MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
    {
        auto const& i_MapMember = Servers;
        MBParsing::JSONObject i_MapObject(MBParsing::JSONObjectType::Aggregate);
        for(auto const& i_Pair : i_MapMember)
        {
            i_MapObject[i_Pair.first] = i_Pair.second.GetJSON();
        }
        ReturnValue["Servers"] = std::move(i_MapObject);
        
    }
    return ReturnValue;
}
const int i_TypeIndexEndLSP_h[] = {1,2,};

}