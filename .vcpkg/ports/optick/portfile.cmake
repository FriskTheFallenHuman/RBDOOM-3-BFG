vcpkg_check_linkage( ONLY_DYNAMIC_LIBRARY )

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO bombomby/optick
    REF 1.4.0.0
    SHA512 9b835163aef7adaa7296c90eb104a742a32021a420f05ae7dd4025bceea0c4f624a1f447684a2c8a88ee8c43fd54ec83809e9434e6a786922b3b07b90f10c64e
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DOPTICK_USE_VULKAN=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup( CONFIG_PATH lib/cmake/optick )

file( REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" )

vcpkg_copy_pdbs()

# Handle copyright
file( INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright )