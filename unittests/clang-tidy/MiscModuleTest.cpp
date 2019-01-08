#include "ClangTidyTest.h"
#include "misc/QualifiersOrder.h"
#include "gtest/gtest.h"

// RUN: echo '{ Checks: "-*,misc-qualifiers-order"}' > %t/.clang-tidy
// RUN: echo '{ Checks: "-*,misc-qualifiers-order", CheckOptions: [{key: misc-qualifiers-order.QualifierAlignment, value: Left}]}' > %t/.clang-tidy

using namespace clang::tidy::misc;

namespace clang {
namespace tidy {
namespace test {

#define QualifiersOrderRight 0

#if !QualifiersOrderRight

TEST(QualifiersOrderTest, CVROrder) {
  EXPECT_NO_CHANGES(QualifiersOrder, "const volatile int i = 0;");
  EXPECT_EQ("const int volatile i = 0;",
            runCheckOnCode<QualifiersOrder>("int const volatile i = 0;"));
  EXPECT_NO_CHANGES(QualifiersOrder,
                    "int *const volatile __restrict i = nullptr;");
}

TEST(QualifiersOrderTest, Basic) {
  EXPECT_NO_CHANGES(QualifiersOrder, "int /**/ i;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int  * ip;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int i; int &ir = i;");
  EXPECT_NO_CHANGES(QualifiersOrder, "/*const*/ const int i = 0;");
  EXPECT_EQ("const   int /*const*/ i = 0;",
            runCheckOnCode<QualifiersOrder>("int /*const*/ const   i = 0;"));
  EXPECT_EQ("__attribute__((unused)) const   int /*const*/ i = 0;",
            runCheckOnCode<QualifiersOrder>(
                "__attribute__((unused)) int /*const*/ const   i = 0;"));
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
}

TEST(QualifiersOrderTest, Static) {
  EXPECT_NO_CHANGES(QualifiersOrder, "static const volatile int i = 0;");
  EXPECT_NO_CHANGES(QualifiersOrder,
                    "static const int f(const volatile int i = 0);");
  EXPECT_NO_CHANGES(QualifiersOrder,
                    "static const int f(const volatile int i = 0) {}");
  EXPECT_EQ(
      "static const int volatile i = 0;",
      runCheckOnCode<QualifiersOrder>("static int const volatile i = 0;"));
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
  // EXPECT_NO_CHANGES(QualifiersOrder, "volatile const int *cip;\n");
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
  EXPECT_NO_CHANGES(QualifiersOrder, "const int &&cir = int();\n");
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
                                     "const C<int> &cCi = Ci;\n");
  EXPECT_NO_CHANGES(QualifiersOrder, "template <typename T> class C {};\n"
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
  EXPECT_EQ("template <typename T> class C { public: typedef int type; };\n"
            "const C<int>::type Ci_c = {};\n"
            "const C<const int>::type Cci_c = {};\n",
            runCheckOnCode<QualifiersOrder>(
                "template <typename T> class C { public: typedef int type; };\n"
                "C<int>::type const Ci_c = {};\n"
                "C<const int>::type const Cci_c = {};\n"));
  EXPECT_EQ(
      "template <typename T> struct C { template <typename U> class D {}; };\n"
      "const C<int>::D<float> CDc = {};\n"
      "const C<const int>::D<const float> cCD = {};\n",
      runCheckOnCode<QualifiersOrder>(
          "template <typename T> struct C { template <typename U> class D {}; };\n"
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
            runCheckOnCode<QualifiersOrder>("namespace out { namespace in {\n"
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
  // FIXME: Put const after attribute.
  EXPECT_EQ(
      "const int __attribute__((unused)) f() __attribute__((unused));",
      runCheckOnCode<QualifiersOrder>(
          "int const __attribute__((unused)) f() __attribute__((unused));"));
  EXPECT_EQ("const int __cdecl f() __attribute__((unused));",
            runCheckOnCode<QualifiersOrder>(
                "int const __cdecl f() __attribute__((unused));"));
  EXPECT_EQ("struct S {\n"
            "const int f(const int i = 0);\n"
            "};\n",
            runCheckOnCode<QualifiersOrder>("struct S {\n"
                                            "int const f(int const i = 0);\n"
                                            "};\n"));
  EXPECT_EQ("const int (parenFunction)();",
            runCheckOnCode<QualifiersOrder>("int const (parenFunction)();"));
  EXPECT_EQ(
      "typedef const int TypedefFunction();\n"
      "TypedefFunction f;\n",
      runCheckOnCode<QualifiersOrder>("typedef int const TypedefFunction();\n"
                                      "TypedefFunction f;\n"));
}

TEST(QualifiersOrderTest, TemplateArguments) {
  EXPECT_NO_CHANGES(QualifiersOrder, "template <typename T = int> class C {};\n"
                                     "C<> c;\n");
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "const C<const int> *cCic = {};\n"
      "const C<const int> *Cic_c = {};\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "const C<const int> *cCic = {};\n"
                                      "C<const int> const *Cic_c = {};\n"));
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "const C<const int > *cCic = {};\n"
      "const C<const int > *Cic_c = {};\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "const C<int const> *cCic = {};\n"
                                      "C<int const> const *Cic_c = {};\n"));
  EXPECT_EQ("template <typename T1, typename T2> class C {};\n"
            "const C<const int , const float > *cCic = {};\n"
            "const C<const int , const float > *Cic_c = {};\n",
            runCheckOnCode<QualifiersOrder>(
                "template <typename T1, typename T2> class C {};\n"
                "const C<int const, float const> *cCic = {};\n"
                "C<int const, float const> const *Cic_c = {};\n"));
  EXPECT_EQ("template <typename T1, unsigned U1, "
            "          typename T2, unsigned U2>\n"
            "class C {};\n"
            "const C<const int , 0, const float , 0> *cCic = {};\n"
            "const C<const int , 0, const float , 0> *Cic_c = {};\n",
            runCheckOnCode<QualifiersOrder>(
                "template <typename T1, unsigned U1, "
                "          typename T2, unsigned U2>\n"
                "class C {};\n"
                "const C<int const, 0, float const, 0> *cCic = {};\n"
                "C<int const, 0, float const, 0> const *Cic_c = {};\n"));
}

TEST(QualifiersOrderTest, Macros) {
  EXPECT_NO_CHANGES(
      QualifiersOrder,
      "#define VARIABLE const volatile int i = 0;\n"
      "#define FUNCTION_DECL int const f(const volatile int i = 0);\n"
      "#define FUNCTION_DEF int const f(const volatile int i) { return i; }\n"
      "#define TYPEDEF typedef int const const_int;\n"
      "#define TEMPLATE_DEF template <typename T1, typename T2,"
      "                               unsigned U1, typename T3>"
      "                     struct S {};\n"
      "#define TEMPLATE_SPEC"
      "     S<int const, const volatile float, 0, S<int, int, 1, int>> s;"
      "\n"
      "VARIABLE\n"
      "FUNCTION_DECL\n"
      "FUNCTION_DEF\n"
      "TYPEDEF\n"
      "TEMPLATE_DEF\n"
      "TEMPLATE_SPEC\n");
  EXPECT_NO_CHANGES(
      QualifiersOrder,
      "#define CONST const\n"
      "\n"
      "#define VARIABLE CONST volatile int i = 0;\n"
      "#define FUNCTION_DECL int CONST f(CONST volatile int i = 0);\n"
      "#define FUNCTION_DEF int CONST f(CONST volatile int i) { return i; }\n"
      "#define TYPEDEF typedef int CONST CONST_int;\n"
      "#define TEMPLATE_DEF template <typename T1, typename T2,"
      "                               unsigned U1, typename T3>"
      "                     struct S {};\n"
      "#define TEMPLATE_SPEC"
      "     S<int CONST, CONST volatile float, 0, S<int, int, 1, int>> s;"
      "\n"
      "VARIABLE\n"
      "FUNCTION_DECL\n"
      "FUNCTION_DEF\n"
      "TYPEDEF\n"
      "TEMPLATE_DEF\n"
      "TEMPLATE_SPEC\n");
}

#else

TEST(QualifiersOrderTest, CVROrder) {
  EXPECT_NO_CHANGES(QualifiersOrder, "int const volatile i = 0;");
  EXPECT_EQ("int const volatile i = 0;",
            runCheckOnCode<QualifiersOrder>("const int volatile i = 0;"));
  EXPECT_NO_CHANGES(QualifiersOrder, "int *const volatile __restrict i = nullptr;");
}

TEST(QualifiersOrderTest, Basic) {
  EXPECT_NO_CHANGES(QualifiersOrder, "int /**/ i;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int  * ip;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int i; int &ir = i;");
  EXPECT_NO_CHANGES(QualifiersOrder, "/*const*/ int const i = 0;");
  EXPECT_EQ("int const   /*const*/ i = 0;",
            runCheckOnCode<QualifiersOrder>("const   int /*const*/ i = 0;"));
  EXPECT_NO_CHANGES(QualifiersOrder, "int const ci = 0;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int const /**/ ci = 0;");
#if 0
  // This check does not work for macros yet.
  EXPECT_NO_CHANGES(QualifiersOrder, "#define CONST const\n"
                                     "int CONST /**/ ci = 0;");
  EXPECT_EQ("#define CONST const\n"
            "int CONST /**/ ci = 0;",
            runCheckOnCode<QualifiersOrder>("#define CONST const\n"
                                            "CONST int /**/ ci = 0;"));
#endif
  EXPECT_EQ("int const ic = 0;\n",
            runCheckOnCode<QualifiersOrder>("const int ic = 0;\n"));
  EXPECT_EQ("typedef int foo;\n"
            "foo const ci = 0;\n"
            "foo const ic = 0;\n",
            runCheckOnCode<QualifiersOrder>("typedef int foo;\n"
                                            "const foo ci = 0;\n"
                                            "foo const ic = 0;\n"));
  EXPECT_EQ("typedef int *foo;\n"
            "foo const ci = nullptr;\n"
            "foo const ic = nullptr;\n",
            runCheckOnCode<QualifiersOrder>("typedef int *foo;\n"
                                            "const foo ci = nullptr;\n"
                                            "foo const ic = nullptr;\n"));
  EXPECT_EQ("auto const ci = 0;\n"
            "auto const ic = 0;\n",
            runCheckOnCode<QualifiersOrder>("const auto ci = 0;\n"
                                            "auto const ic = 0;\n"));
}

TEST(QualifiersOrderTest, Typedefs) {
  EXPECT_NO_CHANGES(QualifiersOrder, "typedef int const foo;");
  EXPECT_EQ("typedef int const foo;",
            runCheckOnCode<QualifiersOrder>("typedef const int foo;"));
  EXPECT_EQ("typedef int const *foo;",
            runCheckOnCode<QualifiersOrder>("typedef const int *foo;"));
  EXPECT_EQ("typedef int const *const foo;",
            runCheckOnCode<QualifiersOrder>("typedef const int *const foo;"));
  EXPECT_EQ("template <typename T> struct S {};\n"
            "typedef S<int const > const *const foo;\n",
            runCheckOnCode<QualifiersOrder>(
                "template <typename T> struct S {};\n"
                "typedef const S<const int> *const foo;\n"));
}

TEST(QualifiersOrderTest, TemplatesBasic) {
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "C<int> const cCi = {};\n"
      "C<int const> const cCci = {};\n"
      "C<int> const Ci_c = {};\n"
      "C<int const> const Cci_c = {};\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "const C<int> cCi = {};\n"
                                      "const C<int const> cCci = {};\n"
                                      "C<int> const Ci_c = {};\n"
                                      "C<int const> const Cci_c = {};\n"));
}

