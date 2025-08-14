# IFF/RIFF Reader – Implementation Design & Pseudocode

**Author:** (you)

**Status:** Draft (implementation-ready skeleton)

---

## 0. Goals & Non‑Goals

**Goals**

* Provide a stable, streaming‑first C++17 core to parse IFF‑85 and RIFF (RIFF/RIFX/RF64) container formats.
* Expose an idiomatic, minimal API with **lambda/functor handlers** (no `void*`).
* Support **form‑scoped dispatch** (same FourCC handled differently under different FORM/RIFF types).
* Offer two walkers:

    1. **Raw walker** (format‑agnostic containers).
    2. **Structured IFF‑85 walker** with PROP defaults and LIST/CAT validation (+ optional default injection).
* Provide optional utilities: a tiny DOM builder and RF64 size override handling.

**Non‑Goals (v1)**

* Writing/authoring files (builder), though design leaves room for it.
* Full AVI/WAVE/AIFF parsers (these live outside the core).
* Memory mapping backend (can be added later via `reader`).

---

## 1. Terminology & Semantics Recap

* **Chunk**: `{ id: FourCC, size: u32 (or logical u64 via RF64), payload, [pad] }`.
* **Container chunks**: IFF‑85: `FORM`, `LIST`, `CAT `, `PROP`. RIFF: `RIFF`, `RIFX`, `RF64`, `LIST`.

    * First 4 bytes of payload are the **type tag** (e.g., `AIFF`, `WAVE`, `ILBM`, `INFO`).
    * Children begin **after** the 4‑byte type tag; the declared size **includes** the type tag.
* **Alignment**: every chunk is padded to **even** total size. `aligned_size = 8 + size + (size & 1)` where `8` is header.
* **Endianness**: IFF‑85 is **big**. RIFF is **little** (`RIFF`, `RF64`). `RIFX` is big.
* **RF64**: 32‑bit size fields may be `0xFFFFFFFF`; `ds64` chunk supplies 64‑bit overrides (file size and selected chunk sizes).

---

## 2. Public API (high‑level)

```text
namespace iff {
  enum class byte_order { little, big };

  struct options {
    bool strict = true;
    uint64_t max_chunk_size = 1<<32;  // guardrail for payload reads
    bool allow_rf64 = true;
    int  max_depth = 64;
    function<void(uint64_t, const char*, string)> on_warning; // optional
  };

  struct parse_error : runtime_error { uint64_t offset; };

  struct fourcc { char[4]; /* ==, hash, to_string */ };

  enum class chunk_source { explicit_data, prop_default };

  struct chunk_header {
    fourcc id;
    uint64_t size;          // payload bytes (excl pad)
    uint64_t file_offset;   // start of header
    bool is_container;
    optional<fourcc> type_tag;  // for containers
    chunk_source source = chunk_source::explicit_data; // default
  };

  struct reader { /* read/seek/tell/size */ };

  class chunk_reader { /* bounded view over payload */ };

  // Handlers: idiomatic C++ (lambdas, functors)
  using handler = function<void(const chunk_header&, chunk_reader&, byte_order)>;

  // Three‑level registry: form → container → id
  class registry {
    void on_in_form(fourcc form_type, fourcc id, handler h);
    void on_in_container(fourcc container_type, fourcc id, handler h);
    void on_id(fourcc id, handler h);
    const handler* find(optional<fourcc> current_form,
                        optional<fourcc> current_container,
                        fourcc id) const; // precedence: form > container > id
  };

  // Planner for selective descent/dispatch
  struct plan {
    vector<pair<optional<fourcc>, fourcc>> want; // (scope?, id)
    int max_depth = 32;
    bool wants_container(fourcc type) const;
    bool wants_chunk(optional<fourcc> scope, fourcc id) const;
  };

  // Raw walk (no IFF‑85 PROP semantics)
  void walk(reader&, const options&, const registry&, const plan* = nullptr);

  // Structured IFF‑85 additions
  struct property_view { optional<chunk_reader> open(fourcc id) const; };

  struct form_context {
    fourcc form_type;
    uint64_t payload_begin;
    uint64_t payload_size;
    size_t   index_in_list;      // position within enclosing LIST<T>
    property_view defaults;      // merged PROP defaults in effect
  };

  using form_cb = function<void(const form_context&)>;
  struct structured_hooks { form_cb on_form_begin; form_cb on_form_end; };

  struct structured_options : options {
    bool inject_prop_defaults = true;  // dispatch default property chunks
    bool validate_list_types  = true;  // LIST/CAT conformance checks
  };

  void walk_iff85(reader&, const structured_options&, const registry&,
                  const plan* = nullptr, const structured_hooks* = nullptr);
}
```

---

## 3. Core Building Blocks – Pseudocode

### 3.1 `detect_root` (RIFF/RIFX/RF64 and IFF‑85)

```pseudocode
detect_root(r: reader, opt: options) -> (group_id: fourcc, form_type: optional<fourcc>, bo: byte_order,
                                         payload_begin: u64, payload_size: u64)
  require r.seek(0)
  buf = r.read_exact(12)  // [id(4), size(4), type(4)]
  id  = fourcc(buf[0..3])
  bo  = (id in {RIFF, RF64}) ? little : big  // RIFX, FORM/LIST/CAT/PROP are big
  sz32 = read_u32(buf[4..7], bo)
  type = fourcc(buf[8..11])

  // containers include type in size; children start after it
  payload_begin = 12
  payload_size  = max(0, sz32 - 4)

  // RF64: optionally pre‑scan top level for ds64 to populate overrides
  if id == RF64 and opt.allow_rf64:
    rf64_state = scan_top_level_for_ds64(r, bo, payload_begin, payload_size, opt)
  else:
    rf64_state = none

  return (id, type, bo, payload_begin, payload_size, rf64_state)
```

**RF64 pre‑scan (top‑level only)**

```pseudocode
scan_top_level_for_ds64(r, bo, begin, size, opt) -> rf64_state
  end = begin + size
  cur = begin
  state = { riff_size_u64: none, overrides: map<offset_or_tag, u64> }
  while cur + 8 <= end:
    id  = read_fourcc_at(r, cur)
    sz  = read_u32_at(r, cur+4, bo)
    pay = cur + 8
    if id == fourcc("ds64") and sz >= 28:
      // spec fields: riff_size, data_size, sample_count, + table of (chunk_id, chunk_size, chunk_pos)
      ds = read_bytes(r, pay, sz)
      state.riff_size_u64 = read_u64(ds[0..7], little)  // ds64 is little under RIFF
      table_count = read_u32(ds[24..27], little)
      off = 28
      for i in 0..table_count-1:
        cid  = fourcc(ds[off..off+3]); off += 4
        csz  = read_u64(ds[off..off+7], little); off += 8
        cpos = read_u64(ds[off..off+7], little); off += 8
        state.overrides[(cid, cpos)] = csz
    cur += 8 + sz + (sz & 1)
  return state
```

