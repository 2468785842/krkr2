set(MOVIE_PATH ${CMAKE_CURRENT_LIST_DIR})
set(MOVIE_SOURCE_FILES
    ${MOVIE_PATH}/krmovie.cpp

    ${MOVIE_PATH}/ffmpeg/AEChannelInfo.cpp
    ${MOVIE_PATH}/ffmpeg/AEFactory.cpp
    ${MOVIE_PATH}/ffmpeg/AEStreamInfo.cpp
    ${MOVIE_PATH}/ffmpeg/AEUtil.cpp
    ${MOVIE_PATH}/ffmpeg/AudioCodecFFmpeg.cpp
    ${MOVIE_PATH}/ffmpeg/AudioCodecPassthrough.cpp
    ${MOVIE_PATH}/ffmpeg/AudioDevice.cpp
    ${MOVIE_PATH}/ffmpeg/BaseRenderer.cpp
    ${MOVIE_PATH}/ffmpeg/BitstreamStats.cpp
    ${MOVIE_PATH}/ffmpeg/Clock.cpp
    ${MOVIE_PATH}/ffmpeg/CodecUtils.cpp
    ${MOVIE_PATH}/ffmpeg/Demux.cpp
    ${MOVIE_PATH}/ffmpeg/DemuxFFmpeg.cpp
    ${MOVIE_PATH}/ffmpeg/DemuxPacket.cpp
    ${MOVIE_PATH}/ffmpeg/FactoryCodec.cpp
    ${MOVIE_PATH}/ffmpeg/InputStream.cpp
    ${MOVIE_PATH}/ffmpeg/krffmpeg.cpp
    ${MOVIE_PATH}/ffmpeg/KRMovieLayer.cpp
    ${MOVIE_PATH}/ffmpeg/KRMoviePlayer.cpp
    ${MOVIE_PATH}/ffmpeg/Message.cpp
    ${MOVIE_PATH}/ffmpeg/MessageQueue.cpp
    ${MOVIE_PATH}/ffmpeg/ProcessInfo.cpp
    ${MOVIE_PATH}/ffmpeg/RenderFlags.cpp
    ${MOVIE_PATH}/ffmpeg/StreamInfo.cpp
    ${MOVIE_PATH}/ffmpeg/Thread.cpp
    ${MOVIE_PATH}/ffmpeg/Timer.cpp
    ${MOVIE_PATH}/ffmpeg/TimeUtils.cpp
    ${MOVIE_PATH}/ffmpeg/VideoCodec.cpp
    ${MOVIE_PATH}/ffmpeg/VideoCodecFFmpeg.cpp
    ${MOVIE_PATH}/ffmpeg/VideoPlayer.cpp
    ${MOVIE_PATH}/ffmpeg/VideoPlayerAudio.cpp
    ${MOVIE_PATH}/ffmpeg/VideoPlayerVideo.cpp
    ${MOVIE_PATH}/ffmpeg/VideoReferenceClock.cpp
    ${MOVIE_PATH}/ffmpeg/VideoRenderer.cpp

    PARENT_SCOPE
)

set(MOVIE_HEADERS_DIR
    ${MOVIE_PATH}/
    ${MOVIE_PATH}/ffmpeg
    PARENT_SCOPE
)