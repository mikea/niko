# Implementation Documentation: Dictionary Structure

This document details the internal structure and implementation of Niko's dictionary system, which manages word definitions, variables, constants, and runtime lookups.

## Overview

Niko uses a vector-based dictionary that stores all defined words, variables, constants, and built-in functions. The dictionary supports lexical scoping through insertion order and provides efficient lookup for word resolution during execution.

## Core Data Structures

### `dict_entry` Structure

Every word in Niko is represented by a `dict_entry` structure:

```cpp
struct dict_entry {
  string  k;         // Word name (key)
  array_p v;         // Word value/definition
  bool    sys : 1  = false;  // System/built-in word
  bool    cons : 1 = false;  // Constant (immutable)
  bool    var : 1  = false;  // Variable (mutable)
  bool    imm : 1  = false;  // Immediate (executes during compilation)
};
```

### Dictionary Storage

```cpp
using dict_t = vector<dict_entry>;

struct inter_t {
  dict_t dict;  // Per-interpreter dictionary
  // ...
};

dict_t global_dict;  // Global dictionary for built-ins
```

## Dictionary Entry Types

### 1. System Words (`sys = true`)

Built-in functions implemented in C++. These are registered at startup and cannot be redefined.

**Value Types:**
- **FFI Functions**: `v` contains `array<ffi_t>` with function pointer(s)
- **Polymorphic Functions**: `v` contains `array<ffi_t>` with multiple overloads for different type combinations

**Examples:**
```cpp
// Simple built-in word
global_dict_add_new({.k = "+", .v = array::atom<ffi_t>(w_plus::call), .sys = true});

// Polymorphic word with type dispatch
global_dict_add_new({.k = "neg", .v = array::create<ffi_t>(ffi_table.size(), ffi_table.begin()), .sys = true});
```

### 2. User-Defined Words (`: word ... ;`)

Compiled sequences of tokens stored as arrays.

**Properties:**
- `sys = false`, `cons = false`, `var = false`
- `v` contains `array<arr_t>` where each element is a token/word reference

**Creation Process:**
1. `:` switches to COMPILE mode, stores word name
2. Tokens are collected in `comp_stack`
3. `;` creates array from collected tokens and stores in dictionary

**Example:**
```niko
: double dup + ;
```
Results in: `dict_entry{.k = "double", .v = [dup_ref, plus_ref]}`

### 3. Constants (`const word`)

Immutable values that push their content when referenced.

**Properties:**
- `cons = true`, `sys = false`, `var = false`
- `v` contains the constant value directly
- Cannot be redefined or modified

**Example:**
```niko
42 const answer
```
Results in: `dict_entry{.k = "answer", .v = 42, .cons = true}`

### 4. Variables (`var word`)

Mutable storage locations that push their current value when referenced.

**Properties:**
- `var = true`, `sys = false`, `cons = false`
- `v` contains the current value
- Can be modified with `!` (store)

**Example:**
```niko
0 var counter
```
Results in: `dict_entry{.k = "counter", .v = 0, .var = true}`

### 5. Immediate Words (`imm = true`)

Words that execute during compilation rather than being compiled into definitions.

**Properties:**
- `imm = true`
- Execute immediately even in COMPILE mode
- Used for control structures and compile-time operations

**Built-in Immediate Words:**
- `:` - Start compilation
- `;` - End compilation  
- `const` - Define constant
- `var` - Define variable
- `literal` - Compile literal value

## Dictionary Operations

### Word Lookup

Dictionary lookup follows **last-defined-wins** semantics:

```cpp
t_dict_entry inter_t::find_entry_idx(const str n) {
  DO(i, dict.size()) {
    size_t j = dict.size() - i - 1;  // Search backwards
    if (n == dict[j].k) return j;
  }
  return dict.size();  // Not found
}
```

This enables:
- **Shadowing**: Later definitions hide earlier ones
- **REPL redefinition**: Words can be redefined interactively
- **Lexical scoping**: Inner scopes can override outer definitions

### Word Execution

The execution path depends on the entry type:

