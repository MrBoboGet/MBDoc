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
ColorInfo ColorInfo::Parse(MBParsing::JSONObject const& ObjectToParse)
{
    ColorInfo ReturnValue;
    ReturnValue.FillObject(ObjectToParse);return ReturnValue;
}
void ColorInfo::FillObject(MBParsing::JSONObject const& ObjectToParse)
{
    if(ObjectToParse.GetType() != MBParsing::JSONObjectType::Aggregate)
    {
        throw std::runtime_error("Error parsing ColorInfo: Object is not of aggregate type");
    }
    if(!ObjectToParse.HasAttribute("DefaultColor"))
    {
        throw std::runtime_error("Error parsing object of type ColorInfo: missing mandatory field DefaultColor");
    }
    if(ObjectToParse["DefaultColor"].GetType() != MBParsing::JSONObjectType::String)
    {
        throw std::runtime_error("Error parsing object of type ColorInfo: member DefaultColor isn't of type string");
    }
    DefaultColor = ObjectToParse["DefaultColor"].GetStringData();
    if(!ObjectToParse.HasAttribute("ColorMap"))
    {
        throw std::runtime_error("Error parsing object of type ColorInfo: missing mandatory field ColorMap");
    }
    {
        auto& i_MapMember = ColorMap;
        for(auto const& i_Pair : ObjectToParse["ColorMap"].GetMapData())
        {
            i_MapMember[i_Pair.first] = i_Pair.second.GetStringData();
        }
    }
    
}
MBParsing::JSONObject ColorInfo::GetJSON() const
{
    MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
    ReturnValue["DefaultColor"] = MBParsing::JSONObject(DefaultColor);
    {
        auto const& i_MapMember = ColorMap;
        MBParsing::JSONObject i_MapObject(MBParsing::JSONObjectType::Aggregate);
        for(auto const& i_Pair : i_MapMember)
        {
            i_MapObject[i_Pair.first] = i_Pair.second;
        }
        ReturnValue["ColorMap"] = std::move(i_MapObject);
        
    }
    return ReturnValue;
}
ColorConfiguration ColorConfiguration::Parse(MBParsing::JSONObject const& ObjectToParse)
{
    ColorConfiguration ReturnValue;
    ReturnValue.FillObject(ObjectToParse);return ReturnValue;
}
void ColorConfiguration::FillObject(MBParsing::JSONObject const& ObjectToParse)
{
    if(ObjectToParse.GetType() != MBParsing::JSONObjectType::Aggregate)
    {
        throw std::runtime_error("Error parsing ColorConfiguration: Object is not of aggregate type");
    }
    if(!ObjectToParse.HasAttribute("Coloring"))
    {
        throw std::runtime_error("Error parsing object of type ColorConfiguration: missing mandatory field Coloring");
    }
    if(ObjectToParse["Coloring"].GetType() != MBParsing::JSONObjectType::Aggregate)
    {
        throw std::runtime_error("Error parsing object of type ColorConfiguration: member Coloring isn't of type aggregate");
    }
    Coloring = ColorInfo::Parse(ObjectToParse["Coloring"]);
    if(!ObjectToParse.HasAttribute("LanguageColorings"))
    {
        throw std::runtime_error("Error parsing object of type ColorConfiguration: missing mandatory field LanguageColorings");
    }
    {
        auto& i_MapMember = LanguageColorings;
        for(auto const& i_Pair : ObjectToParse["LanguageColorings"].GetMapData())
        {
            i_MapMember[i_Pair.first] = LanguageConfig::Parse(i_Pair.second);
        }
    }
    
}
MBParsing::JSONObject ColorConfiguration::GetJSON() const
{
    MBParsing::JSONObject ReturnValue(MBParsing::JSONObjectType::Aggregate);
    ReturnValue["Coloring"] = Coloring.GetJSON();
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
const int i_TypeIndexEndColorConf_h[] = {1,2,3,};

}