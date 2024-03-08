#pragma once
#include <MBLSP/MBLSP.h>
#include "LSP.h"
#include "ColorConf.h"
#include "Coloring.h"

namespace MBDoc
{
    struct ProcessedRegexColoring
    {
        std::unordered_map<ColorTypeIndex,std::vector<std::regex>> Regexes;
    };
    struct ProcessedLanguageColorConfig
    {
        std::string LSP;       
        ProcessedRegexColoring RegexColoring;
    };
    struct ProcessedColorInfo
    {
        TextColor DefaultColor;      
        std::unordered_map<std::string,ColorTypeIndex> ColoringNameToIndex;
        std::vector<TextColor> ColorMap;
    };
    struct ProcessedColorConfiguration
    {
        ProcessedColorInfo ColorInfo; 
        std::unordered_map<std::string,ProcessedLanguageColorConfig> LanguageConfigs;
    };
    std::unique_ptr<MBLSP::LSP_Client> StartLSPServer(LSPServer const& ServerToStart,MBParsing::JSONObject const& RawInitRequest);
    std::unique_ptr<MBLSP::LSP_Client> StartLSPServer(LSPServer const& ServerToStart,MBLSP::InitializeRequest const& InitReq);
    std::unique_ptr<MBLSP::LSP_Client> StartLSPServer(LSPServer const& ServerToStart);
    
    LSPInfo LoadLSPConfig(std::filesystem::path const& PathToParse);
    LSPInfo LoadLSPConfig();
    LSPInfo LoadLSPConfigIfExists();

    ProcessedColorConfiguration LoadColorConfig(std::filesystem::path const& Path);
    ProcessedColorConfiguration LoadColorConfig();
    ProcessedColorConfiguration ProcessColorConfig(ColorConfiguration const& Config);

    std::vector<Coloring> ConvertColoring(std::vector<int> const& LSPColoring,MBLSP::LineIndex const& Index);
    std::vector<int> ConvertColoring(std::vector<Coloring> const& MBDColorings,MBLSP::LineIndex const& Index);
    std::vector<Coloring> GetRegexColorings(std::vector<Coloring> const& PreviousColorings,
                ProcessedRegexColoring const& RegexesToUse,
                std::string const& DocumentContent);
    std::vector<Coloring> CombineColorings(std::vector<Coloring> const& LSPColoring,std::vector<Coloring> const& RegexColoring);
    std::vector<Coloring> RemoveDuplicates(std::vector<Coloring> const& PrioColoring,std::vector<Coloring> const& ColoringsToPrune);
    std::vector<Coloring> RemoveDuplicates(std::vector<Coloring> const& ColoringsToPrune);
}