TEST(QualifiersOrderTest, Pointers) {
  EXPECT_NO_CHANGES(QualifiersOrder, "int const *cip;\n");
  EXPECT_NO_CHANGES(QualifiersOrder, "int const volatile *cip;\n");
  //EXPECT_NO_CHANGES(QualifiersOrder, "int volatile const *cip;\n");
  EXPECT_EQ("int const *cip;\n",
            runCheckOnCode<QualifiersOrder>("const int *cip;\n"));
  EXPECT_NO_CHANGES(QualifiersOrder, "int const *cip = nullptr;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int *const ipc = nullptr;");
  EXPECT_NO_CHANGES(QualifiersOrder, "int const *const cipc = nullptr;");
  EXPECT_EQ(
      "int const *const icpc = nullptr;",
      runCheckOnCode<QualifiersOrder>("const int *const icpc = nullptr;"));
}

TEST(QualifiersOrderTest, References) {
  EXPECT_NO_CHANGES(QualifiersOrder, "int i = 0;\n"
                                     "int const &cir = i;\n");
  EXPECT_EQ("int i = 0;\n"
            "int const &icr = i;",
            runCheckOnCode<QualifiersOrder>("int i = 0;\n"
                                            "const int &icr = i;"));
}

TEST(QualifiersOrderTest, TemplatePointers) {
  EXPECT_NO_CHANGES(QualifiersOrder, "template <typename T> class C {};\n"
                                     "C<int> const *cCi = {};\n"
                                     "C<int const> const *cCci = {};\n");
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "C<int> const *Ci_c = {};\n"
      "C<int const> const *Cci_c = {};\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "const C<int> *Ci_c = {};\n"
                                      "const C<int const> *Cci_c = {};\n"));
}

