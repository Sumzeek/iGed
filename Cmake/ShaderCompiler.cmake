# Core shader compilation function
function(_CompileShaders TARGET_NAME WORKING_DIR SHADERS GENERATED_DIR COPY_DIR TARGET_PLATFORM)
    set(SLANGC_EXECUTABLE "${WORKING_DIR}/slangc.exe")
    set(SPIRVCROSS_EXECUTABLE "${WORKING_DIR}/spirv-cross.exe")
    set(SHADER_COMPILER_PY "${WORKING_DIR}/SlangCompiler.py")

    set(ALL_GENERATED_FILES "")
    foreach (SHADER ${SHADERS})
        # Read file contents, If any line starts with `module`, skip this file
        file(READ "${SHADER}" SHADER_CONTENTS)
        string(REGEX MATCH "^[ \t]*module[ \t(]" MODULE_MATCH "${SHADER_CONTENTS}")
        if (MODULE_MATCH)
            continue()
        endif ()

        # Create compiled json dependency
        get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
        set(JSON_OUTPUT "${GENERATED_DIR}/${TARGET_PLATFORM}/${SHADER_NAME}.json")
        add_custom_command(
                OUTPUT ${JSON_OUTPUT}
                COMMAND ${Python3_EXECUTABLE} ${SHADER_COMPILER_PY}
                ${SLANGC_EXECUTABLE}
                ${SPIRVCROSS_EXECUTABLE}
                ${SHADER}
                ${GENERATED_DIR}
                ${COPY_DIR}
                -target ${TARGET_PLATFORM}
                DEPENDS ${SHADER}
                WORKING_DIRECTORY ${WORKING_DIR}
                COMMENT "Compiling ${SHADER_NAME} to ${TARGET_PLATFORM}"
                VERBATIM
        )

        list(APPEND ALL_GENERATED_FILES ${JSON_OUTPUT})

    endforeach ()

    add_custom_target(${TARGET_NAME}
            DEPENDS ${ALL_GENERATED_FILES}
    )
endfunction()

# Multi-target compilation function
function(CompileToPlatforms TARGET_NAME WORKING_DIR SHADERS GENERATED_DIR COPY_DIR)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(ALL_TARGETS "")

    # Compile to hlsl, spv
    set(PLATFORMS "hlsl;spv")
    foreach (PLATFORM IN LISTS PLATFORMS)
        set(PLATFORM_TARGET "${TARGET_NAME}_${PLATFORM}")
        _CompileShaders(${PLATFORM_TARGET} ${WORKING_DIR} "${SHADERS}" ${GENERATED_DIR} ${COPY_DIR} ${PLATFORM})
        list(APPEND ALL_TARGETS ${PLATFORM_TARGET})
    endforeach ()

    # Make sure to create glsl AFTER spv so dependency makes sense
    _CompileShaders("${TARGET_NAME}_glsl" ${WORKING_DIR} "${SHADERS}" ${GENERATED_DIR} ${COPY_DIR} "glsl")
    add_dependencies("${TARGET_NAME}_glsl" "${TARGET_NAME}_spv")
    list(APPEND ALL_TARGETS "${TARGET_NAME}_glsl")

    # Final target
    add_custom_target(${TARGET_NAME}
            DEPENDS ${ALL_TARGETS}
    )
endfunction()

