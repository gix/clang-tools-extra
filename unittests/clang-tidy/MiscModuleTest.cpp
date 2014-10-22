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

TEST(QualifiersOrderTest, CVROrder) {
  EXPECT_NO_CHANGES(QualifiersOrder, "const volatile int i = 0;");
  //EXPECT_NO_CHANGES(QualifiersOrder, "int const volatile i = 0;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int *const volatile __restrict i = nullptr;");
}

TEST(QualifiersOrderTest, Basic) {
  EXPECT_NO_CHANGES(QualifiersOrder, "int /**/ i;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int  * ip;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int i; int &ir = i;");
  EXPECT_NO_CHANGES(QualifiersOrder, "/*const*/ const int i = 0;");
  EXPECT_EQ("const   int /*const*/ i = 0;",
            runCheckOnCode<QualifiersOrder>("int /*const*/ const   i = 0;"));
  EXPECT_NO_CHANGES(QualifiersOrder, "const int ci = 0;");
  EXPECT_NO_CHANGES(QualifiersOrder, "const /**/ int ci = 0;");
#if 0
  // This check does not work for macros yet.
  EXPECT_NO_CHANGES(QualifiersOrder, "#define CONST const\n"
                                     "CONST /**/ int ci = 0;");
  EXPECT_EQ("#define CONST const\n"
            "CONST int /**/ ci = 0;",
            runCheckOnCode<QualifiersOrder>("#define CONST const\n"
                                            "int CONST /**/ ci = 0;"));
#endif
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
}

TEST(QualifiersOrderTest, TemplatesBasic) {
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "const C<int> cCi = {};\n"
      "const C<const int> cCci = {};\n"
      "const C<int> Ci_c = {};\n"
      "const C<const int> Cci_c = {};\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "const C<int> cCi = {};\n"
                                      "const C<const int> cCci = {};\n"
                                      "C<int> const Ci_c = {};\n"
                                      "C<const int> const Cci_c = {};\n"));
}

TEST(QualifiersOrderTest, Pointers) {
  EXPECT_NO_CHANGES(QualifiersOrder, "const int *cip;\n");
  EXPECT_NO_CHANGES(QualifiersOrder, "const volatile int *cip;\n");
  //EXPECT_NO_CHANGES(QualifiersOrder, "volatile const int *cip;\n");
  EXPECT_EQ("const int *cip;\n",
            runCheckOnCode<QualifiersOrder>("int const *cip;\n"));
  EXPECT_NO_CHANGES(QualifiersOrder, "const int *cip = nullptr;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int *const ipc = nullptr;");
  EXPECT_NO_CHANGES(QualifiersOrder, "const int *const cipc = nullptr;");
  EXPECT_EQ(
      "const int *const icpc = nullptr;",
      runCheckOnCode<QualifiersOrder>("int const *const icpc = nullptr;"));
}

TEST(QualifiersOrderTest, References) {
  EXPECT_NO_CHANGES(QualifiersOrder, "int i = 0;\n"
                                     "const int &cir = i;\n");
  EXPECT_EQ("int i = 0;\n"
            "const int &icr = i;",
            runCheckOnCode<QualifiersOrder>("int i = 0;\n"
                                            "int const &icr = i;"));
}

TEST(QualifiersOrderTest, TemplatePointers) {
  EXPECT_NO_CHANGES(QualifiersOrder, "template <typename T> class C {};\n"
                                     "const C<int> *cCi = {};\n"
                                     "const C<const int> *cCci = {};\n");
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "const C<int> *Ci_c = {};\n"
      "const C<const int> *Cci_c = {};\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "C<int> const *Ci_c = {};\n"
                                      "C<const int> const *Cci_c = {};\n"));
}

