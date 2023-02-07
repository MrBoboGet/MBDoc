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
struct LSPServer
{
    private:
    public:
    std::string CommandName;
    std::vector<std::string> CommandArguments;
    static LSPServer Parse(MBParsing::JSONObject const& ObjectToParse);
    void FillObject(MBParsing::JSONObject const& ObjectToParse);
    MBParsing::JSONObject GetJSON() const;
    
};
struct LSPInfo
{
    private:
    public:
    std::unordered_map<std::string,LSPServer> Servers;
    static LSPInfo Parse(MBParsing::JSONObject const& ObjectToParse);
    void FillObject(MBParsing::JSONObject const& ObjectToParse);
    MBParsing::JSONObject GetJSON() const;
    
};
template<> inline int GetTypeIndex<LSPServer>()
{
return 0;
}
template<> inline int GetTypeIndex<LSPInfo>()
{
return 1;
}

}
