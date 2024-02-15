#include "MBDocCLI.h"
#include <MBCLI/MBCLI.h>
#include "ColorConf.h"
#include "MBDoc.h"

#include <iostream>
#include <memory>
#include <unordered_set>

#include <cstring>

#include "Compilers/RawTerminal.h"
#include "Compilers/HTML.h"
#include "Compilers/Markdown.h"
#include "MBUnicode/MBUnicode.h"

#include <MBSystem/MBSystem.h>
#include <MBUtility/MBFiles.h>
namespace MBDoc
{
    bool DocCLI::p_VerifyArguments(MBCLI::ArgumentListCLIInput const& ArgumentsToVerfiy)
    {
        bool ReturnValue = true;
        std::unordered_set<std::string> ValidFlags = { "check-references","help"};
        for(auto const& Option : ArgumentsToVerfiy.CommandOptions)
        {
            if(ValidFlags.find(Option.first) == ValidFlags.end())
            {
                if(ReturnValue == true)
                {
                    m_AssociatedTerminal.Print("Invalid options found: ");
                    ReturnValue = false;
                }
                m_AssociatedTerminal.Print("--"+Option.first + " ");
            }   
        }
        std::unordered_set<std::string> ValidValueOptinos = { "f","o","s" };
        for(auto const& Option : ArgumentsToVerfiy.CommandArgumentOptions)
        {
            if(ValidValueOptinos.find(Option.first) == ValidValueOptinos.end())
            {
                if(ReturnValue == true)
                {
                    m_AssociatedTerminal.Print("Invalid options found: "); 
                    ReturnValue = false;
                }
                m_AssociatedTerminal.Print("-"+Option.first + " ");
            }   
        }
        if(!ReturnValue)
        {
            m_AssociatedTerminal.Print("\nTerminating");   
        }
        return(ReturnValue);
    }
    std::vector<MBCLI::ArgumentListCLIInput> DocCLI::p_GetInputs(const char** argv,int argc)
    {
        std::vector<MBCLI::ArgumentListCLIInput> ReturnValue;    
        std::vector<size_t> DelimiterPositions;
        for(size_t i = 0; i < argc;i++)
        {
            if(std::strcmp(argv[i],"--") == 0)
            {
                DelimiterPositions.push_back(i);   
            }
        }
        DelimiterPositions.push_back(argc);   
        size_t ArgvBegin = 0;
        for(size_t i = 0; i < DelimiterPositions.size();i++)
        {
            ReturnValue.push_back(MBCLI::ArgumentListCLIInput(DelimiterPositions[i]-ArgvBegin,argv+ArgvBegin));
            ArgvBegin = DelimiterPositions[i];
        }
        return(ReturnValue);
    }
    DocumentBuild DocCLI::p_GetBuild(MBCLI::ArgumentListCLIInput const& CommandInput)
    {
        DocumentBuild ReturnValue;           
        std::string BuildName = ""; 
        std::vector<std::string> BuildFiles;
        for(std::string const& Argument : CommandInput.CommandArguments)
        {
            size_t DotPosition = Argument.find_last_of('.');   
            if(DotPosition == Argument.npos)
            {
                BuildFiles.push_back(Argument); 
                continue;
            }
            if(DotPosition+4 < Argument.size() && std::memcmp(Argument.data()+DotPosition+1,"json",4) == 0)
            {
                if(BuildFiles.size() > 0 || BuildName != "")
                {
                    m_AssociatedTerminal.PrintLine("Can only specify either 1 build json file or multiple mbd sources");   
                    std::exit(1);
                }
                BuildName = Argument;
                continue;
            }
            BuildFiles.push_back(Argument); 
        }
        MBError ParseResult = true;
        if(BuildName != "")
        {
            ReturnValue = DocumentBuild::ParseDocumentBuild(BuildName,ParseResult);
            if(!ParseResult)
            {
                m_AssociatedTerminal.PrintLine("Error parsing json build specification: "+ParseResult.ErrorMessage);   
                std::exit(1);
            }
        }
        else if(BuildFiles.size() > 0)
        {
            for(std::string& Source : BuildFiles)
            {
                //BuildToCompile.BuildFiles.push_back(std::move(Source));
                ReturnValue.DirectoryFiles.push_back(Source);
            }
        }
        return(ReturnValue);
    }
    DocumentFilesystem DocCLI::p_GetFilesystem(DocumentBuild const& Build,std::filesystem::path const& OldBuildPath ,MBCLI::ArgumentListCLIInput const& CommandInput,std::vector<IndexType>& UpdatedFiles)
    {
        DocumentFilesystem ReturnValue; 
        MBError ParseResult = true;
        if(!std::filesystem::exists(OldBuildPath))
        {
            ParseResult = DocumentFilesystem::CreateDocumentFilesystem(Build,m_LSPConf,m_ColorConfiguration,ReturnValue);
        }
        else
        {
            MBError ParseError = false;
            MBParsing::JSONObject OldBuildInfo = MBParsing::ParseJSONObject(OldBuildPath,&ParseError);
            if(!ParseError)
            {
                throw std::runtime_error("Error parsing BuildInfo.json: "+ParseError.ErrorMessage);
            }
            DocumentFilesystem OldBuild;
            FromJSON(OldBuild,OldBuildInfo);
            ParseResult = DocumentFilesystem::CreateDocumentFilesystem(Build,m_LSPConf,m_ColorConfiguration,ReturnValue,false);
            ParseResult = DocumentFilesystem::CreateDocumentFilesystem(Build,m_LSPConf,m_ColorConfiguration,ReturnValue,OldBuild,UpdatedFiles);
        }
        if(!ParseResult)
        {
            m_AssociatedTerminal.PrintLine("Error creating build document filesystem: "+ParseResult.ErrorMessage);   
            std::exit(1);
        }
        return(ReturnValue);
    }
    std::unique_ptr<DocumentCompiler> DocCLI::p_GetCompiler(MBCLI::ArgumentListCLIInput const& CommandInput)
    {
        std::unique_ptr<DocumentCompiler> ReturnValue;
        std::string FormatString; 
        std::string SemanticString;
        auto FormatOptions = CommandInput.CommandArgumentOptions.find("f");
        if(FormatOptions == CommandInput.CommandArgumentOptions.end())
        {
            m_AssociatedTerminal.PrintLine("A output format need to be specified for compilation, terminating");   
            std::exit(1);
        }
        if(FormatOptions->second.size() > 1)
        {
            m_AssociatedTerminal.PrintLine("Only one format can be specified, terminating");   
            std::exit(1);
        }
        FormatString = FormatOptions->second[0];

        auto SemanticOptions = CommandInput.CommandArgumentOptions.find("s");
        if(SemanticOptions != CommandInput.CommandArgumentOptions.end())
        {
            if(SemanticOptions->second.size() > 1)
            {
                m_AssociatedTerminal.PrintLine("Only one semantic can be specified, terminating");   
                std::exit(1);
            }
            SemanticString = SemanticOptions->second[0];
        }

        if(FormatString == "html")
        {
            ReturnValue = std::unique_ptr<DocumentCompiler>(new HTMLCompiler());
        }
        else if(FormatString == "md")
        {
            ReturnValue = std::make_unique<MarkdownCompiler>();
        }
        //else if(FormatString == "md")
        //{
        //    ReturnValue = std::unique_ptr<DocumentCompiler>(new MarkdownCompiler());   
        //}
        //else if(FormatString == "rawterminal")
        //{
        //    ReturnValue = std::unique_ptr<DocumentCompiler>(new RawTerminalCompiler());
        //}
        if(ReturnValue == nullptr)
        {
            m_AssociatedTerminal.PrintLine("Failed to find suitable compiler, terminating");  
            std::exit(1);
        } 
        return(ReturnValue); 
    }
    
