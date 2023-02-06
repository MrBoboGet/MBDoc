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
extern const int i_TypeIndexEnd[];
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
struct ColorConfiguation
{
    private:
    public:
    std::unordered_map<std::string,std::string> ColorMap;
    std::unordered_map<std::string,LanguageConfig> LanguageColorings;
    static ColorConfiguation Parse(MBParsing::JSONObject const& ObjectToParse);
    void FillObject(MBParsing::JSONObject const& ObjectToParse);
    MBParsing::JSONObject GetJSON() const;
    
};
template<> inline int GetTypeIndex<LanguageConfig>()
{
return 0;
}
template<> inline int GetTypeIndex<ColorConfiguation>()
{
return 1;
}

}