**Header size override (used during iteration)**

```pseudocode
maybe_override_size(hdr_offset, id, sz32, bo, rf64_state) -> u64
  if sz32 != 0xFFFFFFFF: return sz32
  if rf64_state is none: return sz32  // leave as 0xFFFFFFFF; walker will clamp
  // prefer exact match by (id, absolute payload offset)
  key = (id, hdr_offset + 8)
  if key in rf64_state.overrides: return rf64_state.overrides[key]
  // fallback: some files only list data chunk; if only riff_size is set, cap by file size
  if rf64_state.riff_size_u64: return rf64_state.riff_size_u64 - 4  // minus type tag, at root
  return sz32
```

### 3.2 `chunk_cursor::children`

```pseudocode
children(r, bo, begin, size, opt, rf64_state) -> iterator
  end = begin + size
  iterator.state = { r, bo, cur = begin, end, opt, rf64_state }

iterator.next() -> item | end
  if state.cur >= state.end: return end
  // read header
  id   = read_fourcc_at(r, state.cur)
  sz32 = read_u32_at(r, state.cur + 4, state.bo)
  sz   = maybe_override_size(state.cur, id, sz32, state.bo, state.rf64_state)
  payload_begin = state.cur + 8
  is_container  = (id in {RIFF,RIFX,RF64,LIST,FORM,CAT ,PROP})

  if is_container:
    type = read_fourcc_at(r, payload_begin)
    payload_begin += 4
    payload_size  = max(0, sz - 4)
  else:
    type = none
    payload_size = sz

  // bounds / guardrails
  total = 8 + sz + (sz & 1)
  if state.cur + total > state.end:
    if opt.strict: throw parse_error(state.cur, "chunk exceeds parent bounds")
    payload_size = clamp(payload_size, 0, state.end - payload_begin)

  if payload_size > opt.max_chunk_size:
    if opt.strict: throw parse_error(state.cur, "chunk too large")
    payload_size = opt.max_chunk_size

  item.hdr  = { id, payload_size, state.cur, is_container, type, source=explicit_data }
  item.payload = chunk_reader(r, payload_begin, payload_size)

  // advance cursor now (consumers may ignore payload)
  state.cur += total
  return item
```

---

## 4. Context Management & Dispatch

### 4.1 Context frame

```pseudocode
struct walk_ctx_frame {
  optional<fourcc> form_type;       // nearest FORM/RIFF type (domain)
  optional<fourcc> container_type;  // immediate container's type tag (LIST/PROP/CAT or FORM)
};

struct walk_ctx_stack { vector<walk_ctx_frame> frames; }

current_form_type(stack)      -> optional<fourcc> = last(frames).form_type
current_container_type(stack) -> optional<fourcc> = last(frames).container_type
```

**Rules when descending**

* Entering `FORM <T>` or `RIFF/RIFX/RF64 <T>`: `form_type := T`, `container_type := T` (children see container as the form’s type).
* Entering `LIST/PROP/CAT <X>`: `form_type` **unchanged**, `container_type := X`.
* Leaving any container: pop frame.

### 4.2 Registry precedence

```pseudocode
dispatch(reg, hdr, payload, bo, stack):
  f = current_form_type(stack)       // nearest domain
  c = current_container_type(stack)  // immediate container
  cb = reg.find(f, c, hdr.id)        // precedence: form → container → id
  if cb: cb(hdr, payload, bo)
```

### 4.3 Planner checks

```pseudocode
should_descend(pl, container_type) -> bool
  return (pl == null) or pl.wants_container(container_type)

should_dispatch(pl, scope, id) -> bool
  return (pl == null) or pl.wants_chunk(scope, id)
```

---

## 5. Walkers

### 5.1 Raw walker (`walk`)

```pseudocode
walk(r, opt, reg, pl=null):
  root = detect_root(r, opt)
  stack.push({ form_type = root.type, container_type = root.type })
  recurse(begin = root.payload_begin, size = root.payload_size)
  stack.pop()

recurse(begin, size):
  for item in children(r, root.bo, begin, size, opt, root.rf64_state):
    hdr, pr = item.hdr, item.payload
    if hdr.is_container:
      stack.push(next_frame_for(hdr))
      if should_descend(pl, *hdr.type_tag):
        recurse(pr.abs_data_offset(), hdr.size)
      stack.pop()
    else:
      if should_dispatch(pl, current_container_type(stack), hdr.id):
        dispatch(reg, hdr, pr, root.bo, stack)
      // optional warning if pr.remaining() > 0 (handler didn't consume)
```

**`next_frame_for(hdr)`**

```pseudocode
if hdr.id in {FORM, RIFF, RIFX, RF64}:
  return { form_type = hdr.type_tag, container_type = hdr.type_tag }
else: // LIST, PROP, CAT, etc.
  return { form_type = current_form_type(stack), container_type = hdr.type_tag }
```

### 5.2 Structured IFF‑85 walker (`walk_iff85`)

Adds PROP collection, LIST/CAT validation, and optional default injection.

#### Data structures

```pseudocode
struct span { uint64_t off; uint64_t len; }  // absolute payload span
struct prop_bank { unordered_map<fourcc, span, fourcc_hash> by_id; }

// For the duration of a LIST<T>, we maintain one prop_bank which accumulates
// defaults from preceding PROP<T> blocks (last‑write‑wins per id).
```

#### Algorithm

