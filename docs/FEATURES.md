# CommonLibF4RD features and migration guide

CommonLibF4RD provides runtime-aware relocations. Existing plugin code can continue to use `REL::ID`, while the relocation layer resolves the requested symbol for the running Fallout 4 runtime and caches the result for the lifetime of the process.

This document describes the public API intended for plugin authors.

## Runtime families

CommonLibF4RD currently distinguishes three runtime families:

| Family | Typical runtime | Description |
| --- | --- | --- |
| OG | 1.10.163 | Original pre-next-generation runtime |
| NG | 1.10.984 | Next-generation runtime |
| AE | 1.11.x | Current runtime family, including 1.11.221 and 1.11.240 |

The active family is selected automatically. Plugin code should not compare executable versions merely to select an ID or a relocation offset.

Install the release database as:

```text
Data/F4SE/Plugins/f4rd-runtime.bin
```

The release file provides the stable OG table used by 1.10.163 together with runtime-aware data for NG and AE. Plugins do not need to select or rename a database according to the executable version.

Runtime-aware relocations do not make class layouts automatically compatible. A plugin must still use definitions and F4SE compatibility metadata that are valid for every runtime it supports.

## Do not hard-block executable patch versions

An F4RD plugin should not reject the game merely because its exact executable version is absent from a hard-coded list. A normal Fallout 4 update must be allowed to reach the Runtime Database and attempt resolution.

Avoid checks like this:

```cpp
const auto version = a_f4se->RuntimeVersion();
if (version != F4SE::RUNTIME_1_11_221 &&
    version != F4SE::RUNTIME_1_11_240) {
    return false;
}
```

Initialize F4SE and let required relocations determine whether the plugin can operate:

```cpp
F4SE::Init(a_f4se);
logger::info("Loaded on runtime {}", a_f4se->RuntimeVersion().string());

return InitializePluginFeatures();
```

Use address- and structure-independence metadata instead of an exact `compatibleVersions` whitelist:

```cpp
data.addressIndependence =
    F4SE::PluginVersionData::kAddressIndependence_Signatures;

data.structureIndependence =
    F4SE::PluginVersionData::kStructureIndependence_1_10_980Layout |
    F4SE::PluginVersionData::kStructureIndependence_1_11_137Layout;
```

Loading may still fail safely when a required ID or callsite cannot be resolved, a symbol is unavailable on the active runtime, or a required ABI/layout validation fails. These are capability failures, not version-number failures.

## Lazy ID resolution

The familiar form remains valid:

```cpp
REL::Relocation<std::uintptr_t> function{
    REL::ID(2229323)
};
```

Only IDs requested by the plugin are resolved during normal operation. Successful results are cached, so repeated use of the same ID does not repeat the full resolution work.

If an ID cannot be resolved, CommonLibF4RD reports a specific failure instead of silently returning an unsafe address.

## Runtime-aware `REL::ID`

`REL::ID` supports one, two, or three IDs:

```cpp
// One portable AE ID. NG and OG may resolve the same logical ID when supported.
REL::ID(2229323)

// OG, AE. NG uses the AE ID.
REL::ID(1546751, 2229323)

// OG, NG, AE.
REL::ID(1546751, 2229323, 2229323)
```

The argument order is always:

```text
REL::ID(OG, NG, AE)
```

Use the shortest form that is known to be correct:

- Use `REL::ID(AE)` when the portable ID has been verified for every supported runtime.
- Use `REL::ID(OG, AE)` when OG needs a different legacy ID and NG shares the AE ID.
- Use `REL::ID(OG, NG, AE)` when all three families require explicit IDs.

If an implicit OG or NG bridge is unavailable, resolution fails with `og_bridge_failed` or `ng_bridge_failed`. Supplying an explicit runtime ID is preferable to guessing an address.

## `REL::VariantOffset`

An ID normally identifies a function or object. Hooks often need an address inside that function. `REL::VariantOffset` selects the correct relative offset for the active runtime family.

```cpp
// The same offset for OG, NG, and AE.
REL::VariantOffset(0x8F7)

// OG, modern. The second value is used by both NG and AE.
REL::VariantOffset(0x921, 0x8F7)

// OG, NG, AE.
REL::VariantOffset(0x921, 0x930, 0x8F7)
```

It can be passed directly to a relocation:

```cpp
REL::Relocation<std::uintptr_t> hookSite{
    REL::ID(1546751, 2229323),
    REL::VariantOffset(0x921, 0x8F7)
};
```

`VariantOffset` selects a runtime family, not an individual executable patch. For example, 1.11.221 and 1.11.240 both use the AE value. If an interior hook position may move between updates within the same family, prefer automatic callsite resolution.

## Automatic callsite offsets

Hard-coded interior offsets are fragile when compiler output changes. `REL::AUTO_CALLSITE` identifies a callsite by its logical relationship:

1. Resolve the owner function.
2. Resolve the function being called.
3. Find the matching call inside the owner function.
4. Use the discovered offset for the running executable.

The default form requires exactly one matching callsite:

```cpp
constexpr REL::ID owner{
    1546751,  // OG
    2229323   // AE; also used by NG
};

constexpr REL::ID target{
    881215,   // OG
    2231148   // AE; also used by NG
};

REL::Relocation<std::uintptr_t> hookSite{
    owner,
    REL::VariantOffset{
        0x921,                         // OG: known fixed offset
        REL::AUTO_CALLSITE(target),    // NG: discover it at runtime
        0x8F7                          // AE: known fixed offset
    }
};
```

Automatic resolution can also be enabled for every runtime family:

