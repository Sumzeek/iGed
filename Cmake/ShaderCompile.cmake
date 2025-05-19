function(CompileToGLSL TARGET_NAME WORKING_DIR SHADERS GENERATED_DIR)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    set(SLANGC_EXECUTABLE "${WORKING_DIR}/slangc.exe")
    set(SHADER_COMPILE_PY "${WORKING_DIR}/ShaderCompile.py")

    # Create glsl folder
    if (NOT EXISTS "${GENERATED_DIR}/glsl")
        file(MAKE_DIRECTORY "${GENERATED_DIR}/glsl")
    endif ()

    set(SHADER_FILE_ARGS "")
    set(ALL_GENERATED_GLSL_FILES "")
    foreach (SHADER ${SHADERS})
        list(APPEND SHADER_FILE_ARGS ${SHADER})

        get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
        set(GLSL_FILE "${GENERATED_DIR}/glsl/${SHADER_NAME}.glsl")
        list(APPEND ALL_GENERATED_GLSL_FILES ${GLSL_FILE})
    endforeach ()

    add_custom_command(
            OUTPUT ${ALL_GENERATED_GLSL_FILES}
            COMMAND ${Python3_EXECUTABLE} ${SHADER_COMPILE_PY}
            ${SLANGC_EXECUTABLE}
            ${GENERATED_DIR}/glsl
            ${SHADER_FILE_ARGS}
            DEPENDS ${SHADERS}
            WORKING_DIRECTORY ${WORKING_DIR}
            COMMENT "Compiling shaders via Python script..."
            VERBATIM
    )

    add_custom_target(${TARGET_NAME}
            DEPENDS ${ALL_GENERATED_GLSL_FILES}
    )

    #    add_custom_target(${TARGET_NAME}
    #            COMMAND ${Python3_EXECUTABLE} ${SHADER_COMPILE_PY}
    #            ${SLANGC_EXECUTABLE}
    #            ${GENERATED_DIR}/glsl
    #            ${SHADER_FILE_ARGS}
    #            WORKING_DIRECTORY ${WORKING_DIR}
    #            COMMENT "Compiling shaders via Python script"
    #            VERBATIM
    #    )

endfunction()

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
#                COMMENT "Compiling & merging ${SHADER} -> ${GLSL_FILE}"
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