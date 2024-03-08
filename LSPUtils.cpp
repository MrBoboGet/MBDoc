#include "LSPUtils.h"
#include <MBUtility/MBFiles.h>
#include <MBUnicode/MBUnicode.h>
#include <MBSystem/MBSystem.h>
#include <MBSystem/BiDirectionalSubProcess.h>
#include <MBUtility/InterfaceAdaptors.h>
#include <MBUtility/MBStrings.h>
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
    std::unique_ptr<MBLSP::LSP_Client> StartLSPServer(LSPServer const& ServerToStart,MBParsing::JSONObject const& RawInitRequest)
    {
        std::unique_ptr<MBLSP::LSP_Client> ReturnValue;
        std::unique_ptr<MBSystem::BiDirectionalSubProcess> SubProcess = 
            std::make_unique<MBSystem::BiDirectionalSubProcess>(ServerToStart.CommandName,ServerToStart.CommandArguments);
        std::unique_ptr<MBUtility::IndeterminateInputStream> InputStream = std::make_unique<MBUtility::NonOwningIndeterminateInputStream>(SubProcess.get());
        ReturnValue = std::make_unique<MBLSP::LSP_Client>(std::move(InputStream),std::move(SubProcess));
        ReturnValue->InitializeServer(RawInitRequest);
        return ReturnValue;
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
    static TextColor h_ParseTextColor(std::string const& TextString)
    {
        TextColor ReturnValue;    
        if(TextString.size() != 6)
        {
            throw std::runtime_error("Error parsing color: unrecognized color \""+TextString+"\"");
        }
        bool Result = true;
        char Data[3];
        for(int i = 0; i < 3;i++)
        {
            Data[i] = MBUtility::HexValueToByte(TextString[i*2],TextString[i*2+1],&Result);
            if(!Result)
            {
                throw std::runtime_error("Error parsing color: unrecognized color \""+TextString+"\"");
            }
        }
        ReturnValue = TextColor(Data[0],Data[1],Data[2]);
        return(ReturnValue);
    }
    ProcessedColorConfiguration LoadColorConfig(std::filesystem::path const& Path)
    {
        ProcessedColorConfiguration ReturnValue;
        std::string Data = MBUtility::ReadWholeFile(MBUnicode::PathToUTF8(Path));
        MBError ParseResult = true;
        MBParsing::JSONObject JSONData = MBParsing::ParseJSONObject(Data,0,nullptr,&ParseResult);
        if(!ParseResult)
        {
            throw std::runtime_error("Error parsing ColorConfig.json as json file: "+ParseResult.ErrorMessage);
        }
        try
        {
            ColorConfiguration DefaultConf;
            DefaultConf.FillObject(JSONData);
            ReturnValue = ProcessColorConfig(DefaultConf);
        }
        catch(std::exception const& e)
        {
            throw std::runtime_error("Error parsing LSPConf.json: "+std::string(e.what())); 
        }
        return(ReturnValue);
    }
    ProcessedColorConfiguration LoadColorConfig()
    {
        return LoadColorConfig(MBSystem::GetUserHomeDirectory()/".mbdoc"/"ColorConfig.json");
    }
    ProcessedColorConfiguration ProcessColorConfig(ColorConfiguration const& Config)
    {
        ProcessedColorConfiguration ReturnValue;
        ReturnValue.ColorInfo.DefaultColor = h_ParseTextColor(Config.Coloring.DefaultColor); 
        ReturnValue.ColorInfo.ColoringNameToIndex["Default"] = 0;
        ReturnValue.ColorInfo.ColorMap.push_back(ReturnValue.ColorInfo.DefaultColor);
        for(auto const& LangConf : Config.LanguageColorings)
        {
            ProcessedLanguageColorConfig NewConf;
            NewConf.LSP = LangConf.second.LSP;
            for(auto const& RegexList : LangConf.second.Regexes)
            {
                ColorTypeIndex& RegexColorType = ReturnValue.ColorInfo.ColoringNameToIndex[RegexList.first]; 
                if(RegexColorType == 0)
                {
                    RegexColorType = ReturnValue.ColorInfo.ColorMap.size();
                    ReturnValue.ColorInfo.ColorMap.push_back(TextColor());
                }
                auto& Regexes = NewConf.RegexColoring.Regexes[RegexColorType];
                for(auto const& Regex : RegexList.second)
                {
                    Regexes.push_back(std::regex(Regex,std::regex_constants::ECMAScript|std::regex_constants::nosubs));
                }
            }
            ReturnValue.LanguageConfigs[LangConf.first] = std::move(NewConf);
        }
        for(auto const& Color : Config.Coloring.ColorMap)
        {
            ColorTypeIndex& Index = ReturnValue.ColorInfo.ColoringNameToIndex[Color.first];
            if(Index == 0)
            {
                Index = ReturnValue.ColorInfo.ColorMap.size();   
                ReturnValue.ColorInfo.ColorMap.push_back(TextColor());
            }
            ReturnValue.ColorInfo.ColorMap[Index] = h_ParseTextColor(Color.second);
        }
        return(ReturnValue);
    }
    std::vector<Coloring> ConvertColoring(std::vector<int> const& LSPColoring,MBLSP::LineIndex const& Index)
    {
        std::vector<Coloring> ReturnValue;
        if(LSPColoring.size() % 5 != 0)
        {
            throw std::runtime_error("LSP coloring integer count not divisible by 5");   
        }
        int Offset = 0;
        int CurrentLine = 0;
        int CurrentCharacter = 0;
        while(Offset < LSPColoring.size())
        {
            Coloring& NewColoring = ReturnValue.emplace_back();
            CurrentLine += LSPColoring[Offset];
            if(LSPColoring[Offset] > 0)
            {
                CurrentCharacter = LSPColoring[Offset+1];   
            }
            else
            {
                CurrentCharacter += LSPColoring[Offset+1];   
            }
            NewColoring.ByteOffset = Index.PositionToByteOffset(CurrentLine,CurrentCharacter);
            NewColoring.Length = LSPColoring[Offset+2];
            NewColoring.Color = LSPColoring[Offset+3];
            Offset += 5;
        }
        std::sort(ReturnValue.begin(),ReturnValue.end());
        return ReturnValue;
    }
    std::vector<int> ConvertColoring(std::vector<Coloring> const& MBDColorings,MBLSP::LineIndex const& Index)
    {
        std::vector<int> ReturnValue;
        int PreviousLine = 0;
        int PreviousCharacter = 0;
        for(auto const& Coloring : MBDColorings)
        {
            auto Position = Index.ByteOffsetToPosition(Coloring.ByteOffset);
            ReturnValue.push_back(Position.line-PreviousLine);
            if(Position.line != PreviousLine)
            {
                ReturnValue.push_back(Position.character);   
            }
            else
            {
                ReturnValue.push_back(Position.character - PreviousCharacter);
            }
            PreviousLine = Position.line;
            PreviousCharacter = Position.character;

            ReturnValue.push_back(Coloring.Length);
            ReturnValue.push_back(Coloring.Color);
            //lossy conversion...
            ReturnValue.push_back(0);
        }
        return ReturnValue;
    }
    std::vector<Coloring> GetRegexColorings(std::vector<Coloring> const& PreviousColorings,
            ProcessedRegexColoring const& RegexesToUse,
            std::string const& DocumentContent)
    {
        std::vector<Coloring> ReturnValue;
        size_t ParseOffset = 0;
        size_t PreviousColorIndex = 0;
        while(ParseOffset < DocumentContent.size())
        {
            size_t TextToInspectEnd = DocumentContent.size();
            if(PreviousColorIndex < PreviousColorings.size())
            {
                if(ParseOffset < PreviousColorings[PreviousColorIndex].ByteOffset)
                {
                    TextToInspectEnd = PreviousColorings[PreviousColorIndex].ByteOffset;
                }
                else if(ParseOffset >= PreviousColorings[PreviousColorIndex].ByteOffset && 
                        ParseOffset < PreviousColorings[PreviousColorIndex].ByteOffset+PreviousColorings[PreviousColorIndex].Length)
                {
                    //in previous coloring, skip
                    ParseOffset = PreviousColorings[PreviousColorIndex].ByteOffset+PreviousColorings[PreviousColorIndex].Length;
                    PreviousColorIndex++;
                    continue;
                }
                else
                {
                    PreviousColorIndex++;
                    continue;
                }
            }
            for(auto const& RegexCategory : RegexesToUse.Regexes)
            {
                ColorTypeIndex CurrentColor = RegexCategory.first;
                for(auto const& Regex : RegexCategory.second)
                {
                    std::sregex_iterator Begin = std::sregex_iterator(DocumentContent.begin()+ParseOffset,
                            DocumentContent.begin()+TextToInspectEnd, Regex);
                    std::sregex_iterator End;
                    while(Begin != End)
                    {
                        assert(Begin->size() == 1);
                        Coloring NewColoring;
                        NewColoring.ByteOffset = ParseOffset+Begin->position();
                        NewColoring.Color = CurrentColor;
                        NewColoring.Length = Begin->length();
                        ReturnValue.push_back(NewColoring);
                        ++Begin;
                    }
                }
            }
            ParseOffset = TextToInspectEnd;
        }
        //maybe uneccessary
        std::sort(ReturnValue.begin(),ReturnValue.end());
        return(ReturnValue);
    }
    bool h_Overlaps(Coloring const& Lhs,Coloring const& Rhs)
    {
        size_t RhsLeftmost = Rhs.ByteOffset;
        size_t RhsRightmost = Rhs.ByteOffset+Rhs.Length;
        if(Lhs.ByteOffset >= RhsLeftmost && Lhs.ByteOffset <= RhsRightmost)
        {
            return(true);   
        }
        else if((Lhs.ByteOffset+Lhs.Length) >= RhsLeftmost && (Lhs.ByteOffset+Lhs.Length) <= RhsRightmost)
        {
            return(true);
        }
        return(false);
    }
    std::vector<Coloring> RemoveDuplicates(std::vector<Coloring> const& PrioColoring,std::vector<Coloring> const& ColoringsToPrune)
    {
        std::vector<Coloring> ReturnValue;
        ReturnValue.reserve(ColoringsToPrune.size());
        size_t PrioColoringIndex = 0;
        size_t PruneIndex = 0;
        while(PrioColoringIndex < PrioColoring.size() && PruneIndex < ColoringsToPrune.size())
        {
            if(h_Overlaps(PrioColoring[PrioColoringIndex],ColoringsToPrune[PruneIndex]))
            {
                PruneIndex++;    
            }
            else if(PrioColoring[PrioColoringIndex].ByteOffset < ColoringsToPrune[PruneIndex].ByteOffset)
            {
                PrioColoringIndex++;   
            }
            else
            {
                ReturnValue.push_back(ColoringsToPrune[PruneIndex]);
                PruneIndex++;
            }
        }
        while(PruneIndex < ColoringsToPrune.size())
        {
            ReturnValue.push_back(ColoringsToPrune[PruneIndex]);   
            PruneIndex++;
        }
        return(ReturnValue);
    }
    std::vector<Coloring> RemoveDuplicates(std::vector<Coloring> const& ColoringsToPrune)
    {
        std::vector<Coloring> ReturnValue;
        ReturnValue.reserve(ColoringsToPrune.size());
        size_t ParseOffset = 0;
        size_t ColorIndex = 0;
        while(ColorIndex < ColoringsToPrune.size())
        {
            if(ParseOffset <= ColoringsToPrune[ColorIndex].ByteOffset)
            {
                ReturnValue.push_back(ColoringsToPrune[ColorIndex]);
                ParseOffset = ColoringsToPrune[ColorIndex].ByteOffset + ColoringsToPrune[ColorIndex].Length;
            }
            ColorIndex++;
        }
        return(ReturnValue);
    }
    std::vector<Coloring> CombineColorings(std::vector<Coloring> const& LSPColoring,std::vector<Coloring> const& RegexColoring)
    {
        std::vector<Coloring> ReturnValue;
        std::vector<Coloring> PrunedRegexColoring = RemoveDuplicates(LSPColoring,RegexColoring);
        ReturnValue.resize(PrunedRegexColoring.size()+LSPColoring.size());
        std::sort(PrunedRegexColoring.begin(),PrunedRegexColoring.end());
        std::merge(PrunedRegexColoring.begin(),PrunedRegexColoring.end(),LSPColoring.begin(),LSPColoring.end(),ReturnValue.begin());
        ReturnValue = RemoveDuplicates(ReturnValue); 
        return ReturnValue;
    }
}