```cpp
REL::Relocation<std::uintptr_t> hookSite{
    owner,
    REL::VariantOffset{
        REL::AUTO_CALLSITE(target)
    }
};
```

The default branch type is a direct call. Direct jumps and either form can be requested explicitly:

```cpp
REL::AUTO_CALLSITE(target, REL::AutoCallsiteBranch::kJump)
REL::AUTO_CALLSITE(target, REL::AutoCallsiteBranch::kCallOrJump)
```

The default unique-match behavior is recommended. It fails with `callsite_not_found` or `callsite_ambiguous` if the relationship is no longer safe to identify.

`REL::AUTO_OFFSET` is only an automatic-offset marker. A relocation using it without an `AUTO_CALLSITE` target cannot determine which instruction is intended and therefore fails deliberately. Use `REL::AUTO_CALLSITE(target)` for an automatically resolved hook position.

## First, last, nth, and all callsites

If the owner intentionally calls the same target more than once, a specific occurrence can be selected:

```cpp
REL::AUTO_CALLSITE_FIRST(target)
REL::AUTO_CALLSITE_LAST(target)
REL::AUTO_CALLSITE_NTH(target, 2)  // zero-based: selects the third match
```

These selectors can also receive an `AutoCallsiteBranch` argument. They are less robust than unique matching because an update may reorder or insert calls.

To inspect or handle every matching callsite, use `REL::resolve_callsites`:

```cpp
const auto calls = REL::resolve_callsites(owner, target);
if (!calls) {
    logger::error("Callsites could not be resolved: {}",
        REL::id_resolve_status_text(calls.status));
    return;
}

for (const auto rva : calls.rvas) {
    REL::Relocation<std::uintptr_t> callsite{
        REL::Offset(rva)
    };

    // Validate the call context before installing a hook.
}
```

Hooking every callsite is not automatically safe. Each location must have the expected calling convention, register state, and logical purpose.

## Diagnostics

Normal operation writes one compact `F4RD OK` message after the first successful resolution. Resolution failures are always logged with `F4RD FAIL`, the ID, and a specific reason.

### Per-plugin trace

Create an empty file next to the plugin DLL before starting the game:

```text
RobCoPatcherRD.dll
RobCoPatcherRD.trace
```

If the `.trace` file exists, it is overwritten and populated with the IDs actually requested by that plugin. Entries contain the resolved RVA. Relocations using `VariantOffset` or `AUTO_CALLSITE` additionally include the selected offset, final RVA, runtime slot, and whether the offset was fixed or automatic.

Tracing is opt-in. Delete or rename the `.trace` file to disable it.

### Complete runtime mapping

Create an empty mapping file next to the DLL:

```text
RobCoPatcherRD.dll
RobCoPatcherRD.mapping
```

If the `.mapping` file exists, CommonLibF4RD overwrites it with a complete `ID RVA` mapping for the current runtime:

```text
343176 0x48A0
224532 0xA8A0
```

If any entries cannot be resolved, a companion file is created automatically:

```text
RobCoPatcherRD.mapping.fail
```

The failure file contains the ID and reason. `runtime_unavailable` means the symbol is known but does not exist in the active runtime; it is different from a broken or missing pattern.

Full mapping is a diagnostic operation and may noticeably increase startup time. It should not be enabled for normal gameplay or release packages.

## Preparing a plugin for future Fallout 4 updates

Runtime-independent function IDs reduce the amount of plugin code that must change after a normal executable update. They cannot guarantee compatibility with every possible code or ABI change.

For the best update resilience:

1. Use `REL::ID` for function starts, globals, RTTI, VTables, events, and singletons instead of hard-coded RVAs.
2. Use the appropriate two- or three-ID overload when a runtime family has a different ID.
3. Use `VariantOffset` only when an interior offset is known to be stable for the entire runtime family.
4. Prefer unique `AUTO_CALLSITE` relationships for callsite hooks that may shift between executable patches.
5. Treat `FIRST`, `LAST`, `NTH`, and `resolve_callsites` as advanced APIs and validate every selected location.
6. Test every supported runtime with a `.trace` file before release.
7. Use `.mapping` only for dedicated validation runs.
8. Keep runtime-specific class layouts and ABI differences separate from address resolution.
9. Use F4SE address- and structure-independence metadata; do not maintain an exact executable-version whitelist.
10. Do not reject an unknown patch number before required IDs, callsites, and ABI assumptions have been validated.

An automatic callsite can survive inserted instructions or a shifted function body when the owner-to-target relationship remains intact. It correctly fails when the call is removed, inlined, changed to an unsupported form, or becomes ambiguous. This failure is safer than installing a hook at a guessed offset.

## Important failure reasons

| Status | Meaning |
| --- | --- |
| `unknown_id` | The ID is not present in the Runtime Database. |
| `runtime_unavailable` | The symbol is known but does not exist in this runtime family. |
| `og_bridge_failed` | A portable AE ID could not be bridged to OG; provide an explicit OG ID. |
| `ng_bridge_failed` | A portable AE ID could not be bridged to NG; provide an explicit NG ID. |
| `pattern_not_found` | No valid address was found in the active executable. |
| `pattern_ambiguous` | More than one address remained and none was safe to select. |
| `semantic_validation_failed` | An address was found but failed its semantic safety checks. |
| `callsite_not_found` | The requested owner-to-target callsite does not exist. |
| `callsite_ambiguous` | Unique callsite resolution found multiple matches. |
| `invalid_callsite` | The owner, target, function boundary, or callsite specification is invalid. |

Do not catch these errors and continue with an arbitrary address. A plugin should stop loading the affected feature or fail initialization safely.
