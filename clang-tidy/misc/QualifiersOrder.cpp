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
namespace {

tok::TokenKind getTokenKind(SourceLocation Loc, const SourceManager &SM,
                            const ASTContext *Context) {
  Token Tok;
  SourceLocation Beginning =
      Lexer::GetBeginningOfToken(Loc, SM, Context->getLangOpts());
  const bool Invalid =
      Lexer::getRawToken(Beginning, Tok, SM, Context->getLangOpts());
  assert(!Invalid && "Expected a valid token.");

  if (Invalid) {
    return tok::NUM_TOKENS;
  }
  return Tok.getKind();
}

SourceLocation
backwardSkipWhitespacesAndComments(const SourceManager &SM,
                                   const clang::ASTContext *Context,
                                   SourceLocation Loc) {
  for (;;) {
    do {
      Loc = Loc.getLocWithOffset(-1);
    } while (isWhitespace(*FullSourceLoc(Loc, SM).getCharacterData()));

    tok::TokenKind TokKind = getTokenKind(Loc, SM, Context);
    if (TokKind == tok::NUM_TOKENS || TokKind != tok::comment) {
      return Loc.getLocWithOffset(1);
    }
    // fast-backward current token
    SourceLocation Beginning =
        Lexer::GetBeginningOfToken(Loc, SM, Context->getLangOpts());
    Loc = Beginning;
  }
}

SourceLocation
forwardSkipWhitespacesAndComments(const SourceManager &SM,
                                  const clang::ASTContext *Context,
                                  SourceLocation Loc) {
  for (;;) {
    while (isWhitespace(*FullSourceLoc(Loc, SM).getCharacterData())) {
      Loc = Loc.getLocWithOffset(1);
    }

    tok::TokenKind TokKind = getTokenKind(Loc, SM, Context);
    if (TokKind == tok::NUM_TOKENS || TokKind != tok::comment) {
      return Loc;
    }
    // fast-forward current token
    Loc = Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
  }
}

StringRef getAsString(const SourceManager &SM, const clang::ASTContext *Context,
                      SourceRange R) {
  if (R.getBegin().isMacroID() ||
      !SM.isWrittenInSameFile(R.getBegin(), R.getEnd()))
    return StringRef();

  const char *Begin = SM.getCharacterData(R.getBegin());
  const char *End = SM.getCharacterData(
      Lexer::getLocForEndOfToken(R.getEnd(), 0, SM, Context->getLangOpts()));

  return StringRef(Begin, End - Begin);
}

SourceRange findToken(const SourceManager &SM, const clang::ASTContext *Context,
                      SourceRange SR, StringRef Text) {
  for (SourceLocation Loc = SR.getBegin(); Loc < SR.getEnd();) {
    // FIXME(mkurdej): Loc can actually be past SR.getEnd()
    while (isWhitespace(*FullSourceLoc(Loc, SM).getCharacterData())) {
      Loc = Loc.getLocWithOffset(1);
    }

    SourceLocation EndLoc =
        Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
    StringRef TokenText = getAsString(SM, Context, SourceRange(Loc, EndLoc));
    if (TokenText == Text) {
      return SourceRange(Loc, EndLoc);
    }
    // fast-forward current token
    Loc = Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
  }
  // Not found token of this kind in the given range
  return SourceRange();
}

} // end anonymous namespace

QualifiersOrder::QualifiersOrder(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context) {
  StringRef QualifierAlignmentString = Options.get("QualifierAlignment", "Left");
  QualifierAlignment =
      QualifierAlignmentString.equals_lower("right") ? QAS_Right : QAS_Left;
}

void QualifiersOrder::registerMatchers(MatchFinder *Finder) {
  // Finder->addMatcher(qualType(isConstQualified()).bind("const"), this);
  // Finder->addMatcher(qualType(isVolatileQualified()).bind("volatile"), this);
  // Finder->addMatcher(qualType(isConstQualified(), isVolatileQualified())
  //                       .bind("const volatile"),
  //                   this);
  Finder->addMatcher(varDecl().bind("var"), this);
  // TODO(mkurdej): declarations in if/for/while: ?Stmt()
  // TODO(mkurdej): function/method arguments: functionDecl
  // TODO(mkurdej): typedefs typedefType()
}

void QualifiersOrder::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "QualifierAlignment",
                QualifierAlignment == QAS_Right ? "Right" : "Left");
  // TODO: QualifierOrder: CRV|CVR|RCV|RVC|VCR|VRC
}

