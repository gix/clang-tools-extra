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
    // Fast-forward current token.
    Loc = Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
  }
}

StringRef getAsString(const SourceManager &SM, const clang::ASTContext *Context,
                      SourceRange R) {
  if (R.getBegin().isMacroID() ||
      !SM.isWrittenInSameFile(R.getBegin(), R.getEnd()))
    return StringRef();

  const char *Begin = SM.getCharacterData(R.getBegin());
  const char *End = SM.getCharacterData(R.getEnd());
  return StringRef(Begin, End - Begin);
}

SourceRange findToken(const SourceManager &SM, const clang::ASTContext *Context,
                      SourceRange SR, StringRef Text) {
  if (SR.isInvalid())
    return SourceRange();
  assert(SR.isValid());
  for (SourceLocation Loc = SR.getBegin(); Loc < SR.getEnd();) {
    // FIXME(mkurdej): Loc can actually be past SR.getEnd()
    while (isWhitespace(*FullSourceLoc(Loc, SM).getCharacterData())) {
      Loc = Loc.getLocWithOffset(1);
    }

    SourceLocation EndLoc =
        Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
    StringRef TokenText = getAsString(SM, Context, SourceRange(Loc, EndLoc));
    if (TokenText == Text ||
        (TokenText.back() == '>' &&
         TokenText.substr(0, TokenText.size() - 1) == Text))
      return SourceRange(Loc, EndLoc);
    // fast-forward current token
    Loc = Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
  }
  // Not found token of this kind in the given range
  return SourceRange();
}

SourceRange findTokenBackwards(const SourceManager &SM,
                               const clang::ASTContext *Context,
                               SourceLocation Loc, StringRef Text) {
  assert(Loc.isValid());
  for (;;) {
    while (isWhitespace(*FullSourceLoc(Loc, SM).getCharacterData())) {
      Loc = Loc.getLocWithOffset(-1);
      assert(Loc.isValid());
    }

    // SourceLocation EndLoc = Loc;
    Loc = Lexer::GetBeginningOfToken(Loc, SM, Context->getLangOpts());
    SourceLocation EndLoc =
        Lexer::getLocForEndOfToken(Loc, 0, SM, Context->getLangOpts());
    StringRef TokenText = getAsString(SM, Context, SourceRange(Loc, EndLoc));
    if (TokenText == Text)
      return SourceRange(Loc, EndLoc);
    Loc = Loc.getLocWithOffset(-1);
    assert(Loc.isValid());
    if (Loc.isInvalid())
      break;
  }
  // Not found token of this kind in the given range
  return SourceRange();
}

TypeLoc getInnerPointeeLoc(TypeLoc TL, SourceLocation *SigilLoc = nullptr) {
  for (;;) {
    UnqualTypeLoc UTL = TL.getUnqualifiedLoc();
    if (auto PTL = UTL.getAs<PointerTypeLoc>()) {
      TL = PTL.getPointeeLoc();
      if (SigilLoc)
        *SigilLoc = PTL.getSigilLoc();
    } else if (auto RTL = UTL.getAs<ReferenceTypeLoc>()) {
      TL = RTL.getPointeeLoc();
      if (SigilLoc)
        *SigilLoc = RTL.getSigilLoc();
    } else if (auto ATL = UTL.getAs<AttributedTypeLoc>()) {
      TL = ATL.getNextTypeLoc();
    } else {
      return TL;
    }
  }
}

SourceRange getRangeBeforeType(TypeLoc TL, SourceLocation StartLoc) {
  UnqualTypeLoc UTL = TL.getUnqualifiedLoc();
  SourceRange USR = UTL.getSourceRange();
  return SourceRange(StartLoc, USR.getBegin());
}

SourceRange getRangeAfterType(const SourceManager &SM,
                              const ASTContext *Context, TypeLoc TL,
                              SourceLocation EndLoc) {
  // Find end location: before variable name or before the first '*' or '&'.
  SourceLocation SigilLoc;
  TL = getInnerPointeeLoc(TL, &SigilLoc);
  if (SigilLoc.isValid())
    EndLoc = SigilLoc.getLocWithOffset(-1);
  UnqualTypeLoc UTL = TL.getUnqualifiedLoc();

  // Get inner type of an elaborated type location (e.g. namespace).
  auto ETL = UTL.getAs<ElaboratedTypeLoc>();
  if (!ETL.isNull()) {
    TL = ETL.getNamedTypeLoc();
    UTL = TL.getUnqualifiedLoc();
  }

  // Find start location, go past inner unqualified type.
  SourceRange SR = TL.getSourceRange();
  SourceLocation StartLoc =
      Lexer::getLocForEndOfToken(SR.getBegin(), 0, SM, Context->getLangOpts());

  // For a template specialization, go past the closing bracket.
  TemplateSpecializationTypeLoc UTSTL =
      UTL.getAs<TemplateSpecializationTypeLoc>();
  if (!UTSTL.isNull())
    StartLoc = UTSTL.getRAngleLoc().getLocWithOffset(1);

  return SourceRange(StartLoc, EndLoc);
}