#function(CompileToGLSL TARGET_NAME WORKING_DIR SHADERS GENERATED_DIR)
#    find_package(Python3 REQUIRED COMPONENTS Interpreter)
#
#    set(SLANGC_EXECUTABLE "${WORKING_DIR}/slangc.exe")
#    set(SHADER_COMPILE_PY "${WORKING_DIR}/ShaderCompile.py")
#
#    # Create glsl folder
#    if (NOT EXISTS "${GENERATED_DIR}/glsl")
#        file(MAKE_DIRECTORY "${GENERATED_DIR}/glsl")
#    endif ()
#
#    set(ALL_GENERATED_GLSL_FILES "")
#
#    foreach (SHADER ${SHADERS})
#        get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
#
#        set(POTENTIAL_OUTPUTS
#                "${GENERATED_DIR}/glsl/${SHADER_NAME}.vs"
#                "${GENERATED_DIR}/glsl/${SHADER_NAME}.tcs"
#                "${GENERATED_DIR}/glsl/${SHADER_NAME}.tes"
#                "${GENERATED_DIR}/glsl/${SHADER_NAME}.gs"
#                "${GENERATED_DIR}/glsl/${SHADER_NAME}.fs"
#                "${GENERATED_DIR}/glsl/${SHADER_NAME}.cs"
#        )
#
#        add_custom_command(
#                OUTPUT ${POTENTIAL_OUTPUTS}
#                COMMAND ${Python3_EXECUTABLE} ${SHADER_COMPILE_PY}
#                ${SLANGC_EXECUTABLE}
#                ${SHADER}
#                ${GENERATED_DIR}/glsl
#                -target glsl
#                DEPENDS ${SHADER}
#                WORKING_DIRECTORY ${WORKING_DIR}
#                VERBATIM
#        )
#
#        list(APPEND ALL_GENERATED_GLSL_FILES ${POTENTIAL_OUTPUTS})
#
#    endforeach ()
#
#    add_custom_target(${TARGET_NAME}
#            DEPENDS ${ALL_GENERATED_GLSL_FILES}
#    )
#
#endfunction()

#function(CompileToGLSL TARGET_NAME WORKING_DIR SHADERS GENERATED_DIR)
#    set(SLANGC_EXECUTABLE "${WORKING_DIR}/slangc.exe")
#
#    set(ALL_GENERATED_GLSL_FILES "")
#
#    if (UNIX)
#        execute_process(COMMAND chmod a+x ${SLANGC_EXECUTABLE})
#    endif ()
#
#    # Create glsl folder
#    if (NOT EXISTS "${GENERATED_DIR}/glsl")
#        file(MAKE_DIRECTORY "${GENERATED_DIR}/glsl")
#    endif ()
#
#    foreach (SHADER ${SHADERS})
#        get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
#
#        set(VERT_FILE "${GENERATED_DIR}/glsl/${SHADER_NAME}.vert")
#        set(FRAG_FILE "${GENERATED_DIR}/glsl/${SHADER_NAME}.frag")
#        set(GLSL_FILE "${GENERATED_DIR}/glsl/${SHADER_NAME}.glsl")
#        add_custom_command(
#                OUTPUT ${GLSL_FILE}
#                COMMAND ${SLANGC_EXECUTABLE} ${SHADER}
#                -profile glsl_450 -target glsl -entry vsMain -o ${VERT_FILE}
#                COMMAND ${SLANGC_EXECUTABLE} ${SHADER}
#                -profile glsl_450 -target glsl -entry psMain -o ${FRAG_FILE}
#                COMMAND ${CMAKE_COMMAND} -E echo "#type vertex" > ${GLSL_FILE}
#                COMMAND ${CMAKE_COMMAND} -E cat ${VERT_FILE} >> ${GLSL_FILE}
#                COMMAND ${CMAKE_COMMAND} -E echo "\n#type fragment" >> ${GLSL_FILE}
#                COMMAND ${CMAKE_COMMAND} -E cat ${FRAG_FILE} >> ${GLSL_FILE}
#                # Delete intermediate files
#                COMMAND ${CMAKE_COMMAND} -E rm -f ${VERT_FILE}
#                COMMAND ${CMAKE_COMMAND} -E rm -f ${FRAG_FILE}
#                DEPENDS ${SHADER}
#                WORKING_DIRECTORY "${WORKING_DIR}"
#                COMMENT "Compiling & merging: ${SHADER} -> ${GLSL_FILE}"
#                VERBATIM
#        )
#
#        list(APPEND ALL_GENERATED_GLSL_FILES ${GLSL_FILE})
#
#    endforeach ()
#
#    add_custom_target(${TARGET_NAME}
#            DEPENDS ${ALL_GENERATED_GLSL_FILES} SOURCES ${SHADERS})
#
#endfunction()
