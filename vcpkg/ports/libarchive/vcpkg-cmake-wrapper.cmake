_find_package(${ARGS})

if(LibArchive_FOUND AND TARGET LibArchive::LibArchive)
    # This overlay enables libxml2 for XAR support. CMake's FindLibArchive only
    # imports archive.lib/libarchive.a and omits its static XML dependency.
    find_package(LibXml2 CONFIG REQUIRED)
    set_property(TARGET LibArchive::LibArchive APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES LibXml2::LibXml2)
endif()
