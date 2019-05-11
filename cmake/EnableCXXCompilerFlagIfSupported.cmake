# The code in this file was taken with minimal changes from a StackOverflow answer by Quantumboredom:
# https://stackoverflow.com/a/33266748/1051764
include(CheckCXXCompilerFlag)

function(enable_cxx_compiler_flag_if_supported FLAG)
    string(FIND "${CMAKE_CXX_FLAGS}" "${FLAG}" FLAG_ALREADY_SET)
    if(FLAG_ALREADY_SET EQUAL -1)
        check_cxx_compiler_flag("${FLAG}" CXX_COMPILER_SUPPORTS_FLAG_${FLAG})
        if(CXX_COMPILER_SUPPORTS_FLAG_${FLAG})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" PARENT_SCOPE)
        endif()
        #unset("CXX_COMPILER_SUPPORTS_FLAG_${FLAG}" CACHE)
    endif()
endfunction()
