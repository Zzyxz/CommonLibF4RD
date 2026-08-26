# CommonLibF4RD

CommonLibF4RD is a C++20 library for F4SE plugin authors. It keeps the familiar CommonLibF4 API while adding runtime-aware ID resolution, runtime-family IDs and offsets, automatic callsite discovery, and opt-in relocation diagnostics.

## Supported runtime families

| Family | Runtime |
| --- | --- |
| OG | Fallout 4 1.10.163 |
| NG | Fallout 4 1.10.984 |
| AE | Fallout 4 1.11.x, including 1.11.221 and 1.11.240 |

The active runtime family is selected automatically. Plugins should not reject an executable solely because its exact patch version is not explicitly listed.

## Requirements

- Windows x64
- Visual Studio 2022 with Desktop development with C++
- CMake 3.21 or newer
- vcpkg
- F4SE for the runtime being tested

Set the `VCPKG_ROOT` environment variable before using the included CMake preset.

## Adding CommonLibF4RD to a plugin

Add the repository as a source dependency:

```text
git submodule add https://github.com/Zzyxz/CommonLibF4RD.git external/CommonLibF4RD
```

Add the library and link its public target:

```cmake
add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/external/CommonLibF4RD/CommonLibF4"
    CommonLibF4
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        CommonLibF4::CommonLibF4
)
```

Include the CommonLibF4RD umbrella header from the plugin''s precompiled header:

```cpp
#include <F4SE/F4SE.h>
```

Individual `RE` and `REL` headers assume that the umbrella header has already supplied the shared platform and standard-library declarations.

The plugin's vcpkg manifest must provide these packages:

```json
{
  "dependencies": [
    "boost-stl-interfaces",
    "fmt",
    "rsm-mmio",
    "spdlog",
    "zydis"
  ]
}
```

## Building the included example

```text
cmake --preset vs2022-windows-vcpkg
cmake --build --preset vs2022-release
```

When CommonLibF4RD is included from another CMake project, the example plugin is not built unless `COMMONLIBF4RD_BUILD_EXAMPLE` is enabled explicitly.

## Runtime Database

End users need the CommonLibF4RD Runtime Database at:

```text
Data/F4SE/Plugins/f4rd-runtime.bin
```

The Runtime Database is distributed separately from this source repository. A plugin does not select or rename the file for a particular executable version.

## Plugin compatibility metadata

Use F4SE address- and structure-independence metadata instead of an exact executable-version whitelist:

```cpp
data.addressIndependence =
    F4SE::PluginVersionData::kAddressIndependence_Signatures;

data.structureIndependence =
    F4SE::PluginVersionData::kStructureIndependence_1_10_980Layout |
    F4SE::PluginVersionData::kStructureIndependence_1_11_137Layout;
```

Required IDs, callsites, and ABI assumptions must still fail safely when they cannot be validated.

## Public relocation APIs

The [features and migration guide](docs/FEATURES.md) documents:

- `REL::ID(AE)`, `REL::ID(OG, AE)`, and `REL::ID(OG, NG, AE)`
- `REL::VariantOffset`
- `REL::AUTO_CALLSITE` and callsite selectors
- per-plugin `.trace` diagnostics
- complete `.mapping` validation
- resolver failure states and update-safe plugin behavior

## Release packaging

A normal plugin release should contain the plugin DLL and its required assets. Do not include `.trace`, `.mapping`, `.mapping.fail`, or PDB files unless the package is specifically intended for diagnostics.

## License

CommonLibF4RD is available under the MIT License. See [LICENSE](LICENSE).
