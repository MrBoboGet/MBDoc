#include <iostream>
#include "MBDoc.h"
int main()
{
    MBDoc::DocumentSource Document;
    MBDoc::DocumentParsingContext Parser;
    std::filesystem::current_path(std::filesystem::current_path().parent_path().parent_path());
    std::cout<<std::filesystem::current_path()<<std::endl;
    //Document = Parser.ParseSource(std::filesystem::path("Docs/index.mbd"));
    Document = Parser.ParseSource(std::filesystem::path("Docs/Format.mbd"));
    //Document = Parser.ParseSource(std::filesystem::path("Test.mbd"));
    //MBDoc::MarkdownCompiler Compiler;
    MBDoc::HTTPCompiler Compiler;
    std::vector<MBDoc::DocumentSource> Sources;
    Sources.push_back(std::move(Document));
    Compiler.Compile(Sources);
}
