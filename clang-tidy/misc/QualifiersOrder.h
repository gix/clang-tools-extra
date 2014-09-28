//===--- CVQualifiersOrder.h - clang-tidy -----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_CV_QUALIFIERS_ORDER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_CV_QUALIFIERS_ORDER_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {

/// \brief Order (before/after type) of 'const' ('restrict' and 'volatile'?) qualifiers
class QualifiersOrder: public ClangTidyCheck {
public:
  QualifiersOrder(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_CV_QUALIFIERS_ORDER_H
