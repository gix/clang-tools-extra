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
  UnqualTypeLoc UTL = TL.getUnqualifiedLoc();
  SourceRange USR = UTL.getSourceRange();
  return SourceRange(Var->getLocStart(), USR.getBegin());
}

SourceRange getRangeAfterType(const SourceManager &SM,
                              const ASTContext *Context, const VarDecl *Var) {
  SourceLocation VarNameLoc = Var->getLocation();
  VarNameLoc = Lexer::GetBeginningOfToken(VarNameLoc.getLocWithOffset(-1), SM,
                                          Context->getLangOpts());
  TypeSourceInfo *TSI = Var->getTypeSourceInfo();
  TypeLoc TL = TSI->getTypeLoc();

  // Find end location: before variable name or before the first '*' or '&'.
  SourceLocation EndLoc = VarNameLoc;
  for (;;) {
    auto PTL = TL.getAs<PointerTypeLoc>();
    if (!PTL.isNull()) {
      TL = PTL.getPointeeLoc();

      EndLoc = PTL.getSigilLoc().getLocWithOffset(-1);
      continue;
    }

    auto RTL = TL.getAs<ReferenceTypeLoc>();
    if (!RTL.isNull()) {
      TL = RTL.getPointeeLoc();
      EndLoc = RTL.getSigilLoc().getLocWithOffset(-1);
      continue;
    }
    break;
  }
  UnqualTypeLoc UTL = TL.getUnqualifiedLoc();

  // Get inner type of an elaborated type location (e.g. namespace).
  auto ETL = UTL.getAs<ElaboratedTypeLoc>();
  if (!ETL.isNull()) {
    TL = ETL.getNamedTypeLoc();
    UTL = TL.getUnqualifiedLoc();
  }

  // Find start location: after inner type without qualifiers.
  SourceRange SR = TL.getSourceRange();
  SourceLocation StartLoc =
      Lexer::getLocForEndOfToken(SR.getBegin(), 0, SM, Context->getLangOpts());
  TemplateSpecializationTypeLoc UTSTL =
      UTL.getAs<TemplateSpecializationTypeLoc>();
  if (!UTSTL.isNull())
    StartLoc = UTSTL.getRAngleLoc().getLocWithOffset(1);
  return SourceRange(StartLoc, EndLoc);
}

SourceRange findQualifier(const SourceManager &SM, const ASTContext *Context,
                          SourceRange LHS, SourceRange RHS,
                          StringRef Qualifier) {
  // TODO: assert((ConstOnLeft && !ConstOnRight) || (!ConstOnLeft &&
  // ConstOnRight));
  SourceRange LeftConstR = findToken(SM, Context, LHS, Qualifier);
  if (LeftConstR.isValid()) {
    return LeftConstR;
  }
  return findToken(SM, Context, RHS, Qualifier);
}

Qualifiers getInnerTypeQualifiers(QualType QT) {
  // Use `QualType::getAsString()` to check for qualifiers, as they are always
  // left to the type. The order of qualifiers in returned string is:
  // `const volatile restrict` (cf. `clang::Qualifiers::print`).
  Qualifiers Quals;
  const std::string QTName = QT.getAsString();
  std::istringstream Stream(QTName);

  std::string Qualifier;
  Stream >> Qualifier;
  if (Qualifier == "const") {
    Quals.addConst();
    Stream >> Qualifier;
  }
  if (Qualifier == "volatile") {
    Quals.addVolatile();
    Stream >> Qualifier;
  }
  if (Qualifier == "restrict")
    Quals.addRestrict();
  return Quals;
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

  QualType VarType = Var->getType();
  Qualifiers Quals = getInnerTypeQualifiers(VarType);
  if (!Quals.hasConst())
    return;

  // Find const qualifier before or after type.
  SourceRange LHS = getRangeBeforeType(SM, Context, Var);
  SourceRange RHS = getRangeAfterType(SM, Context, Var);
  SourceRange ConstR = findQualifier(SM, Context, LHS, RHS, "const");
  assert(ConstR.isValid());

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
    // CharSourceRange CharRange = CharSourceRange::getTokenRange(ConstR);
    // CharSourceRange CharRange = Lexer::makeFileCharRange(
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
