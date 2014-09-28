//===--- CVQualifiersOrder.cpp - clang-tidy -------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "QualifiersOrder.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace ast_matchers {
AST_MATCHER(QualType, isVolatileQualified) {
  return Node.isVolatileQualified();
}
} // namespace ast_matchers

namespace tidy {

void QualifiersOrder::registerMatchers(MatchFinder *Finder) {
  //Finder->addMatcher(qualType(isConstQualified()).bind("const"), this);
  //Finder->addMatcher(qualType(isVolatileQualified()).bind("volatile"), this);
  //Finder->addMatcher(qualType(isConstQualified(), isVolatileQualified())
  //                       .bind("const volatile"),
  //                   this);
  Finder->addMatcher(varDecl().bind("var"), this);
  // TODO(mkurdej): declarations in if/for/while: ?Stmt()
  // TODO(mkurdej): function/method arguments: functionDecl
}

void QualifiersOrder::check(const MatchFinder::MatchResult &Result) {
  const VarDecl *Var = Result.Nodes.getStmtAs<VarDecl>("var");
  const SourceManager &SM = *Result.SourceManager;

  assert(Var != nullptr);
  if (!Var) {
    llvm_unreachable("Expected valid variable declaration");
  }
  QualType VarType = Var->getType();
  //QualType CanonicalVarType = VarType.getCanonicalType();
  // FIXME(mkurdej): only top-level const qualifier is considered
  bool LocalConstQualified = VarType.isLocalConstQualified();
  //TODO(mkurdej): restrict, volatile
  //bool LocalVolatileQualified = VarType.isLocalVolatileQualified();
  //if (!VarType.hasQualifiers()) {
  if (!LocalConstQualified) {
    // Nothing to do.
    return;
  }

  //SourceType = SourceType.getNonReferenceType();

  //DiagnosticBuilder Diag = diag(
  //    Method->getLocation(),
  //    OnlyVirtualSpecified
  //        ? "Prefer using 'override' or (rarely) 'final' instead of 'virtual'"
  //        : "Annotate this function with 'override' or (rarely) 'final'");

  //CharSourceRange FileRange = Lexer::makeFileCharRange(
  //    CharSourceRange::getTokenRange(Method->getSourceRange()), Sources,
  //    Result.Context->getLangOpts());

  //if (!FileRange.isValid())
  //  return;

  //// FIXME: Instead of re-lexing and looking for specific macros such as
  //// 'ABSTRACT', properly store the location of 'virtual' and '= 0' in each
  //// FunctionDecl.
  //SmallVector<Token, 16> Tokens = ParseTokens(FileRange, Sources,
  //                                            Result.Context->getLangOpts());

  //// Add 'override' on inline declarations that don't already have it.
  //if (!HasFinal && !HasOverride) {
  //  SourceLocation InsertLoc;
  //  StringRef ReplacementText = "override ";

  //  if (Method->hasAttrs()) {
  //    for (const clang::Attr *A : Method->getAttrs()) {
  //      if (!A->isImplicit()) {
  //        InsertLoc = Sources.getExpansionLoc(A->getLocation());
  //        break;
  //      }
  //    }
  //  }

  //  if (InsertLoc.isInvalid() && Method->doesThisDeclarationHaveABody() &&
  //      Method->getBody() && !Method->isDefaulted())
  //    InsertLoc = Method->getBody()->getLocStart();

  //  if (!InsertLoc.isValid()) {
  //    if (Tokens.size() > 2 && GetText(Tokens.back(), Sources) == "0" &&
  //        GetText(Tokens[Tokens.size() - 2], Sources) == "=") {
  //      InsertLoc = Tokens[Tokens.size() - 2].getLocation();
  //    } else if (GetText(Tokens.back(), Sources) == "ABSTRACT") {
  //      InsertLoc = Tokens.back().getLocation();
  //    }
  //  }

  //  if (!InsertLoc.isValid()) {
  //    InsertLoc = FileRange.getEnd();
  //    ReplacementText = " override";
  //  }
  //  Diag << FixItHint::CreateInsertion(InsertLoc, ReplacementText);
  //}

  //if (HasFinal && HasOverride) {
  //  SourceLocation OverrideLoc = Method->getAttr<OverrideAttr>()->getLocation();
  //  Diag << FixItHint::CreateRemoval(
  //      CharSourceRange::getTokenRange(OverrideLoc, OverrideLoc));
  //}

  //if (Method->isVirtualAsWritten()) {
  //  for (Token Tok : Tokens) {
  //    if (Tok.is(tok::raw_identifier) && GetText(Tok, Sources) == "virtual") {
  //      Diag << FixItHint::CreateRemoval(CharSourceRange::getTokenRange(
  //          Tok.getLocation(), Tok.getLocation()));
  //      break;
  //    }
  //  }
  //}
}

} // namespace tidy
} // namespace clang