TEST(QualifiersOrderTest, TemplateReferences) {
  EXPECT_NO_CHANGES(QualifiersOrder, "template <typename T> class C {};\n"
                                     "C<int> Ci = {};\n"
                                     "const C<int> &cCi = C;\n"
                                     "C<const int> Cci = {};\n"
                                     "const C<const int> &cCci = Cci;\n");
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "C<int> Ci = {};\n"
      "const C<int> &Ci_c = Ci;\n"
      "C<const int> Cci = {};\n"
      "const C<const int> &Cci_c = Cci;\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "C<int> Ci = {};\n"
                                      "C<int> const &Ci_c = Ci;\n"
                                      "C<const int> Cci = {};\n"
                                      "C<const int> const &Cci_c = Cci;\n"));
}

TEST(QualifiersOrderTest, NestedTypes) {
  EXPECT_EQ("template <typename T> class C { typedef int type; };\n"
            "const C<int>::type Ci_c = Ci;\n"
            "const C<const int>::type Cci_c = Cci;\n",
            runCheckOnCode<QualifiersOrder>(
                "template <typename T> class C { typedef int type; };\n"
                "C<int>::type const Ci_c = Ci;\n"
                "C<const int>::type const Cci_c = Cci;\n"));
  EXPECT_EQ(
      "template <typename T> struct C { template <typename U> class D; };\n"
      "const C<int>::D<float> CDc = {};\n"
      "const C<const int>::D<const float> cCD = {};\n",
      runCheckOnCode<QualifiersOrder>(
          "template <typename T> struct C { template <typename U> class D; };\n"
          "C<int>::D<float> const CDc = {};\n"
          "C<const int>::D<const float> const cCD = {};\n"));
}

TEST(QualifiersOrderTest, Namespaces) {
  EXPECT_EQ("namespace out { namespace in {\n"
            "class C {};\n"
            "} // namespace in\n"
            "} // namespace out\n"
            "const out::in::C cC = {};\n"
            "const out::in::C Cc = {};\n",
            runCheckOnCode<QualifiersOrder>(
                "namespace out { namespace in {\n"
                "class C {};\n"
                "} // namespace in\n"
                "} // namespace out\n"
                "const out::in::C cC = {};\n"
                "out::in::C const Cc = {};\n"));
  EXPECT_EQ("namespace out { namespace in {\n"
            "template <typename T> class C {};\n"
            "} // namespace in\n"
            "} // namespace out\n"
            "const out::in::C<int> Ci_c = {};\n"
            "const out::in::C<const int> Cci_c = {};\n",
            runCheckOnCode<QualifiersOrder>(
                "namespace out { namespace in {\n"
                "template <typename T> class C {};\n"
                "} // namespace in\n"
                "} // namespace out\n"
                "out::in::C<int> const Ci_c = {};\n"
                "out::in::C<const int> const Cci_c = {};\n"));
}

TEST(QualifiersOrderTest, FunctionMethodLambdaArguments) {
  EXPECT_EQ("void f(const int i = 0);",
            runCheckOnCode<QualifiersOrder>("void f(int const i = 0);"));
  EXPECT_EQ("void f(const int i = 0) {}",
            runCheckOnCode<QualifiersOrder>("void f(int const i = 0) {}"));
  EXPECT_EQ("struct S {\n"
            "void f(const int i = 0);\n"
            "};\n",
            runCheckOnCode<QualifiersOrder>("struct S {\n"
                                            "void f(int const i = 0);\n"
                                            "};\n"));
  EXPECT_EQ("int main() {\n"
            "auto f = [](const int i) { return 0; };\n"
            "}\n",
            runCheckOnCode<QualifiersOrder>(
                "int main() {\n"
                "auto f = [](int const i) { return 0; };\n"
                "}\n"));
}

TEST(QualifiersOrderTest, FunctionMethodLambdaReturnType) {
  EXPECT_EQ("const int f();",
            runCheckOnCode<QualifiersOrder>("int const f();"));
}

/*
TEST(QualifiersOrderTest, TemplateArguments) {
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "const C<const int> *cCic = {};\n"
      "const C<const int> *Cic_c = {};\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "const C<int const> *cCic = {};\n"
                                      "C<int const> const *Cic_c = {};\n"));
}
*/

// TODO(mkurdej): function return type, lambda arguments

} // namespace test
} // namespace tidy
} // namespace clang
