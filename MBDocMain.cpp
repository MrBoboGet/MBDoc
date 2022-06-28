#include <iostream>
#include "MBDoc.h"
#include <iostream>
int main()
{
    std::filesystem::current_path(std::filesystem::current_path().parent_path().parent_path());
    std::cout << std::filesystem::current_path() << std::endl;
    //MBDoc::DocumentSource Document;
    //MBDoc::DocumentParsingContext Parser;
    //std::cout<<std::filesystem::current_path()<<std::endl;
    //Document = Parser.ParseSource(std::filesystem::path("Docs/Format.mbd"));
    //MBDoc::HTTPCompiler Compiler;
    //std::vector<MBDoc::DocumentSource> Sources;
    //Sources.push_back(std::move(Document));
    //Compiler.Compile(Sources);
    MBError ParseError = true;
    MBDoc::DocumentBuild Build = MBDoc::DocumentBuild::ParseDocumentBuild("Docs/MBDocBuild.json", ParseError);
    if (!ParseError)
    {
        std::cout << "Error parsing build: " + ParseError.ErrorMessage;
        return(1);
    }
    MBDoc::DocumentFilesystem Filesystem;
    ParseError = MBDoc::DocumentFilesystem::CreateDocumentFilesystem(Build, &Filesystem);
    if (!ParseError)
    {
        std::cout << "Error creating filesystem: " + ParseError.ErrorMessage;
        return(-1);
    }
    MBDoc::DocumentPath Result = Filesystem.ResolveReference(MBDoc::DocumentPath::ParsePath("/CodeIndex.mbd",ParseError),"Format.mbd", ParseError);
    std::cout << Result.GetString() << std::endl;
    Result = Filesystem.ResolveReference(MBDoc::DocumentPath::ParsePath("/CodeIndex.mbd", ParseError), "{Format.mbd}", ParseError);
    std::cout << Result.GetString() << std::endl;
}