```pseudocode
walk_iff85(r, sopt, reg, pl=null, hooks=null):
  root = detect_root(r, sopt)
  stack.push({ form_type = root.type, container_type = root.type })
  recurse_iff85(begin = root.payload_begin, size = root.payload_size,
                enclosing_list_props = null, form_index = 0)
  stack.pop()

recurse_iff85(begin, size, enclosing_list_props, form_index):
  for item in children(r, root.bo, begin, size, sopt, root.rf64_state):
    hdr, pr = item.hdr, item.payload

    if hdr.is_container and hdr.id == LIST:
      ensure_types_valid_if_needed(list_type = *hdr.type_tag)
      // LIST<T>: collect PROP<T> (before any FORM<T>), then walk FORMs with defaults
      bank = prop_bank{}
      // first pass over LIST's children without recursion: index and collect
      index = index_children(r, root.bo, pr.abs_data_offset(), hdr.size, sopt)

      seen_form = false
      for ch in index:
        if ch.id == PROP:
          if sopt.validate_list_types and ch.type_tag != hdr.type_tag:
            handle_error_or_warn(ch, "PROP type mismatch")
          if seen_form:
            handle_error_or_warn(ch, "PROP after FORM in LIST")
          // collect defaults from this PROP<T>
          for leaf in children(r, root.bo, ch.payload_begin, ch.payload_size, sopt, root.rf64_state):
            bank.by_id[leaf.hdr.id] = span{ leaf.payload.abs_data_offset(), leaf.hdr.size }
        else if ch.id == FORM:
          if sopt.validate_list_types and ch.type_tag != hdr.type_tag:
            handle_error_or_warn(ch, "FORM type mismatch in LIST")
          seen_form = true
          // FORM<T>: walk with defaults
          form_ctx = form_context{ form_type = *ch.type_tag,
                                   payload_begin = ch.payload_begin,
                                   payload_size = ch.payload_size,
                                   index_in_list = form_index,
                                   defaults = make_property_view(r, bank) }
          if hooks && hooks.on_form_begin: hooks.on_form_begin(form_ctx)

          // optional injection: synthesize default chunks BEFORE actual ones
          if sopt.inject_prop_defaults:
            for (id, sp) in bank.by_id (stable order: e.g., sort by id bytes):
              fake_hdr = { id=id, size=sp.len, file_offset=sp.off-8, is_container=false,
                           type_tag=none, source=prop_default }
              cr = chunk_reader(&r, sp.off, sp.len)
              if should_dispatch(pl, current_container_type(stack), id):
                dispatch(reg, fake_hdr, cr, root.bo, stack)

          // now walk actual FORM children
          stack.push({ form_type = ch.type_tag, container_type = ch.type_tag })
          recurse_iff85(ch.payload_begin, ch.payload_size, /*enclosing_list_props*/ &bank, /*form_index*/ form_index)
          stack.pop()

          if hooks && hooks.on_form_end: hooks.on_form_end(form_ctx)
          form_index += 1
        else:
          // Other containers inside LIST<T> (rare) – recurse raw
          stack.push(frame_for_container(hdr))
          recurse_iff85(ch.payload_begin, ch.payload_size, enclosing_list_props, form_index)
          stack.pop()

    else if hdr.is_container and hdr.id == CAT_:
      // CAT<T>: FORMs only (no PROP semantics); optionally validate
      for sub in children(...):
        if sopt.validate_list_types and (sub.hdr.id != FORM or sub.hdr.type_tag != hdr.type_tag):
          handle_error_or_warn(sub.hdr, "CAT must contain FORM<T>")
        stack.push({ form_type = sub.hdr.type_tag, container_type = sub.hdr.type_tag })
        recurse_iff85(sub.payload.abs_data_offset(), sub.hdr.size, /*enclosing_list_props*/ null, /*form_index*/ 0)
        stack.pop()

    else if hdr.is_container and (hdr.id == FORM or hdr.id in {RIFF,RIFX,RF64}):
      // Standalone FORM/RIFF domain (no PROP defaults)
      if hooks && hooks.on_form_begin:
        hooks.on_form_begin({ form_type=*hdr.type_tag, payload_begin=pr.abs_data_offset(), payload_size=hdr.size,
                              index_in_list=0, defaults=empty_property_view })
      stack.push({ form_type = hdr.type_tag, container_type = hdr.type_tag })
      recurse_iff85(pr.abs_data_offset(), hdr.size, /*props*/ null, /*idx*/ 0)
      stack.pop()
      if hooks && hooks.on_form_end: hooks.on_form_end(...)

    else if hdr.is_container:
      // Other containers (LIST without type? nested LIST INFO in RIFF, etc.)
      stack.push(frame_for_container(hdr))
      if should_descend(pl, *hdr.type_tag):
        recurse_iff85(pr.abs_data_offset(), hdr.size, enclosing_list_props, form_index)
      stack.pop()

    else:
      // leaf chunk
      scope = current_container_type(stack)   // immediate container
      if should_dispatch(pl, scope, hdr.id):
        dispatch(reg, hdr, pr, root.bo, stack)
```

**Helper: `index_children`** (single pass without recursion)

```pseudocode
index_children(r, bo, begin, size, opt) -> vector<child_info>
  vec = []
  for item in children(r, bo, begin, size, opt, root.rf64_state):
    vec.push({ id=item.hdr.id, type_tag=item.hdr.type_tag,
               payload_begin=item.payload.abs_data_offset(),
               payload_size=item.hdr.size })
  return vec
```

**Helper: `make_property_view`**

```pseudocode
make_property_view(r, bank: prop_bank) -> property_view
  pv = property_view{}
  pv.impl = { r_ptr=&r, map=bank.by_id }
  pv.open(id):
    if id not in map: return none
    sp = map[id]
    return chunk_reader(r_ptr, sp.off, sp.len)
  return pv
```

**Helper: `handle_error_or_warn`**

```pseudocode
handle_error_or_warn(hdr_or_offset, message):
  if options.strict: throw parse_error(offset_of(hdr_or_offset), message)
  else if options.on_warning: options.on_warning(offset_of(hdr_or_offset), "STRUCT", message)
```

---

## 6. Safety & Correctness Rules

1. **Even‑byte alignment**: cursor always advances by `8 + size + (size & 1)`.
2. **Bounded reads**: `chunk_reader` clamps to `hdr.size`; handlers cannot over‑read.
3. **Overflow guards**: detect `cur + total > end` and clamp/throw.
4. **Depth guard**: abort/throw when exceeding `options.max_depth`.
5. **RF64 size overrides**: apply at header parse time; when not present, treat `0xFFFFFFFF` as unknown and clamp to parent bounds (or error in strict mode).
6. **Handler discipline**: if `chunk_reader.remaining() > 0` after handler returns, optionally warn and skip remainder.

---

## 7. Performance Notes

* **Streaming‑first**: no payload copies; PROP defaults stored as spans (offset+len). Optional injection reuses readers.
* **`std::function`**: capture small state by reference to benefit from SBO; avoid large captures in hot paths.
* **I/O**: prefer buffered `istream` or a custom `reader` on top of `pread`/mmap later.

---

## 8. Optional DOM Builder (for tools/tests)

```pseudocode
struct node {
  chunk_header hdr;
  vector<node> children;
  enum storage { none, offset_only, inline_small } store = offset_only;
  uint64_t data_off = 0; vector<byte> small;
}

build_tree(r, opt, limits) -> node
  // limits.inline_threshold, limits.max_total_inline, limits.max_depth, limits.only_ids
  // Walk using raw walk; for each chunk:
  if hdr.is_container:
     push node; recurse; pop
  else:
     if hdr.size <= inline_threshold and total_inline + hdr.size <= max_total_inline:
        read bytes into node.small; store=inline_small
     else:
        node.data_off = pr.abs_data_offset(); store=offset_only
```

---

## 9. Examples of Registry Usage

**AIFF (form‑scoped)**

