# Debugging guru meditations on PortaPack

Notes and tooling for turning a guru screen into a source line, and for catching
two failure modes that are invisible at build time.

Set `ARM_TOOLCHAIN` once if your toolchain is not on `PATH`:

```sh
export ARM_TOOLCHAIN=/path/to/arm-none-eabi-      # note the trailing dash
```

All three scripts also auto-detect `./armbin/bin/arm-none-eabi-`.

---

## Reading the guru screen

`draw_guru_meditation()` prints `r0`–`r3`, `r12`, `lr`, `pc`, and sometimes `cfsr`.

- **`pc`** is where it died.
- **`lr`** is the return address of the function that was executing — i.e. *the caller*.

`lr` is usually the more useful of the two. If `pc` is a wild value with no
symbol but `lr` looks sane, that is an **indirect call through a bad pointer**
(a `blx` on a corrupted `std::function`, a stale vtable, a branch to a function
that is not resident). Look up `lr` — it tells you the call site.

The hint string matters:

| Hint | Where it comes from | Means |
|---|---|---|
| `Hard Fault` | `HardFaultVector`, `application/debug.cpp` | generic fault, look at `pc`/`lr` |
| `Stack Overflow` | same handler, when `get_free_stack_space() < 16` | the 4 kB M0 stack really was consumed |
| `M4 Guru` header | `event_m0.cpp` | fault is in the baseband, not the app |

`Stack Overflow` is **not** a heap or fragmentation problem. See "Stack budget".

---

## 1. Translating an address — `guru_lookup.py`

The classic approach:

```sh
arm-none-eabi-gdb -q -ex "x/3i 0xADDR" --batch firmware/application/application.elf
```

works for the main firmware, but gives nonsense for **external apps**. External
apps are linked at a placeholder `0xADxxxxxx` address (see `external.ld`) and
copied at runtime to their own `memory_location` in local SRAM, around
`0x1008xxxx`. The guru shows the runtime address, which does not exist in the ELF.

The translation is:

```
link_addr = section_vma + (runtime_addr - memory_location)
```

`memory_location` is the first 32-bit word of the app's `.ppma`; the section VMA
comes from `objdump -h`. `guru_lookup.py` does this for you:

```sh
# guru showed lr 0x10085CF9, pc 0x0FF36284
firmware/tools/guru_lookup.py 0x10085CF9 0x0FF36284

# each app loads at its own address, so several can match; narrow it if you know
firmware/tools/guru_lookup.py --app waterfall_designer 0x10085CF9

# M4 fault - resolve against the baseband image instead
firmware/tools/guru_lookup.py --baseband adsbrx 0x10081234
```

It prints the demangled symbol, the source line **including inlined frames**
(`addr2line -i`, which plain `x/3i` does not give you), and a disassembly window
with the faulting instruction marked.

> **Resolve against the exact binary that crashed.** Addresses shift on every
> build. If you have already rebuilt, the answer will be confidently wrong.

---

## 2. Stack budget — `stack_usage.py`

The M0 process stack is **4096 bytes** total (`__process_stack_size__ = 0x1000`
in `LPC43xx_M0.ld`). That is the whole budget for the UI thread, external apps
included — they get their own 32 kB code/data region but share this stack. The
heap is ~43 kB and is rarely the problem.

The main hazard is `File`. `ffconf.h` sets `_FS_TINY 0` and `_MAX_SS 512`, so
every `FIL` carries a 512-byte sector cache and `File` holds one by value:

| | stack |
|---|---|
| one `File` local | ~560 B |
| `copy_file()` (two `File`s + 512 B block buffer) | ~1784 B |
| `Gradient::load_file()` | ~760 B |
| `run_external_app()` (holds a `File`) | ~776 B |
| `BufferLineReader::read_line()` | ~192 B |

Two or three nested `File` locals under a UI event-dispatch chain is enough to
overflow.

