function(CompileToGLSL TARGET_NAME WORKING_DIR SHADERS GENERATED_DIR)
    set(SLANGC_EXECUTABLE "${WORKING_DIR}/slangc.exe")

    set(ALL_GENERATED_GLSL_FILES "")

    if (UNIX)
        execute_process(COMMAND chmod a+x ${SLANGC_EXECUTABLE})
    endif ()

    # Create glsl folder
    if (NOT EXISTS "${GENERATED_DIR}/glsl")
        file(MAKE_DIRECTORY "${GENERATED_DIR}/glsl")
    endif ()

    foreach (SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)

        set(VERT_FILE "${GENERATED_DIR}/glsl/${SHADER_NAME}.vert")
        set(FRAG_FILE "${GENERATED_DIR}/glsl/${SHADER_NAME}.frag")

        add_custom_command(
                OUTPUT ${VERT_FILE}
                COMMAND ${SLANGC_EXECUTABLE} ${SHADER}
                -profile glsl_450
                -target glsl
                -entry vsMain
                -o ${VERT_FILE}
                WORKING_DIRECTORY "${WORKING_DIR}"
                DEPENDS ${SHADER}
                COMMENT "Compiling shader: ${SHADER} -> ${VERT_FILE}"
                VERBATIM
        )

        add_custom_command(
                OUTPUT ${FRAG_FILE}
                COMMAND ${SLANGC_EXECUTABLE} ${SHADER}
                -profile glsl_450
                -target glsl
                -entry psMain
                -o ${FRAG_FILE}
                WORKING_DIRECTORY "${WORKING_DIR}"
                DEPENDS ${SHADER}
                COMMENT "Compiling shader: ${SHADER} -> ${VERT_FILE}"
                VERBATIM
        )

        # message(WARNING "Compiling shader: ${SHADER} -> ${VERT_FILE}")

        list(APPEND ALL_GENERATED_GLSL_FILES ${VERT_FILE})
        list(APPEND ALL_GENERATED_GLSL_FILES ${FRAG_FILE})

    endforeach ()

    add_custom_target(${TARGET_NAME}
            DEPENDS ${ALL_GENERATED_GLSL_FILES} SOURCES ${SHADERS})

endfunction()