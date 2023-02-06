#include "ColorConf.h"
namespace MBDoc
{
LanguageConfig LanguageConfig::Parse(MBParsing::JSONObject const& ObjectToParse)
{
    LanguageConfig ReturnValue;
    ReturnValue.FillObject(ObjectToParse);return ReturnValue;
}
void LanguageConfig::FillObject(MBParsing::JSONObject const& ObjectToParse)
{
    if(ObjectToParse.GetType() != MBParsing::JSONObjectType::Aggregate)
    {
        throw std::runtime_error("Error parsing LanguageConfig: Object is not of aggregate type");
    }
    if(ObjectToParse.HasAttribute("LSP"))
    {
        if(ObjectToParse["LSP"].GetType() != MBParsing::JSONObjectType::String)
        {
            throw std::runtime_error("Error parsing object of type LanguageConfig: member LSP isn't of type string");
        }
        LSP = ObjectToParse["LSP"].GetStringData();
        
    }
    if(!ObjectToParse.HasAttribute("Regexes"))
    {
        throw std::runtime_error("Error parsing object of type LanguageConfig: missing mandatory field Regexes");
    }
    {
        auto& i_MapMember = Regexes;
        for(auto const& i_Pair : ObjectToParse["Regexes"].GetMapData())
        {
            i_MapMember[i_Pair.first] = MBParsing::JSONObject::FromJSON<std::vector<std::string>>(i_Pair.second);
        }
    }
    
}
MBParsing::JSONObject LanguageConfig::GetJSON() const
{
    MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
    ReturnValue["LSP"] = MBParsing::JSONObject(LSP);
    {
        auto const& i_MapMember = Regexes;
        MBParsing::JSONObject i_MapObject(MBParsing::JSONObjectType::Aggregate);
        for(auto const& i_Pair : i_MapMember)
        {
            i_MapObject[i_Pair.first] = MBParsing::JSONObject::ToJSON(i_Pair.second);
        }
        ReturnValue["Regexes"] = std::move(i_MapObject);
        
    }
    return ReturnValue;
}
ColorConfiguation ColorConfiguation::Parse(MBParsing::JSONObject const& ObjectToParse)
{
    ColorConfiguation ReturnValue;
    ReturnValue.FillObject(ObjectToParse);return ReturnValue;
}
void ColorConfiguation::FillObject(MBParsing::JSONObject const& ObjectToParse)
{
    if(ObjectToParse.GetType() != MBParsing::JSONObjectType::Aggregate)
    {
        throw std::runtime_error("Error parsing ColorConfiguation: Object is not of aggregate type");
    }
    if(!ObjectToParse.HasAttribute("ColorMap"))
    {
        throw std::runtime_error("Error parsing object of type ColorConfiguation: missing mandatory field ColorMap");
    }
    {
        auto& i_MapMember = ColorMap;
        for(auto const& i_Pair : ObjectToParse["ColorMap"].GetMapData())
        {
            i_MapMember[i_Pair.first] = i_Pair.second.GetStringData();
        }
    }
    if(!ObjectToParse.HasAttribute("LanguageColorings"))
    {
        throw std::runtime_error("Error parsing object of type ColorConfiguation: missing mandatory field LanguageColorings");
    }
    {
        auto& i_MapMember = LanguageColorings;
        for(auto const& i_Pair : ObjectToParse["LanguageColorings"].GetMapData())
        {
            i_MapMember[i_Pair.first] = LanguageConfig::Parse(i_Pair.second);
        }
    }
    
}
MBParsing::JSONObject ColorConfiguation::GetJSON() const
{
    MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
    {
        auto const& i_MapMember = ColorMap;
        MBParsing::JSONObject i_MapObject(MBParsing::JSONObjectType::Aggregate);
        for(auto const& i_Pair : i_MapMember)
        {
            i_MapObject[i_Pair.first] = i_Pair.second;
        }
        ReturnValue["ColorMap"] = std::move(i_MapObject);
        
    }
    {
        auto const& i_MapMember = LanguageColorings;
        MBParsing::JSONObject i_MapObject(MBParsing::JSONObjectType::Aggregate);
        for(auto const& i_Pair : i_MapMember)
        {
            i_MapObject[i_Pair.first] = i_Pair.second.GetJSON();
        }
        ReturnValue["LanguageColorings"] = std::move(i_MapObject);
        
    }
    return ReturnValue;
}
const int i_TypeIndexEnd[] = {1,2,};

}