void QualifiersOrder::check(const MatchFinder::MatchResult &Result) {
  const VarDecl *Var = Result.Nodes.getStmtAs<VarDecl>("var");
  const SourceManager &SM = *Result.SourceManager;
  const ASTContext *Context = Result.Context;

  assert(Var != nullptr);
  if (!Var) {
    llvm_unreachable("Expected valid variable declaration");
  }

  QualType VarType = Var->getType();
  QualType PointeeType = VarType;
  const std::string VarTypeString = VarType.getAsString();
  // QualType CanonicalVarType = VarType.getCanonicalType();
  if (PointeeType->isPointerType() || PointeeType->isReferenceType()) {
      return;
  }
  while (PointeeType->isPointerType() || PointeeType->isReferenceType()) {
    PointeeType = PointeeType->getPointeeType();
    const std::string PointeeTypeString = PointeeType.getAsString();
    (void)PointeeTypeString;
  }

  bool LocalConstQualified = VarType.isLocalConstQualified();
  bool PointeeLocalConstQualified = PointeeType.isLocalConstQualified();
  // TODO(mkurdej): restrict, volatile
  //bool LocalVolatileQualified = VarType.isLocalVolatileQualified();
  // if (!VarType.getLocalCVRQualifiers()) {

  if (!(LocalConstQualified || PointeeLocalConstQualified)) {
    // Nothing to do.
    return;
  }

  auto SR = Var->getSourceRange();

  // In:                    int ii = 0;
  // Var->getLocation()           ^
  // getLocWithOffset(-1)        ^
  // GetBeginningOfToken        ^
  // backwardSkip              ^
  // ReplacementRange       ~~~
  
  // TODO: solution is to move "const  "
  TypeSourceInfo *TSI = Var->getTypeSourceInfo();
  TypeLoc TL = TSI->getTypeLoc();
  SourceRange TypeWithoutQualifiersRange = TL.getLocalSourceRange();
  SourceRange TypeLocRange = TL.getSourceRange();
  SourceRange ConstRange = findToken(SM, Context, SR, "const");
  // if pointer || reference
  //TypeLoc NextTL = TL.getNextTypeLoc();
  UnqualTypeLoc UnqualTL = TL.getUnqualifiedLoc();
  SourceRange UnqualTypeWithoutQualifiersRange = UnqualTL.getLocalSourceRange();
  SourceRange UnqualTypeLocRange = UnqualTL.getSourceRange();

  SourceLocation Loc = TL.getLocStart();
  if (Loc.isInvalid() || Loc.isMacroID())
    return;
  // Look through qualification.
  if (auto QualLoc = TL.getAs<QualifiedTypeLoc>())
    TL = QualLoc.getUnqualifiedLoc();
  auto BuiltinLoc = TL.getAs<BuiltinTypeLoc>();
  if (BuiltinLoc)
    return;


  SourceLocation VarNameLoc = Var->getLocation();
  VarNameLoc = Lexer::GetBeginningOfToken(VarNameLoc.getLocWithOffset(-1), SM,
                                          Context->getLangOpts());
  VarNameLoc = VarNameLoc.getLocWithOffset(1);
  VarNameLoc = backwardSkipWhitespacesAndComments(SM, Context, VarNameLoc);

  SourceRange ReplacementRange(SR.getBegin(), VarNameLoc);
  const std::string TypeString = VarType.getAsString();

  SourceLocation LeftInsertLoc = SR.getBegin();
  // TODO
  SourceLocation RightInsertLoc;
  SourceLocation InsertLoc;
  if (QualifierAlignment == QAS_Left) {
    InsertLoc = LeftInsertLoc;
  } else if (QualifierAlignment == QAS_Right) {
    InsertLoc = RightInsertLoc;
  } else {
    llvm_unreachable("Invalid QualifierAlignmentStyle");
  }
  if (ConstRange.getBegin() != InsertLoc) {
    auto Diag = diag(SR.getBegin(), "wrong order of qualifiers");
    Diag << FixItHint::CreateRemoval(ConstRange);
    Diag << FixItHint::CreateInsertion(InsertLoc, "const ");
    // Diag << FixItHint::CreateInsertionFromRange(InsertLoc, ConstRange, "const ");
  }
}

} // namespace tidy
} // namespace clang
