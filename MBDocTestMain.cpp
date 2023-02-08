#include "MBDoc.h"
#include <iostream>

std::vector<std::string> TestLinkResolving()
{
    std::vector<std::string> ReturnValue;
    MBError ParseError;
    MBDoc::DocumentBuild LinkResolveBuild = MBDoc::DocumentBuild::ParseDocumentBuild("LinkResolution/LinkResolve.json",ParseError);
    if(!ParseError)
    {
        ReturnValue.push_back("Error in testing link resolving: Error parsing LinkResolve.json: "+ParseError.ErrorMessage);
        return(ReturnValue);
    }
    MBDoc::DocumentFilesystem LinkResolveFilesystem;
    //ParseError = MBDoc::DocumentFilesystem::CreateDocumentFilesystem(LinkResolveBuild,LinkResolveFilesystem);
   
    //Current Dir and PathToResolve
    std::vector<std::pair<std::string,std::string>> TruePairs = 
    {
        {"/Top2.mbd","Top1.mbd"},
        {"/Top2.mbd","Top1.mbd#Top1 Top"},
        {"/Top2.mbd","Top1.mbd#Top1 Sub"},
        {"/Top2.mbd","Top1.mbd#Top1 SubSub"},
        {"/Top2.mbd","#Top2 Top"},
        {"/Top2.mbd","#Top2 Sub"},
        {"/Top2.mbd","#Top2 SubSub"},
    };
    std::vector<std::pair<std::string,std::string>> FalsePairs
    {
        {"/Top2.mbd","Top3.mbd"},   
    };
    for(auto const& Pair : TruePairs)
    {
        ParseError = MBError(true);
        MBDoc::DocumentPath DocPath = MBDoc::DocumentPath::ParsePath(Pair.first,ParseError);
        if(!ParseError)
        {
            ReturnValue.push_back("Error parsing path \""+Pair.first+"\": "+ParseError.ErrorMessage);
            ParseError = MBError(true);
            continue;
        }
        LinkResolveFilesystem.ResolveReference(DocPath,Pair.second,ParseError);
        if(!ParseError)
        {
            ReturnValue.push_back("Error resolving path \""+Pair.second+"\" in document \""+Pair.first+"\": "+ParseError.ErrorMessage);
            ParseError = MBError(true);
        }
    }
    for(auto const& Pair : FalsePairs)
    {
        ParseError = MBError(true);
        MBDoc::DocumentPath DocPath = MBDoc::DocumentPath::ParsePath(Pair.first,ParseError);
        if(!ParseError)
        {
            ReturnValue.push_back("Error parsing path \""+Pair.first+"\": "+ParseError.ErrorMessage);
            ParseError = MBError(true);
            continue;
        }
        LinkResolveFilesystem.ResolveReference(DocPath,Pair.second,ParseError);
        if(ParseError)
        {
            ReturnValue.push_back("Incorrect path  \""+Pair.second+"\" resolved in document \""+Pair.first+"\": "+ParseError.ErrorMessage);
            ParseError = MBError(true);
        }
    }
    return(ReturnValue);
}
//Assumes that it's run in the Test subdirectory
int main(int argc,const char** argv)
{
    bool ErrorOccured = false;
    std::vector<std::vector<std::string>(*)()> Tests = {TestLinkResolving};
    for(auto const& Test : Tests)
    {
        std::vector<std::string> NewErrors = Test();
        for(std::string& Error : NewErrors)
        {
            ErrorOccured = true;
            std::cout<<Error<<"\n";
        }
    }
    std::cout.flush();
    if(ErrorOccured)
    {
        return(1);   
    }
    return(0); 
}
