# ImageOps only consumes core/imgproc. Leave upstream behavior intact for other
# consumers and reject incompatible feature combinations instead of dropping APIs.
if("imageops" IN_LIST FEATURES)
    set(_krkr2_extra_features ${FEATURES})
    list(REMOVE_ITEM _krkr2_extra_features core imageops openmp)
    if(_krkr2_extra_features)
        message(FATAL_ERROR
            "opencv4[imageops] only supports core/imgproc and optional openmp; "
            "incompatible features: ${_krkr2_extra_features}. "
            "Disable default features or remove imageops to build upstream modules.")
    endif()
    list(APPEND ADDITIONAL_BUILD_FLAGS "-DBUILD_LIST=core,imgproc")
endif()