```pseudocode
reg.on_in_form(AIFF, COMM, [](hdr, r, bo){ parse_aiff_comm(hdr, r, bo); });
reg.on_in_form(AIFF, SSND, [](hdr, r, bo){ parse_aiff_ssnd(hdr, r, bo); });
```

**RIFF INFO (container‑scoped)**

```pseudocode
reg.on_in_container(INFO, INAM, [](hdr, r, bo){ parse_info_string("name", r); });
reg.on_in_container(INFO, IART, [](hdr, r, bo){ parse_info_string("artist", r); });
```

**Global fallback**

```pseudocode
reg.on_id(JUNK, [](hdr, r, bo){ r.skip(r.remaining()); });
```

---

## 10. Unit Test Plan (Doctest)

* **Happy paths**

    * Minimal `RIFF WAVE` (`fmt ` + `data`).
    * `RIFX` variant.
    * `RF64` with `ds64` (root and `data` sizes overridden).
    * `FORM AIFF` (`COMM` + `SSND`).
    * `FORM ILBM` (`BMHD` + `CMAP` + `BODY`).
    * `LIST INFO` under WAVE/AVI.
    * `LIST ILBM` with `PROP ILBM` defaults + multiple `FORM ILBM`.
    * `CAT AIFF` with multiple `FORM AIFF`.
* **Edge cases**

    * Odd payload sizes (pad byte handling).
    * Truncated file (in header or payload).
    * Oversized chunk (beyond parent bounds).
    * Deep nesting (depth cap).
    * Handlers leaving unread bytes (warning path).
* **Selective plan**

    * Verify `plan.want` filters descent and dispatch.
* **Precedence**

    * Same FourCC with different form handlers (AIFF vs ILBM) – ensure form handler wins over container/global.

---

## 11. Extensibility & Versioning

* Keep public API within `namespace iff::v1` (macro `IFF_READER_API_VERSION`).
* New features (e.g., `on_container_begin/end`, additional walkers) can be added without breaking handler signatures.
* RF64 table expansion: can later add more precise override keys without ABI break.

---

## 12. Implementation Checklist

* [ ] Implement `fourcc`, hashing, and helpers (`rd_u16/u32/u64`).
* [ ] Implement `reader` adapters (`istream_reader`, `span_reader`).
* [ ] Implement `chunk_reader` (bounded, with `read_pod`, `read_bytes`).
* [ ] Implement RF64 pre‑scan and `maybe_override_size`.
* [ ] Implement `chunk_cursor::children` iterator.
* [ ] Implement `registry` with `on_in_form/on_in_container/on_id` and precedence lookup.
* [ ] Implement `plan` with `wants_container/wants_chunk`.
* [ ] Implement **raw `walk`** with context stack.
* [ ] Implement **structured `walk_iff85`**: LIST/CAT validation, PROP collect, optional injection.
* [ ] Add warnings (`on_warning`) where handlers leave unread bytes.
* [ ] (Optional) Implement DOM builder.
* [ ] Doctest coverage as per test plan.

---

## 13. Pseudocode Fragments – Frequently Used Parsers

**AIFF `COMM`**

```pseudocode
parse_aiff_comm(h, r, bo):
  channels = r.read_u16(bo)
  frames   = r.read_u32(bo)
  bits     = r.read_u16(bo)
  rate80   = r.read_80bit_extended()
  // ... convert 80‑bit to double, store in state
```

**WAVE `fmt `**

```pseudocode
parse_wave_fmt(h, r, little):
  tag      = r.read_u16(little)
  ch       = r.read_u16(little)
  rate     = r.read_u32(little)
  byterate = r.read_u32(little)
  align    = r.read_u16(little)
  bps      = r.read_u16(little)
  r.skip(r.remaining())  // extensions possible
```

**ILBM `CMAP`**

```pseudocode
parse_ilbm_cmap(h, r, bo):
  palette = []
  while r.remaining() >= 3:
    R = r.read_u8(); G = r.read_u8(); B = r.read_u8()
    palette.push((R<<16) | (G<<8) | B)
```

---

## 14. Open Questions / Decisions

* **RF64 overrides**: do we also try a fallback key by `(id, ordinal index within parent)` for some encoders that omit positions? (Can be added later.)
* **80‑bit float**: include a helper to parse AIFF extended float now or later?
* **DOM**: include in v1 or keep as separate header?

---

## 15. Quick Start (consumer)

```pseudocode
rd  = istream_reader(file)
reg = registry{}
reg.on_in_form(AIFF, COMM, [](h,r,bo){ parse_aiff_comm(h,r,bo) })
reg.on_in_form(AIFF, SSND, [](h,r,bo){ parse_aiff_ssnd(h,r,bo) })

opt = structured_options{}
opt.strict = true
opt.inject_prop_defaults = true

pl = plan{}
pl.want = [ (AIFF, COMM), (AIFF, SSND) ]

walk_iff85(rd, opt, reg, &pl)
```

---

**End of document.**

---

## 16. Reference Implementation (C++17, header-only)

> Single-file header you can drop in as `iff_reader.hpp`. Snake\_case; Option A handlers; form/container/id precedence; raw and structured walkers with PROP defaults. RF64 `ds64` top-level scan included (basic overrides by `(id, payload_offset)`).

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iosfwd>
#include <istream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace iff {

// ===================== basics =====================

enum class byte_order { little, big };

struct options {
  bool strict = true;
  std::uint64_t max_chunk_size = (1ull << 32);
  bool allow_rf64 = true;
  int  max_depth = 64;
  std::function<void(std::uint64_t, const char*, const std::string&)> on_warning{};
};

struct parse_error : std::runtime_error {
  std::uint64_t offset{};
  explicit parse_error(std::uint64_t off, const std::string& msg)
    : std::runtime_error(msg), offset(off) {}
};

struct fourcc {
  std::array<char,4> b{};
  constexpr fourcc() = default;
  constexpr explicit fourcc(const char(&s)[5]) : b{ s[0], s[1], s[2], s[3] } {}
  std::string to_string() const { return std::string(b.data(), 4); }
  bool operator==(const fourcc& o) const { return b == o.b; }
  bool operator!=(const fourcc& o) const { return !(*this == o); }
};

struct fourcc_hash {
  std::size_t operator()(const fourcc& f) const noexcept {
    std::uint32_t v; std::memcpy(&v, f.b.data(), 4);
    return (static_cast<std::size_t>(v) * 0x9E3779B1u) ^ 0x85EBCA6Bu;
  }
};

namespace fcc {
  static constexpr fourcc RIFF{"RIFF"};
  static constexpr fourcc RIFX{"RIFX"};
  static constexpr fourcc RF64{"RF64"};
  static constexpr fourcc LIST{"LIST"};
  static constexpr fourcc FORM{"FORM"};
  static constexpr fourcc CAT_{"CAT "};
  static constexpr fourcc PROP{"PROP"};
}

