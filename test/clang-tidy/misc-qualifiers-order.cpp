// RUN: %check_clang_tidy %s misc-qualifiers-order %t --

// CVROrder
const int volatile civ = 0;
int const volatile icv = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const int volatile civ = 0;
int *const volatile __restrict ipcvr = nullptr;

// Basic
int /**/ i;
int *ip;
int &ir = i;
const   int /*const*/ cic_c = 0;
/*const*/ int const   c_ic1 = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: wrong order of qualifiers
// CHECK-FIXES: /*const*/ const int   c_ic1 = 0;
const int ci = 0;
const int /**/ ci_ = 0;
int const /**/ ic_ = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const int /**/ ic_ = 0;
typedef int foo;
const foo cf = 0;
foo const fc = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const foo fc = 0;
typedef int *foop;
const foop cfp = nullptr;
foop const fpc = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const foop fpc = nullptr;
const auto ca = 0;
auto const ac = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const auto ac = 0;

// Arrays
const int constant_array_ci[1] = {0};
int const constant_array_ic[1] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const int constant_array_ic[1] = {0};
const int incomplete_array_ci[] = {0};
int const incomplete_array_ic[] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const int incomplete_array_ic[] = {0};
const int array_size = 1;
const int dependent_sized_array_ci[array_size] = {0};
int const dependent_sized_array_ic[array_size] = {0};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const int dependent_sized_array_ic[array_size] = {0};

template <typename T>
struct S {
  typedef int type;
  template <typename U>
  class D {};
};

const S<int> cSi = {};
const S<const int> cSci = {};
S<int> const Si_c = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const S<int> Si_c = {};
S<const int> const Sci_c = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const S<const int> Sci_c = {};

// Typedefs
typedef int const const_int;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: wrong order of qualifiers
// CHECK-FIXES: typedef const int const_int;
typedef int const *const_int_p;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: wrong order of qualifiers
// CHECK-FIXES: typedef const int *const_int_p;
typedef int const *const const_int_p_const;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: wrong order of qualifiers
// CHECK-FIXES: typedef const int *const const_int_p_const;
typedef S<int> const *const const_Sic_p_const;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: wrong order of qualifiers
// CHECK-FIXES: typedef const S<int> *const const_Sic_p_const;
typedef S<int const> *const Sci_p_const;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: wrong order of qualifiers
// CHECK-FIXES: typedef S<const int > *const Sci_p_const;

// Pointers
const int *cip;
const volatile int *cvip;
volatile const int *vcip;
int const *icp;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const int *icp;
int *const ipc = nullptr;
const int *const cipc = nullptr;
int const *const icpc = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const int *const icpc = nullptr;

// References
const int &cir = i;
const volatile int &cvir = i;
volatile const int &vcir = i;
int const &icr = i;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const int &icr = i;

// Static
static const volatile int scvi = 0;
static const int *scip = nullptr;
volatile const static int vcsi = 0;
static int const *sicp = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: static const int *sicp = nullptr;

// Attributes
const int __cdecl ci_cdeclf();
int const __cdecl ic_cdeclf();
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const int __cdecl ic_cdeclf();

// TemplatePointers
const S<int> *cSi_p = {};
const S<const int> *cSci_p = {};
S<int> const *Si_cp = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const S<int> *Si_cp = {};
S<const int> const *Sci_cp = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const S<const int> *Sci_cp = {};

// TemplateReferences
const S<int> &cSi_r = Si_c;
S<int> const &Si_cr = Si_c;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const S<int> &Si_cr = Si_c;
const S<const int> &cSci_r = Sci_c;
S<const int> const &Sic_cr = Sci_c;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const S<const int> &Sic_cr = Sci_c;

// NestedTypes
const S<int>::type cSi_t = 0;
S<int>::type const Si_tc = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const S<int>::type Si_tc = 0;
const S<const int>::type cSci_c = 0;
S<const int>::type const Sci_tc = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const S<const int>::type Sci_tc = 0;

S<int>::D<float> const cSDf = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const S<int>::D<float> cSDf = {};
S<const int>::D<const float> const cSDfc = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const S<const int>::D<const float> cSDfc = {};

