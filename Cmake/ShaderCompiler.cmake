# Shader compilation function using DEPFILE for incremental builds
function(CompileShaders TARGET_NAME WORKING_DIR SHADERS OUTPUT_DIR PLATFORMS)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    set(SLANGC_EXECUTABLE "${WORKING_DIR}/slangc.exe")
    set(SHADER_COMPILER_PY "${WORKING_DIR}/SlangCompiler.py")

    set(ALL_SHADER_OUTPUTS "")

    # Build comma-separated platform list
    string(REPLACE ";" "," PLATFORMS_CSV "${PLATFORMS}")

    foreach (SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME_WE)

        # The .shader.json acts as the stamp file for this shader
        set(STAMP_FILE "${OUTPUT_DIR}/${SHADER_NAME}/${SHADER_NAME}.shader.json")
        set(DEPFILE "${CMAKE_CURRENT_BINARY_DIR}/deps/${SHADER_NAME}.d")

        add_custom_command(
                OUTPUT ${STAMP_FILE}
                COMMAND ${Python3_EXECUTABLE} ${SHADER_COMPILER_PY}
                ${SLANGC_EXECUTABLE}
                "${SHADER}"
                ${OUTPUT_DIR}
                -targets ${PLATFORMS_CSV}
                -depfile ${DEPFILE}
                DEPENDS ${SHADER}
                DEPFILE ${DEPFILE}
                WORKING_DIRECTORY ${WORKING_DIR}
                COMMENT "Compiling shader: ${SHADER_NAME} (${PLATFORMS_CSV})"
                VERBATIM
        )

        list(APPEND ALL_SHADER_OUTPUTS ${STAMP_FILE})
    endforeach ()

    add_custom_target(${TARGET_NAME} ALL
            DEPENDS ${ALL_SHADER_OUTPUTS}
    )
endfunction()
