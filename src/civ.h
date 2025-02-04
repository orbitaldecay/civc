// civc header file.
//
// version: 0.0.2

#ifndef __CIV_H
#define __CIV_H

// Types and functions
#include <stdbool.h> // bool
#include <stdlib.h>  // size_t, NULL, malloc, exit, etc.
#include <stdint.h>  // usize16_t, etc
#include <string.h>  // (mem|str)(cmp|cpy|move|set), strlen
#include <setjmp.h>  // setjmp

// Testing and debuggin
#include <assert.h>  // assert
#include <stdio.h>   // printf
#include <errno.h>   // errno global


#if UINTPTR_MAX == 0xFFFFFFFF
#define RSIZE   4
#else
#define RSIZE   8
#endif

#define ALIGN1      1
#define ALIGN_SLOT  RSIZE

#define not  !
#define and  &&
#define or   ||
#define msk  &
#define jn   |

#define eprintf(F, ...)   fprintf(stderr, F __VA_OPT__(,) __VA_ARGS__)

// #################################
// # Core Types and common methods

// Core Types
typedef uint8_t              U1;
typedef uint16_t             U2;
typedef uint32_t             U4;
typedef uint64_t             U8;
typedef size_t               S;

typedef int8_t               I1;
typedef int16_t              I2;
typedef int32_t              I4;
typedef int64_t              I8;
#if RSIZE == 4
typedef I4                   ISlot;
#else
typedef I8                   ISlot;
#endif

extern const U1* emptyNt; // empty null-terminated string

// ####
// # Core Structs
typedef struct { U1*   dat;   U2 len;                    } Slc;
typedef Slc                                                Str;
typedef struct { U1*   dat;   U2 len;  U2 cap;           } Buf;
typedef struct { S*    dat;   U2 len;  U2 cap;           } SBuf;
typedef struct { U1*   dat;   U2 len;  U2 cap; U2 plc;   } PlcBuf;
typedef struct { S*    dat;   U2 sp;   U2 cap;           } Stk;
typedef struct { U1    len;   U1 dat[];                  } CStr;
typedef struct { U1*   dat;   U2 head; U2 tail; U2 _cap; } Ring;

typedef struct _Sll {
  struct _Sll* next;
  S            dat;
} Sll;

typedef struct _Dll {
  struct _Dll* next;
  struct _Dll* prev;
  void* dat;
} Dll;

typedef struct { Dll* start; } DllRoot;

// ####
// # Debug types and functions

typedef struct {
  SBuf trace;
  char** messages;
  struct sigcontext* ctx; // can be NULL
  int sig;
} Trace;

void  Trace_print(Trace* t);
void  Trace_free(Trace* t);

// ####
// # Core functions

void defaultErrPrinter();
void runErrPrinter();

// Get the required addition/subtraction to ptr to achieve alignment
S align(S ptr, U2 alignment);

// Most common allignment
static inline S alignment(S sz) {
  switch (sz) {
    case 1: return 1;
    case 2: return 2;
    case 3:
    case 4: return 4;
    default: return RSIZE;
  }
}

// Clear or set bits
#define bitClr(V, MASK)      ((V) & (~(MASK)))
#define bitSet(V, SET, MASK) (bitClr(V, MASK) | (SET))

// ##
// # Big Endian (unaligned) Fetch/Store
// Big endian store the largest values first. They are frequently used for
// networking code or where bytes are packed together without being aligned.
//
// The naming is confusing, since "big endian" makes you think the "big" value
// would be at the "end". This is not the case. The name was originally derived
// from Gulliver's travels where the "Liliputians", aka the Little
// Putians(Endians), break the little part of the egg on their "head". For an
// array, the start is also called the head. The Liliputians went to war with
// their neighbors (who broke the big side of the egg on their head) due to the
// difference of this custom.  Similarily, hardware architects at the dawn of
// computing were going to war over big/little endianness.

