# CommonLibF4 target

Link the public CMake target:

```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibF4::CommonLibF4)
```

Required vcpkg packages: `boost-stl-interfaces`, `fmt`, `rsm-mmio`, `spdlog`, and `zydis`.

See the repository [README](../README.md) and [features guide](../docs/FEATURES.md) for complete setup and API documentation.