SourceRange findQualifier(const SourceManager &SM, const ASTContext *Context,
                          SourceRange LHS, SourceRange RHS,
                          StringRef Qualifier) {
  SourceRange LeftConstR = findToken(SM, Context, LHS, Qualifier);
  if (LeftConstR.isValid()) {
    return LeftConstR;
  }
  return findToken(SM, Context, RHS, Qualifier);
}

Qualifiers getInnerTypeQualifiers(TypeLoc TL) {
  TL = getInnerPointeeLoc(TL);
  Qualifiers Quals = TL.getType().getLocalQualifiers();
  return Quals;
}

} // namespace

namespace ast_matchers {

const internal::VariadicDynCastAllOfMatcher<Decl, TypedefDecl> typedefDecl;

AST_MATCHER(TypeLoc, isTemplateSpecializationTypeLoc) {
  TypeLoc PointeeTL = getInnerPointeeLoc(Node).getUnqualifiedLoc();
  if (PointeeTL.getTypeLocClass() == TypeLoc::TemplateSpecialization)
    return true;
  return false;
}

} // namespace ast_matchers

namespace tidy {

QualifiersOrder::QualifiersOrder(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      QualifierAlignment(Options.get("QualifierAlignment", "Left") == "Right"
                             ? QAS_Right
                             : QAS_Left) {}

void QualifiersOrder::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "QualifierAlignment",
                QualifierAlignment == QAS_Right ? "Right" : "Left");
  // TODO(mkurdej): QualifierOrder: CRV|CVR|RCV|RVC|VCR|VRC
  //    static, __attribute__, __declspec, [[attribute]]
}

void QualifiersOrder::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(varDecl().bind("var"), this);
  Finder->addMatcher(functionDecl().bind("function"), this);
  Finder->addMatcher(typedefDecl().bind("typedef"), this);
  Finder->addMatcher(
      typeLoc(isTemplateSpecializationTypeLoc()).bind("template-spec-loc"),
      this);
}

void QualifiersOrder::check(const MatchFinder::MatchResult &Result) {
  const SourceManager &SM = *Result.SourceManager;
  const ASTContext *Context = Result.Context;

  if (auto Var = Result.Nodes.getStmtAs<VarDecl>("var")) {
    TypeSourceInfo *TIS = Var->getTypeSourceInfo();
    if (!TIS)
      return;
    checkQualifiers(SM, Context, TIS->getTypeLoc(),
                    SourceRange(Var->getLocStart(), Var->getLocation()));
  } else if (auto Fun = Result.Nodes.getStmtAs<FunctionDecl>("function")) {
    SourceRange R(Fun->getLocStart(), Fun->getLocation());
    TypeSourceInfo *TIS = Fun->getTypeSourceInfo();
    if (!TIS)
      return;
    TypeLoc FunTL = TIS->getTypeLoc();
    if (auto ATL = FunTL.getAs<AttributedTypeLoc>())
      FunTL = ATL.getNextTypeLoc();
    if (auto PTL = FunTL.getAs<ParenTypeLoc>())
      FunTL = PTL.getNextTypeLoc();
    if (auto TTL = FunTL.getAs<TypedefTypeLoc>()) {
      TypedefNameDecl *TND = TTL.getTypedefNameDecl();
      if (!TND)
        return;
      TIS = TND->getTypeSourceInfo();
      if (!TIS)
        return;
      FunTL = TIS->getTypeLoc();
      R = FunTL.getSourceRange();
    }
    if (auto FTL = FunTL.getAs<FunctionTypeLoc>())
      checkQualifiers(SM, Context, FTL.getReturnLoc(), R);
  } else if (auto TD = Result.Nodes.getStmtAs<TypedefDecl>("typedef")) {
    SourceRange R = TD->getSourceRange();
    if (R.isInvalid() || R.getBegin().isMacroID())
      return;
    // Go past "typedef" keyword.
    SourceLocation PastTypedefLoc =
        Lexer::getLocForEndOfToken(R.getBegin(), 0, SM, Context->getLangOpts());
    PastTypedefLoc =
        forwardSkipWhitespaceAndComments(SM, Context, PastTypedefLoc);
    R.setBegin(PastTypedefLoc);
    checkQualifiers(SM, Context, TD->getTypeSourceInfo()->getTypeLoc(), R);
  } else if (auto TSL = Result.Nodes.getStmtAs<TypeLoc>("template-spec-loc")) {
    TypeLoc PointeeTL = getInnerPointeeLoc(*TSL).getUnqualifiedLoc();
    auto TSTL = PointeeTL.getAs<TemplateSpecializationTypeLoc>();
    if (!TSTL)
      return;

    unsigned int NumArgs = TSTL.getNumArgs();
    if (NumArgs == 0)
      return;

    SourceLocation StartLoc = TSTL.getLAngleLoc();
    if (StartLoc.isInvalid() || StartLoc.isMacroID())
      return;
    StartLoc = StartLoc.getLocWithOffset(1);
    for (unsigned int Arg = 0; Arg < NumArgs - 1; ++Arg) {
      TemplateArgumentLoc TAL = TSTL.getArgLoc(Arg);
      TemplateArgumentLoc NextTAL = TSTL.getArgLoc(Arg + 1);
      SourceLocation EndLoc = NextTAL.getSourceRange().getBegin();
      if (EndLoc.isMacroID())
        continue;
      // Find a comma ',' going backwards, because EndLoc overlaps the next TAL.
      EndLoc = findTokenBackwards(SM, Context, EndLoc, ",").getBegin();
      SourceLocation NewEndLoc = forwardSkipWhitespaceAndComments(
          SM, Context, EndLoc.getLocWithOffset(1));
      if (TAL.getArgument().getKind() != TemplateArgument::Type) {
        StartLoc = NewEndLoc;
        continue;
      }
      TypeLoc TL = TAL.getTypeSourceInfo()->getTypeLoc();
      checkQualifiers(SM, Context, TL, SourceRange(StartLoc, EndLoc));
      StartLoc = NewEndLoc;
    }
    TemplateArgumentLoc TAL = TSTL.getArgLoc(NumArgs - 1);
    if (TAL.getArgument().getKind() != TemplateArgument::Type)
      return;
    TypeLoc TL = TAL.getTypeSourceInfo()->getTypeLoc();
    SourceLocation EndLoc = TSTL.getRAngleLoc();
    checkQualifiers(SM, Context, TL, SourceRange(StartLoc, EndLoc));
  } else {
    llvm_unreachable("Invalid match");
  }
}