inline std::uint16_t rd_u16(const unsigned char* p, byte_order bo) {
  return (bo==byte_order::little)
    ? (std::uint16_t(p[0]) | (std::uint16_t(p[1])<<8))
    : (std::uint16_t(p[1]) | (std::uint16_t(p[0])<<8));
}
inline std::uint32_t rd_u32(const unsigned char* p, byte_order bo) {
  if (bo==byte_order::little) return
    (std::uint32_t(p[0])      ) | (std::uint32_t(p[1])<<8 ) |
    (std::uint32_t(p[2])<<16 ) | (std::uint32_t(p[3])<<24 );
  return
    (std::uint32_t(p[3])      ) | (std::uint32_t(p[2])<<8 ) |
    (std::uint32_t(p[1])<<16 ) | (std::uint32_t(p[0])<<24 );
}
inline std::uint64_t rd_u64(const unsigned char* p, byte_order bo) {
  if (bo==byte_order::little) { std::uint64_t v=0; for (int i=7;i>=0;--i) v=(v<<8)|p[i]; return v; }
  std::uint64_t v=0; for (int i=0;i<8;++i) v=(v<<8)|p[i]; return v;
}

// ===================== reader abstraction =====================

struct reader {
  virtual ~reader() = default;
  virtual std::size_t read(void* dst, std::size_t n) = 0;
  virtual bool seek(std::uint64_t pos) = 0;
  virtual std::uint64_t tell() const = 0;
  virtual std::uint64_t size() const = 0;
};

class istream_reader final : public reader {
public:
  explicit istream_reader(std::istream& is) : m_is(is) {
    auto cur = m_is.tellg();
    m_is.seekg(0, std::ios::end);
    m_size = static_cast<std::uint64_t>(m_is.tellg());
    m_is.seekg(cur);
  }
  std::size_t read(void* dst, std::size_t n) override {
    m_is.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(n));
    return static_cast<std::size_t>(m_is.gcount());
  }
  bool seek(std::uint64_t pos) override {
    m_is.clear(); m_is.seekg(static_cast<std::streamoff>(pos), std::ios::beg);
    return static_cast<bool>(m_is);
  }
  std::uint64_t tell() const override {
    return static_cast<std::uint64_t>(const_cast<std::istream&>(m_is).tellg());
  }
  std::uint64_t size() const override { return m_size; }
private:
  std::istream& m_is; std::uint64_t m_size{};
};

class span_reader final : public reader {
public:
  span_reader(const std::byte* data, std::size_t n) : m_base(data), m_size(n) {}
  std::size_t read(void* dst, std::size_t n) override {
    const auto left = m_size - m_pos; const auto k = n>left?left:n;
    if (k) std::memcpy(dst, m_base + m_pos, k); m_pos += k; return k;
  }
  bool seek(std::uint64_t pos) override { if (pos>m_size) return false; m_pos = static_cast<std::size_t>(pos); return true; }
  std::uint64_t tell() const override { return m_pos; }
  std::uint64_t size() const override { return m_size; }
private:
  const std::byte* m_base{}; std::size_t m_size{}; std::size_t m_pos{};
};

// ===================== chunk IO =====================

enum class chunk_source { explicit_data, prop_default };

struct chunk_header {
  fourcc id{}; std::uint64_t size{}; std::uint64_t file_offset{}; bool is_container{};
  std::optional<fourcc> type_tag{}; chunk_source source = chunk_source::explicit_data;
};

class chunk_reader {
public:
  chunk_reader() = default;
  chunk_reader(reader* r, std::uint64_t data_abs_offset, std::uint64_t data_size)
  : m_r(r), m_begin(data_abs_offset), m_size(data_size), m_pos(0) {}

  std::size_t read(void* dst, std::size_t n) {
    const auto k = n > remaining() ? remaining() : n; if (!k) return 0;
    m_r->seek(m_begin + m_pos); const auto got = m_r->read(dst, k); m_pos += got; return got;
  }
  bool skip(std::size_t n) { if (n>remaining()) return false; m_pos += n; return true; }
  std::size_t remaining() const { return static_cast<std::size_t>(m_size - m_pos); }
  std::uint64_t abs_data_offset() const { return m_begin; }

  template<class T> T read_pod(byte_order bo) {
    unsigned char buf[sizeof(T)]{}; if (read(buf, sizeof(T)) != sizeof(T)) throw parse_error(m_begin + m_pos, "short read");
    if constexpr (sizeof(T)==1) return static_cast<T>(buf[0]);
    if constexpr (sizeof(T)==2) return static_cast<T>(rd_u16(buf, bo));
    if constexpr (sizeof(T)==4) return static_cast<T>(rd_u32(buf, bo));
    if constexpr (sizeof(T)==8) return static_cast<T>(rd_u64(buf, bo));
  }
  std::string read_bytes(std::size_t n) { std::string s; s.resize(n); auto got = read(&s[0], n); s.resize(got); return s; }
private:
  reader* m_r{}; std::uint64_t m_begin{}, m_size{}, m_pos{};
};

// ===================== rf64 support =====================

struct rf64_state {
  std::optional<std::uint64_t> riff_size_u64{}; // logical riff size (incl type)
  struct key { fourcc id; std::uint64_t payload_off; bool operator==(const key& o) const { return id==o.id && payload_off==o.payload_off; } };
  struct key_hash { std::size_t operator()(const key& k) const noexcept { return fourcc_hash{}(k.id) ^ (std::hash<std::uint64_t>{}(k.payload_off)<<1); } };
  std::unordered_map<key, std::uint64_t, key_hash> overrides; // (id, abs payload off) -> u64 size
};

inline fourcc read_fourcc_at(reader& r, std::uint64_t off) {
  unsigned char buf[4]; if (!r.seek(off) || r.read(buf,4)!=4) throw parse_error(off, "read fourcc failed");
  fourcc f; std::memcpy(f.b.data(), buf, 4); return f;
}
inline std::uint32_t read_u32_at(reader& r, std::uint64_t off, byte_order bo) {
  unsigned char buf[4]; if (!r.seek(off) || r.read(buf,4)!=4) throw parse_error(off, "read u32 failed");
  return rd_u32(buf, bo);
}

