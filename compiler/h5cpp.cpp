/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

// Declares clang::SyntaxOnlyAction.
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
// Declares llvm::cl::extrahelp.
#include <llvm/Support/CommandLine.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclTemplate.h>
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

#include <set>
#include <stack>
#include <algorithm>
#include <iostream>
#include <fstream>
//#include <h5cpp/macros.h>

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;

#include "producer.hpp"
//#include "producer_text.hpp"
#include "producer_h5.hpp"
#include "consumer.hpp"

/// variables
StatementMatcher h5templateMatcher = callExpr( allOf(
	hasDescendant( declRefExpr( to( varDecl().bind("variableDecl")  ) ) ),
	hasDescendant( declRefExpr( to(
		functionDecl( allOf(
			eachOf(
				hasName("h5::write"),  hasName("h5::create"),  hasName("h5::read"), hasName("h5::append"), // dataset
				hasName("h5::awrite"), hasName("h5::acreate"), hasName("h5::aread") // attributes
			),
			/* locate T template argument, and declarations of T
			 * h5::write<T>( ... ) 
			 */
			hasTemplateArgument(0,  refersToType( qualType( eachOf( // all inners must match qualType
				/* if 'T' := some_struct */
				hasDeclaration( cxxRecordDecl(isStruct()).bind("cxxRecordDecl")),
				/* if 'T' := std::container<some_struct>*/
				hasDeclaration( classTemplateSpecializationDecl(
					hasTemplateArgument(0,  refersToType( qualType( eachOf(
					// T:=std::container<some_struct>
					hasDeclaration(	cxxRecordDecl(isStruct()).bind("cxxRecordDecl")),
					// T:=std::vector<std::vector<some_struct>>   rugged array of structs
					hasDeclaration( classTemplateSpecializationDecl(
						hasTemplateArgument(0,  refersToType( qualType( 
						hasDeclaration(	cxxRecordDecl(isStruct()).bind("cxxRecordDecl"))))))
					)
					))/*.bind("TemplateArg")*/ ))/*End templSpecDecl */ ) ),
				hasDeclaration( cxxRecordDecl( isClass()  ).bind("classDecl")) )
			) /*.bind("TemplateArg")*/ )),
			isTemplateInstantiation()
	)) /*END functionDecl*/ )))
));

static llvm::cl::OptionCategory MyToolCategory("h5cpp options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

/* the actual program */
int main(int argc, const char **argv) {
	//FIXME: last argument is the generated file, this could be improved upon
	std::string arg( strdup( argv[argc-1]) );
	std::string path = arg.substr(2);
	argc --;

	std::cerr <<
		"H5CPP: Copyright (c) 2018     , VargaConsulting, Toronto,ON Canada\n"
	   	"LLVM : Copyright (c) 2003-2010, University of Illinois at Urbana-Champaign.\n"
	;

	CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
	ClangTool Tool(OptionsParser.getCompilations(),
				 OptionsParser.getSourcePathList());
	H5TemplateCallback<H5Producer> callback( path );
	MatchFinder Finder;
	Finder.addMatcher(h5templateMatcher, &callback );
	Tool.setDiagnosticConsumer( new IgnoringDiagConsumer() );
	//TODO: 
	// in first pass h5::operators trip, as template specializations are not yet generated
	// for now the entire diagnostic messages are disabled and error diagnostics left for final
	// compile phase. 
	return  Tool.run( newFrontendActionFactory (&Finder).get());
}

