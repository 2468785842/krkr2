set(COCOS_NETWORK_SRC "")
set(COCOS_NETWORK_HEADER
    network/CCDownloader-curl.h
    network/CCIDownloaderImpl.h
    network/CCDownloader.h
    network/Uri.h
    network/HttpClient.h
    network/HttpResponse.h
    network/HttpRequest.h
    network/HttpCookie.h
)

if(BUILD_EXT_WEBSOCKETS)
    list(APPEND COCOS_NETWORK_HEADER
        network/WebSocket.h
        network/SocketIO.h
    )
endif()

if(ANDROID)
    list(APPEND COCOS_NETWORK_HEADER network/CCDownloader-android.h)
    list(APPEND COCOS_NETWORK_SRC
        network/HttpClient-android.cpp
        network/CCDownloader.cpp
        network/CCDownloader-android.cpp
        network/Uri.cpp
    )
elseif(APPLE)
    list(APPEND COCOS_NETWORK_HEADER
        network/CCDownloader-apple.h
        network/HttpAsynConnection-apple.h
    )
    list(APPEND COCOS_NETWORK_SRC
        network/CCDownloader-apple.mm
        network/HttpClient.cpp
        network/CCDownloader.cpp
        network/CCDownloader-curl.cpp
        network/Uri.cpp
    )
else()
    list(APPEND COCOS_NETWORK_SRC
        network/HttpClient.cpp
        network/CCDownloader.cpp
        network/CCDownloader-curl.cpp
        network/Uri.cpp
    )
endif()

if(BUILD_EXT_WEBSOCKETS)
    list(APPEND COCOS_NETWORK_SRC
        network/SocketIO.cpp
        network/WebSocket.cpp
    )
endif()
