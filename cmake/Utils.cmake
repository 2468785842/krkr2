function(remove_interface_link_libraries targets_to_modify items_to_remove)
    foreach(target ${targets_to_modify})
        # 获取当前目标的 INTERFACE_LINK_LIBRARIES
        get_target_property(interface_link_libraries ${target} INTERFACE_LINK_LIBRARIES)
        
        if(NOT interface_link_libraries)
            message(WARNING "Target '${target}' has no INTERFACE_LINK_LIBRARIES property.")
            continue()
        endif()

        # 打印当前目标的库列表
        message(STATUS "Before removal for ${target}: ${interface_link_libraries}")

        # 移除指定项
        foreach(item ${items_to_remove})
            list(REMOVE_ITEM interface_link_libraries ${item})
        endforeach()

        # 打印修改后的库列表
        message(STATUS "After removal for ${target}: ${interface_link_libraries}")

        # 更新 INTERFACE_LINK_LIBRARIES 属性
        set_target_properties(${target} PROPERTIES INTERFACE_LINK_LIBRARIES "${interface_link_libraries}")
    endforeach()
endfunction()


function(find_static_lib_path target_name)
    macro(find_property_and_return target_name property_name)
        string(REGEX REPLACE ".*::" "" target "${target_name}")
        get_target_property(${target}-lib ${target_name} ${property_name})
        if(NOT ${target}-lib-NOTFOUND)
            return(PROPAGATE ${target}-lib)
        endif()
    endmacro()

    find_property_and_return(${target_name} LOCATION)
    find_property_and_return(${target_name} IMPORTED_LOCATION)
    find_property_and_return(${target_name} IMPORTED_LOCATION_DEBUG)
    find_property_and_return(${target_name} IMPORTED_LOCATION_RELEASE)
endfunction()
