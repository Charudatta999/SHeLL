# =========================================================
# CompilerWarnings.cmake
# =========================================================
# Provides a function to apply strict compiler warnings to a target.
# Usage: target_set_warnings(my_target)
# =========================================================

function(target_set_warnings target)

    set(GCC_CLANG_WARNINGS
        # --- Core Warnings ---
        -Wall
        -Wextra
        -Wpedantic

        # --- Shadow & Scope ---
        -Wshadow
        -Wnon-virtual-dtor

        # --- Conversions ---
        -Wold-style-cast
        -Wcast-align
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion

        # --- Format & Usage ---
        -Wformat=2
        -Wunused
        -Woverloaded-virtual
        -Wnull-dereference

        # --- Treat warnings as errors in CI (opt-in) ---
        -Werror
    )

    set(GCC_ONLY_WARNINGS
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
    )

    set(MSVC_WARNINGS
        /W4
        /permissive-
        /w14640   # thread-unsafe static initialization
        /w14826   # conversion from wider to narrower type
        /w14265   # class has virtual functions but destructor is not virtual
        /w14928   # illegal copy-initialization
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${target} PRIVATE ${GCC_CLANG_WARNINGS} ${GCC_ONLY_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE ${GCC_CLANG_WARNINGS})
    elseif(MSVC)
        target_compile_options(${target} PRIVATE ${MSVC_WARNINGS})
    else()
        message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}. No warnings set.")
    endif()

endfunction()