static inline U2 ftBE2(U1* p) { return (*p<<8) + *(p + 1); }
static inline U4 ftBE4(U1* p) {
  return (*p << 24) + (*(p + 1)<<16) + (*(p + 2)<<8) + *(p + 3);
}

static inline void srBE2(U1* p, U2 value) { *p = value>>8; *(p+1) = value; }
static inline void srBE4(U1* p, U4 value) {
 *p = value>>24; *(p+1) = value>>16; *(p+2) = value>>8; *(p+3) = value;
}

U4   ftBE(U1* p, S size);
void srBE(U1* p, S size, U4 value);

// ##
// # min/max
#define MIN_DEF { if(a < b) return a; return b; }
static inline U4   U4_min (U4  a, U4  b) MIN_DEF
static inline S    S_min(S a, S b) MIN_DEF

#define MAX_DEF { if(a < b) return a; return b; }
static inline U4   U4_max (U4  a, U4  b) MAX_DEF
static inline S S_max(S a, S b) MAX_DEF

// ##
// # div
static inline U4 U4_ceil(U4 a, U4 b) { return (a / b) + (a % b != 0); }

// #################################
// # Slc: data slice of up to 64KiB indexes (0x10,000)
// Slice is just a data pointer and a len.
Slc  Slc_frNt(U1* s); // from null-terminated str
Slc  Slc_frCStr(CStr* c);
I4   Slc_cmp(Slc a, Slc b);
#define Slc_eq(A, B)   (0 == Slc_cmp(A, B))


// Perform 'to = from'. Return the number of bytes moved.
// This will always move the most bytes it can (attempting to fill 'to').
U2   Slc_move(Slc to, Slc from);

#define _ntLit(STR)        .dat = STR, .len = sizeof(STR) - 1
#define SLC(STR)           ((Slc){ _ntLit(STR) })
#define Buf_ntLit(STR)     ((Buf)    { _ntLit(STR), .cap = sizeof(STR) - 1 })
#define PlcBuf_ntLit(STR)  ((PlcBuf) { _ntLit(STR), .cap = sizeof(STR) - 1 })
#define Slc_lit(...)  (Slc){           \
    .dat = (U1[]){__VA_ARGS__},        \
    .len = sizeof((U1[]){__VA_ARGS__}) \
  }

// Example:
//   Buf_var(myBuf, 20); Buf_extend(&myBuf, someData);
#define Buf_var(NAME, CAP) \
  U1 LINED(_bufDat)[CAP]; Buf NAME = (Buf){.dat = LINED(_bufDat), .cap = CAP}
#define PlcBuf_var(NAME, CAP) \
  U1 LINED(_bufDat)[CAP]; PlcBuf NAME = (PlcBuf){.dat = LINED(_bufDat), .cap = CAP}

Slc Slc_slc(Slc* s, U2 start, U2 end);
Slc Buf_slc(Buf* b, U2 start, U2 end);

// Use this with printf using the '%.*s' format, like so:
//
//   printf("Printing my slice: %.*s\n", Dat_fmt(slc));
//
// This is safe to use with CStr, Buf and PlcBuf.
#define Dat_fmt(DAT)   (DAT).len, (DAT).dat
#define CStr_fmt(C)    (C)->len,  (C)->dat

// #################################
// # Buf + PlcBuf: buffers of up to 64KiB indexes (0x10,000)
// Buffers are a data pointer, a length (used data) and a capacity
// PlcBuf has an additional field "plc" to keep place while processing a buffer.
static inline Slc* Buf_asSlc(Buf* b)       { return (Slc*) b; }
static inline Slc* PlcBuf_asSlc(PlcBuf* p) { return (Slc*) p; }
static inline Buf* PlcBuf_asBuf(PlcBuf* p) { return (Buf*) p; }

static inline Slc PlcBuf_plcAsSlc(PlcBuf* p) {
  return (Slc) {.dat = p->dat + p->plc, .len = p->len - p->plc};
}

