# Making SIMD Actually Happen in C++ with MSVC and Visual Studio

## Course purpose

This is a focused, implementation-oriented course on one problem:

> **Given a particular C++ loop, how do I get MSVC to execute its bulk work with SIMD instructions, and how do I prove that it actually did so?**

This is not a general introduction to SIMD. The learner already understands SIMD at the conceptual level and understands why independent loop iterations are candidates for parallel execution.

The course instead treats SIMD generation as a compiler-debugging problem:

```text
C++ loop
   ↓
What prevents MSVC from vectorizing it?
   ↓
Choose the matching intervention
   ↓
Compile
   ↓
Read vectorizer diagnostics
   ↓
Inspect generated machine code
   ↓
Confirm SIMD execution
```

The central distinction throughout the course is:

```text
SIMD-friendly source
        ≠
compiler hint
        ≠
compiler assumption
        ≠
target ISA availability
        ≠
auto-vectorization decision
        ≠
explicit SIMD operation
        ≠
proof of emitted SIMD machine code
```

The practical target used throughout is x64 MSVC with AVX2 selected as the reference ISA:

```text
/O2 /arch:AVX2 /Qvec-report:2
```

Current MSVC also supports newer `/arch` targets, including AVX-512 and AVX10.x, but this course intentionally uses AVX2 so that the compiler-controlled versus explicit-SIMD distinction remains concrete and consistent. Microsoft documents `/arch:AVX2` as enabling AVX2 for x64 code generation; it does not tell the compiler to vectorize every suitable loop. [Microsoft Learn — `/arch (x64)`](https://learn.microsoft.com/en-us/cpp/build/reference/arch-x64?view=msvc-170)

---

# How to use the course

Do not approach the examples as code that should merely be made faster. Approach them as controlled compiler experiments.

For each nontrivial example, record:

```text
1. Original source
2. Compiler options
3. /Qvec-report:2 output
4. The reported obstacle, if any
5. Exactly one intervention
6. New /Qvec-report:2 output
7. Disassembly of the target loop
8. Correctness result
9. Benchmark result
```

The rule “one intervention at a time” is important. If the baseline reports aliasing and you simultaneously add `__restrict`, `__assume`, `ivdep`, `/Ob3`, and `/fp:fast`, you have learned almost nothing about causality.

The compiler report is evidence about the vectorizer's decision. The disassembly is evidence about the generated program. A benchmark tells you whether the resulting program is useful for the chosen workload. These are different questions.

---

# PART I — What “Ensuring SIMD” Actually Means

## 1. The five control levels

For this course, five practical levels are enough.

### Level 1 — SIMD-friendly source

```cpp
void add(const float* a, const float* b, float* out, int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

The source exposes a regular loop with unit-stride accesses and elementwise arithmetic.

That is a **candidate**, not a guarantee.

### Level 2 — Give the compiler more freedom or information

Examples:

```cpp
#pragma loop(ivdep)
for (int i = 0; i < n; ++i)
    out[i] = a[i] + b[i];
```

and:

```cpp
void add(float* __restrict a,
         float* __restrict b,
         float* __restrict out,
         int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

These affect what the optimizer may assume or what analysis it performs. They still do not contain a direct instruction saying “emit an AVX2 loop.”

### Level 3 — Select the target instruction set

```text
/arch:AVX2
```

This changes the machine-instruction set available to code generation. A compiler cannot emit AVX2 instructions for a target that was not selected as permitting them.

But:

```text
/arch:AVX2
```

still does not mean:

```text
every vectorizable loop → AVX2
```

### Level 4 — Explicit SIMD operations

```cpp
#include <immintrin.h>

const __m256 x = _mm256_loadu_ps(a);
const __m256 y = _mm256_loadu_ps(b);
const __m256 z = _mm256_add_ps(x, y);
_mm256_storeu_ps(out, z);
```

The source now explicitly asks for a 256-bit packed floating-point operation.

This is the strongest source-level mechanism in the course for controlling SIMD computation.

### Level 5 — Verify the binary

The final engineering question is empirical:

```text
What did the compiler actually emit?
```

For auto-vectorization, use the vectorizer report and disassembly. For explicit intrinsics, disassembly remains the final way to verify the generated machine code.

---

## 2. Permission, information, optimization, and instruction generation

Use this mental model:

```text
                      target information
                            │
                         /arch
                            │
                            ▼
source ───────► optimizer / vectorizer ───────► machine code
  │                    ▲      ▲
  │                    │      │
  │               hints       │ assumptions
  │            ivdep, etc.    │ restrict, alignment,
  │                           │ __assume
  │                           │
  └────────── explicit intrinsics bypass the
              “discover SIMD from scalar loop” problem
              for the operations they express
```

The important distinction is between four statements:

```text
“I allow AVX2.”
“I tell the compiler a fact.”
“I encourage a transformation.”
“I explicitly write a vector operation.”
```

Those statements are not interchangeable.

---

## 3. Why `/O2 + /arch:AVX2` is not a SIMD switch

Consider:

```cpp
void saxpy(const float* a,
           const float* b,
           float* out,
           int n,
           float c)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + c * b[i];
}
```

With:

```text
/O2 /arch:AVX2
```

MSVC has permission to generate optimized AVX2 code, but still has to decide whether the loop is legal and profitable to transform.

It may need to reason about:

```text
aliasing
loop-carried dependencies
memory access geometry
control flow
calls
floating-point semantics
trip count
instruction support
cost
```

Therefore the course never uses “the build has `/arch:AVX2`” as evidence that the target loop is SIMD.

---

# PART II — The MSVC Vectorizer as a Debugging Target

## 4. Start with diagnostics, not modifications

The central diagnostic switch is:

```text
/Qvec-report:2
```

Microsoft documents level 2 as reporting vectorized loops and non-vectorized loops with a reason code. The option is a reporting facility; it does not itself enable vectorization. [Microsoft Learn — `/Qvec-report`](https://learn.microsoft.com/en-us/cpp/build/reference/qvec-report-auto-vectorizer-reporting-level?view=msvc-170)

In Visual Studio, this can be supplied through:

```text
Project Properties
→ Configuration Properties
→ C/C++
→ Code Generation
→ Additional Options
→ /Qvec-report:2
```

For a command-line experiment:

```text
cl /O2 /arch:AVX2 /Qvec-report:2 file.cpp
```

The exact command line used by the project should always be checked in Build Output. Project properties can differ between Debug/Release, x64/Win32, or different configurations.

---

## 5. Reason codes are hypotheses about the failed transformation

Typical MSVC vectorizer diagnostics include categories such as:

```text
1200  loop-carried data dependency
1203  non-contiguous memory access
1301  loop stride not +1
1303  too few iterations
1100  control-flow issue
1101  unsupported conversion
1102  non-vectorizable operation
1103  variable shift issue
1105  reduction-related issue
1500–1505 aliasing / runtime disambiguation issues
```

Treat the reason code as the start of an investigation, not as a replacement for understanding the loop.

The correct workflow is:

```text
reported obstacle
        ↓
look at the source
        ↓
prove that the obstacle is real
        ↓
choose the narrowest valid intervention
```

Do not apply an intervention merely because its name sounds relevant.

---

## 6. Legality versus profitability

The vectorizer needs both.

### Legality

The transformed vector loop must preserve the required program semantics.

Typical legality questions:

```text
Can iterations overlap through memory?
Is there a true recurrence?
Are the addresses representable by the vector operation?
Can the operation be expressed with available instructions?
Are floating-point reorderings permitted?
```

### Profitability

Even if SIMD is legal, the compiler can decide that the transformation is not worthwhile.

For example:

```cpp
void tiny(float* a, const float* b)
{
    for (int i = 0; i < 3; ++i)
        a[i] += b[i];
}
```

There is little work to amortize setup, loop control, and any required remainder handling.

The lesson is important:

```text
“vectorizable”
    and
“compiler chooses vectorization”
```

are distinct propositions.

---

# PART III — The Baseline MSVC Build

## 7. `/O2`

Syntax:

```text
/O2
```

Purpose: selects the speed-oriented optimization configuration.

Visual Studio:

```text
Project Properties
→ Configuration Properties
→ C/C++
→ Optimization
→ Optimization
→ Maximize Speed (/O2)
```

Microsoft describes `/O2` as a combination of optimizations aimed at fast code. [Microsoft Learn — `/O` options](https://learn.microsoft.com/en-us/cpp/build/reference/o-options-optimize-code?view=msvc-170)

What it does for this course:

```text
baseline optimized compilation
```

What it does not do:

```text
force vectorization
force AVX2
force a vector width
force one loop to use SIMD
```

Use `/O2` for the main auto-vectorization experiments unless an exercise intentionally changes it.

---

## 8. `/arch:AVX2`

Syntax:

```text
/arch:AVX2
```

Visual Studio:

```text
Project Properties
→ Configuration Properties
→ C/C++
→ Code Generation
→ Enable Enhanced Instruction Set
→ Advanced Vector Extensions 2 (/arch:AVX2)
```

Current MSVC supports `/arch:AVX2` and newer targets including `/arch:AVX512`, `/arch:AVX10.1`, and `/arch:AVX10.2`. The course deliberately keeps AVX2 as the reference target. `/arch:AVX2` also defines `__AVX2__`. [Microsoft Learn — `/arch (x64)`](https://learn.microsoft.com/en-us/cpp/build/reference/arch-x64?view=msvc-170)

What it changes:

```text
available target ISA
```

What it does not change:

```text
the legality of an actually dependent loop
```

What it does not guarantee:

```text
scalar loop → vector loop
```

A useful experiment is:

```text
/O2 /Qvec-report:2
```

versus:

```text
/O2 /arch:AVX2 /Qvec-report:2
```

Ask two independent questions:

```text
Did the vectorizer's decision change?
Did the generated instruction family change?
```

---

## 9. `/Qvec-report:2`

Syntax:

```text
/Qvec-report:2
```

Purpose:

```text
observe vectorizer decisions
```

It is diagnostic information, not a code-generation command.

A useful compiler-experiment build is:

```text
/O2 /arch:AVX2 /Qvec-report:2
```

Keep this configuration fixed while investigating one source-level obstacle.

---

## 10. Secondary options: `/Ob2`, `/Ob3`, `/GL`, `/LTCG`

These matter because compiler visibility can expose a vectorization opportunity, but none is a SIMD command.

### `/Ob2`

```text
/Ob2
```

This is the normal optimization-time inline-expansion mode under `/O1` and `/O2`.

### `/Ob3`

```text
/Ob3
```

This requests more aggressive inlining than `/Ob2` but does not guarantee inlining.

Visual Studio exposes `/Ob2` through the Inline Function Expansion property. `/Ob3` can be added under C/C++ → Command Line → Additional Options. Microsoft explicitly describes inline expansion controls as suggestions rather than guarantees. [Microsoft Learn — `/Ob`](https://learn.microsoft.com/en-us/cpp/build/reference/ob-inline-function-expansion?view=msvc-170)

Important:

```text
/Ob2 and /Ob3 are NOT loop-unrolling switches.
```

### `/GL`

```text
/GL
```

Enables whole-program optimization at compile time, allowing the compiler to use information across modules.

### `/LTCG`

```text
/LTCG
```

Enables link-time code generation and is used with the whole-program optimization workflow.

Their SIMD role is indirect:

```text
more cross-module visibility
        ↓
more opportunities for inlining / global reasoning
        ↓
possibly clearer vectorization opportunity
```

They do not mean:

```text
“vectorize this loop.”
```

Microsoft documents `/GL` as whole-program optimization and `/LTCG` as link-time code generation. [Microsoft Learn — `/GL`](https://learn.microsoft.com/en-us/cpp/build/reference/gl-whole-program-optimization?view=msvc-170)

---

# PART IV — Why MSVC Fails: Dependencies

## 11. The strongest obstacle: a real loop-carried dependency

BAD CODE:

```cpp
void prefix(float* a, int n)
{
    for (int i = 1; i < n; ++i)
        a[i] = a[i - 1] + a[i];
}
```

Iteration `i` needs the value produced by iteration `i - 1`.

The dependence is:

```text
i-1 → i
```

This is not merely something the compiler failed to prove. It is an actual recurrence.

A vectorizer cannot simply compute eight adjacent iterations independently and preserve the same sequential recurrence semantics.

MSVC reports loop-carried dependency situations with diagnostics such as reason code `1200`.

### Wrong intervention

```cpp
#pragma loop(ivdep)
for (int i = 1; i < n; ++i)
    a[i] = a[i - 1] + a[i];
```

This does not make the dependence disappear.

### Correct reasoning

```text
Is the dependency real?
    ↓
YES
    ↓
change the algorithm or choose a different vector formulation
```

Do not use a compiler assertion to lie about an algorithmic dependency.

### Verification

The report should still identify the dependence unless the algorithm has genuinely changed. If you replace the algorithm with a parallel prefix formulation, verify the new algorithm independently.

---

## 12. Suspicious-looking dependency that is actually an aliasing problem

Consider:

```cpp
void update(float* a, const float* b, int n)
{
    for (int i = 1; i < n; ++i)
        a[i] = b[i - 1] + 1.0f;
}
```

The index expression contains `i - 1`, but the read is from `b`, not necessarily from the value produced by the previous iteration.

Now the important question is whether `a` and `b` can overlap.

This is why dependency analysis and alias analysis must be kept separate.

```text
index relation
    ≠
actual memory dependence
```

The next part addresses this distinction directly.

---

# PART V — Why MSVC Fails: Aliasing and `__restrict`

## 13. The basic aliasing problem

BAD CODE:

```cpp
void add(float* a,
         float* b,
         float* out,
         int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

A compiler has to respect valid calls in which the pointer ranges overlap unless the language/program contract rules that out.

A vector transformation may perform multiple loads before stores, changing observable behavior if overlapping regions create loop-carried effects.

MSVC documents aliasing-related vectorizer reason codes in the 1500–1505 range.

---

## 14. `__restrict` as an exact compiler-facing statement

Syntax:

```cpp
void add(float* __restrict a,
         float* __restrict b,
         float* __restrict out,
         int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

The important meaning is not “these are arrays.”

It is:

```text
within the relevant scope, the restricted access is governed by a
non-aliasing promise.
```

MSVC's `__restrict` is a Microsoft extension available in C++ and C. [Microsoft Learn — `__restrict`](https://learn.microsoft.com/en-us/cpp/cpp/extension-restrict?view=msvc-170)

### When it is safe

```cpp
float a[1024];
float b[1024];
float out[1024];

add(a, b, out, 1024);
```

The storage regions are intentionally separate.

### When it is unsafe

```cpp
float buffer[2048];

add(buffer,
    buffer + 1,
    buffer + 2,
    1024);
```

Do not add `__restrict` simply to silence the vectorizer.

---

## 15. What `__restrict` can change in generated code

Without enough alias information, MSVC may construct a versioned loop:

```text
check possible overlap
        ↓
no overlap → vector loop
        │
        └── overlap → scalar-safe path
```

With valid non-alias information:

```text
__restrict
    ↓
compiler can reason directly about independence
    ↓
possibly simpler vector loop
```

This is one of the most useful distinctions in practical SIMD debugging:

```text
compiler couldn't prove it
        ≠
compiler proved it was false
```

Runtime versioning is often the compiler's way of dealing with the first case.

---

## 16. What to measure

Compile both versions:

```cpp
void add(float* a, float* b, float* out, int n);
```

and:

```cpp
void add(float* __restrict a,
         float* __restrict b,
         float* __restrict out,
         int n);
```

Compare:

```text
/Qvec-report:2
runtime pointer checks in assembly
main loop shape
scalar fallback
performance
```

Do not assume that `__restrict` must change the instruction mnemonics. It may instead remove a versioning check or simplify the control flow around a vector loop.

---

# PART VI — Memory Access Pattern, Stride, and Data Layout

## 17. Unit stride versus non-unit stride

BAD CODE:

```cpp
void scale_even(float* a, int n)
{
    for (int i = 0; i < n; i += 2)
        a[i] *= 2.0f;
}
```

The accessed sequence is:

```text
a[0], a[2], a[4], a[6], ...
```

MSVC documents reason code `1301` for a loop stride other than +1, and reason code `1203` for non-contiguous memory access.

A non-unit stride is not automatically impossible to vectorize, but it is a materially less direct vectorization pattern.

---

## 18. Do not confuse “strided” with “dependent”

A loop can be perfectly independent while still having poor memory geometry.

```cpp
for (int i = 0; i < n; ++i)
    out[i] = a[2 * i] + b[2 * i];
```

There is no recurrence here.

The issue is that the useful elements are spaced apart:

```text
a0, a2, a4, a6, ...
```

The appropriate intervention is therefore not `ivdep`.

It is a memory-layout or loop-structure intervention.

---

## 19. Transform the representation when possible

Suppose the original representation is:

```text
x0 y0 x1 y1 x2 y2 x3 y3 ...
```

and the hot operation needs all `x` values.

A structure-of-arrays layout gives:

```text
x0 x1 x2 x3 ...
y0 y1 y2 y3 ...
```

Then the hot loop becomes:

```cpp
void scale_x(float* x, int n)
{
    for (int i = 0; i < n; ++i)
        x[i] *= 2.0f;
}
```

The key transformation is:


```text
non-contiguous field access
        ↓
contiguous array
```

This is not a compiler hint. You changed the data geometry.

---

## 20. Unit-stride pointer normalization

A useful source transformation is moving constant offsets outside the hot loop.

Less transparent form:

```cpp
void work(float* a, int offset, int n)
{
    for (int i = 0; i < n; ++i)
        a[offset + i] *= 2.0f;
}
```

More canonical form:

```cpp
void work(float* a, int offset, int n)
{
    float* p = a + offset;

    for (int i = 0; i < n; ++i)
        p[i] *= 2.0f;
}
```

The second loop presents a simple base pointer plus unit-stride induction.

The important caveat is that MSVC may already perform this normalization. Do not transform source merely to make it look optimized. Use the vectorizer report and assembly to determine whether there is a real difference.

---

## 21. AoS versus SoA is a tool, not a rule

Array-of-structures:

```cpp
struct Item
{
    float x;
    float y;
    float z;
    float weight;
};

void scale_x(Item* p, int n)
{
    for (int i = 0; i < n; ++i)
        p[i].x *= 2.0f;
}
```

The `x` fields are separated by the object stride.

Structure-of-arrays:

```cpp
struct Items
{
    float* x;
    float* y;
    float* z;
    float* weight;
};

void scale_x(const Items& p, int n)
{
    for (int i = 0; i < n; ++i)
        p.x[i] *= 2.0f;
}
```

Now the hot field is contiguous.

But do not use SoA merely because “SIMD likes SoA.” If the application repeatedly needs whole objects, the structural trade-off can be different. The course is interested in the specific loop you want to vectorize.

---

## 22. Nested loops: decide which loop should become SIMD

Given:

```cpp
for (int i = 0; i < rows; ++i)
{
    for (int j = 0; j < cols; ++j)
        out[i * cols + j] = a[i * cols + j] + b[i * cols + j];
}
```

The inner loop has unit-stride accesses.

That is usually the natural vectorization target.

Do not ask only:

```text
“Was the nested loop vectorized?”
```

Ask:

```text
Which loop was vectorized?
How many elements does one SIMD instruction represent?
What happens to the outer loop?
Was there loop interchange?
```

The distinction becomes critical for multidimensional arrays, matrix kernels, and transposes.

---

# PART VII — Alignment as Compiler Information

## 23. First principle: SIMD does not require alignment

AVX2 supports unaligned memory operations.

Therefore this is legal:

```cpp
const __m256 x = _mm256_loadu_ps(p);
```

An unknown alignment does not imply that SIMD is impossible.

Alignment can still matter because it can affect:

```text
load/store instruction selection
prologue or peel handling
compiler cost modeling
```

Therefore the course asks two distinct questions:

```text
Did alignment information make vectorization possible?
```

and:

```text
Did alignment information change the generated loads/stores?
```

---

## 24. `alignas`

Static or automatic storage:

```cpp
alignas(32) float data[1024];
```

This establishes a 32-byte alignment requirement for that object.

C++ syntax is simply:

```cpp
alignas(32)
```

No runtime instruction is generated merely because the declaration uses `alignas`.

The key distinction is:

```text
object is physically aligned
```

versus:

```text
compiler has alignment information at this pointer expression
```

---

## 25. Dynamic aligned allocation

MSVC provides:

```cpp
#include <malloc.h>

float* allocate32(std::size_t n)
{
    return static_cast<float*>(
        _aligned_malloc(n * sizeof(float), 32));
}

void release32(float* p)
{
    _aligned_free(p);
}
```

Do not pair `_aligned_malloc` with ordinary `delete` or ordinary `free`.

The dynamic allocation operation establishes the storage property. The optimizer may still need that property communicated at a later pointer expression.

---

## 26. `std::assume_aligned`

C++20:

```cpp
#include <memory>

float* aligned = std::assume_aligned<32>(raw);
```

The result should be used:

```cpp
void scale(float* raw, int n)
{
    float* aligned = std::assume_aligned<32>(raw);

    for (int i = 0; i < n; ++i)
        aligned[i] *= 2.0f;
}
```

The facility communicates that the returned pointer meets the requested alignment. If the premise is false, behavior is undefined.

The subtle but important point is that this is not an allocation operation. It does not make an unaligned pointer aligned. It tells the compiler to rely on a property that must already be true.

---

## 27. `__assume`

MSVC-specific optimizer assertion:

```cpp
#include <cstdint>

void scale(float* p, int n)
{
    __assume((reinterpret_cast<std::uintptr_t>(p) & 31u) == 0);

    for (int i = 0; i < n; ++i)
        p[i] *= 2.0f;
}
```

`__assume(expression)` tells the optimizer to treat the expression as true on reachable code until the relevant state changes. It does not execute a runtime check. Microsoft documents invalid assumptions as potentially causing unpredictable behavior. [Microsoft Learn — `__assume`](https://learn.microsoft.com/en-us/cpp/intrinsics/assume?view=msvc-170)

The engineering rule is:

```text
prove alignment at the memory-system boundary
→ communicate it at the hot loop boundary
```

---

## 28. Alignment is not a justification for `_mm256_load_ps` by itself

Correct:

```cpp
alignas(32) float data[1024];

const __m256 x = _mm256_load_ps(data);
```

Potentially incorrect:

```cpp
float* p = obtain_some_pointer();

const __m256 x = _mm256_load_ps(p);
```

unless the program has an actual alignment guarantee for `p`.

If alignment is not guaranteed, use:

```cpp
_mm256_loadu_ps(p);
```

or establish and communicate a real alignment contract first.

---

# PART VIII — `#pragma loop(ivdep)` and Loop Directives

## 29. Exact syntax

MSVC supports:

```cpp
#pragma loop(ivdep)
for (...)
    ...
```

and also:

```cpp
#pragma loop(no_vector)
for (...)
    ...
```

The directive is placed immediately before the loop.

Microsoft documents:

```text
ivdep       hint to ignore vector dependencies
no_vector   prevent auto-vectorization for the following loop
hint_parallel(n)  automatic parallelization hint, not the SIMD mechanism
```

[Microsoft Learn — `loop` pragma](https://learn.microsoft.com/en-us/cpp/preprocessor/loop?view=msvc-170)

---

## 30. What `ivdep` actually changes

The point of:

```cpp
#pragma loop(ivdep)
```

is not:

```text
“use AVX2.”
```

It is:

```text
“for vectorization analysis, treat vector dependencies according to this programmer assertion.”
```

This is therefore an **assumption about dependencies**, not an explicit SIMD request.

---

## 31. Correct versus incorrect `ivdep`

Incorrect:

```cpp
#pragma loop(ivdep)
for (int i = 1; i < n; ++i)
    a[i] = a[i - 1] + 1.0f;
```

The recurrence is real.

Potentially correct use:

```cpp
#pragma loop(ivdep)
for (int i = 0; i < n; ++i)
    out[i] = source[i] + constant[i];
```

provided you have independently established that the memory relationships really satisfy the required independence.

The directive should never be used merely because a compiler report is inconvenient.

---

## 32. `no_vector` as an experimental control

Although it is not a technique for making SIMD happen, it is useful scientifically:

```cpp
#pragma loop(no_vector)
for (int i = 0; i < n; ++i)
    out[i] = a[i] + b[i];
```

This lets you compare:

```text
same scalar algorithm
    ↓
auto-vectorization enabled
vs
explicitly suppressed
```

It is useful for establishing whether a performance change actually came from auto-vectorization.

Treat it as a lab instrument, not as a main course topic.

---

# PART IX — Source Transformations That Expose Vectorization Opportunities

## 33. The transformation principle

The most useful source transformations do one of three things:

```text
remove an obstacle
make an invariant visible
make the memory/dataflow pattern canonical
```

They are not style rules. They are targeted responses to compiler diagnostics.

---

## 34. Hoist loop-invariant work

BAD CODE:

```cpp
void scale(float* a, int n, float x, float y)
{
    for (int i = 0; i < n; ++i)
        a[i] *= x * y;
}
```

Transformed:

```cpp
void scale(float* a, int n, float x, float y)
{
    const float scale = x * y;

    for (int i = 0; i < n; ++i)
        a[i] *= scale;
}
```

This is not usually a trick that must be performed manually; optimized compilers often perform invariant motion themselves.

The lesson is more general:

> When a compiler cannot see the invariant, make it obvious enough to test whether the optimization changes.

Verification:

```text
/Qvec-report:2
assembly
```

Do not assume that source simplification itself proves anything.

---

## 35. Expose a canonical induction variable

Prefer a hot loop like:

```cpp
float* p = a + offset;

for (int i = 0; i < n; ++i)
    p[i] *= 2.0f;
```

over a structure in which the base pointer itself changes each iteration:

```cpp
for (int i = 0; i < n; ++i)
{
    a += 1;
    a[-1] *= 2.0f;
}
```

The second form can force the vectorizer to reason through a changing array base. The first form states the canonical access explicitly.

---

## 36. Expose loop bounds

BAD CODE:

```cpp
int get_count();

void work(float* a)
{
    for (int i = 0; i < get_count(); ++i)
        a[i] *= 2.0f;
}
```

Transformed:

```cpp
void work(float* a)
{
    const int n = get_count();

    for (int i = 0; i < n; ++i)
        a[i] *= 2.0f;
}
```

This does not guarantee a different vectorizer decision. It creates a clearer invariant for the optimizer.

The more important lesson is that a function call in the loop condition is itself something worth investigating.

---

## 37. Remove function calls from the hot loop

BAD CODE:

```cpp
float transform(float x);

void apply(const float* in, float* out, int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = transform(in[i]);
}
```

The optimizer may not have enough information to replace the call with vector operations.

Expose the definition:

```cpp
inline float transform(float x)
{
    return x * x + 1.0f;
}
```

Then:

```cpp
void apply(const float* in, float* out, int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = transform(in[i]);
}
```

The actual question is whether MSVC inlines it.

You can test:

```text
/O2 /Ob2
```

versus:

```text
/O2 /Ob3
```

but `/Ob3` is a more aggressive suggestion, not a guarantee.

---

## 38. Function-pointer calls are a different problem

Consider:

```cpp
float (*func)(float, float);

for (int i = 0; i < n; ++i)
    out[i] = func(a[i], b[i]);
```

This is substantially less transparent than a directly visible expression.

Possible transformations include:

```text
function pointer
    ↓
known callable
    ↓
inline definition
    ↓
direct expression
```

or:

```text
runtime callback
    ↓
explicit vector implementation of the callback
```

The course does not say “function calls always prevent SIMD.” It teaches you to ask whether the call prevents the compiler from exposing a vectorizable expression.

---

## 39. Split a complicated loop

BAD CODE:

```cpp
void update(float* values,
            int* tags,
            short* weights,
            int n)
{
    for (int i = 0; i < n; ++i)
    {
        values[i] *= 2.0f;
        tags[i] += 1;
        weights[i] = static_cast<short>(weights[i] + 1);
    }
}
```

Possible transformation:

```cpp
void update(float* values,
            int* tags,
            short* weights,
            int n)
{
    for (int i = 0; i < n; ++i)
        values[i] *= 2.0f;

    for (int i = 0; i < n; ++i)
        tags[i] += 1;

    for (int i = 0; i < n; ++i)
        weights[i] = static_cast<short>(weights[i] + 1);
}
```

Why it can help:

```text
one heterogeneous loop
        ↓
multiple regular kernels
```

Trade-off:

```text
more loop traversals
possibly more memory traffic
```

Therefore verify both vectorization and total runtime.

---

## 40. Replace difficult control flow with dataflow when appropriate

BAD CODE:

```cpp
void clamp_positive(const float* a, float* out, int n)
{
    for (int i = 0; i < n; ++i)
    {
        if (a[i] > 0.0f)
            out[i] = a[i];
        else
            out[i] = 0.0f;
    }
}
```

Candidate source transformation:

```cpp
void clamp_positive(const float* a, float* out, int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] > 0.0f ? a[i] : 0.0f;
}
```

This does not force SIMD. It exposes a lane-wise selection that a vectorizer may lower to comparison plus selection.

The rule is:

```text
branch over independent elements
    ↓
can sometimes become
compare + mask/select
```

Do not claim that every branch becomes SIMD. Inspect the report and assembly.

---

## 41. Split a mixed-direction access

BAD CODE:

```cpp
for (int i = 0; i < n; ++i)
{
    out[i] = a[i] + b[i];
    reverse[i] = a[n - 1 - i];
}
```

The first statement is unit stride; the second walks backwards.

A transformation can separate them:

```cpp
for (int i = 0; i < n; ++i)
    out[i] = a[i] + b[i];

for (int i = 0; i < n; ++i)
    reverse[i] = a[n - 1 - i];
```

This may allow one loop to become an especially clean SIMD kernel.

Again, extra traversal cost must be benchmarked.

---

## 42. Compile-time loop bounds can change the optimization problem

General function:

```cpp
void scale(float* p, int n)
{
    for (int i = 0; i < n; ++i)
        p[i] *= 2.0f;
}
```

Compile-time size:

```cpp
template<int N>
void scale(float (&p)[N])
{
    for (int i = 0; i < N; ++i)
        p[i] *= 2.0f;
}
```

The second function gives the compiler stronger information about the iteration count.

That can affect:

```text
unrolling
remainder handling
loop versioning
constant propagation
profitability decisions
```

It still does not create a blanket SIMD guarantee.

---

# PART X — Loop Unrolling and Its Interaction with SIMD

## 43. Unrolling is not vectorization

Scalar source:

```cpp
for (int i = 0; i < n; ++i)
    a[i] += b[i];
```

Manual unrolling by four:

```cpp
for (int i = 0; i + 3 < n; i += 4)
{
    a[i + 0] += b[i + 0];
    a[i + 1] += b[i + 1];
    a[i + 2] += b[i + 2];
    a[i + 3] += b[i + 3];
}
```

This is still scalar source semantics.

The compiler may subsequently vectorize the unrolled loop.

Therefore:

```text
unroll
    ≠
SIMD
```

---

## 44. `/Ob` is about inlining, not unrolling

Do not make this false association:

```text
/Ob3 → aggressive loop unrolling
```

The real meaning is:

```text
/Ob3 → more aggressive function inlining
```

Microsoft documents `/Ob2` as the normal optimizing inline expansion level and `/Ob3` as a more aggressive mode. Neither guarantees a particular function expansion. [Microsoft Learn — `/Ob`](https://learn.microsoft.com/en-us/cpp/build/reference/ob-inline-function-expansion?view=msvc-170)

---

## 45. Manual unrolling can help or hurt

Potential benefit:

```text
more visible independent work
less loop-control overhead
```

Potential cost:

```text
larger code
more live values
more register pressure
possible spills
harder compiler analysis
```

Therefore the experiment is:

```text
original loop
→ compiler-optimized assembly

manually unrolled loop
→ compiler-optimized assembly
```

The source form alone does not tell you which is better.

---

# PART XI — Floating-Point Semantics and SIMD

## 46. Why reductions are different

Elementwise:

```cpp
for (int i = 0; i < n; ++i)
    out[i] = a[i] + b[i];
```

does not require changing the order among independent additions.

Reduction:

```cpp
float sum(const float* a, int n)
{
    float s = 0.0f;

    for (int i = 0; i < n; ++i)
        s += a[i];

    return s;
}
```

does.

A vector reduction normally computes several partial sums, changing the association of floating-point operations.

For finite values:

```text
(a + b) + c
```

can differ bitwise from:

```text
a + (b + c)
```

Therefore the floating-point model can affect vectorization legality.

---

## 47. `/fp:precise`

Syntax:

```text
/fp:precise
```

Visual Studio:

```text
Project Properties
→ Configuration Properties
→ C/C++
→ Code Generation
→ Floating Point Model
→ Precise (/fp:precise)
```

The relevant lesson is not “precise is slow.”

The relevant lesson is:

```text
stricter FP semantics
    ↓
less freedom to reassociate floating-point operations
    ↓
some reduction transformations may be unavailable
```

It does not disable all SIMD.

---

## 48. `/fp:fast`

Syntax:

```text
/fp:fast
```

Visual Studio:

```text
Project Properties
→ Configuration Properties
→ C/C++
→ Code Generation
→ Floating Point Model
→ Fast (/fp:fast)
```

The relaxed model permits more transformations, including floating-point reassociation that can make a reduction vectorizable.

That does not mean:

```text
/fp:fast → SIMD
```

It means:

```text
/fp:fast
    ↓
larger legal transformation space
    ↓
vectorization may become possible
```

Microsoft currently lists `/fp:fast`, `/fp:precise`, and `/fp:strict` as floating-point model options. [Microsoft Learn — compiler options](https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=msvc-170)

---

## 49. Controlled reduction experiment

Source:

```cpp
float sum(const float* a, int n)
{
    float s = 0.0f;

    for (int i = 0; i < n; ++i)
        s += a[i];

    return s;
}
```

Build A:

```text
/O2 /arch:AVX2 /fp:precise /Qvec-report:2
```

Build B:

```text
/O2 /arch:AVX2 /fp:fast /Qvec-report:2
```

Record:

```text
vectorizer decision
reason code
assembly
numerical result
```

Use a numerical reference with higher precision when testing error.

Do not accept “it vectorized” as the end of the exercise. You changed the numerical contract as well as the optimizer's legal transformation space.

---

# PART XII — Intrinsics: The Direct SIMD Mechanism

## 50. Include the AVX/AVX2 intrinsic definitions

```cpp
#include <immintrin.h>
```

The Microsoft intrinsic reference lists x86/x64 intrinsics and their required technologies. [Microsoft Learn — x86 intrinsics list](https://learn.microsoft.com/en-us/cpp/intrinsics/x86-intrinsics-list?view=msvc-170)

For this course, the core type is:

```cpp
__m256
```

which represents a 256-bit vector used for packed single-precision floating-point operations.

Conceptually:

```text
__m256
┌───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┐
│ f0    │ f1    │ f2    │ f3    │ f4    │ f5    │ f6    │ f7    │
└───────┴───────┴───────┴───────┴───────┴───────┴───────┴───────┘
```

One `__m256` therefore carries eight 32-bit floats.

---

## 51. Predict intrinsic names from their grammar

A name such as:

```cpp
_mm256_add_ps
```

can be parsed as:

```text
_mm256   256-bit vector form
add      operation
ps       packed single-precision floating point
```

Common suffixes:

```text
ps      packed single-precision float
pd      packed double-precision float
epi8    signed 8-bit integer lanes
epi16   signed 16-bit integer lanes
epi32   signed 32-bit integer lanes
epi64   signed 64-bit integer lanes
epu8    unsigned 8-bit integer lanes
...
```

Common operation names:

```text
load / loadu
store / storeu
add / sub / mul / div
fmadd
min / max
cmp
and / or / xor
blend
shuffle
permute
convert
```

The goal is not to memorize arbitrary names. It is to infer the likely intrinsic from the required operation and lane type.

---

## 52. `_mm256_load_ps`

Syntax:

```cpp
__m256 _mm256_load_ps(float const* p);
```

Conceptually:

```text
result[0] = p[0]
result[1] = p[1]
...
result[7] = p[7]
```

It is the aligned-load form.

Typical AVX instruction family:

```text
vmovaps
```

Use it only when the pointer satisfies the alignment contract required by the intrinsic.

---

## 53. `_mm256_loadu_ps`

Syntax:

```cpp
__m256 _mm256_loadu_ps(float const* p);
```

Same logical eight lanes, but no alignment requirement of the aligned form.

Typical instruction family:

```text
vmovups
```

This is often the correct default when the program does not have a reliable 32-byte alignment contract.

A common mistake is choosing `_mm256_load_ps` because alignment “usually happens.” Do not make a machine-code assumption without a program-level guarantee.

---

## 54. `_mm256_store_ps`

Syntax:

```cpp
void _mm256_store_ps(float* p, __m256 a);
```

Conceptually:

```text
p[0] = a lane 0
...
p[7] = a lane 7
```

Typical instruction family:

```text
vmovaps
```

The destination must satisfy the corresponding alignment requirement.

---

## 55. `_mm256_storeu_ps`

Syntax:

```cpp
void _mm256_storeu_ps(float* p, __m256 a);
```

Typical instruction family:

```text
vmovups
```

Use when alignment cannot safely be assumed.

---

## 56. `_mm256_add_ps`

Syntax:

```cpp
__m256 _mm256_add_ps(__m256 a, __m256 b);
```

Lane semantics:

```text
a0+b0
a1+b1
...
a7+b7
```

Conceptual scalar loop:

```cpp
for (int i = 0; i < 8; ++i)
    z[i] = a[i] + b[i];
```

Typical instruction:

```text
vaddps
```

---

## 57. `_mm256_sub_ps`

Syntax:

```cpp
__m256 _mm256_sub_ps(__m256 a, __m256 b);
```

Semantics:

```text
a[i] - b[i]
```

Typical instruction:

```text
vsubps
```

---

## 58. `_mm256_mul_ps`

Syntax:

```cpp
__m256 _mm256_mul_ps(__m256 a, __m256 b);
```

Semantics:

```text
a[i] * b[i]
```

Typical instruction:

```text
vmulps
```

---

## 59. `_mm256_div_ps`

Syntax:

```cpp
__m256 _mm256_div_ps(__m256 a, __m256 b);
```

Semantics:

```text
a[i] / b[i]
```

Typical instruction family:

```text
vdivps
```

The fact that one instruction operates on eight lanes does not tell you its latency, throughput, or overall kernel performance. Those questions belong to later measurement.

---

## 60. `_mm256_set1_ps`

Syntax:

```cpp
__m256 _mm256_set1_ps(float a);
```

Conceptual result:

```text
[a a a a a a a a]
```

This is the natural operation for a scalar constant applied independently to eight lanes:

```cpp
const __m256 c = _mm256_set1_ps(scale);
```

A common generated form is broadcast from a scalar or equivalent constant materialization; verify exact instructions in context.

---

## 61. `_mm256_fmadd_ps`

Syntax:

```cpp
__m256 _mm256_fmadd_ps(__m256 a,
                       __m256 b,
                       __m256 c);
```

Conceptual semantics:

```text
a[i] * b[i] + c[i]
```

with fused multiply-add semantics.

Typical instruction family:

```text
vfmadd...
```

FMA support must exist on the runtime target. Compile for a target that permits the instruction set and verify the generated code.

---

## 62. Compare operations

Example:

```cpp
const __m256 mask =
    _mm256_cmp_ps(x, zero, _CMP_GT_OQ);
```

Conceptually each lane becomes a mask representing:

```text
x[i] > zero
```

The important transformation is:

```text
control decision per scalar element
        ↓
mask per vector lane
```

Masks are the bridge between scalar `if` logic and vector dataflow.

---

## 63. `_mm256_blendv_ps`

Syntax:

```cpp
__m256 _mm256_blendv_ps(__m256 a,
                        __m256 b,
                        __m256 mask);
```

Conceptually:

```text
mask lane true  → choose b
mask lane false → choose a
```

For example:

```cpp
__m256 clamp_positive(__m256 x)
{
    const __m256 zero = _mm256_setzero_ps();
    const __m256 mask = _mm256_cmp_ps(
        x, zero, _CMP_GT_OQ);

    return _mm256_blendv_ps(zero, x, mask);
}
```

This is explicit vector dataflow. There is no compiler decision about whether the scalar `if` should become SIMD; you wrote the vector compare and selection yourself.

---

## 64. Bitwise operations

Common `__m256` operations include:

```cpp
_mm256_and_ps
_mm256_or_ps
_mm256_xor_ps
```

They operate on bit patterns.

A common sign-bit operation is:

```cpp
const __m256 sign = _mm256_set1_ps(-0.0f);
const __m256 negated = _mm256_xor_ps(x, sign);
```

The relevant idea is:

```text
floating-point representation
        ↓
sign bit
        ↓
bitwise manipulation
```

Use such transformations only when the representation-level semantics are appropriate to the operation.

---

## 65. Min and max

Common packed-float intrinsics:

```cpp
_mm256_min_ps
_mm256_max_ps
```

Conceptually:

```text
r[i] = min(a[i], b[i])
r[i] = max(a[i], b[i])
```

Do not blindly assume that language-level `min` and machine-level `min` have identical behavior for every special floating-point value. Test NaNs and signed zero when those details are part of your correctness contract.

---

## 66. Shuffles versus permutations

Think of these as data movement operations.

```text
arithmetic intrinsic
    → changes values

shuffle/permute intrinsic
    → changes lane positions
```

Representative examples include:

```cpp
_mm256_shuffle_ps
_mm256_permutevar8x32_ps
```

Do not add a permutation merely because it is “SIMD.” A lane rearrangement has a purpose and can consume execution resources.

The design question is:

```text
What lane layout makes the arithmetic natural?
```

---

## 67. Conversions

Representative operations:

```cpp
_mm256_cvtps_epi32
_mm256_cvtepi32_ps
```

A conversion intrinsic is particularly useful when the compiler's scalar form of the conversion blocks auto-vectorization but a supported vector conversion exists.

The naming pattern can be read as:

```text
cvt + source representation + destination representation
```

Always verify the precise rounding/semantic behavior required by the operation.

---

# PART XIII — Intrinsics and the Meaning of “Guarantee”

## 68. Auto-vectorization is a compiler choice

Source:

```cpp
void add(const float* a,
         const float* b,
         float* out,
         int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

You supplied scalar semantics.

MSVC decides whether to represent those semantics as:

```text
scalar loop
vector loop
vector loop + scalar tail
multiple versions
some other equivalent form
```

---

## 69. Intrinsics change the level of control

Explicit AVX2:

```cpp
#include <immintrin.h>

void add_avx2(const float* a,
              const float* b,
              float* out,
              int n)
{
    int i = 0;

    for (; i + 8 <= n; i += 8)
    {
        const __m256 va = _mm256_loadu_ps(a + i);
        const __m256 vb = _mm256_loadu_ps(b + i);
        const __m256 vc = _mm256_add_ps(va, vb);

        _mm256_storeu_ps(out + i, vc);
    }

    for (; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

The source explicitly contains the eight-lane operations.

This is fundamentally different from asking the auto-vectorizer to discover that representation.

---

## 70. What intrinsics guarantee and what they do not

An intrinsic specifies target-specific vector semantics to the compiler.

It does not necessarily constitute a one-source-line → one-machine-instruction contract.

The backend may:

```text
eliminate an operation
fold constants
combine operations
select an equivalent encoding
schedule instructions differently
```

Therefore use this hierarchy:

```text
plain scalar C++
    ↓
compiler decides whether to vectorize

compiler hints / assumptions
    ↓
more information, still compiler-controlled

intrinsics
    ↓
explicit target-specific vector operations

disassembly
    ↓
proof of the emitted machine code
```

If the requirement is literally “I need to know what instructions are in the binary,” inspect the binary.

---

# PART XIV — Visual Studio Assembly Verification

## 71. Verification has two stages

### Stage A — vectorizer report

```text
/Qvec-report:2
```

Answers:

```text
Did MSVC's auto-vectorizer decide to vectorize this loop?
Why or why not?
```

### Stage B — disassembly

Answers:

```text
What machine instructions did the compiler emit?
```

Neither stage substitutes for the other.

---

## 72. Read Build Output first

Confirm that the actual compile command contains the options you intended:

```text
/O2
/arch:AVX2
/Qvec-report:2
```

A common mistake is changing a property in one configuration and inspecting another executable.

Do not infer compiler configuration from the Visual Studio UI alone. Verify the command line for the source file being compiled.

---

## 73. Open optimized disassembly

Use a Release/x64 optimized build.

A practical procedure is:

```text
1. Build the Release x64 configuration.
2. Start debugging the binary.
3. Break inside or near the target function.
4. Open Debug → Windows → Disassembly.
5. Locate the target function.
6. Locate the loop back-edge.
7. Identify its memory operations and arithmetic.
```

Optimized code will not preserve a one-to-one source layout. The compiler may reorder, merge, unroll, peel, or eliminate operations.

Your job is to reconstruct the loop's actual execution pattern.

---

## 74. Recognize scalar versus packed floating-point instructions

Typical scalar single-precision operations:

```text
movss
addss
subss
mulss
divss
```

Typical packed AVX single-precision operations:

```text
vmovups
vmovaps
vaddps
vsubps
vmulps
vdivps
```

FMA family:

```text
vfmadd...
```

A critical detail:

```text
vaddss = scalar
vaddps = packed
```

The `v` prefix alone does not mean “vectorized across multiple elements.”

---

## 75. Recognize vector width from the register class

For AVX2 floating-point code you will commonly see:

```text
ymm0
ymm1
ymm2
...
```

A `ymm` register can hold 256 bits.

For `float` values:

```text
256 / 32 = 8 lanes
```

Therefore a packed single-precision operation such as:

```text
vaddps ymm0, ymm1, ymm2
```

operates on eight 32-bit float lanes.

For AVX-encoded scalar operations, you can still see `xmm` registers. Do not classify an instruction from register size alone; also inspect the opcode semantics.

---

## 76. Finding the main vector loop

A vectorized function commonly contains a pattern similar to:

```text
load vector
compute vector
store vector
update pointer/index by vector width
loop back
```

For example:

```text
vmovups   ymm0, [rdi+rax*4]
vmovups   ymm1, [rsi+rax*4]
vaddps    ymm0, ymm0, ymm1
vmovups   [rdx+rax*4], ymm0
add       rax, 8
cmp       rax, rcx
jl        loop
```

The exact syntax and register allocation vary by compiler version.

The important structural evidence is:

```text
one iteration of the machine loop processes 8 floats
```

rather than:

```text
one scalar float per iteration
```

---

## 77. Scalar remainder loops are normal

Suppose:

```cpp
for (int i = 0; i < n; ++i)
    out[i] = a[i] + b[i];
```

and `n = 19`.

An AVX2 implementation can process:

```text
8 + 8 = 16 elements
```

then finish:

```text
3 elements scalar
```

The resulting assembly may therefore contain both:

```text
vaddps
```

and:

```text
addss
```

That does not imply that vectorization failed.

The correct question is:

```text
Was the bulk region vectorized?
```

---

## 78. Runtime alias checks can produce multiple loops

You may find:

```text
pointer-range checks
        ↓
vector fast path
        ↓
scalar fallback
```

This is not contradictory evidence.

It means the compiler did not have enough static information to select one path unconditionally, so it generated a guarded transformation.

Compare this with a `__restrict` version and see whether the guard disappears.

---

## 79. How to prove the target loop was vectorized

Seeing one `vaddps` in a function is insufficient.

To prove the intended loop was vectorized, identify:

```text
1. the control-flow loop
2. the loads feeding the vector operation
3. the vector arithmetic
4. the vector stores
5. the induction-variable update
6. the loop-back edge
```

Then establish that the loop corresponds to the source loop you are studying.

---

## 80. What not to conclude from assembly

Do not conclude:

```text
“there is a ymm register, so my loop is SIMD.”
```

Do not conclude:

```text
“there is a vaddps somewhere, so every iteration is vectorized.”
```

Do not conclude:

```text
“there are scalar instructions, so vectorization failed.”
```

Do not conclude:

```text
“vmovups means the pointer is unaligned.”
```

Do not conclude:

```text
“vmovaps means the entire function is aligned and vectorized.”
```

Instead reconstruct the actual machine loop.

---

# PART XV — The Repeatable Vectorization-Debugging Workflow

## 81. The complete workflow

When a loop is not vectorized:

```text
Step 1
Compile an optimized build.
```

Use:

```text
/O2 /arch:AVX2
```

```text
Step 2
Enable diagnostics.
```

```text
/Qvec-report:2
```

```text
Step 3
Locate the exact compiler message for the target loop.
```

```text
Step 4
Classify the reported obstacle.
```

```text
Dependency
Aliasing
Stride / memory access
Alignment information
Function call / visibility
Control flow
Unsupported operation
Loop size / profitability
Floating-point semantics
```

```text
Step 5
Prove that the obstacle actually applies to the loop.
```

```text
Step 6
Apply exactly one intervention.
```

```text
Step 7
Compile again.
```

```text
Step 8
Read /Qvec-report:2 again.
```

```text
Step 9
Inspect disassembly.
```

```text
Step 10
Confirm the target loop, not unrelated code, contains packed instructions.
```

```text
Step 11
Run correctness tests.
```

```text
Step 12
Benchmark the isolated kernel.
```

This is the central method of the course.

---

## 82. Why one change at a time matters

Suppose the baseline reports:

```text
possible aliasing
```

You change:

```text
__restrict
__assume
ivdep
/fp:fast
/Ob3
```

and now get SIMD.

You cannot tell which fact mattered.

Instead:

```text
baseline
    ↓
aliasing report
    ↓
add __restrict
    ↓
report changes
    ↓
inspect assembly
```

This is not merely a pedagogical preference. It is how you debug heuristic compiler transformations without destroying causal information.

---

## 83. The intervention should match the reason

Use this mapping as a starting hypothesis:

```text
real dependency
    → algorithmic restructuring

possible aliasing
    → __restrict when the contract permits it

non-unit stride
    → loop/data-layout transformation

alignment uncertainty
    → establish and communicate alignment

false compiler dependency concern
    → justified ivdep

call blocks visibility
    → inline/expose/eliminate the call

control-flow complexity
    → restructure or express lane selection as dataflow

FP reduction restriction
    → inspect /fp model

unsupported scalar operation
    → find supported vector form or use explicit intrinsics

too little work
    → enlarge kernel/batch work or accept scalar code

still not enough compiler control
    → explicit intrinsics
```

Never use this table without reading the actual report and source first.

---

# PART XVI — Technique Decision Tree

## 84. “MSVC didn't vectorize my loop. What do I try next?”

```text
START
  │
  ▼
/O2 + /arch:AVX2 + /Qvec-report:2
  │
  ▼
Read the vectorizer message
  │
  ├── Real loop-carried dependency?
  │       │
  │       ├── YES
  │       │    → redesign algorithm / loop structure
  │       │
  │       └── NO
  │            → investigate aliasing or proof visibility
  │
  ├── Possible aliasing?
  │       │
  │       ├── Can you prove non-aliasing?
  │       │      │
  │       │      ├── YES → __restrict
  │       │      └── NO  → retain safe semantics or redesign
  │       │
  │       └── inspect runtime versioning
  │
  ├── Non-unit stride / non-contiguous access?
  │       │
  │       └── change loop order / layout / representation
  │
  ├── Alignment uncertainty?
  │       │
  │       ├── actual alignment guaranteed?
  │       │      │
  │       │      ├── YES → communicate it
  │       │      │          alignas / assume_aligned / __assume
  │       │      │
  │       │      └── NO  → use unaligned access
  │       │
  │       └── remember: unknown alignment ≠ no SIMD
  │
  ├── Function call?
  │       │
  │       ├── body available → expose/inlining experiment
  │       └── non-vectorizable operation → explicit vector form
  │
  ├── Control flow?
  │       │
  │       └── simplify / mask / select where appropriate
  │
  ├── Floating-point reduction?
  │       │
  │       ├── reassociation legal? → inspect /fp
  │       └── not legal? → preserve semantics
  │
  ├── Unsupported operation?
  │       │
  │       ├── source transform to supported operation
  │       └── explicit intrinsic/vector implementation
  │
  ├── Too little work?
  │       │
  │       └── assess profitability rather than forcing SIMD
  │
  └── Compiler still refuses or exact SIMD control is required
          │
          ▼
      explicit AVX2 intrinsics
          │
          ▼
      inspect disassembly
          │
          ▼
      verify correctness and performance
```

The important thing is why each branch exists.

```text
compiler obstacle
    ↓
missing information / bad structure
    ↓
matching intervention
```

This is not a bag of tricks.

---

# PART XVII — Practical Exercises

## Universal exercise protocol

Every exercise follows:

```text
1. Predict.
2. Compile.
3. Read /Qvec-report:2.
4. Apply the assigned intervention.
5. Recompile.
6. Read the report again.
7. Inspect assembly.
8. Verify correctness.
9. Benchmark.
```

Exercises that explicitly test instruction selection should add:

```text
10. Identify the intended instruction sequence.
```

---

## Exercise 1 — Baseline auto-vectorization

Code:

```cpp
void add(float* a, const float* b, int n)
{
    for (int i = 0; i < n; ++i)
        a[i] += b[i];
}
```

Compile:

```text
/O2 /arch:AVX2 /Qvec-report:2
```

Questions:

```text
Will MSVC vectorize it?
Which loop was reported?
What does the assembly look like?
Is there a scalar remainder?
```

Do not write intrinsics for this exercise.

---

## Exercise 2 — Aliasing

Start with:

```cpp
void add(float* a,
         float* b,
         float* out,
         int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

Then create:

```cpp
void add(float* __restrict a,
         float* __restrict b,
         float* __restrict out,
         int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

Record:

```text
vectorizer output before
vectorizer output after
runtime alias guards before/after
assembly before/after
```

Write a correctness test that includes only calls satisfying the restricted contract.

---

## Exercise 3 — Alignment information

Baseline:

```cpp
void scale(float* p, int n)
{
    for (int i = 0; i < n; ++i)
        p[i] *= 2.0f;
}
```

Test these storage forms:

```cpp
alignas(32) float a[4096];
```

and:

```cpp
float* a = static_cast<float*>(
    _aligned_malloc(4096 * sizeof(float), 32));
```

Then communicate an existing alignment guarantee with:

```cpp
float* a = std::assume_aligned<32>(raw);
```

Questions:

```text
Does vectorization change?
Does the memory instruction change?
Did the compiler require the alignment to vectorize?
```

The important result may be “SIMD existed in every version, but the load/store form changed.”

---

## Exercise 4 — Dependency diagnosis

Compare:

```cpp
void f(float* a, int n)
{
    for (int i = 1; i < n; ++i)
        a[i] = a[i - 1] + 1.0f;
}
```

with:

```cpp
void f(float* a, const float* b, int n)
{
    for (int i = 1; i < n; ++i)
        a[i] = b[i - 1] + 1.0f;
}
```

For each:

```text
Is the dependence real?
What memory relation matters?
Would ivdep be legitimate?
What other compiler obstacle remains?
```

Do the analysis before modifying either loop.

---

## Exercise 5 — `ivdep`

Construct a loop for which you can establish that the compiler's vector-dependency concern does not correspond to a real dependence under the program's contract.

Baseline:

```cpp
for (...)
    ...
```

Then:

```cpp
#pragma loop(ivdep)
for (...)
    ...
```

Required evidence:

```text
report before
report after
assembly before
assembly after
correctness
```

Also build a deliberately dependent loop and explain why applying `ivdep` to it is invalid.

---

## Exercise 6 — Non-unit stride

Start with:

```cpp
void scale_even(float* a, int n)
{
    for (int i = 0; i < n; i += 2)
        a[i] *= 2.0f;
}
```

Construct a representation in which the processed values are stored contiguously.

Compare:

```text
original access pattern
transformed access pattern
compiler report
assembly
benchmark
```

The objective is not “find a clever intrinsic.” It is to understand when changing memory geometry is the better intervention.

---

## Exercise 7 — Function-call barrier

Start with:

```cpp
float transform(float x);

void apply(const float* in, float* out, int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = transform(in[i]);
}
```

Test these variants independently:

```text
1. out-of-line function
2. visible inline definition
3. /Ob2
4. /Ob3
5. directly exposed expression
```

Determine which intervention actually changes the vectorizer decision, if any.

Do not infer causality from `/Ob3` merely because the final build is faster.

---

## Exercise 8 — Floating-point reduction

Use:

```cpp
float sum(const float* a, int n)
{
    float s = 0.0f;

    for (int i = 0; i < n; ++i)
        s += a[i];

    return s;
}
```

Compare:

```text
/O2 /arch:AVX2 /fp:precise /Qvec-report:2
```

with:

```text
/O2 /arch:AVX2 /fp:fast /Qvec-report:2
```

Record:

```text
vectorization decision
assembly
numerical difference
```

Use a high-precision reference to measure error.

---

## Exercise 9 — Explicit AVX2 add

Implement:

```cpp
#include <immintrin.h>

void add_avx2(const float* a,
              const float* b,
              float* out,
              int n)
{
    int i = 0;

    for (; i + 8 <= n; i += 8)
    {
        const __m256 va = _mm256_loadu_ps(a + i);
        const __m256 vb = _mm256_loadu_ps(b + i);
        const __m256 vc = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(out + i, vc);
    }

    for (; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

Identify the assembly corresponding to:

```text
load
add
store
loop induction
scalar tail
```

The exercise is successful only when you can point to the actual target loop in disassembly.

---

## Exercise 10 — Scalar, compiler-assisted, explicit SIMD

Implement the same mathematical kernel three ways.

### A. Scalar source

```cpp
void add_scalar(const float* a,
                const float* b,
                float* out,
                int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

### B. Compiler-assisted

```cpp
void add_assisted(const float* __restrict a,
                  const float* __restrict b,
                  float* __restrict out,
                  int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

### C. Explicit AVX2

Use `_mm256_loadu_ps`, `_mm256_add_ps`, and `_mm256_storeu_ps`.

Compare:

```text
source-level control
vectorizer decision
assembly
correctness
runtime
```

This is the capstone distinction:

```text
compiler-controlled SIMD
→ compiler-assisted SIMD
→ explicit SIMD
```

---

# PART XVIII — Advanced Transfer Exercises

These exercises use more realistic code shapes without changing the course's central objective.

## Exercise 11 — Nested loop selection

Given:

```cpp
for (int i = 0; i < rows; ++i)
{
    for (int j = 0; j < cols; ++j)
        out[i * cols + j] = a[i * cols + j] + b[i * cols + j];
}
```

Determine which loop MSVC vectorizes and why.

Then change the dimensions so that `cols` becomes very small and observe whether profitability changes.

---

## Exercise 12 — Compile-time trip count

Compare:

```cpp
void scale(float* p, int n)
{
    for (int i = 0; i < n; ++i)
        p[i] *= 2.0f;
}
```

with a templated fixed-size form:

```cpp
template<int N>
void scale(float (&p)[N])
{
    for (int i = 0; i < N; ++i)
        p[i] *= 2.0f;
}
```

Inspect whether the compiler changes:

```text
loop structure
unrolling
remainder handling
instruction count
```

Do not assume a change must occur.

---

## Exercise 13 — Mixed access patterns

Given:

```cpp
for (int i = 0; i < n; ++i)
{
    out[i] = a[i] + b[i];
    reverse[i] = a[n - 1 - i];
}
```

Separate the operations into loops.

Determine whether the clean unit-stride loop is vectorized.

Then decide whether the total program improved or merely one loop became easier to optimize.

---

## Exercise 14 — Masked operation

Start with:

```cpp
for (int i = 0; i < n; ++i)
{
    if (a[i] > threshold)
        out[i] = a[i];
    else
        out[i] = threshold;
}
```

Create an explicit AVX2 compare + blend implementation.

Verify:

```text
lane semantics
assembly
NaN behavior if relevant
performance
```

---

## Exercise 15 — Compiler still refuses

Take a loop that remains scalar after a valid attempt with:

```text
/O2
/arch:AVX2
__restrict
alignment information
source restructuring
```

Do not immediately add more hints.

Determine whether one of these is true:

```text
unsupported operation
unfavorable data layout
insufficient trip count
compiler limitation
operation requires nontrivial lane manipulation
```

Then implement the operation with AVX2 intrinsics.

The objective is to experience the transition from:

```text
asking the compiler
```

to:

```text
specifying the vector dataflow yourself
```

---

# PART XIX — A Compact MSVC SIMD Lab

## 85. Baseline source

Use a single file for controlled experiments:

```cpp
#include <immintrin.h>
#include <cstddef>

void add_scalar(const float* a,
                const float* b,
                float* out,
                int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}

void add_restrict(const float* __restrict a,
                  const float* __restrict b,
                  float* __restrict out,
                  int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}

void add_avx2(const float* a,
              const float* b,
              float* out,
              int n)
{
    int i = 0;

    for (; i + 8 <= n; i += 8)
    {
        const __m256 va = _mm256_loadu_ps(a + i);
        const __m256 vb = _mm256_loadu_ps(b + i);
        const __m256 vc = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(out + i, vc);
    }

    for (; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

Compile the source as:

```text
/O2 /arch:AVX2 /Qvec-report:2
```

Then compare the three implementations.

---

# PART XX — Reference Patterns for Common Situations

## 86. “MSVC says possible aliasing”

First inspect the contract.

If the ranges genuinely cannot overlap:

```cpp
void kernel(float* __restrict a,
            float* __restrict b,
            float* __restrict out,
            int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}
```

Then compare the report and assembly.

If overlap is possible, do not use `__restrict` merely to obtain a vector loop.

---

## 87. “MSVC says dependency”

Ask:

```text
Is the dependence real?
```

If yes:

```text
restructure the algorithm
```

If no:

```text
determine whether the issue is actually aliasing
or a dependency that the compiler cannot disprove
```

Only then consider:

```cpp
#pragma loop(ivdep)
```

---

## 88. “MSVC says non-unit stride”

Ask:

```text
Can the data be stored contiguously for this kernel?
```

If yes:

```text
design the hot representation around unit stride
```

If no:

```text
consider whether the access pattern is still worth auto-vectorizing
or whether explicit vector operations are needed
```

Do not start with `ivdep`; stride is not a dependency problem.

---

## 89. “MSVC says too few iterations”

Do not assume that a pragma will solve this.

Ask:

```text
Can the kernel be batched?
Can several small operations be combined?
Is the fixed size actually known at compile time?
```

Sometimes scalar execution is exactly what the optimizer should choose.

The objective is not “SIMD at any cost.” The objective is deliberate SIMD generation when it is technically appropriate.

---

## 90. “The loop vectorized but has scalar code around it”

Inspect the control flow.

Possible explanations:

```text
scalar prologue
vector bulk loop
scalar remainder
runtime alias fallback
```

This is often normal.

Your question is:

```text
What fraction of the real work executes in the vector loop?
```

---

## 91. “I see `vmovups`, so alignment didn't work”

Not necessarily.

SIMD can be vectorized with unaligned loads.

The correct test is:

```text
Did vectorization happen?
```

then separately:

```text
Did the alignment assumption change load/store selection or loop setup?
```

Do not use the load instruction alone as evidence that SIMD failed.

---

# PART XXI — Technique Reference Table

| Technique | Problem / role | Exact syntax | What it tells or changes | Guarantees SIMD? | Verification | Main danger |
|---|---|---|---|---|---|---|
| `/O2` | Baseline speed optimization | `/O2` | Selects speed-oriented optimization set | No | Build command line, report, assembly | None specific to SIMD; Debug builds mislead analysis |
| `/arch:AVX2` | Enables AVX2 target generation | `/arch:AVX2` | Makes AVX2 available to target code generation | No | Command line, `__AVX2__`, disassembly | Runtime CPU must support target ISA |
| `/Qvec-report:2` | Diagnose vectorization | `/Qvec-report:2` | Reports vectorized and non-vectorized loops with reasons | No | Build Output | Diagnostic only |
| `__restrict` | Remove aliasing uncertainty when true | `float* __restrict p` | Gives MSVC a non-aliasing promise for the restricted access | No | Report, runtime guards, assembly | Invalid contract can make code incorrect |
| `#pragma loop(ivdep)` | Tell vectorizer to ignore vector dependencies | `#pragma loop(ivdep)` immediately before loop | Changes dependency treatment for vectorization | No | Report + assembly | Wrong on a truly dependent loop |
| `alignas` | Establish object alignment | `alignas(32) float a[1024];` | Aligns the declared object | No | Runtime address check + assembly | Does not automatically prove arbitrary derived pointer alignment |
| `_aligned_malloc` | Obtain dynamically aligned storage | `_aligned_malloc(bytes, 32)` | Allocates storage with requested alignment | No | Address check + assembly | Must use `_aligned_free` |
| `std::assume_aligned` | Communicate existing pointer alignment | `std::assume_aligned<32>(p)` | Returns a pointer carrying the alignment assumption | No | Assembly + correctness | Undefined behavior if assumption is false |
| `__assume` | Feed an optimizer assertion to MSVC | `__assume(condition);` | Optimizer treats condition as true on reachable code | No | Assembly + correctness | Invalid reachable assumption can cause unpredictable behavior |
| `/fp:precise` | Preserve stricter FP semantics | `/fp:precise` | Restricts some FP reorderings | No | Report + assembly + numerical tests | Some reassociation-based vectorization may be unavailable |
| `/fp:fast` | Relax FP transformation restrictions | `/fp:fast` | Permits more reordering/reassociation/contraction | No | Report + assembly + numerical tests | Numerical behavior can change |
| `/Ob2` | Normal optimization-time inlining | `/Ob2` | Allows compiler-directed inline expansion | No | Assembly, report | Inlining not guaranteed |
| `/Ob3` | More aggressive inlining | `/Ob3` | Increases inline-expansion aggressiveness | No | Assembly, report | Code size/register pressure; still not guaranteed |
| `/GL` | Whole-program optimization | `/GL` | Gives compiler cross-module information | No | Compare cross-module generated code | Build/toolchain constraints |
| `/LTCG` | Link-time code generation | `/LTCG` | Performs link-time code generation | No | Final binary | Longer link/build; workflow must match compilation |
| AVX2 intrinsics | Explicit vector operations | `#include <immintrin.h>` + `_mm256_*` | Expresses target-specific SIMD operations directly | Strongest source-level control; verify binary | Disassembly | ISA mismatch, semantic mistakes, tail handling |
| `_mm256_load_ps` | Aligned 8-float load | `_mm256_load_ps(p)` | Loads eight floats using aligned form | Explicit vector operation | Disassembly | Pointer must meet alignment requirement |
| `_mm256_loadu_ps` | Unaligned 8-float load | `_mm256_loadu_ps(p)` | Loads eight floats without aligned-form requirement | Explicit vector operation | Disassembly | No alignment guarantee conveyed |
| `_mm256_store_ps` | Aligned 8-float store | `_mm256_store_ps(p, v)` | Stores eight floats using aligned form | Explicit vector operation | Disassembly | Destination alignment required |
| `_mm256_storeu_ps` | Unaligned 8-float store | `_mm256_storeu_ps(p, v)` | Stores eight floats without aligned-form requirement | Explicit vector operation | Disassembly | No alignment guarantee conveyed |
| `_mm256_add_ps` | Eight packed float additions | `_mm256_add_ps(a, b)` | Lane-wise add | Explicit | Disassembly | Lane/type confusion |
| `_mm256_sub_ps` | Eight packed float subtractions | `_mm256_sub_ps(a, b)` | Lane-wise subtraction | Explicit | Disassembly | Operand-order mistakes |
| `_mm256_mul_ps` | Eight packed float multiplications | `_mm256_mul_ps(a, b)` | Lane-wise multiplication | Explicit | Disassembly | Normal FP semantics |
| `_mm256_div_ps` | Eight packed float divisions | `_mm256_div_ps(a, b)` | Lane-wise division | Explicit | Disassembly | High cost; not performance-equivalent to add/mul |
| `_mm256_fmadd_ps` | Eight fused multiply-adds | `_mm256_fmadd_ps(a, b, c)` | `a[i]*b[i]+c[i]` with FMA semantics | Explicit | Disassembly | Requires appropriate ISA; numerical semantics differ from unfused sequence |
| `_mm256_set1_ps` | Broadcast scalar to 8 lanes | `_mm256_set1_ps(x)` | Produces `[x x x x x x x x]` | Explicit | Disassembly | Constant materialization may differ |
| `_mm256_cmp_ps` | Packed comparison | `_mm256_cmp_ps(a,b,predicate)` | Produces per-lane mask | Explicit | Disassembly | Predicate semantics must be chosen correctly |
| `_mm256_blendv_ps` | Lane-wise selection | `_mm256_blendv_ps(a,b,mask)` | Selects between vectors per lane | Explicit | Disassembly | Mask polarity/source ordering |
| `_mm256_min_ps` / `_mm256_max_ps` | Lane-wise min/max | `_mm256_min_ps(a,b)` | Packed min/max semantics | Explicit | Disassembly + edge-case tests | NaN/signed-zero semantics |
| `_mm256_and_ps` / `_mm256_or_ps` / `_mm256_xor_ps` | Bit-level vector operations | `_mm256_and_ps(a,b)` etc. | Bitwise operations on packed values | Explicit | Disassembly | Treats floating-point objects as bit patterns |
| `_mm256_shuffle_ps` | Lane rearrangement | `_mm256_shuffle_ps(a,b,imm8)` | Rearranges selected lanes | Explicit | Disassembly + lane test | Immediate control mistakes |
| `_mm256_permutevar8x32_ps` | General lane permutation | `_mm256_permutevar8x32_ps(a,index)` | Rearranges eight 32-bit lanes | Explicit | Disassembly | Extra data movement |
| `_mm256_cvtps_epi32` | Float → integer conversion | `_mm256_cvtps_epi32(a)` | Converts packed floats | Explicit | Disassembly + rounding tests | Rounding/conversion semantics |
| `_mm256_cvtepi32_ps` | Integer → float conversion | `_mm256_cvtepi32_ps(a)` | Converts packed int32 to float | Explicit | Disassembly | Numeric conversion semantics |

---

# PART XXII — Evidence Hierarchy

When the question is specifically “Did SIMD actually happen?”, use this hierarchy.

```text
Level 0
The source looks SIMD-friendly.

Level 1
SIMD-enabling options or assumptions were supplied.

Level 2
MSVC reports that the loop was vectorized.

Level 3
The target loop in the disassembly contains packed instructions.

Level 4
The vector loop has the expected loads, computation, stores, and control flow.

Level 5
Correctness tests pass under the new assumptions.

Level 6
The isolated workload shows a measured benefit where benefit is expected.
```

The upper levels are not redundant with the lower levels.

For example:

```text
report says vectorized
```

does not automatically tell you:

```text
which exact instructions were chosen
```

and:

```text
assembly contains vaddps
```

does not automatically prove:

```text
the intended loop is the source of that instruction
```

---

# PART XXIII — Benchmarking the Right Thing

## 92. SIMD verification is not the same as performance validation

Suppose an entire program performs:

```text
input generation
formatting
file output
FFT
post-processing
Python/image processing
```

A whole-program timing result tells you little about a small SIMD kernel.

The correct experiment isolates the operation:

```text
kernel
→ warmup
→ repeated execution
→ prevent dead-code elimination
→ measure
```

Then separately verify the final application.

---

## 93. Compare equivalent implementations

For a kernel, compare:

```text
scalar source
compiler-assisted source
explicit intrinsic source
```

Keep fixed:

```text
compiler version
optimization options
ISA target
input size
input data
benchmark harness
```

A change in any of these can confound the result.

---

## 94. Use correctness before trusting performance

For each SIMD implementation:

```text
scalar reference
        ↓
SIMD implementation
        ↓
compare outputs
```

For floating-point kernels, use a numerical tolerance appropriate to the operation. For operations whose bitwise result is required, compare bitwise and do not silently substitute a looser tolerance.

When introducing:

```text
__restrict
__assume
std::assume_aligned
/fp:fast
```

correctness validation becomes part of the optimization experiment because you changed assumptions or semantics.

---

# PART XXIV — Final Mental Model

## 95. The full progression

The course's final model is:

```text
                compiler-controlled SIMD
                        │
                        │
        clean source + /O2 + /arch target
                        │
                        ▼
                auto-vectorizer
                        │
          ┌─────────────┴─────────────┐
          │                           │
       accepts                     refuses
          │                           │
          ▼                           ▼
     inspect report             inspect reason
                                      │
                                      ▼
                              targeted intervention
                                      │
                   __restrict / alignment / ivdep /
                  source restructuring / inlining / fp
                                      │
                                      ▼
                         compiler-assisted SIMD
                                      │
                                      │ still insufficient?
                                      ▼
                           explicit AVX2 intrinsics
                                      │
                                      ▼
                              assembly verification
```

The most important habit is:

> **Do not begin with a SIMD technique. Begin with the compiler's reason for not using SIMD.**

The second most important habit is:

> **Do not treat compiler diagnostics as the final proof. Read the generated machine code.**

The third is:

> **Do not confuse explicit vector source operations with a literal one-to-one promise about opcode spelling. Verify the final binary.**

---

# PART XXV — Final Capstone

## 96. The real task

Take one real C++ loop that you care about.

Do not begin by rewriting it.

### Phase A — Baseline

Compile:

```text
/O2 /arch:AVX2 /Qvec-report:2
```

Record the report.

### Phase B — Diagnosis

Determine which category applies:

```text
dependency
aliasing
stride/layout
alignment information
function visibility
control flow
unsupported operation
profitability
floating-point semantics
```

### Phase C — Minimal intervention

Apply exactly one technique.

### Phase D — Recompile

Read the report again.

### Phase E — Machine-code proof

Open the disassembly and identify:

```text
main vector loop
vector loads
vector arithmetic
vector stores
induction update
remainder/fallback paths
```

### Phase F — Correctness

Compare with the baseline implementation.

### Phase G — Performance

Benchmark the isolated kernel.

### Phase H — Escalation

If auto-vectorization remains inadequate, implement the core operation with AVX2 intrinsics.

Then inspect the resulting assembly again.

The capstone is complete only when you can explain the entire chain:

```text
original source
→ compiler obstacle
→ chosen intervention
→ changed compiler decision
→ resulting assembly
→ correctness
→ measured effect
```

That chain is the practical skill this course is designed to produce.

---

# Appendix A — Current MSVC documentation map

Use Microsoft documentation as the authority for option syntax and toolset behavior. The exact optimizer decision and emitted assembly remain compiler-version dependent.

1. `/O` options: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/build/reference/o-options-optimize-code?view=msvc-170)
2. `/arch (x64)`: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/build/reference/arch-x64?view=msvc-170)
3. `/Qvec-report`: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/build/reference/qvec-report-auto-vectorizer-reporting-level?view=msvc-170)
4. Vectorizer messages: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/error-messages/tool-errors/vectorizer-and-parallelizer-messages?view=msvc-170)
5. `__restrict`: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/cpp/extension-restrict?view=msvc-170)
6. `loop` pragma: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/preprocessor/loop?view=msvc-170)
7. `__assume`: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/intrinsics/assume?view=msvc-170)
8. `/Ob`: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/build/reference/ob-inline-function-expansion?view=msvc-170)
9. `/GL`: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/build/reference/gl-whole-program-optimization?view=msvc-170)
10. `/LTCG`: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/build/reference/ltcg-link-time-code-generation?view=msvc-170)
11. `/fp`: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior?view=msvc-170)
12. x86/x64 intrinsics: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/intrinsics/x86-intrinsics-list?view=msvc-170)
13. C++ alignment declarations: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/cpp/alignment-cpp-declarations?view=msvc-170)
14. `_aligned_malloc`: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc?view=msvc-170)
15. `std::assume_aligned`: [cppreference](https://en.cppreference.com/w/cpp/memory/assume_aligned)

---

# Appendix B — Exact Visual Studio build baseline

For the main experiments, establish one Release x64 configuration.

Recommended baseline:

```text
Configuration: Release
Platform:      x64
Optimization:  /O2
ISA:           /arch:AVX2
Diagnostics:   /Qvec-report:2
```

For explicit intrinsic experiments:

```text
Configuration: Release
Platform:      x64
Optimization:  /O2
ISA:           /arch:AVX2
```

For floating-point reduction experiments:

```text
/O2 /arch:AVX2 /fp:precise /Qvec-report:2
```

versus:

```text
/O2 /arch:AVX2 /fp:fast /Qvec-report:2
```

For inlining experiments:

```text
/O2 /arch:AVX2 /Qvec-report:2 /Ob2
```

versus:

```text
/O2 /arch:AVX2 /Qvec-report:2 /Ob3
```

Do not mix independent experimental variables unless the exercise explicitly asks for a combined configuration.

---

# Appendix C — The one-page checklist

```text
┌──────────────────────────────────────────────────────────┐
│              MAKE SIMD ACTUALLY HAPPEN                   │
├──────────────────────────────────────────────────────────┤
│ 1. Build Release x64                                     │
│ 2. Use /O2                                               │
│ 3. Select /arch:AVX2                                     │
│ 4. Enable /Qvec-report:2                                 │
│ 5. Identify the exact target loop                        │
│ 6. Read the compiler reason                              │
│ 7. Classify the obstacle                                 │
│ 8. Apply one justified intervention                       │
│ 9. Recompile                                             │
│10. Read the report again                                 │
│11. Inspect the target loop in disassembly                │
│12. Identify packed operations                             │
│13. Account for scalar cleanup/versioned paths             │
│14. Verify the target ISA                                 │
│15. Run correctness tests                                 │
│16. Benchmark the isolated kernel                         │
│17. Escalate to intrinsics when auto-vectorization         │
│    does not provide enough control                        │
└──────────────────────────────────────────────────────────┘
```

The practical progression is therefore not:

```text
learn SIMD
→ write intrinsics
```

It is:

```text
observe compiler
→ diagnose
→ intervene
→ verify
→ escalate control only when necessary
```

---

# Final statement

The course is successful when the learner can take an arbitrary realistic MSVC loop and answer, with evidence:

```text
Why didn't MSVC vectorize this?

What exact fact or transformation is missing?

What MSVC/C++ syntax communicates that fact?

Did the compiler's decision actually change?

What machine instructions were generated?

Did the main work execute in SIMD?

Is the result correct?

Did the SIMD implementation improve the measured kernel?
```

The learner should be able to move deliberately between three control modes:

```text
compiler-controlled SIMD
        ↓
compiler-assisted SIMD
        ↓
explicit intrinsic-based SIMD
```

without confusing any of them with a guarantee that has not been verified in the generated machine code.