TEST(QualifiersOrderTest, TemplateReferences) {
  EXPECT_NO_CHANGES(QualifiersOrder, "template <typename T> class C {};\n"
                                     "C<int> Ci = {};\n"
                                     "C<int> const &cCi = C;\n");
  EXPECT_NO_CHANGES(QualifiersOrder, "template <typename T> class C {};\n"
                                     "C<int const> Cci = {};\n"
                                     "C<int const> const &cCci = Cci;\n");
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "C<int> Ci = {};\n"
      "C<int> const &Ci_c = Ci;\n"
      "C<int const > Cci = {};\n"
      "C<int const > const &Cci_c = Cci;\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "C<int> Ci = {};\n"
                                      "const C<int> &Ci_c = Ci;\n"
                                      "C<const int> Cci = {};\n"
                                      "const C<const int> &Cci_c = Cci;\n"));
}

TEST(QualifiersOrderTest, NestedTypes) {
  EXPECT_EQ("template <typename T> class C { typedef int type; };\n"
            "C<int>::type const Ci_c = Ci;\n"
            "C<int const>::type const Cci_c = Cci;\n",
            runCheckOnCode<QualifiersOrder>(
                "template <typename T> class C { typedef int type; };\n"
                "const C<int>::type Ci_c = Ci;\n"
                "const C<int const>::type Cci_c = Cci;\n"));
  EXPECT_EQ(
      "template <typename T> struct C { template <typename U> class D; };\n"
      "C<int>::D<float> const CDc = {};\n"
      "C<int const>::D<float const> const cCD = {};\n",
      runCheckOnCode<QualifiersOrder>(
          "template <typename T> struct C { template <typename U> class D; };\n"
          "const C<int>::D<float> CDc = {};\n"
          "const C<int const>::D<float const> cCD = {};\n"));
}