static inline void Buf_clear(Buf* b) { b->len = 0; }
static inline void _Buf_add(Buf* b, U1 v) { b->dat[b->len++] = v; }
static inline bool Buf_isFull(Buf* b) { return b->len == b->cap; }

// Attempt to extend Buf.
void Buf_add(Buf* b, U1 v);
void Buf_addBE2(Buf* b, U2 v);
void Buf_addBE4(Buf* b, U4 v);
void Buf_extend(Buf* b, Slc s);
void Buf_extendNt(Buf* b, U1* s);

static inline void PlcBuf_extend(PlcBuf* b, Slc s) { Buf_extend((Buf*) b, s); }

// #################################
// # Stk: efficient first-in last-out buffer.
// Stacks "grow down" so that indexes can be accessed using positive offsets.

// Initialize the stack. The cap must be the total number of Slots
// (NOT the size in bytes)
#define Stk_init(DAT, CAP) (Stk) (Stk) {.dat = DAT, .sp = CAP, .cap = CAP}
#define Stk_clear(STK)     ((STK)->sp = (STK)->cap)


// Get the number of slots in use.
#define Stk_len(STK)             ((STK)->cap - (STK)->sp)

S Stk_pop(Stk* stk); // pop a value from the stack, reducing it's len
S Stk_top(Stk* stk); // Get stack top (without altering stack)
S* Stk_topRef(Stk* stk); // get the reference to the top
void Stk_add(Stk* stk, S value); // add a value to the stack
                                    //
#define Stk_add2(STK, A, B)     Stk_add(STK, A);     Stk_add(STK, B)
#define Stk_add3(STK, A, B, C)  Stk_add2(STK, A, B); Stk_add(STK, C)
#define Stk_pop2(STK, A, B)     B = Stk_pop(STK);    A = Stk_pop(STK)
#define Stk_pop3(STK, A, B, C)  Stk_pop2(STK, B, C); A = Stk_pop(STK)

// #################################
// # Ring: a lock-free ring buffer.
// Data is written to the tail and read from the head.
#define Ring_init(DAT, datLen)   (Ring){.dat = DAT, ._cap = datLen}
#define Ring_var(NAME, CAP)     \
  U1 LINED(_ringDat)[CAP + 1]; Ring NAME = Ring_init(LINED(_ringDat), CAP + 1)
#define Ring_drop(RING, ARENA)   Xr(ARENA, free, (RING)->dat, (RING)->_cap, 1)

#define Ring_isEmpty(R)     ((R)->head ==  (R)->tail)
#define Ring_isFull(R)      ((R)->head == ((R)->tail + 1) % (R)->_cap)

U2   Ring_len(Ring* r);
U1   Ring_get(Ring* r, U2 i);

// The capacity of the Ring is one less than the buffer capacity.
#define Ring_cap(RING)    ((RING)->_cap - 1)
#define Ring_remain(RING) (Ring_cap(RING) - Ring_len(RING))

static inline void Ring_clear(Ring* r) { r->head = 0; r->tail = 0; }

// Get the current character and advance the head.
// Returns NULL if empty.
// Typically this is used like:  while(( c = Ring_next(r) )) { ... }
U1*  Ring_next(Ring* r);

// Warning: Panics if length not sufficient
U1     Ring_pop(Ring* r); 
void   Ring_push(Ring* r, U1 c);
void   Ring_extend(Ring* r, Slc s);

// This API is for:
// 1. Get an available contiguous slice of memory and store some data in it.
//    Note: Ring_avail MAY NOT be the entire amount of available memory. It is
//    only the contiguous memory. To fill the Ring you may need to call this
//    twice.
// 2. Record how much was used with incTail.
Slc  Ring_avail(Ring* r);
static inline void Ring_incTail(Ring* r, U2 inc) {
  r->tail = (r->tail + inc) % r->_cap;
}

// This API is for:
// 1. Get the first "chunk" of data and use some amount of it.
// 2. incHead by the amount used.
#define Ring_fmt1(R)  Dat_fmt(Ring_1st(R))
#define Ring_fmt2(R)  Dat_fmt(Ring_2nd(R))

