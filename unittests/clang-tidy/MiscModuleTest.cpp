#include "ClangTidyTest.h"
#include "misc/ArgumentCommentCheck.h"
#include "misc/QualifiersOrder.h"
#include "gtest/gtest.h"

// RUN: echo '{ Checks: "-*,misc-qualifiers-order"}' > %t/.clang-tidy
// RUN: echo '{ Checks: "-*,misc-qualifiers-order", CheckOptions: [{key: misc-qualifiers-order.QualifierAlignment, value: Left}]}' > %t/.clang-tidy

namespace clang {
namespace tidy {
namespace test {

#define EXPECT_NO_CHANGES(Check, Code)                                         \
  EXPECT_EQ(Code, runCheckOnCode<Check>(Code))

TEST(ArgumentCommentCheckTest, CorrectComments) {
  EXPECT_NO_CHANGES(ArgumentCommentCheck,
                    "void f(int x, int y); void g() { f(/*x=*/0, /*y=*/0); }");
  EXPECT_NO_CHANGES(ArgumentCommentCheck,
                    "struct C { C(int x, int y); }; C c(/*x=*/0, /*y=*/0);");
}

TEST(ArgumentCommentCheckTest, ThisEditDistanceAboveThreshold) {
  EXPECT_NO_CHANGES(ArgumentCommentCheck,
                    "void f(int xxx); void g() { f(/*xyz=*/0); }");
}

TEST(ArgumentCommentCheckTest, OtherEditDistanceAboveThreshold) {
  EXPECT_EQ("void f(int xxx, int yyy); void g() { f(/*xxx=*/0, 0); }",
            runCheckOnCode<ArgumentCommentCheck>(
                "void f(int xxx, int yyy); void g() { f(/*Xxx=*/0, 0); }"));
  EXPECT_EQ("struct C { C(int xxx, int yyy); }; C c(/*xxx=*/0, 0);",
            runCheckOnCode<ArgumentCommentCheck>(
                "struct C { C(int xxx, int yyy); }; C c(/*Xxx=*/0, 0);"));
}

TEST(ArgumentCommentCheckTest, OtherEditDistanceBelowThreshold) {
  EXPECT_NO_CHANGES(ArgumentCommentCheck,
                    "void f(int xxx, int yyy); void g() { f(/*xxy=*/0, 0); }");
}

TEST(QualifiersOrderTest, Basic) {
  EXPECT_NO_CHANGES(QualifiersOrder, "int /**/ i;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int  * ip;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int i; int &ir = i;");
  EXPECT_NO_CHANGES(QualifiersOrder, "/*const*/ const int i = 0;");
  EXPECT_EQ("const int /*const*/ i = 0;",
            runCheckOnCode<QualifiersOrder>("int /*const*/ const i = 0;"));
  EXPECT_NO_CHANGES(QualifiersOrder, "const int ci = 0;");
  EXPECT_NO_CHANGES(QualifiersOrder, "const /**/ int ci = 0;");
  EXPECT_EQ("const int ic = 0;\n",
            runCheckOnCode<QualifiersOrder>("int const ic = 0;\n"));
  EXPECT_EQ("typedef int foo;\n"
            "const foo ci = 0;\n"
            "const foo ic = 0;\n",
            runCheckOnCode<QualifiersOrder>("typedef int foo;\n"
                                            "const foo ci = 0;\n"
                                            "foo const ic = 0;\n"));
  EXPECT_EQ("typedef int *foo;\n"
            "const foo ci = nullptr;\n"
            "const foo ic = nullptr;\n",
            runCheckOnCode<QualifiersOrder>("typedef int *foo;\n"
                                            "const foo ci = nullptr;\n"
                                            "foo const ic = nullptr;\n"));
  EXPECT_EQ("const auto ci = 0;\n"
            "const auto ic = 0;\n",
            runCheckOnCode<QualifiersOrder>("const auto ci = 0;\n"
                                            "auto const ic = 0;\n"));
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "const C<int> cCi = {};\n"
      "const C<const int> cCci = {};\n"
      //"const C<const int> cCic = {};\n"
      "const C<int> Ci_c = {};\n"
      "const C<const int> Cci_c = {};\n"
      //"const C<const int> Cic_c = {};\n"
      ,
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "const C<int> cCi = {};\n"
                                      "const C<const int> cCci = {};\n"
                                      //"const C<int const> cCic = {};\n"
                                      "C<int> const Ci_c = {};\n"
                                      "C<const int> const Cci_c = {};\n"
                                      //"C<int const> const Cic_c = {};\n"
                                      ));
}

TEST(QualifiersOrderTest, Pointers) {
  EXPECT_NO_CHANGES(QualifiersOrder, "const int *cip;\n");
  EXPECT_EQ("const int *cip;\n",
            runCheckOnCode<QualifiersOrder>("int const *cip;\n"));
  EXPECT_NO_CHANGES(QualifiersOrder, "int *const cip = nullptr;\n");
  EXPECT_EQ(
      "int *const ipc = nullptr;\n"
      "const int *const cipc = nullptr;\n"
      "const int *const icpc = nullptr;\n",
      runCheckOnCode<QualifiersOrder>("int *const ipc = nullptr;\n"
                                      "const int *const cipc = nullptr;\n"
                                      "int const *const icpc = nullptr;\n"));
}

//TEST(QualifiersOrderTest, References) {
//  EXPECT_NO_CHANGES(QualifiersOrder, "int i = 0;\n"
//                                     "const int &cir = i;\n");
//  EXPECT_EQ("int i = 0;\n"
//            "const int &icr = i;",
//            runCheckOnCode<QualifiersOrder>("int i = 0;\n"
//                                            "int const &icr = i;"));
//}

// TODO(mkurdej): template <typename T> struct C {}; C<const int> const Ccic; C<int const> const Cicc;

} // namespace test
} // namespace tidy
} // namespace clang
