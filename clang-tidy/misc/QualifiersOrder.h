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
#include "llvm/Support/YAMLTraits.h"

namespace clang {
namespace tidy {

struct QualifiersOrderOptions {
  QualifiersOrderOptions() : QualifierAlignment(QAS_Left) {}

  /// Defines the placement/alignment of CVR qualifiers
  enum /*class*/ QualifierAlignmentStyle {
    /// Leave qualifiers as they are (useful if only sorting is needed)
    // TODO(mkurdej): implement qualifier sorting
    QAS_None,
    /// Put qualifiers on the left side of (before) the type
    QAS_Left,
    /// Put qualifiers on the right side of (behind) the type
    QAS_Right
  };

  QualifierAlignmentStyle QualifierAlignment;
};

/// \brief Order (before/after type) of 'const' ('restrict' and 'volatile'?) qualifiers
class QualifiersOrder: public ClangTidyCheck {
public:
  QualifiersOrder(StringRef Name, ClangTidyContext *Context);
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

public:
  /// Defines the placement/alignment of CVR qualifiers
  enum /*class*/ QualifierAlignmentStyle {
    /// Leave qualifiers as they are (useful if only sorting is needed)
    // TODO(mkurdej): implement qualifier sorting
    QAS_None,
    /// Put qualifiers on the left side of (before) the type
    QAS_Left,
    /// Put qualifiers on the right side of (behind) the type
    QAS_Right
  };

private:
  template <typename T>
  std::string getYamlInput(llvm::StringRef LocalName, T Default);

private:
  friend struct llvm::yaml::MappingTraits<QualifiersOrder>;
  QualifierAlignmentStyle QualifierAlignment;
  QualifiersOrderOptions Style;
};

} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_CV_QUALIFIERS_ORDER_H