```cpp
void inter_t::entry(dict_entry* e) {
  if (e->cons || e->var) {
    PUSH(e->v);  // Constants and variables push their value
    return;
  }
  
  array* a = e->v;
  switch (a->t) {
    case T_FFI: {
      // Execute built-in function
      ffi f = *a->data<ffi_t>();
      f(*this, stack);
      break;
    }
    case T_ARR: {
      // Execute compiled word
      DO_ARRAY(a, arr_t, i, e) {
        if (e->t == T_DICT_ENTRY) {
          if (e->q) PUSH(e);      // Quoted word
          else entry(e);          // Execute word
        }
      }
      break;
    }
  }
}
```

## Type System Integration

### Dictionary Entry References

Words can be referenced as first-class values using the quote operator (`'`):

```niko
dup'  ( pushes reference to dup word )
```

This creates an array of type `T_DICT_ENTRY` containing the dictionary index:

```cpp
typedef u64 t_dict_entry;
struct dict_entry_t {
  using t = u64;                           // Dictionary index
  static constexpr type_t e = T_DICT_ENTRY;
};
```

### Higher-Order Word Integration

Dictionary entries are passed to higher-order words like `,fold`, `,apply`:

```cpp
// In higher-order word implementation
DEF_WORD(",apply", apply) {
  POP_DICT_ENTRY(y);  // Extract dictionary index
  POP(x);
  DO(i, x->n) {
    PUSH(x->atom_i(i));
    inter.entry(y);    // Execute word by index
  };
  PUSH(cat(stack, x->n));
}
```

## Memory Management

### Reference Counting

Dictionary entries use reference-counted arrays (`array_p`):

```cpp
struct dict_entry {
  array_p v;  // Automatically managed
};
```

### Dictionary Lifecycle

1. **Global Dictionary**: Created at startup, contains built-ins
2. **Interpreter Dictionary**: Copy of global + user definitions
3. **Entry Values**: Reference-counted arrays with automatic cleanup

### Word Redefinition

```cpp
dict_entry* prev = inter.find_entry(inter.comp);
if (prev) {
  prev->v = a;  // Replace value (old array released automatically)
  prev->cons = prev->var = false;
} else {
  inter.dict.push_back(dict_entry(inter.comp, a));  // New entry
}
```

## Compilation Process

### Mode Switching

The interpreter has two modes:

```cpp
enum { INTERPRET, COMPILE } mode = INTERPRET;
```

### Compilation Flow

1. **Start** (`:` word):
   ```cpp
   inter.mode = inter_t::COMPILE;
   inter.comp = word_name;
   ```

2. **Token Collection**:
   - Normal words → compiled as dictionary references
   - Immediate words → executed immediately
   - Literals → stored directly

3. **End** (`;` word):
   ```cpp
   array_p a = array::create<arr_t>(inter.comp_stack.len(), inter.comp_stack.begin());
   inter.dict.push_back({inter.comp, a});
   inter.mode = inter_t::INTERPRET;
   ```

### Quote Handling

During compilation, quoted words are stored as dictionary references:

```cpp
case TOK_WORD: {
  size_t e = find_entry_idx(t.text);
  dict_entry* en = lookup_entry(e);
  if (mode == INTERPRET || en->imm) 
    entry(en);
  else 
    PUSH(array::atom<dict_entry_t>(e));  // Compile reference
}
```

## Error Handling

### Redefinition Protection

```cpp
if (inter.find_entry_idx(next) < inter.dict.size()) 
  panicf("`{}` can't be redefined", next);
```

Applied to:
- Constants (`const`)
- Variables (`var`) 
- System words

### Execution Validation

```cpp
CHECK(e < dict.size(), "bad dict entry");
CHECK(x->t == T_DICT_ENTRY, "dict entry expected");
```

## Performance Characteristics

### Lookup Complexity
- **Time**: O(n) linear search from end
- **Space**: O(1) per lookup
- **Cache**: No caching (simple implementation)

### Memory Usage
- **Dictionary**: ~32 bytes per entry (string + metadata)
- **Values**: Reference-counted, shared when possible
- **Compilation**: Temporary stack during definition

### Optimization Opportunities

1. **Hash Table**: Replace linear search with hash lookup
2. **String Interning**: Reduce memory for repeated names  
3. **Bytecode**: Compile to indexed operations vs. dictionary lookups
4. **Local Variables**: Stack-based locals for better cache behavior

