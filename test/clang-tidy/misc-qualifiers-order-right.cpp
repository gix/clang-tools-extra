// RUN: $(dirname %s)/check_clang_tidy_fix.sh %s misc-qualifiers-order %t -config="{CheckOptions: [{key: misc-qualifiers-order.QualifierAlignment, value: Right}]}" --
// REQUIRES: shell

// CVROrder
int const volatile icv = 0;
const int volatile civ = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: int const volatile civ = 0;
int *const volatile __restrict ipcvr = nullptr;

// Basic
int /**/ i;
int *ip;
int &ir = i;
/*const*/ int const c_ic1 = 0;
const   int /*const*/ cic_c = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: int const   /*const*/ cic_c = 0;
int const ci = 0;
int const /**/ ci_ = 0;
const int ic = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: int const ic = 0;
typedef int foo;
foo const fc = 0;
const foo cf = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: foo const fc = 0;
typedef int *foop;
foop const fpc = nullptr;
const foop cfp = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: foop const cfp = nullptr;
auto const ac = 0;
const auto ca = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: auto const ca = 0;

template <typename T>
struct S {
  typedef int type;
  template <typename U>
  class D {};
};

S<int> const Si_c = {};
S<int const> const Sic_c = {};
const S<int> cSi = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: S<int> const cSi = {};
const S<int const> cSic = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: S<int const> const cSic = {};

// Typedefs
typedef const int const_int;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: wrong order of qualifiers
// CHECK-FIXES: typedef int const const_int;
typedef const int *const_int_p;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: wrong order of qualifiers
// CHECK-FIXES: typedef int const *const_int_p;
typedef const int *const const_int_p_const;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: wrong order of qualifiers
// CHECK-FIXES: typedef int const *const const_int_p_const;
typedef const S<int> *const const_Sic_p_const;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: wrong order of qualifiers
// CHECK-FIXES: typedef S<int> const *const const_Sic_p_const;
typedef S<const int> *const Sci_p_const;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: wrong order of qualifiers
// CHECK-FIXES: typedef S<int const > *const Sci_p_const;

// Pointers
int const *icp;
int const volatile *icvp;
int volatile const *ivcp;
const int *cip;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: int const *cip;
int *const ipc = nullptr;
int const *const icpc = nullptr;
const int *const cipc = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: int const *const cipc = nullptr;

// References
int const &icr = i;
int const volatile &icvr = i;
int volatile const &ivcr = i;
const int &cir = i;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: int const &cir = i;

// Static
static volatile int const svic = 0;
static int const *sicp = nullptr;
volatile static int const vsic = 0;
static const int *scip = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: static int const *scip = nullptr;

// Attributes
int const __cdecl ic_cdeclf();
const int __cdecl ci_cdeclf();
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: int const __cdecl ci_cdeclf();

// TemplatePointers
S<int> const *Si_cp = {};
S<int const> const *cSic_cp = {};
const S<int> *cSi_p = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: S<int> const *cSi_p = {};
const S<int const> *cSic_p = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: S<int const> const *cSic_p = {};

// TemplateReferences
S<int> const &Si_cr = Si_c;
const S<int> &cSi_r = Si_c;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: S<int> const &cSi_r = Si_c;
S<int const> const &Sic_cr = Sic_c;
const S<int const> &cSic_r = Sic_c;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: S<int const> const &cSic_r = Sic_c;

// NestedTypes
S<int>::type const Si_tc = 0;
const S<int>::type cSi_t = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: S<int>::type const cSi_t = 0;
S<int const>::type const Sci_tc = 0;
const S<int const>::type cSci_c = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: S<int const>::type const cSci_c = 0;

const S<int>::D<float> cSDf = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: S<int>::D<float> const cSDf = {};
const S<int const>::D<float const> cSDfc = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: S<int const>::D<float const> const cSDfc = {};

// Namespaces
namespace out {
namespace in {
class C {};
template <typename T>
class TC {};
} // namespace in
} // namespace out
out::in::C const oiCc = {};
const out::in::C coiC = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: out::in::C const coiC = {};

out::in::TC<int> const oiTCi_c = {};
const out::in::TC<int> c_oiTCi = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: out::in::TC<int> const c_oiTCi = {};
out::in::TC<int const> const oiTCic_c = {};
const out::in::TC<int const> c_oiTCic = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: out::in::TC<int const> const c_oiTCic = {};

// FunctionMethodLambdaArguments
void f(const int i = 0);
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: wrong order of qualifiers
// CHECK-FIXES: void f(int const i = 0);
void f(const int i) {}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: wrong order of qualifiers
// CHECK-FIXES: void f(int const i) {}
struct FS {
  void f(const int i = 0);
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: wrong order of qualifiers
// CHECK-FIXES: void f(int const i = 0);
};
void FS::f(const int i) {}
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: wrong order of qualifiers
// CHECK-FIXES: void FS::f(int const i) {}
void lambda() {
  auto f = [](const int i) { return 0; };
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: wrong order of qualifiers
// CHECK-FIXES: auto f = [](int const i) { return 0; };
}

// FunctionMethodLambdaReturnType
const int rf();
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: int const rf();
struct RS {
  const int f(const int i = 0);
// CHECK-MESSAGES: :[[@LINE-1]]:3: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:15: warning: wrong order of qualifiers
// CHECK-FIXES: int const f(int const i = 0);
};

// TemplateArguments
S<int const> const *Sic_cp = {};
S<const int> const *Sci_cp = {};
// CHECK-MESSAGES: :[[@LINE-1]]:3: warning: wrong order of qualifiers
// CHECK-FIXES: S<int const > const *Sci_cp = {};
const S<const int> *cSci_p = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:9: warning: wrong order of qualifiers
// CHECK-FIXES: S<int const > const *cSci_p = {};

template <typename T1, typename T2> class TC2 {};
template <typename T1, unsigned U1, typename T2, unsigned U2> class TC4 {};

const TC2<const int, const float> *cTC2ci_cf_p = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:11: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-3]]:22: warning: wrong order of qualifiers
// CHECK-FIXES: TC2<int const , float const > const *cTC2ci_cf_p = {};
TC2<const int, const float> const *TC2ci_cf_cp = {};
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:16: warning: wrong order of qualifiers
// CHECK-FIXES: TC2<int const , float const > const *TC2ci_cf_cp = {};

TC4<int const, 0, float const, 0> const* TC4ic_fc_cp = {};
const TC4<const int, 1, const float, 1> *cT42ci_cf_p = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:11: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-3]]:25: warning: wrong order of qualifiers
// CHECK-FIXES: TC4<int const , 1, float const , 1> const *cT42ci_cf_p = {};
TC4<const int, 2, const float, 2> const *TC4ci_cf_cp = {};
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:19: warning: wrong order of qualifiers
// CHECK-FIXES: TC4<int const , 2, float const , 2> const *TC4ci_cf_cp = {};