Slc Ring_1st(Ring* r);
Slc Ring_2nd(Ring* r);
static inline void Ring_incHead(Ring* r, U2 inc) {
  r->head = (r->head + inc) % r->_cap;
}

I4  Ring_cmpSlc(Ring* r, Slc s);

// Remove dat[:plc], shifting data[plc:len] to the left.
//
// This is extremely useful when reading files: a few bytes (i.e. a word, a
// line) can be processed at a time, then File_read called again.
void PlcBuf_shift(PlcBuf*);

// CStr
// Declare a CStr global. It's your job to assert that the LEN is valid.
static inline Slc CStr_asSlc(CStr* c) { return (Slc) {c->dat, .len=c->len}; }
static inline Slc CStr_asSlcMaybe(CStr* c) {
  if(c) return CStr_asSlc(c);
  return (Slc){NULL, .len=0};
}

#define CStr_ntLitUnchecked(NAME, LEN, STR) \
  char _CStr_ ## NAME[1 + sizeof(STR)] = LEN STR; \
  CStr* NAME = (CStr*) _CStr_ ## NAME;

// Declare a CStr variable with auto-assert.
// The assert runs every time the function is called (not performant),
// but this is fine for one-off functions or tests.
#define CStr_ntVar(NAME, LEN, STR)      \
  static CStr_ntLitUnchecked(NAME, LEN, STR); \
  assert(CStr_varAssert(__LINE__, STR, LEN));

static inline Slc Slc_ntLit(U1* s) {
  if(not s) return (Slc) { .dat = NULL };
  return (Slc) { .dat = s, .len = strlen(s) };
}

static inline bool CStr_varAssert(U4 line, U1* str, U1* len) {
  if(1 != strlen(len)) {
    eprintf("ERROR CStr_var [line=%u]: LEN must be single byte (line=%u)");
    return false;
  }
  if(len[0] != strlen(str)) {
    eprintf("ERROR CStr_var [line=%u]: Use LEN = \"\\x%.2X\"\n", line, strlen(str));
    return false;
  }
  return true;
}

CStr* CStr_init(CStr* this, Slc s);

// #################################
// # Sll: Singly Linked List
void Sll_add(Sll** root, Sll* node);
Sll* Sll_pop(Sll** root);
S    Sll_len(Sll* node);

// Reverse the linked list, returning the new start.
Sll* Sll_reverse(Sll* node);

// #################################
// # Dll: Doubly Linked List

// Add to next: to -> a ==> to -> b -> a
void Dll_add(Dll* to, Dll* node);

// Pop from next: from -> b -> a ==> from -> a (return b)
//
// Note: from->prev is never used, so the following is safe:
//
//   Dll* root = ...;
//   Dll* b = Dll_pop((Dll*)&root);
Dll* Dll_pop(Dll* from);

// Remove from chain: a -> node -> b ==> a -> b
Dll* Dll_remove(Dll* node);

// Add to root, preserving prev/next nodes.
//
//     root              root
//       v      ==>        v
// a <-> b           a <-> node <-> b
void DllRoot_add(DllRoot* root, Dll* node);

Dll* DllRoot_pop(DllRoot* root);

// #################################
// # Binary Search Tree
typedef struct _CBst {
  struct _CBst* l; struct _CBst* r;
  CStr* key;
} CBst;

// Find slice in CBst, starting at `*node`. Set result to `*node`
// Else, the return value is the result of `Slc_cmp(node.ckey, slc)`
//
// This can be used like this:
//   CBst* node = NULL;
//   I4 cmp = CBst_find(&node, SLC("myNode"));
//   // if   not node    : *node was null (CBst is empty)
//   // elif cmp == 0    : *node key == "myNode"
//   // elif cmp < 0     : *node key <  "myNode"
//   // else cmp > 0     : *node key >  "myNode"
I4   CBst_find(CBst** node, Slc slc);

