# =========================================================
# Sanitizers.cmake
# =========================================================
# Provides a function to enable AddressSanitizer and
# UndefinedBehaviorSanitizer on a target in Debug builds.
# Usage: target_enable_sanitizers(my_target)
# =========================================================

option(ENABLE_SANITIZERS "Enable AddressSanitizer + UBSan in Debug builds" ON)

function(target_enable_sanitizers target)

    if(NOT ENABLE_SANITIZERS)
        message(STATUS "Sanitizers disabled via ENABLE_SANITIZERS=OFF")
        return()
    endif()

    if(MSVC)
        # MSVC supports /fsanitize=address but not UBSan
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${target} PRIVATE /fsanitize=address)
            message(STATUS "MSVC AddressSanitizer enabled for ${target}")
        endif()
        return()
    endif()

    # --- GCC / Clang ---
    set(SANITIZER_FLAGS
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        target_compile_options(${target} PRIVATE ${SANITIZER_FLAGS})
        target_link_options(${target} PRIVATE ${SANITIZER_FLAGS})
        message(STATUS "ASan + UBSan enabled for target: ${target}")
    else()
        message(STATUS "Sanitizers skipped (build type: ${CMAKE_BUILD_TYPE})")
    endif()

endfunction()
