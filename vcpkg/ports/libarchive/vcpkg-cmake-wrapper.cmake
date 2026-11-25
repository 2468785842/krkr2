_find_package(${ARGS})

if(LibArchive_FOUND AND TARGET LibArchive::LibArchive)
    # FindLibArchive imports only the archive library. Keep every codec enabled
    # by this overlay on its imported target, including static link consumers.
    find_package(LibXml2 CONFIG REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(BZip2 REQUIRED)
    find_package(LibLZMA REQUIRED)
    find_package(lz4 CONFIG REQUIRED)
    find_package(zstd CONFIG REQUIRED)
    set_property(TARGET LibArchive::LibArchive APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES LibXml2::LibXml2 ZLIB::ZLIB BZip2::BZip2
        LibLZMA::LibLZMA lz4::lz4 zstd::libzstd)
    if(WIN32)
        # ENABLE_CNG is the Windows default (ZIP encryption and digest APIs).
        set_property(TARGET LibArchive::LibArchive APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES bcrypt)
    endif()
endif()
