find_package(OpenCASCADE REQUIRED)
find_package(Netgen REQUIRED)

add_executable("mesh" mesh.cpp)
target_compile_options("mesh" PRIVATE ${CPP_WARNING_FLAGS} ${EXTRA_CPP_FLAGS})
target_link_libraries("mesh" PRIVATE TKMath TKernel TKOffset TKSTL TKernel TKLCAF TKXCAF TKXDESTEP TKBinXCAF TKRWMesh TKMesh TKBool TKMath TKernel nglib)