```sh
# worst offenders
firmware/tools/stack_usage.py build/firmware/application/application.elf

# one function
firmware/tools/stack_usage.py build/.../application.elf --grep copy_file

# add up a call chain and check it against the 4 kB budget
firmware/tools/stack_usage.py build/.../application.elf \
    --chain main run_external_app copy_file f_open
```

### Why not just grep for `sub sp`

Thumb-1 cannot encode `sub sp, #imm` above 508 bytes, so the frames that matter
are emitted as a negative constant loaded from the literal pool:

```
ldr r4, [pc, #728]   ; (2e0 <...>)
add sp, r4           ; r4 = 0xfffffcfc = -772
```

A naive grep sees `sub sp, #24` and misses the 772-byte frame right next to it.
`stack_usage.py` resolves the literal.

### Avoiding overflow

- Give every `File` its own function so its frame dies before the next step;
  `__attribute__((noinline))` pins that if the chain is tight.
- Do not do file I/O inside a callback that is already several frames deep.
  `text_prompt()` and `FileLoadView` both invoke your handler and *then* call
  `nav.pop()` — defer the work with `nav.set_on_pop()` and it runs shallow.
  (That also fixes a modal pushed from such a handler being popped instead of
  the keyboard.)
- Constructors of external apps run under `run_external_app()`, ~776 B down.
  Do not call `copy_file()` from there.

---

## 3. Section leakage between external apps — `check_external_symbol_placement.py`

Rules in `external.ld` match input **section names**, not namespaces. A glob like

```
*(*ui*external_app*level*);
```

also matches

```
.text._ZN2ui12external_app18waterfall_designer21WaterfallDesignerView12on_add_levelEv
                                                                      ^^^^^ contains "level"
```

Section assignment is first-match-wins, so a symbol whose mangled name merely
*contains* another app's name is linked into that app's region. Only one
external app is resident at a time, so calling it branches into unmapped memory.

This is silent at build time and shows up as a hard fault with a wild `pc`:

```
adf613d4 <..._M_invoke  (button_add_level.on_select)>:
adf613d8:   bl   ade11968 <...on_add_level()>     <-- the 'level' app's region
```

`export_external_apps.py` already warns about *data* words pointing at another
app (`WARNING: External code address ...`). It cannot catch this case: a `bl` is
PC-relative, so the target never appears as a literal word. The offset survives
the copy to the runtime base verbatim, which is exactly why the resulting `pc`
looks like garbage rather than like another app's address.

```sh
firmware/tools/check_external_symbol_placement.py
```

Exits non-zero and names the offending symbols. Worth running after touching
`external.ld`, adding an app, or naming a method after an existing app.

Fix by scoping the rule to the app's own sources:

```diff
-        *(*ui*external_app*level*);
+        */external/level/*(*ui*external_app*level*);
```

> `LPC43xx_M0.ld` does `INCLUDE external.ld`. Make sure both are in
> `LINK_DEPENDS` (`firmware/application/CMakeLists.txt`) or editing the section
> rules will not relink and your change will look like a no-op.

---

## Worked example

Guru showed `lr 0x10085CF9`, `pc 0x0FF36284`.

1. `pc` resolves to nothing, `lr` is in local SRAM → external app, indirect or
   out-of-range branch. Look up `lr`.
2. `guru_lookup.py --app waterfall_designer 0x10085CF9` →
   `0x10085CF8 - 0x1008491C = 0x13DC`, link address `0xADF613DC`, which is
   `ui_waterfall_designer.cpp:144`, the `button_add_level.on_select` lambda.
3. Disassembly showed `bl ade11968` — the `level` app's region, not
   `0xADF6xxxx`.
4. `check_external_symbol_placement.py` confirmed four symbols in the wrong
   region, all containing `level` in their mangled names.
5. The reported `pc` follows arithmetically: the `bl` offset (`-0x14FA74`) is
   PC-relative and survives the copy, so at runtime
   `0x10085CF8 - 0x14FA74 = 0x0FF36284`. Both registers reproduce exactly.