// Namespaces
namespace out {
namespace in {
class C {};
template <typename T>
class TC {};
} // namespace in
} // namespace out
const out::in::C coiC = {};
out::in::C const oiCc = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const out::in::C oiCc = {};

const out::in::TC<int> c_oiTCi = {};
out::in::TC<int> const oiTCi_c = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const out::in::TC<int> oiTCi_c = {};
const out::in::TC<const int> c_oiTCic = {};
out::in::TC<const int> const oiTCic_c = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const out::in::TC<const int> oiTCic_c = {};

// FunctionMethodLambdaArguments
void f(int const i = 0);
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: wrong order of qualifiers
// CHECK-FIXES: void f(const int i = 0);
void f(int const i) {}
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: wrong order of qualifiers
// CHECK-FIXES: void f(const int i) {}
struct FS {
  void f(int const i = 0);
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: wrong order of qualifiers
// CHECK-FIXES: void f(const int i = 0);
};
void FS::f(int const i) {}
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: wrong order of qualifiers
// CHECK-FIXES: void FS::f(const int i) {}
void lambda() {
  auto f = [](int const i) { return 0; };
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: wrong order of qualifiers
// CHECK-FIXES: auto f = [](const int i) { return 0; };
}

// FunctionMethodLambdaReturnType
int const rf();
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-FIXES: const int rf();
struct RS {
  int const f(int const i = 0);
// CHECK-MESSAGES: :[[@LINE-1]]:3: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:15: warning: wrong order of qualifiers
// CHECK-FIXES: const int f(const int i = 0);
};

// TemplateArguments
const S<int const> *cSic_p = {};
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: wrong order of qualifiers
// CHECK-FIXES: const S<const int > *cSic_p = {};
S<int const> const *Sic_cp = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:3: warning: wrong order of qualifiers
// CHECK-FIXES: const S<const int > *Sic_cp = {};

template <typename T1, typename T2> class TC2 {};
template <typename T1, unsigned U1, typename T2, unsigned U2> class TC4 {};

TC2<int const, float const> const *TC2ic_fc_cp = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:5: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-3]]:16: warning: wrong order of qualifiers
// CHECK-FIXES: const TC2<const int , const float > *TC2ic_fc_cp = {};
const TC2<int const , float const > *cTC2ic_fc_p = {};
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:23: warning: wrong order of qualifiers
// CHECK-FIXES: const TC2<const int , const float > *cTC2ic_fc_p = {};

const TC4<const int, 0, const float, 0> *cT42ci_cf_p = {};
TC4<int const, 1, float const, 1> const *T42ic_fc_cp = {};
// CHECK-MESSAGES: :[[@LINE-1]]:1: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:5: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-3]]:19: warning: wrong order of qualifiers
// CHECK-FIXES: const TC4<const int , 1, const float , 1> *T42ic_fc_cp = {};
const TC4<int const, 2, float const, 2> *cTC4ic_fc_p = {};
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: wrong order of qualifiers
// CHECK-MESSAGES: :[[@LINE-2]]:25: warning: wrong order of qualifiers
// CHECK-FIXES: const TC4<const int , 2, const float , 2> *cTC4ic_fc_p = {};


// Casts
void casts(void* p) {
  (const int*)p;
  (int const*)p;
// CHECK-MESSAGES: :[[@LINE-1]]:4: warning: wrong order of qualifiers
// CHECK-FIXES: (const int*)p;

  static_cast<const int*>(p);
  static_cast<int const*>(p);
// CHECK-MESSAGES: :[[@LINE-1]]:14: warning: wrong order of qualifiers
// CHECK-FIXES: static_cast<const int *>(p);

  reinterpret_cast<const int*>(p);
  reinterpret_cast<int const*>(p);
// CHECK-MESSAGES: :[[@LINE-1]]:19: warning: wrong order of qualifiers
// CHECK-FIXES: reinterpret_cast<const int *>(p);

  const_cast<const void*>(p);
  const_cast<void const*>(p);
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: wrong order of qualifiers
// CHECK-FIXES: const_cast<const void *>(p);
}


// Fields
struct F
{
    const int ci = 0;
    const int /**/ ci_ = 0;
    int const /**/ ic_ = 0;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: wrong order of qualifiers
// CHECK-FIXES: const int /**/ ic_ = 0;
};