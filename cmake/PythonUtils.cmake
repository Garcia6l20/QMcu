if (NOT Python_EXECUTABLE)
    find_package (Python COMPONENTS Interpreter)
    set(Python_EXECUTABLE ${Python_EXECUTABLE} CACHE INTERNAL "Python interpreter")
    message(STATUS "using python interpreter: ${Python_EXECUTABLE}")
endif()

function(add_python_generator target)
    
    set(options)
    set(singleValueArgs SCRIPT MODULE)
    set(multiValueArgs OUTPUT ARGS DEPENDS PYTHONPATH)
    cmake_parse_arguments(ARGS "${options}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ARGS_OUTPUT)
        message(FATAL_ERROR "OUTPUT argument missing")
    endif()

    if (NOT ARGS_SCRIPT AND NOT ARGS_MODULE)
        message(FATAL_ERROR "SCRIPT/MODULE argument missing")
    endif()

    if (NOT ARGS_PYTHONPATH)
        set(ARGS_PYTHONPATH ${PROJECT_ROOT})
    endif()

    list(JOIN ARGS_PYTHONPATH ":" ARGS_PYTHONPATH)

    list(APPEND ARGS_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${ARGS_SCRIPT})

    set(COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${ARGS_PYTHONPATH}
        ${Python_EXECUTABLE})
    
    if (ARGS_SCRIPT)
        set(COMMAND ${COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/${ARGS_SCRIPT})
    else()
        set(COMMAND ${COMMAND} -m ${ARGS_MODULE})
    endif()

    add_custom_command(
        OUTPUT ${ARGS_OUTPUT}
        COMMAND ${COMMAND}
        ARGS ${ARGS_ARGS}
        DEPENDS ${ARGS_DEPENDS}
        COMMENT "Generating ${ARGS_OUTPUT}"
    )

    add_custom_target(${target} ALL DEPENDS ${ARGS_OUTPUT})

endfunction()

#
# simple wrapper around execute_process for python code command
#
macro(python_execute command)

    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c ${command}
        ${ARGN}
    )

endmacro()