// Add a node to the tree, modifying *root if the node becomes root.
//
// Returns NULL if `add.key` did not exist in the tree. Else returns the
// existing node (which is no longer in the tree).
CBst* CBst_add(CBst** root, CBst* add);

// #################################
// # Error Handling and Testing

// Compiler state to disable error logs when expecting an error.
#define TEST(NAME) \
  void test_ ## NAME () {                  \
    jmp_buf localErrJmp;                   \
    Fiber fb;                              \
    Fiber_init(&fb, &localErrJmp);         \
    Civ_init(&fb);                         \
    eprintf("## Testing " #NAME "...\n");  \
    if(setjmp(localErrJmp)) {              \
      eprintf("!! test failed with error !!\n"); \
      exit(1);                             \
    }
#define END_TEST  }

#define ERR_EXPECTED  (Fiber_EXPECT_ERR & civ.fb->state)

#define SET_ERR(E)  if(true) { \
  civ.fb->err = E; \
  if(not ERR_EXPECTED) runErrPrinter(); \
  longjmp(*civ.fb->errJmp, 1); }
#define ASSERT(C, E)   do { if(!(C)) { SET_ERR(SLC(E)); } } while(0)
#define ASSERT_NO_ERR()    assert(!civ.fb->err)

#define TASSERT_EQ(EXPECT, CODE) if(1) { \
  typeof(EXPECT) __result = CODE; \
  if((EXPECT) != __result) { \
    eprintf("!!! Assertion failed: 0x%X == 0x%X\n", EXPECT, __result); \
    SET_ERR(SLC("assertion failed, see stderr")); \
  } }

#define TASSERT_SLC_EQ(EXPECT, S)       \
  if(Slc_cmp(SLC(EXPECT), S)) {   \
    eprintf("!!! Slices not equal: \n  " EXPECT "\n  %.*s\n", Dat_fmt(S)); \
    assert(false); }

#define TASSERT_RING_EQ(EXPECT, RING)       \
  if(Ring_cmpSlc(RING, SLC(EXPECT))) {   \
    eprintf("!!! Ring not equal: \n  " EXPECT "\n  %.*s%.*s\n", Ring_fmt1(RING), Ring_fmt2(RING)); \
    assert(false); }

#define TASSERT_STK(EXPECT, STK)  TASSERT_EQ(EXPECT, Stk_pop(STK))

// Macro expansion shenanigans. Note that a plain foo ## __LINE__ expands to the
// literal string "foo__LINE__", when you wanted "foo362" (when line=362)
#define _JOIN(A, B) A ## B
#define JOIN(A, B) _JOIN(A, B)
#define LINED(A)    JOIN(A, __LINE__)

// Execute CODE. HANDLE will be executed if an error longjmp occurs.
#define HANDLE_ERR(CODE, HANDLE) \
  jmp_buf* LINED(prevJmp) = civ.fb->errJmp;                \
  jmp_buf LINED(newJmp); civ.fb->errJmp = &LINED(newJmp);  \
  if(setjmp(LINED(newJmp))) {             \
    civ.fb->errJmp = LINED(prevJmp);      \
    HANDLE;                               \
  } else { CODE; }



// Execute CODE and expect an error longjmp
#define EXPECT_ERR(CODE)                  \
  civ.fb->state |= Fiber_EXPECT_ERR;      \
  HANDLE_ERR(                             \
    { CODE; ASSERT(false, "expected error never happend"); } \
    , civ.fb->state &= ~Fiber_EXPECT_ERR)

