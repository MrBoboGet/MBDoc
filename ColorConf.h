#pragma once
#include <string>
#include <variant>

#include <stdexcept>
#include <vector>
#include <MBUtility/Optional.h>
#include <unordered_map>
#include <MBParsing/MBParsing.h>
namespace MBDoc
{
extern const int i_TypeIndexEndColorConf_h[];
template<typename T> inline int GetTypeIndex();
struct LanguageConfig
{
    private:
    public:
    std::string LSP = std::string("");
    std::unordered_map<std::string,std::vector<std::string>> Regexes;
    static LanguageConfig Parse(MBParsing::JSONObject const& ObjectToParse);
    void FillObject(MBParsing::JSONObject const& ObjectToParse);
    MBParsing::JSONObject GetJSON() const;
    
};
struct ColorInfo
{
    private:
    public:
    std::string DefaultColor;
    std::unordered_map<std::string,std::string> ColorMap;
    static ColorInfo Parse(MBParsing::JSONObject const& ObjectToParse);
    void FillObject(MBParsing::JSONObject const& ObjectToParse);
    MBParsing::JSONObject GetJSON() const;
    
};
struct ColorConfiguration
{
    private:
    public:
    ColorInfo Coloring;
    std::unordered_map<std::string,LanguageConfig> LanguageColorings;
    static ColorConfiguration Parse(MBParsing::JSONObject const& ObjectToParse);
    void FillObject(MBParsing::JSONObject const& ObjectToParse);
    MBParsing::JSONObject GetJSON() const;
    
};
template<> inline int GetTypeIndex<LanguageConfig>()
{
return 0;
}
template<> inline int GetTypeIndex<ColorInfo>()
{
return 1;
}
template<> inline int GetTypeIndex<ColorConfiguration>()
{
return 2;
}

}