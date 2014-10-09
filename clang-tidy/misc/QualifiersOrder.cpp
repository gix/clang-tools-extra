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
#include "llvm/Support/YAMLTraits.h"
#include <sstream>

using namespace clang::ast_matchers;
using clang::tidy::QualifiersOrder;

namespace llvm {
namespace yaml {
template <>
struct ScalarEnumerationTraits<QualifiersOrder::QualifierAlignmentStyle> {
  static void enumeration(IO &IO,
                          QualifiersOrder::QualifierAlignmentStyle &Value) {
    IO.enumCase(Value, "None", QualifiersOrder::QAS_None);
    IO.enumCase(Value, "Left", QualifiersOrder::QAS_Left);
    IO.enumCase(Value, "Right", QualifiersOrder::QAS_Right);
  }
};
} // namespace yaml
} // namespace llvm

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
backwardSkipWhitespaceAndComments(const SourceManager &SM,
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
forwardSkipWhitespaceAndComments(const SourceManager &SM,
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
  assert(SR.isValid());
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

} // namespace

QualifiersOrder::QualifiersOrder(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      QualifierAlignment(Options.get("QualifierAlignment", QAS_Left)) {}

void QualifiersOrder::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "QualifierAlignment", QualifierAlignment);
  // TODO: QualifierOrder: CRV|CVR|RCV|RVC|VCR|VRC
}

void QualifiersOrder::registerMatchers(MatchFinder *Finder) {
  // Finder->addMatcher(qualType(isConstQualified()).bind("const"), this);
  // Finder->addMatcher(qualType(isVolatileQualified()).bind("volatile"), this);
  // Finder->addMatcher(qualType(isConstQualified(), isVolatileQualified())
  //                       .bind("const volatile"),
  //                   this);
  Finder->addMatcher(varDecl().bind("var"), this);
#if 0
  Finder->addMatcher(typeLoc().bind("type"), this);
#endif
  // TODO(mkurdej): declarations in if/for/while: ?Stmt()
  // TODO(mkurdej): function/method arguments: functionDecl
  // TODO(mkurdej): typedefs typedefType()
}

SourceRange getRangeBeforeType(const SourceManager & /*SM*/,
                               const ASTContext * /*Context*/,
                               const VarDecl *Var) {
  TypeSourceInfo *TSI = Var->getTypeSourceInfo();
  TypeLoc TL = TSI->getTypeLoc();
  SourceRange SR = TL.getSourceRange();
  SourceRange LSR = TL.getLocalSourceRange();

  if (LSR.isValid())
    return SourceRange(Var->getLocStart(), LSR.getBegin());
  else
    return SourceRange(Var->getLocStart(), SR.getBegin());
}

SourceRange getRangeAfterType(const SourceManager &SM,
                              const ASTContext *Context, const VarDecl *Var) {
  SourceLocation VarNameLoc = Var->getLocation();
  VarNameLoc = Lexer::GetBeginningOfToken(VarNameLoc.getLocWithOffset(-1), SM,
                                          Context->getLangOpts());
  // FIXME: can be inside SR (skip type or search from right)
  TypeSourceInfo *TSI = Var->getTypeSourceInfo();
  TypeLoc TL = TSI->getTypeLoc();
  SourceRange SR = TL.getSourceRange();
  return SourceRange(SR.getEnd(), VarNameLoc);
}

SourceRange findQualifier(const SourceManager &SM, const ASTContext *Context,
                          SourceRange LHS, SourceRange RHS,
                          StringRef Qualifier) {
  // TODO: assert((ConstOnLeft && !ConstOnRight) || (!ConstOnLeft && ConstOnRight));
  SourceRange LeftConstR = findToken(SM, Context, LHS, Qualifier);
  if (LeftConstR.isValid()) {
    return LeftConstR;
  }
  return findToken(SM, Context, RHS, Qualifier);
}