inline rf64_state scan_top_level_for_ds64(reader& r, byte_order bo, std::uint64_t begin, std::uint64_t size, const options& opt) {
  rf64_state st{}; auto end = begin + size; auto cur = begin;
  while (cur + 8 <= end) {
    auto id = read_fourcc_at(r, cur);
    auto sz32 = read_u32_at(r, cur+4, bo);
    auto pay = cur + 8;
    if (id == fourcc{"ds64"} && sz32 >= 28) {
      // ds64 itself is little-endian per RIFF; safe to read raw and interpret as LE.
      std::vector<unsigned char> ds(sz32); r.seek(pay); r.read(ds.data(), sz32);
      st.riff_size_u64 = rd_u64(&ds[0], byte_order::little);
      auto table_count  = rd_u32(&ds[24], byte_order::little);
      std::size_t off = 28;
      for (std::uint32_t i=0; i<table_count; ++i) {
        if (off + 20 > ds.size()) break;
        fourcc cid; std::memcpy(cid.b.data(), &ds[off], 4); off += 4;
        auto csz = rd_u64(&ds[off], byte_order::little); off += 8;
        auto cpos = rd_u64(&ds[off], byte_order::little); off += 8;
        st.overrides.emplace(rf64_state::key{cid, cpos}, csz);
      }
    }
    cur += 8 + sz32 + (sz32 & 1u);
  }
  (void)opt; return st;
}

inline std::uint64_t maybe_override_size(std::uint64_t hdr_off, fourcc id, std::uint32_t sz32, const rf64_state* st) {
  if (sz32 != 0xFFFFFFFFu) return sz32;
  if (!st) return sz32; // unknown -> leave as is; bounds will clamp
  auto pay = hdr_off + 8;
  auto it = st->overrides.find(rf64_state::key{id, pay});
  if (it != st->overrides.end()) return it->second;
  return sz32;
}

// ===================== root detection =====================

struct root_info {
  fourcc group{}; std::optional<fourcc> type{}; byte_order bo{byte_order::little};
  std::uint64_t payload_begin{}; std::uint64_t payload_size{}; std::optional<rf64_state> rf64{};
};

inline root_info detect_root(reader& r, const options& opt) {
  unsigned char hdr[12]; if (!r.seek(0) || r.read(hdr,12)!=12) throw parse_error(0, "file too small");
  fourcc id; std::memcpy(id.b.data(), hdr, 4);
  byte_order bo = byte_order::little; bool is_riff = false;
  if (id == fcc::RIFF || id == fcc::RF64) { bo = byte_order::little; is_riff = true; }
  else if (id == fcc::RIFX || id == fcc::FORM || id == fcc::LIST || id == fcc::CAT_ || id == fcc::PROP) { bo = byte_order::big; }
  else throw parse_error(0, "unknown root id: "+id.to_string());

  auto sz32 = rd_u32(&hdr[4], bo); fourcc type; std::memcpy(type.b.data(), &hdr[8], 4);
  std::uint64_t payload_begin = 12; std::uint64_t payload_size = (sz32>=4)?(std::uint64_t(sz32)-4):0;

  root_info rt; rt.group=id; rt.type=type; rt.bo=bo; rt.payload_begin=payload_begin; rt.payload_size=payload_size;
  if (id == fcc::RF64 && opt.allow_rf64) { rt.rf64 = scan_top_level_for_ds64(r, bo, payload_begin, payload_size, opt); }

  // sanity vs file size
  auto file_sz = r.size(); auto total = 8ull + std::uint64_t(sz32) + (sz32 & 1u);
  if (total > file_sz && opt.strict) throw parse_error(0, "root size exceeds file size");
  (void)is_riff; return rt;
}

// ===================== cursor =====================

class chunk_cursor {
public:
  struct item { chunk_header hdr; chunk_reader payload; };

  static chunk_cursor children(reader& r, byte_order bo, std::uint64_t begin, std::uint64_t size,
                               const options& opt, const rf64_state* st) {
    return chunk_cursor(r, bo, begin, size, opt, st);
  }

  class iterator {
  public:
    using value_type = item; using reference = const item&; using pointer = const item*;
    using difference_type = std::ptrdiff_t; using iterator_category = std::input_iterator_tag;

    iterator() = default;
    iterator(reader* rd, byte_order bo, std::uint64_t cur, std::uint64_t end,
             const options* opt, const rf64_state* st)
      : m_r(rd), m_bo(bo), m_cur(cur), m_end(end), m_opt(opt), m_st(st) { load(); }

    reference operator*()  const { return m_item; }
    pointer   operator->() const { return &m_item; }

    iterator& operator++() { advance(); load(); return *this; }
    bool operator==(const iterator& o) const { return m_r==o.m_r && m_cur==o.m_cur && m_end==o.m_end; }
    bool operator!=(const iterator& o) const { return !(*this==o); }
  private:
    void load() {
      if (!m_r || m_cur >= m_end) { m_r=nullptr; return; }
      unsigned char hdr8[8]; if (!m_r->seek(m_cur) || m_r->read(hdr8,8)!=8) {
        if (m_opt->strict) throw parse_error(m_cur, "failed to read chunk header"); m_r=nullptr; return; }
      fourcc id; std::memcpy(id.b.data(), hdr8, 4);
      auto sz32 = rd_u32(&hdr8[4], m_bo); auto sz = maybe_override_size(m_cur, id, sz32, m_st);
      auto payload_begin = m_cur + 8; bool is_container = (id==fcc::RIFF||id==fcc::RIFX||id==fcc::RF64||id==fcc::LIST||id==fcc::FORM||id==fcc::CAT_||id==fcc::PROP);
      std::optional<fourcc> type{}; std::uint64_t payload_size = sz;
      if (is_container) {
        unsigned char t[4]; if (!m_r->seek(payload_begin) || m_r->read(t,4)!=4) {
          if (m_opt->strict) throw parse_error(payload_begin, "failed to read type tag"); m_r=nullptr; return; }
        type = fourcc{}; std::memcpy(type->b.data(), t, 4); payload_begin += 4; payload_size = (sz>=4)?(sz-4):0;
      }
      auto total = 8ull + sz + (sz & 1u);
      if (m_cur + total > m_end) {
        if (m_opt->strict) throw parse_error(m_cur, "chunk exceeds parent bounds");
        payload_size = (m_end > payload_begin) ? (m_end - payload_begin) : 0;
      }
      if (payload_size > m_opt->max_chunk_size) {
        if (m_opt->strict) throw parse_error(m_cur, "chunk too large"); payload_size = m_opt->max_chunk_size;
      }
      m_item.hdr = { id, payload_size, m_cur, is_container, type, chunk_source::explicit_data };
      m_item.payload = chunk_reader(m_r, payload_begin, payload_size);
      m_advance = total;
    }
    void advance() { if (!m_r) return; m_cur += m_advance; if (m_cur >= m_end) m_r=nullptr; }

    reader* m_r{}; byte_order m_bo{}; std::uint64_t m_cur{}, m_end{}, m_advance{}; const options* m_opt{}; const rf64_state* m_st{}; item m_item{};
  };

