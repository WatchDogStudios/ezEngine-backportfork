set(EZ_D3D12_AGILITY_SDK_DIR "EZ_D3D12_AGILITY_SDK_DIR-NOTFOUND" CACHE PATH "Directory of D3D12 Agility SDK")
set(EZ_D3D12_AGILITY_SDK_INCLUDE_DIR "EZ_D3D12_AGILITY_SDK_INCLUDE_DIR-NOTFOUND" CACHE PATH "Directory of D3D12 Agliity SDK Includes")
set(EZ_BUILD_EXPERIMENTAL_D3D12_SUPPORT ON CACHE BOOL "Add support for D3D12. PC/Xbox")

mark_as_advanced(FORCE EZ_D3D12_AGILITY_SDK_DIR)
mark_as_advanced(FORCE EZ_D3D12_AGILITY_SDK_INCLUDE_DIR)
mark_as_advanced(FORCE EZ_BUILD_EXPERIMENTAL_D3D12_SUPPORT)

# NOTE: This shouldnt be fixed Aglity Version. Find a way to "adjust this."
set(EZ_D3D12_AGILITY_SDK_PACKAGE_PATH "${CMAKE_BINARY_DIR}/packages/Microsoft.Direct3D.D3D12.1.711.3/build/native")
set(EZ_D3D12_AGILITY_SDK_PACKAGE_PATH_INCLUDE "${EZ_D3D12_AGILITY_SDK_PACKAGE_PATH}/include")

macro(ez_requires_d3d12)
    ez_requires_d3d()
    ez_requires(EZ_BUILD_EXPERIMENTAL_D3D12_SUPPORT)
endmacro()

function(ez_export_target_dx12 TARGET_NAME)
    
    ez_requires_d3d12()
    # Install D3D12 Aglity SDK for the latest sdk.
    ez_nuget_init()
    
    message("D3D12 SDK DLL PATH: ${CMAKE_SOURCE_DIR}/${EZ_CMAKE_RELPATH_CODE}/ThirdParty/D3D12SDK")
    # Path where d3d12 dlls will end up on build.
    # NOTE: Should we allow the user to change this?
    set(EZ_D3D12_RESOURCES ${CMAKE_BINARY_DIR}/x64)

    if(NOT EXISTS ${EZ_D3D12_RESOURCES})
        file(MAKE_DIRECTORY ${EZ_D3D12_RESOURCES})
    endif()

    execute_process(COMMAND ${NUGET} restore ${CMAKE_SOURCE_DIR}/${EZ_CMAKE_RELPATH}/CMakeUtils/packages.config -PackagesDirectory ${CMAKE_BINARY_DIR}/packages
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})


    add_custom_command(TARGET ${TARGET_NAME}
	PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EZ_D3D12_AGILITY_SDK_PACKAGE_PATH}/bin/x64/D3D12Core.dll ${EZ_D3D12_RESOURCES}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EZ_D3D12_AGILITY_SDK_PACKAGE_PATH}/bin/x64/d3d12SDKLayers.dll ${EZ_D3D12_RESOURCES}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    target_include_directories(${PROJECT_NAME} PUBLIC ${EZ_D3D12_AGILITY_SDK_PACKAGE_PATH})
endfunction()

# #####################################
# ## ez_link_target_dx12(<target>)
# #####################################
function(ez_link_target_dx12 TARGET_NAME)
    # ez_export_target_dx12(${TARGET_NAME})
    target_link_libraries(${TARGET_NAME}
    PRIVATE
    RendererDX12
    )
endfunction()