    CommonCompilationOptions DocCLI::p_GetOptions(std::vector<MBCLI::ArgumentListCLIInput> const& CommandInputs)
    {
        CommonCompilationOptions ReturnValue;    
        auto OutputOption = CommandInputs[0].CommandArgumentOptions.find("o");
        if(OutputOption == CommandInputs[0].CommandArgumentOptions.end())
        {
            m_AssociatedTerminal.PrintLine("Compilation requires a output directory");  
            std::exit(1);
        }
        if(OutputOption->second.size() > 1)
        {
            m_AssociatedTerminal.PrintLine("Can only specify one output directory"); 
            std::exit(1);
        }
        ReturnValue.OutputDirectory = OutputOption->second[0];
        if(CommandInputs.size() > 1)
        {
            ReturnValue.CompilerSpecificOptions = CommandInputs[1];   
        }
        return(ReturnValue);
    }
    void DocCLI::p_Help(MBCLI::ArgumentListCLIInput const& Input)
    {
        std::unordered_map<std::string, const char*> HelpIndex =
//#include "TempOut/HelpInclude.i"
//            ;
        {{"change me",nullptr}};
        if (Input.CommandArguments.size() > 1)  
        {
            m_AssociatedTerminal.PrintLine("Can only help with one thing at a time");
            std::exit(1);
        }
        else if (Input.CommandArguments.size() == 0)
        {
            m_AssociatedTerminal.PrintLine(HelpIndex["CLI.mbd"]);
        }
        else if (HelpIndex.find(Input.CommandArguments[0]) != HelpIndex.end())
        {
            m_AssociatedTerminal.PrintLine(HelpIndex[Input.CommandArguments[0]]);
        }
        else
        {
            m_AssociatedTerminal.PrintLine("No help available :(");
            std::exit(1);
        }

    }
    LSPInfo DocCLI::p_ParseLSPConfig(std::filesystem::path const& PathToParse)
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
    ProcessedColorConfiguration DocCLI::p_ParseColorConfiguration(std::filesystem::path const& PathToParse)
    {
        ProcessedColorConfiguration ReturnValue;
        std::string Data = MBUtility::ReadWholeFile(MBUnicode::PathToUTF8(PathToParse));
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
    int DocCLI::Run(const char** argv,int argc)
    {
        try 
        {
            std::filesystem::path HomeDir = MBSystem::GetUserHomeDirectory()/".mbdoc";
            if(std::filesystem::is_regular_file(HomeDir/"LSPConfig.json"))
            {
                m_LSPConf = p_ParseLSPConfig(HomeDir/"LSPConfig.json");
            }
            if(std::filesystem::is_regular_file(HomeDir/"ColorConfig.json"))
            {
                m_ColorConfiguration = p_ParseColorConfiguration(HomeDir/"ColorConfig.json");
            }

            std::vector<MBCLI::ArgumentListCLIInput> CommandInputs = p_GetInputs(argv, argc);
            if (!p_VerifyArguments(CommandInputs[0]))
            {
                return(1);
            }
            if (CommandInputs[0].CommandOptions.find("help") != CommandInputs[0].CommandOptions.end())
            {
                p_Help(CommandInputs[0]);
                return(0);
            }
            CommonCompilationOptions CompileOptions = p_GetOptions(CommandInputs);
            std::unique_ptr<DocumentCompiler> CompilerToUse = p_GetCompiler(CommandInputs[0]);
            DocumentBuild BuildToCompile = p_GetBuild(CommandInputs[0]);
            std::filesystem::path OutDir = CompileOptions.OutputDirectory;
            std::filesystem::path BuildInfoPath = OutDir/"BuildInfo.json";
            bool UsingBuildInfo = false;
            if(std::filesystem::exists(BuildInfoPath))
            {
                UsingBuildInfo = true;
            }
            std::vector<IndexType> UpdatedFiles;
            DocumentFilesystem BuildFilesystem = p_GetFilesystem(BuildToCompile, BuildInfoPath, CommandInputs[0],UpdatedFiles);
            std::unordered_set<bool> FileIsUpdated = std::unordered_set<bool>(UpdatedFiles.begin(),UpdatedFiles.end());
            if (CommandInputs[0].CommandOptions.find("check-references") != CommandInputs[0].CommandOptions.end())
            {
                auto Iterator = BuildFilesystem.begin();
                while (!Iterator.HasEnded())
                {
                    if (!Iterator.EntryIsDirectory())
                    {
                        MBDoc::DocumentPath CurrentPath = Iterator.GetCurrentPath();
                        MBDoc::DocumentSource const& CurrentSource = Iterator.GetDocumentInfo();
                        //for (std::string const& Reference : CurrentSource.References)
                        //{
                        //    MBError ReferenceResult = true;
                        //    BuildFilesystem.ResolveReference(CurrentPath, Reference, ReferenceResult);
                        //    if (!ReferenceResult)
                        //    {
                        //        std::cout << "Error resolving reference " + Reference + " in file " + CurrentPath.GetString() << std::endl;
                        //    }
                        //}
                    }
                    Iterator++;
                }
                return(0);
            }
            else
            {
                CompilerToUse->AddOptions(CompileOptions);
                CompilerToUse->PeekDocumentFilesystem(BuildFilesystem);
                auto FilesystemIterator = BuildFilesystem.begin();
                while(!FilesystemIterator.HasEnded())
                {
                    if(!FilesystemIterator.EntryIsDirectory() && 
                            (!UsingBuildInfo || FileIsUpdated.find(FilesystemIterator.GetFileIndex()) != FileIsUpdated.end()))
                    {
                        DocumentPath CurrentPath = FilesystemIterator.GetCurrentPath();
                        CompilerToUse->CompileDocument(CurrentPath,FilesystemIterator.GetDocumentInfo());
                    } 
                    FilesystemIterator++;        
                }
                if(BuildInfoPath != "")
                {
                    std::ofstream OutFile(BuildInfoPath);
                    OutFile<<ToJSON(BuildFilesystem).ToString();;
                }
                //CompilerToUse->Compile(BuildFilesystem, CompileOptions);
            }
            return(0);
        }
        catch (std::exception const& e)
        {
            std::cout << e.what() << std::endl;
            std::exit(1);
        }
        std::exit(0);
    }      
}