// #################################
// # Methods and Roles
// C is a VERY annoying language when trying to improve the ergonomics of
// defining methods. There is no way (at compile time) to safely define a
// function as one with a different type at compile.
//
// The DECLARE/DEFINE METHOD macros below create both the MyType_method
// function and a M_MyType_method function reference which casts MyType_method
// with a "void* this" as the first parameter, so it can be used in the Role.
//
// In your .h file:
// - use DECLARE_METHOD(someReturnType, MyType,myMethod, arg1)
//   - Note: this also declares M_MyType_myMethod for you.
// - Define a MyType_mRole function which returns a method pointer for the
//   relevant Role.
// - declare a "Role MyType_asRole(MyType*)" function
//
// In your .c file:
// - use DEFINE_METHOD(someReturnType, MyType,myMethod, arg1)
//   - Note: this also defines M_MyType_myMethod for you.
// - use DEFINE_METHODS(Role, MyType_mRole, .myMethod=M_MyType_myMethod)
// - define the "Role MyType_asRole(MyType*)" function using the above method.
//
// For an example, see civ's implementation of the BBA methods.

#define DECLARE_METHOD(RETURNS, TYPE, NAME, ...)                          \
  extern RETURNS (*M_ ## TYPE ## _ ## NAME)(void* __VA_OPT__(,) __VA_ARGS__);    \
  RETURNS TYPE ## _ ## NAME(TYPE* this __VA_OPT__(,) __VA_ARGS__)

#define DEFINE_METHOD(RETURNS, TYPE, NAME, ...)                           \
  RETURNS (*M_ ## TYPE ## _ ## NAME)(void* __VA_OPT__(,) __VA_ARGS__) =   \
    ( RETURNS (*)(void* __VA_OPT__(,) __VA_ARGS__) ) TYPE ## _ ## NAME;   \
  RETURNS TYPE ## _ ## NAME(TYPE* this __VA_OPT__(,) __VA_ARGS__)

#define DEFINE_METHODS(RETURNS, NAME, ...) \
  RETURNS* NAME() { \
    static RETURNS m = {0}; \
    static bool notInit = true; \
    if(notInit) m = (RETURNS) { __VA_ARGS__ }; \
    return &m; \
  }

// Role Execute: Xr(myRole, meth, a, b) -> myRole.m.meth(myRole.d, a, b)
#define Xr(R, M, ...)    (R).m->M((R).d __VA_OPT__(,) __VA_ARGS__)

// Declare role method:
//   Role_METHOD(myFunc, U1, U2) -> void (*)(void*, U1, U2) myFunc
#define Role_METHOD(M, ...)      ((void (*)(void* __VA_OPT__(,) __VA_ARGS__)) M)
#define Role_METHODR(M, R, ...)  ((R    (*)(void* __VA_OPT__(,) __VA_ARGS__)) M)

// #################################
// # BA: Block Allocator
#define BLOCK_PO2  12
#define BLOCK_SIZE  (1<<BLOCK_PO2)
#define BLOCK_AVAIL (BLOCK_SIZE - (sizeof(U2) * 2))
#define BLOCK_END  0xFF

typedef struct {
  U1 dat[BLOCK_AVAIL];
  U2 bot;
  U2 top;
} Block;

typedef struct _BANode {
  struct _BANode* next; struct _BANode* prev; // Dll
  Block* block;
} BANode;

typedef struct { BANode* free; S len; } BA;

Dll*     BANode_asDll(BANode* node);
DllRoot* BA_asDllRoot(BA* ba);

BANode* BA_alloc(BA* ba);
void BA_free(BA* ba, BANode* node);
void BA_freeAll(BA* ba, BANode* nodes);

// Free an array of nodes and blocks. Typically used to initialize BA.
void BA_freeArray(BA* ba, S len, BANode nodes[], Block blocks[]);

// #################################
// # Arena Role
typedef struct {
  void  (*drop)            (void* d);
  void* (*alloc)           (void* d, S sz, U2 alignment);
  Slc*  (*free)            (void* d, void* dat, S sz, U2 alignment);
  S  (*maxAlloc)        (void* d);
} MArena;


typedef struct { const MArena* m; void* d; } Arena;

// Methods that depend on arena
Slc* Sll_free(Sll* node, U2 nodeSz, Arena a);

CStr* CStr_new(Arena a, Slc s);

static inline Buf Buf_new(Arena a, U2 cap) { // Note: check that buf.dat != NULL
  return (Buf) { .dat = Xr(a,alloc, cap, 1), .cap = cap, };
}

static inline bool Buf_free(Buf* b, Arena a) {
  return Xr(a,free, b->dat, b->cap, 1);
}

static inline PlcBuf PlcBuf_new(Arena arena, U2 cap) {
  return (PlcBuf) { .dat = Xr(arena, alloc, cap, 1), .cap = cap };
}

static inline bool PlcBuf_free(PlcBuf* p, Arena a) {
  return Buf_free(PlcBuf_asBuf(p), a);
}

Trace Trace_new(Arena* a, int cap);

// #################################
// # Resource Role
typedef struct {
  // Perform resource cleanup and drop any non-POD data (i.e. buffers, etc)
  // This must NOT free the `d` pointer itself.
  //
  // Return true if drop is done. Returning false means it will be called again
  // after other resources are dropped.
  bool (*drop)            (void* d, Arena a);

  // Get the Sll view of the resource. This is used by Arenas to track resources
  // tied to them so they can be dropped.
  Sll* (*resourceLL)       (void* d);
} MResource;

typedef struct { const MResource* m; void* d; } Resource;
static inline Resource* Arena_asResource(Arena* a) { return (Resource*) a; }

// #################################
// # BBA: Block Bump Arena
// This is a bump arena. Allocations "bump" the len (unaligned) or cap (aligned)
// indexes. Frees reverse the bump and MUST be done in reverse order (or skipped
// if you drop the whole arena). Free must use the exact same arguments as
// alloc.
//
// This allocator is great for data that just grows and is rarely freed or the
// free-order is very controlled. This is probably the fastest possible
// arbitrary-sized allocator.
//
// Fngi uses this allocator for "growing" the code heap. Therefore,
// unaligned allocations always grow from top to bottom.

typedef struct { BA* ba; BANode* dat; } BBA;

DllRoot* BBA_asDllRoot(BBA* bba);
Arena    BBA_asArena(BBA* b);
#define  BBA_block(BBA) ((BBA)->dat->block)

DECLARE_METHOD(void, BBA,drop);   // BBA_drop
DECLARE_METHOD(S , BBA,spare); // BBA_spare
DECLARE_METHOD(void*, BBA,alloc, S sz, U2 alignment); // BBA_alloc

// BBA_free: return any error slc.
DECLARE_METHOD(Slc* , BBA,free , void* data, S sz, U2 alignment); // BBA_free
DECLARE_METHOD(S , BBA,maxAlloc); // BBA_maxAlloc

MArena* mBBAGet();


// #################################
// # Civ Global Environment

// fiberState bitfield
#define Fiber_EXPECT_ERR   (0x80 /*disable error logging*/)

typedef struct _Fiber {
  struct _Fiber* next;
  struct _Fiber* prev;
  void* variant;
  U2 state;
  jmp_buf*   errJmp;
  Slc err;
} Fiber;

typedef struct {
  BA         ba;    // root block allocator
  Fiber*     fb;    // currently executing fiber

  // Misc
  void (*errPrinter)();
} Civ;

extern char**     ARGV;
extern Civ        civ;

void Civ_init(Fiber* fb);

static inline void Fiber_init(Fiber* fb, jmp_buf* errJmp) {
  *fb = (Fiber) {
    .errJmp = errJmp,
  };
}

// #################################
// # File
// Unlike many roles, the File role requires the data structure to follow the
// below. This is because interacting with files are inherently interacting with
// buffers.
//
// System-specific data can expand on this, such as storing the file name/etc
// as a child-class of File.
//
// The file uses a PlcBuf. Typically the user will ingest some number of bytes,
// moving buf.plc and then calling PlcBuf_shift to clear data and read more
// bytes from the file.

#define File_seek_SET  1 // seek from beginning
#define File_seek_CUR  2 // seek from current position
#define File_seek_END  3 // seek from end

typedef struct {
  Ring      ring;         // buffer for reading or writing data
  U2        code;         // status or error (File_*)
} BaseFile;

typedef struct {
  // Resource methods
  void      (*drop)            (void* d, Arena a);
  Sll*      (*resourceLL)      (void* d);

  // Close a file
  void      (*close) (void* d);

  // Open a file. Platform must define File_(RDWR|RDONLY|WRONLY|TRUNC)
  void      (*open)  (void* d, Slc path, S options);

  // Stop async operations (may be noop)
  void      (*stop)  (void* d);

  // Seek in the file whence=File_seek_(SET|CUR|END)
  void      (*seek)  (void* d, ISlot offset, U1 whence);

  // Read from a file into d buffer.
  void      (*read)  (void* d);

  // Get the base file pointer. This is between read and write so that File can
  // be converted to those Roles at zero cost.
  BaseFile* (*asBase) (void* d);

  // Write to a file from d buffer.
  void      (*write)(void* d);
} MFile;

typedef struct { const MFile* m; void* d; } File;  // Role

typedef struct {
  void          (*read)   (void* d);
  BaseFile*     (*asBase) (void* d);
} MReader;
typedef struct { const MReader* m; void* d; } Reader;
static inline Reader File_asReader(File f) {
  return (Reader) { .m = (MReader*) &f.m->read, .d = f.d };
}

typedef struct {
  BaseFile* (*asBase) (void* d);
  void      (*write)  (void* d);
} MWriter;
typedef struct { const MWriter* m; void* d; } Writer;
static inline Writer File_asWriter(File f) {
  return (Writer) { .m = (MWriter*) &f.m->asBase, .d = f.d };
}

static inline Resource* File_asResource(File* f) {
  return (Resource*) f;
}

// Extend data to file's ring, writing if necessary.
void File_extend(File f, Slc s);
void Writer_extend(Writer w, Slc s);

// Get the pointer to index, reading if necessary.
U1* Reader_get(Reader f, U2 i);

#define File_CLOSED   0x00

#define File_SEEKING  0x10
#define File_READING  0x11
#define File_WRITING  0x12
#define File_STOPPING 0x13

#define File_DONE     0xD0
#define File_STOPPED  0xD1
#define File_EOF      0xD2

#define File_ERROR    0xE0
#define File_EIO      0xE2
void File_panicOpen(void* d, Slc, S); // unsuported open
void File_panic(void* d); // used to panic for unsported method
void File_noop(void* d);  // used as noop for some file methods

static inline bool BaseFile_eof(BaseFile* b) {
  return (File_EOF == b->code) and Ring_isEmpty(&b->ring);
}

static inline bool File_eof(File f)     { return BaseFile_eof(Xr(f,asBase)); }
static inline bool Reader_eof(Reader f) { return BaseFile_eof(Xr(f,asBase)); }

// #################################
// # BufFile
// A file backed by a buffer.

typedef struct {
  Sll*      nextResource;
  Ring      ring;
  U2        code;
  PlcBuf    b;
} BufFile;

MFile* BufFile_mFile();

static inline BufFile BufFile_init(Ring r, Buf b) {
  return (BufFile) {
    .ring = r,
    .code = File_DONE,
    .b = (PlcBuf){.dat = b.dat, .len = b.len, .cap = b.cap},
  };
}

// Typical use:
// BufFile_var(f, 16, "An example string."");
#define BufFile_var(NAME, ringCap, STR)              \
  U1 LINED(_ringDat)[ringCap + 1];                      \
  BufFile NAME = BufFile_init( \
    Ring_init(LINED(_ringDat), ringCap + 1), \
    Buf_ntLit(STR));

DECLARE_METHOD(void      , BufFile,drop, Arena a);
DECLARE_METHOD(Sll*      , BufFile,resourceLL);
DECLARE_METHOD(void      , BufFile,seek, ISlot offset, U1 whence);
DECLARE_METHOD(void      , BufFile,read);
DECLARE_METHOD(BaseFile* , BufFile,asBase);
DECLARE_METHOD(void      , BufFile,write);
File BufFile_asFile(BufFile* d);

#endif // __CIV_H