TEST(QualifiersOrderTest, Namespaces) {
  EXPECT_EQ("namespace out { namespace in {\n"
            "class C {};\n"
            "} // namespace in\n"
            "} // namespace out\n"
            "out::in::C const cC = {};\n"
            "out::in::C const Cc = {};\n",
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
            "out::in::C<int> const Ci_c = {};\n"
            "out::in::C<int const> const Cci_c = {};\n",
            runCheckOnCode<QualifiersOrder>(
                "namespace out { namespace in {\n"
                "template <typename T> class C {};\n"
                "} // namespace in\n"
                "} // namespace out\n"
                "const out::in::C<int> Ci_c = {};\n"
                "const out::in::C<int const> Cci_c = {};\n"));
}

TEST(QualifiersOrderTest, FunctionMethodLambdaArguments) {
  EXPECT_EQ("void f(int const i = 0);",
            runCheckOnCode<QualifiersOrder>("void f(const int i = 0);"));
  EXPECT_EQ("void f(int const i = 0) {}",
            runCheckOnCode<QualifiersOrder>("void f(const int i = 0) {}"));
  EXPECT_EQ("struct S {\n"
            "void f(int const i = 0);\n"
            "};\n",
            runCheckOnCode<QualifiersOrder>("struct S {\n"
                                            "void f(const int i = 0);\n"
                                            "};\n"));
  EXPECT_EQ("int main() {\n"
            "auto f = [](int const i) { return 0; };\n"
            "}\n",
            runCheckOnCode<QualifiersOrder>(
                "int main() {\n"
                "auto f = [](const int i) { return 0; };\n"
                "}\n"));
}

TEST(QualifiersOrderTest, FunctionMethodLambdaReturnType) {
  EXPECT_EQ("int const f();",
            runCheckOnCode<QualifiersOrder>("const int f();"));
  EXPECT_EQ("struct S {\n"
            "int const f(int const i = 0);\n"
            "};\n",
            runCheckOnCode<QualifiersOrder>("struct S {\n"
                                            "const int f(const int i = 0);\n"
                                            "};\n"));
}

TEST(QualifiersOrderTest, TemplateArguments) {
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "C<int const> const *cCic = {};\n"
      "C<int const> const *Cic_c = {};\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "const C<int const> *cCic = {};\n"
                                      "C<int const> const *Cic_c = {};\n"));
  EXPECT_EQ(
      "template <typename T> class C {};\n"
      "C<int const > const *cCic = {};\n"
      "C<int const > const *Cic_c = {};\n",
      runCheckOnCode<QualifiersOrder>("template <typename T> class C {};\n"
                                      "const C<const int> *cCic = {};\n"
                                      "C<const int> const *Cic_c = {};\n"));
  EXPECT_EQ("template <typename T1, typename T2> class C {};\n"
            "C<int const , float const > const *cCic = {};\n"
            "C<int const , float const > const *Cic_c = {};\n",
            runCheckOnCode<QualifiersOrder>(
                "template <typename T1, typename T2> class C {};\n"
                "const C<const int, const float> *cCic = {};\n"
                "C<const int, const float> const *Cic_c = {};\n"));
}

#endif

} // namespace test
} // namespace tidy
} // namespace clang