void QualifiersOrder::checkQualifiers(const SourceManager &SM,
                                      const ASTContext *Context, TypeLoc TL,
                                      SourceRange R) {
  assert(R.isValid());
  assert(TL);
  // Skip macros.
  if (R.getBegin().isMacroID())
    return;
  // Check if the type is const-qualified.
  Qualifiers Quals = getInnerTypeQualifiers(TL);
  if (!Quals.hasConst())
    return;

  // Find const qualifier of the inner (leftmost) type.
  SourceRange LHS = getRangeBeforeType(TL, R.getBegin());
  SourceRange RHS = getRangeAfterType(SM, Context, TL, R.getEnd());
  SourceRange ConstR = findQualifier(SM, Context, LHS, RHS, "const");
  if (ConstR.isInvalid()) {
    // It happens when const comes from a macro expansion.
    return;
  }

  // Skip whitespace and comments following const.
  ConstR.setEnd(forwardSkipWhitespaceAndComments(SM, Context, ConstR.getEnd()));

  // Define insert location, respectively, left and right to the type.
  SourceLocation InsertLoc;
  SourceLocation MiddleLoc = LHS.getEnd();
  bool MaybeAddSpaceBefore = false, MaybeAddSpaceAfter = false;
  switch (QualifierAlignment) {
  case QAS_Left:
    MaybeAddSpaceAfter = true;
    InsertLoc = MiddleLoc;
    if ((ConstR.getBegin() < MiddleLoc) || (ConstR.getBegin() == MiddleLoc)) {
      // Already on the left side.
      return;
    }
    break;
  case QAS_Right:
    MaybeAddSpaceBefore = true;
    InsertLoc = RHS.getBegin();
    while (isWhitespace(*FullSourceLoc(InsertLoc, SM).getCharacterData())) {
      InsertLoc = InsertLoc.getLocWithOffset(1);
    }
    assert(InsertLoc.isValid());
    if (!(ConstR.getBegin() < InsertLoc)) {
      // Already on the right side.
      return;
    }
    break;
  default:
    llvm_unreachable("Invalid QualifierAlignmentStyle");
  }

  auto Diag = diag(R.getBegin(), "wrong order of qualifiers");

  // Add a space if necessary.
  const char *ConstFront = SM.getCharacterData(InsertLoc.getLocWithOffset(-1));
  bool MustAddSpaceBefore =
      MaybeAddSpaceBefore && ConstFront && !isWhitespace(*ConstFront);
  if (MustAddSpaceBefore)
    Diag << FixItHint::CreateInsertion(InsertLoc, /*Code=*/" ");

  // Move qualifier: insert first and then remove.
  CharSourceRange CharRange = CharSourceRange::getCharRange(ConstR);
  Diag << FixItHint::CreateInsertionFromRange(InsertLoc, CharRange);

  // Add a space if necessary.
  const char *ConstBack =
      SM.getCharacterData(ConstR.getEnd().getLocWithOffset(-1));
  bool MustAddSpaceAfter =
      MaybeAddSpaceAfter && ConstBack && !isWhitespace(*ConstBack);
  if (MustAddSpaceAfter)
    Diag << FixItHint::CreateInsertion(InsertLoc, /*Code=*/" ");

  // FixItHint::CreateRemoval removes a closed (token) range [a, b] and we
  // want to remove a half-open (char) range [a, b).
  SourceRange RemovalR(ConstR.getBegin(), ConstR.getEnd().getLocWithOffset(-1));
  Diag << FixItHint::CreateRemoval(RemovalR);
}

} // namespace tidy
} // namespace clang