  iterator begin() const { return iterator(m_r, m_bo, m_begin, m_end, &m_opt, m_st); }
  iterator end()   const { return iterator(); }

private:
  chunk_cursor(reader& r, byte_order bo, std::uint64_t begin, std::uint64_t size, const options& opt, const rf64_state* st)
    : m_r(&r), m_bo(bo), m_begin(begin), m_end(begin+size), m_opt(opt), m_st(st) {}
  reader* m_r{}; byte_order m_bo{}; std::uint64_t m_begin{}, m_end{}; options m_opt; const rf64_state* m_st{};
};

// ===================== registry =====================

using handler = std::function<void(const chunk_header&, chunk_reader&, byte_order)>;

class registry {
public:
  void on_in_form(fourcc form_type, fourcc id, handler h)       { by_form_[{form_type,id}] = std::move(h); }
  void on_in_container(fourcc container_type, fourcc id, handler h) { by_container_[{container_type,id}] = std::move(h); }
  void on_id(fourcc id, handler h)                               { by_id_[id] = std::move(h); }

  const handler* find(const std::optional<fourcc>& current_form,
                      const std::optional<fourcc>& current_container,
                      const fourcc& id) const {
    if (current_form)     { auto it = by_form_.find(key{*current_form, id}); if (it!=by_form_.end()) return &it->second; }
    if (current_container){ auto it = by_container_.find(key{*current_container, id}); if (it!=by_container_.end()) return &it->second; }
    if (auto it = by_id_.find(id); it!=by_id_.end()) return &it->second; return nullptr;
  }
private:
  struct key { fourcc a, b; bool operator==(const key& o) const { return a==o.a && b==o.b; } };
  struct key_hash { std::size_t operator()(const key& k) const noexcept { return fourcc_hash{}(k.a) ^ (fourcc_hash{}(k.b)<<1); } };
  std::unordered_map<fourcc, handler, fourcc_hash> by_id_{};
  std::unordered_map<key, handler, key_hash> by_container_{};
  std::unordered_map<key, handler, key_hash> by_form_{};
};

// ===================== plan =====================

struct plan {
  std::vector<std::pair<std::optional<fourcc>, fourcc>> want{}; int max_depth = 32;
  bool wants_container(const fourcc& type) const {
    if (want.empty()) return true; for (auto& p : want) if (p.first && *p.first==type) return true; return false;
  }
  bool wants_chunk(const std::optional<fourcc>& scope, const fourcc& id) const {
    if (want.empty()) return true; for (auto& p : want) { bool scope_ok = !p.first || (scope && *scope==*p.first); if (scope_ok && p.second==id) return true; } return false;
  }
};

// ===================== walk (raw) =====================

struct ctx_frame { std::optional<fourcc> form_type; std::optional<fourcc> container_type; };

inline void walk(reader& r, const options& opt, const registry& reg, const plan* pl = nullptr) {
  auto rt = detect_root(r, opt); std::vector<ctx_frame> stack; stack.push_back({ rt.type, rt.type });
  std::function<void(std::uint64_t,std::uint64_t,int)> rec = [&](std::uint64_t begin, std::uint64_t size, int depth){
    if (depth > opt.max_depth) { if (opt.strict) throw parse_error(begin, "max depth exceeded"); return; }
    auto cur = chunk_cursor::children(r, rt.bo, begin, size, opt, rt.rf64 ? &*rt.rf64 : nullptr);
    for (auto it = cur.begin(); it != cur.end(); ++it) {
      const auto& h = it->hdr; auto pr = it->payload; // copy reader view
      if (h.is_container) {
        ctx_frame next;
        if (h.id==fcc::FORM || h.id==fcc::RIFF || h.id==fcc::RIFX || h.id==fcc::RF64) next = { h.type_tag, h.type_tag };
        else next = { stack.back().form_type, h.type_tag };
        stack.push_back(next);
        if (!pl || pl->wants_container(*h.type_tag)) rec(pr.abs_data_offset(), h.size, depth+1);
        stack.pop_back();
      } else {
        auto scope = stack.back().container_type; if (!pl || pl->wants_chunk(scope, h.id)) {
          if (auto* cb = reg.find(stack.back().form_type, scope, h.id)) (*cb)(h, pr, rt.bo);
        }
        if (pr.remaining() && opt.on_warning) opt.on_warning(h.file_offset, "PARTIAL_READ", "handler did not consume full payload; skipping remainder");
      }
    }
  };
  rec(rt.payload_begin, rt.payload_size, 1); stack.pop_back();
}

// ===================== structured IFF-85 =====================

struct span { std::uint64_t off{}, len{}; };
struct prop_bank { std::unordered_map<fourcc, span, fourcc_hash> by_id; };

struct property_view {
  const reader* r{}; const std::unordered_map<fourcc, span, fourcc_hash>* map{};
  std::optional<chunk_reader> open(const fourcc& id) const {
    if (!r || !map) return std::nullopt; auto it = map->find(id); if (it==map->end()) return std::nullopt; return chunk_reader(const_cast<reader*>(r), it->second.off, it->second.len);
  }
};

struct form_context { fourcc form_type{}; std::uint64_t payload_begin{}; std::uint64_t payload_size{}; std::size_t index_in_list{}; property_view defaults{}; };
using form_cb = std::function<void(const form_context&)>;
struct structured_hooks { form_cb on_form_begin{}, on_form_end{}; };

struct structured_options : options { bool inject_prop_defaults = true; bool validate_list_types = true; };

struct child_info { fourcc id; std::optional<fourcc> type_tag; std::uint64_t payload_begin; std::uint64_t payload_size; std::uint64_t file_offset; };

inline std::vector<child_info> index_children(reader& r, byte_order bo, std::uint64_t begin, std::uint64_t size,
                                             const options& opt, const rf64_state* st) {
  std::vector<child_info> v; auto cur = chunk_cursor::children(r, bo, begin, size, opt, st);
  for (auto it = cur.begin(); it != cur.end(); ++it) {
    v.push_back(child_info{ it->hdr.id, it->hdr.type_tag, it->payload.abs_data_offset(), it->hdr.size, it->hdr.file_offset });
  }
  return v;
}

inline property_view make_property_view(const reader& r, const prop_bank& bank) {
  return property_view{ &r, &bank.by_id };
}