void QualifiersOrder::check(const MatchFinder::MatchResult &Result) {
  const SourceManager &SM = *Result.SourceManager;
  const ASTContext *Context = Result.Context;

  auto mark = [=, &SM](SourceRange R, StringRef Mark) {
    // assert(SR.isValid());
    auto StartLoc = R.getBegin();
    auto EndLoc = R.getEnd();
    if (EndLoc.isValid()) {
      EndLoc =
          Lexer::getLocForEndOfToken(EndLoc, 0, SM, Context->getLangOpts());
    }
    if (R.isValid()) {
      auto Diag = diag(StartLoc, Mark);
      // Diag << FixItHint::CreateRemoval(SR);
      Diag << FixItHint::CreateInsertion(StartLoc, "[" + Mark.str() + "[[");
      Diag << FixItHint::CreateInsertion(EndLoc, "]]" + Mark.str() + "]");
    }
  };

  const VarDecl *Var = Result.Nodes.getStmtAs<VarDecl>("var");
  assert(Var);
  if (!Var)
    llvm_unreachable("Expected valid variable declaration");

#if 0
  const TypeLoc *TLP = Result.Nodes.getStmtAs<TypeLoc>("type");
  if (!TLP)
    return;
  TypeLoc TL = *TLP;
#endif

  // TODO: refactor: getInnerTypeQualifiers();
  QualType VarType = Var->getType();
  const std::string VarTypeString = VarType.getAsString();
  // Check VarTypeString for const, as it is always at the beginning.
  auto SpaceOffset = VarTypeString.find(' ', 0);
  if (SpaceOffset == std::string::npos)
    return;
  auto Qualifier = VarTypeString.substr(0, SpaceOffset);
  bool HasConst = (Qualifier == "const");
  if (HasConst) {
    auto PreviousSpaceOffset = SpaceOffset;
    SpaceOffset = VarTypeString.find(' ', PreviousSpaceOffset + 1);
    if (SpaceOffset != std::string::npos)
      Qualifier = VarTypeString.substr(PreviousSpaceOffset + 1,
                                       SpaceOffset - (PreviousSpaceOffset + 1));
  }
  bool HasVolatile = (Qualifier == "volatile");

  if (!HasConst)
    return;

  // Get the most inner pointee type.
  QualType PointeeType = VarType;
  while (PointeeType->isPointerType() || PointeeType->isReferenceType()) {
    PointeeType = PointeeType->getPointeeType();
    const std::string PointeeTypeString = PointeeType.getAsString();
    (void)PointeeTypeString;
    continue;
  }

  // Find const qualifier before or after type.
  SourceRange LHS = getRangeBeforeType(SM, Context, Var);
  SourceRange RHS = getRangeAfterType(SM, Context, Var);
  SourceRange ConstR = findQualifier(SM, Context, LHS, RHS, "const");

  if (ConstR.isInvalid()) {
    if (VarType != PointeeType) {
      // TEST ranges
      TypeSourceInfo *TSI = Var->getTypeSourceInfo();
      TypeLoc TL = TSI->getTypeLoc();
      SourceRange LocalSR = TL.getLocalSourceRange();
      SourceRange SR = TL.getSourceRange();

      mark(SR, "TL");
      mark(LocalSR, "LTL");
      if (auto QualTL = TL.getAs<QualifiedTypeLoc>()) {
        TL = QualTL.getUnqualifiedLoc();
        mark(QualTL.getSourceRange(), "QTL");
      }
    }
    return;
  }

#if 0
  // TypeLoc ParamTL = AllParamDecls[I]->getTypeSourceInfo()->getTypeLoc();
  // ReferenceTypeLoc RefTL = ParamTL.getAs<ReferenceTypeLoc>();
  // SourceRange Range(AllParamDecls[I]->getLocStart(), ParamTL.getLocEnd());
  // CharSourceRange CharRange = Lexer::makeFileCharRange(
  //     CharSourceRange::getTokenRange(Range), SM, LangOptions());

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
  SourceRange ConstR = findToken(SM, Context, SR, "const");
  // if pointer || reference
  //TypeLoc NextTL = TL.getNextTypeLoc();
  UnqualTypeLoc UnqualTL = TL.getUnqualifiedLoc();
  SourceRange UnqualTypeWithoutQualifiersRange = UnqualTL.getLocalSourceRange();
  SourceRange UnqualTypeLocRange = UnqualTL.getSourceRange();

  SourceLocation Loc = TL.getLocStart();
  if (Loc.isInvalid() || Loc.isMacroID())
    return;
  // Look through qualification.
  if (auto QualTL = TL.getAs<QualifiedTypeLoc>())
    TL = QualTL.getUnqualifiedLoc();
  //auto BuiltinTL = TL.getAs<BuiltinTypeLoc>();
  //if (BuiltinTL)
  //  return;

  diag(TL.getLocStart(), "TL.getLocStart()");
  diag(TL.getLocEnd(), "TL.getLocEnd()");

  SourceLocation VarNameLoc = Var->getLocation();
  VarNameLoc = Lexer::GetBeginningOfToken(VarNameLoc.getLocWithOffset(-1), SM,
                                          Context->getLangOpts());
  VarNameLoc = VarNameLoc.getLocWithOffset(1);
  VarNameLoc = backwardSkipWhitespaceAndComments(SM, Context, VarNameLoc);

  SourceRange ReplacementRange(SR.getBegin(), VarNameLoc);
  const std::string TypeString = VarType.getAsString();
#endif

  // Skip whitespace and comments following const.
  ConstR.setEnd(forwardSkipWhitespaceAndComments(SM, Context, ConstR.getEnd()));

  // Define insert locations, respectively, left and right to the type.
  SourceLocation LeftInsertLoc = Var->getLocStart();
  // TODO(mkurdej): directly after the most inner (leftmost) type without
  // qualifiers.
  SourceLocation RightInsertLoc = RHS.getBegin();
  SourceLocation InsertLoc;
  if (QualifierAlignment == QAS_Left) {
    InsertLoc = LeftInsertLoc;
  } else if (QualifierAlignment == QAS_Right) {
    InsertLoc = RightInsertLoc;
  } else {
    llvm_unreachable("Invalid QualifierAlignmentStyle");
  }
  if (ConstR.getBegin() != InsertLoc) {
    auto Diag = diag(LeftInsertLoc, "wrong order of qualifiers");
    // Move qualifier.
    CharSourceRange CharRange = CharSourceRange::getCharRange(ConstR);
    //CharSourceRange CharRange = CharSourceRange::getTokenRange(ConstR);
    //CharSourceRange CharRange = Lexer::makeFileCharRange(
    //    CharSourceRange::getTokenRange(ConstR), SM, LangOptions());
    Diag << FixItHint::CreateInsertionFromRange(InsertLoc, CharRange);
    // FixItHint::CreateRemoval removes a closed (token) range [a, b] and we
    // want a half-open (char) range [a, b).
    SourceRange RemovalR(ConstR.getBegin(),
                         ConstR.getEnd().getLocWithOffset(-1));
    Diag << FixItHint::CreateRemoval(RemovalR);
  }
}

} // namespace tidy
} // namespace clang