## Debug and Introspection

### Development Tools

- `\\s` - Print stack contents
- `\\i` - Print interpreter status
- `\\mem` - Memory usage statistics

### Dictionary Inspection

Currently no built-in words for dictionary introspection, but could be added:

```niko
words     ( -- ) print all defined words
see word  ( -- ) show definition of word  
forget word ( -- ) remove word from dictionary
```

## Word Fusing Optimization

### Overview

Word fusing is a runtime optimization that allows specialized implementations for combinations of quoted words and adverbs (higher-order words starting with comma).

### Implementation

#### Fusion Detection

The optimization is implemented in the `entry` function for dictionary entries:

```cpp
void inter_t::entry(dict_entry* e) {
  // Word fusing optimization for adverbs (words starting with comma)
  if (e->k.size() > 0 && e->k[0] == ',' && !stack.empty()) {
    auto& top = stack.peek(0);
    if (top.t == T_DICT_ENTRY) {
      // Get the quoted word name
      t_dict_entry quoted_idx = *top.data<dict_entry_t>();
      dict_entry* quoted_word = lookup_entry(quoted_idx);
      
      // Try to find fused word: "quoted_word,adverb"
      dict_entry* fused = try_fused_word(quoted_word->k, e->k);
      if (fused) {
        // Found fused word - remove quoted word from stack and execute fused version
        stack.drop();
        return entry(fused);
      }
    }
  }
  // ... normal execution continues
}
```

#### Fusion Lookup

```cpp
dict_entry* inter_t::try_fused_word(const str& first_word, const str& second_word) {
  // Construct fused name: "first,second"
  string fused_name = string(first_word) + string(second_word);
  return find_entry(fused_name);
}
```

### Execution Flow

1. **Adverb Detection**: Check if current word starts with `,`
2. **Stack Inspection**: Verify top of stack contains quoted word (`T_DICT_ENTRY`)
3. **Name Construction**: Concatenate `word_name + adverb_name`
4. **Dictionary Lookup**: Search for fused word in dictionary
5. **Execution Branch**:
   - If found: Remove quoted word, execute fused implementation
   - If not found: Continue with normal adverb execution

### Performance Characteristics

#### Overhead
- **When fused word exists**: Single dictionary lookup, stack drop
- **When fused word doesn't exist**: Single dictionary lookup (miss)
- **When not applicable**: Single character check (`e->k[0] == ','`)

#### Memory Usage
- **Temporary string**: Allocated during fusion lookup
- **No persistent overhead**: No additional storage required

### Use Cases

#### Mathematical Optimizations
```niko
: dup,fold dup * sum ;        ( x² sum via SIMD )
: sqrt,apply sqrt' ,apply ;   ( vectorized sqrt )
```

#### Algorithmic Shortcuts  
```niko
: +,fold sum ;               ( use built-in sum )
: reverse,apply reverse' ,apply ; ( no-op optimization )
```

#### Domain-Specific Operations
```niko
: normalize,apply dup max /' ,apply ; ( normalize to [0,1] )
: gradient,pairwise -' ,pairwise ;    ( compute differences )
```

### Design Considerations

#### Naming Convention
- **Predictable**: Simple concatenation makes fused names obvious
- **Conflict-free**: Comma separators avoid naming conflicts
- **Readable**: `word,adverb` clearly indicates the fusion relationship

#### Semantic Preservation
- **Transparent**: Fused words should produce equivalent results
- **Optional**: Removing fused words shouldn't break programs
- **Composable**: Fused words can themselves use higher-order words

#### Error Handling
- **Graceful degradation**: Missing fused words fall back automatically
- **No validation**: Fused words can have completely different implementations
- **User responsibility**: Correctness of fused implementations

### Future Enhancements

#### Multi-Level Fusing
```niko
word1' word2' ,adverb  → word1,word2,adverb
```

#### Compile-Time Detection
- Static analysis to identify fusion opportunities
- Automatic generation of optimized fused words

#### Performance Profiling
- Runtime statistics for fusion hit/miss rates
- Identification of optimization opportunities

This completes the comprehensive documentation of Niko's dictionary structure and word fusing optimization.