inline void walk_iff85(reader& r, const structured_options& sopt, const registry& reg, const plan* pl = nullptr, const structured_hooks* hooks = nullptr) {
  auto rt = detect_root(r, sopt); std::vector<ctx_frame> stack; stack.push_back({ rt.type, rt.type });

  std::function<void(std::uint64_t,std::uint64_t,int,std::size_t, prop_bank*)> rec =
  [&](std::uint64_t begin, std::uint64_t size, int depth, std::size_t form_index, prop_bank* enclosing_props){
    if (depth > sopt.max_depth) { if (sopt.strict) throw parse_error(begin, "max depth exceeded"); return; }
    auto cur = chunk_cursor::children(r, rt.bo, begin, size, sopt, rt.rf64 ? &*rt.rf64 : nullptr);
    for (auto it = cur.begin(); it != cur.end(); ++it) {
      auto h = it->hdr; auto pr = it->payload;

      if (h.is_container && h.id == fcc::LIST) {
        auto list_children = index_children(r, rt.bo, pr.abs_data_offset(), h.size, sopt, rt.rf64 ? &*rt.rf64 : nullptr);
        prop_bank bank{}; bool seen_form = false; std::size_t local_index = 0;
        for (const auto& ch : list_children) {
          if (ch.id == fcc::PROP) {
            if (sopt.validate_list_types && ch.type_tag != h.type_tag) {
              if (sopt.strict) throw parse_error(ch.file_offset, "PROP type mismatch in LIST");
              else if (sopt.on_warning) sopt.on_warning(ch.file_offset, "STRUCT", "PROP type mismatch in LIST");
            }
            if (seen_form) {
              if (sopt.strict) throw parse_error(ch.file_offset, "PROP after FORM in LIST");
              else if (sopt.on_warning) sopt.on_warning(ch.file_offset, "STRUCT", "PROP after FORM in LIST");
            }
            auto prop_cur = chunk_cursor::children(r, rt.bo, ch.payload_begin, ch.payload_size, sopt, rt.rf64 ? &*rt.rf64 : nullptr);
            for (auto pit = prop_cur.begin(); pit != prop_cur.end(); ++pit) {
              bank.by_id[pit->hdr.id] = span{ pit->payload.abs_data_offset(), pit->hdr.size };
            }
          } else if (ch.id == fcc::FORM) {
            if (sopt.validate_list_types && ch.type_tag != h.type_tag) {
              if (sopt.strict) throw parse_error(ch.file_offset, "FORM type mismatch in LIST");
              else if (sopt.on_warning) sopt.on_warning(ch.file_offset, "STRUCT", "FORM type mismatch in LIST");
            }
            // form begin
            if (hooks && hooks->on_form_begin) {
              hooks->on_form_begin(form_context{ *ch.type_tag, ch.payload_begin, ch.payload_size, local_index, make_property_view(r, bank) });
            }
            // inject defaults before actual children
            if (sopt.inject_prop_defaults) {
              // stable order by raw FourCC bytes for determinism
              std::vector<std::pair<fourcc, span>> pairs; pairs.reserve(bank.by_id.size());
              for (auto& kv : bank.by_id) pairs.push_back(kv);
              std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b){ return std::memcmp(a.first.b.data(), b.first.b.data(), 4) < 0; });
              for (auto& kv : pairs) {
                chunk_header fh{ kv.first, kv.second.len, kv.second.off - 8, false, std::nullopt, chunk_source::prop_default };
                chunk_reader cr(&r, kv.second.off, kv.second.len);
                auto scope = stack.back().container_type; if (!pl || pl->wants_chunk(scope, fh.id)) {
                  if (auto* cb = reg.find(stack.back().form_type, scope, fh.id)) (*cb)(fh, cr, rt.bo);
                }
              }
            }
            // recurse into FORM children
            stack.push_back({ ch.type_tag, ch.type_tag });
            rec(ch.payload_begin, ch.payload_size, depth+1, local_index, &bank);
            stack.pop_back();
            if (hooks && hooks->on_form_end) {
              hooks->on_form_end(form_context{ *ch.type_tag, ch.payload_begin, ch.payload_size, local_index, make_property_view(r, bank) });
            }
            seen_form = true; ++local_index;
          } else {
            // other container under LIST – recurse raw
            ctx_frame next{ stack.back().form_type, ch.type_tag };
            stack.push_back(next); rec(ch.payload_begin, ch.payload_size, depth+1, form_index, enclosing_props); stack.pop_back();
          }
        }
      }
      else if (h.is_container && h.id == fcc::CAT_) {
        auto kids = index_children(r, rt.bo, pr.abs_data_offset(), h.size, sopt, rt.rf64 ? &*rt.rf64 : nullptr);
        for (const auto& ch : kids) {
          if (sopt.validate_list_types && (ch.id != fcc::FORM || ch.type_tag != h.type_tag)) {
            if (sopt.strict) throw parse_error(ch.file_offset, "CAT must contain FORM<T>");
            else if (sopt.on_warning) sopt.on_warning(ch.file_offset, "STRUCT", "CAT child not FORM<T>");
          }
          stack.push_back({ ch.type_tag, ch.type_tag }); rec(ch.payload_begin, ch.payload_size, depth+1, 0, nullptr); stack.pop_back();
        }
      }
      else if (h.is_container && (h.id == fcc::FORM || h.id==fcc::RIFF || h.id==fcc::RIFX || h.id==fcc::RF64)) {
        if (hooks && hooks->on_form_begin) hooks->on_form_begin(form_context{ *h.type_tag, pr.abs_data_offset(), h.size, 0, property_view{} });
        stack.push_back({ h.type_tag, h.type_tag }); rec(pr.abs_data_offset(), h.size, depth+1, 0, nullptr); stack.pop_back();
        if (hooks && hooks->on_form_end) hooks->on_form_end(form_context{ *h.type_tag, pr.abs_data_offset(), h.size, 0, property_view{} });
      }
      else if (h.is_container) {
        // generic container (e.g., LIST INFO in RIFF)
        stack.push_back({ stack.back().form_type, h.type_tag });
        if (!pl || pl->wants_container(*h.type_tag)) rec(pr.abs_data_offset(), h.size, depth+1, form_index, enclosing_props);
        stack.pop_back();
      }
      else {
        auto scope = stack.back().container_type; if (!pl || pl->wants_chunk(scope, h.id)) {
          if (auto* cb = reg.find(stack.back().form_type, scope, h.id)) (*cb)(h, pr, rt.bo);
        }
        if (pr.remaining() && sopt.on_warning) sopt.on_warning(h.file_offset, "PARTIAL_READ", "handler did not consume full payload; skipping remainder");
      }
    }
  };

  rec(rt.payload_begin, rt.payload_size, 1, 0, nullptr); stack.pop_back();
}

} // namespace iff
```

### Notes

* The structured walker uses a light two-pass within `LIST<T>` to collect `PROP<T>` defaults and then walk `FORM<T>`s. Defaults are injected in a stable order (sorted by raw FourCC) before actual FORM children when `inject_prop_defaults=true`.
* RF64 handling covers the common case: 32-bit size `0xFFFFFFFF` overridden by `ds64` table keyed by `(id, payload_offset)`.
* Handlers are standard `std::function` and use **form → container → id** precedence for disambiguation.
* All naming is **snake\_case** as requested.

```
```
