if(NOT DEFINED PTSD_SOURCE_DIR)
    message(FATAL_ERROR "PTSD_SOURCE_DIR is required")
endif()

set(IMAGE_SOURCE "${PTSD_SOURCE_DIR}/src/Util/Image.cpp")
file(READ "${IMAGE_SOURCE}" IMAGE_CONTENT)

set(ORIGINAL_LOAD_SURFACE [=[
std::shared_ptr<SDL_Surface> LoadSurface(const std::string &filepath) {
    auto surface = std::shared_ptr<SDL_Surface>(IMG_Load(filepath.c_str()),
                                                SDL_FreeSurface);

    if (surface == nullptr) {
        surface = {GetMissingTextureSDLSurface(), SDL_FreeSurface};
        LOG_ERROR("Failed to load image: '{}'", filepath);
        LOG_ERROR("{}", IMG_GetError());
    }

    return surface;
}
]=])

set(PATCHED_LOAD_SURFACE [=[
std::shared_ptr<SDL_Surface> LoadSurface(const std::string &filepath) {
    auto rawSurface = std::shared_ptr<SDL_Surface>(IMG_Load(filepath.c_str()),
                                                   SDL_FreeSurface);

    if (rawSurface == nullptr) {
        rawSurface = {GetMissingTextureSDLSurface(), SDL_FreeSurface};
        LOG_ERROR("Failed to load image: '{}'", filepath);
        LOG_ERROR("{}", IMG_GetError());
    }

    auto convertedSurface = std::shared_ptr<SDL_Surface>(
        SDL_ConvertSurfaceFormat(rawSurface.get(), SDL_PIXELFORMAT_ABGR8888, 0),
        SDL_FreeSurface);

    if (convertedSurface == nullptr) {
        LOG_ERROR("Failed to convert image surface to ABGR8888: '{}'", filepath);
        LOG_ERROR("{}", SDL_GetError());
        return rawSurface;
    }

    return convertedSurface;
}
]=])

string(FIND "${IMAGE_CONTENT}" "${ORIGINAL_LOAD_SURFACE}" ORIGINAL_POSITION)
if(ORIGINAL_POSITION EQUAL -1)
    string(FIND "${IMAGE_CONTENT}" "${PATCHED_LOAD_SURFACE}" PATCHED_POSITION)
    if(PATCHED_POSITION EQUAL -1)
        message(FATAL_ERROR
            "PTSD Image.cpp no longer matches v0.2; review the local image-format patch"
        )
    endif()
    return()
endif()

string(REPLACE
    "${ORIGINAL_LOAD_SURFACE}"
    "${PATCHED_LOAD_SURFACE}"
    IMAGE_CONTENT
    "${IMAGE_CONTENT}"
)
file(WRITE "${IMAGE_SOURCE}" "${IMAGE_CONTENT}")

# 像素風素材改用 GL_NEAREST 取樣，避免透明邊緣（RGB 為黑、alpha=0）被雙線性
# 內插混進角色輪廓而產生黑邊；CLAMP_TO_EDGE 避免邊緣取到對側像素。
set(TEXTURE_SOURCE "${PTSD_SOURCE_DIR}/src/Core/Texture.cpp")
file(READ "${TEXTURE_SOURCE}" TEXTURE_CONTENT)

set(ORIGINAL_TEXTURE_FILTER [=[
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
]=])

set(PATCHED_TEXTURE_FILTER [=[
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
]=])

string(FIND "${TEXTURE_CONTENT}" "${ORIGINAL_TEXTURE_FILTER}" TEXTURE_ORIGINAL_POSITION)
if(TEXTURE_ORIGINAL_POSITION EQUAL -1)
    string(FIND "${TEXTURE_CONTENT}" "${PATCHED_TEXTURE_FILTER}" TEXTURE_PATCHED_POSITION)
    if(TEXTURE_PATCHED_POSITION EQUAL -1)
        message(FATAL_ERROR
            "PTSD Texture.cpp no longer matches v0.2; review the local texture-filter patch"
        )
    endif()
    return()
endif()

string(REPLACE
    "${ORIGINAL_TEXTURE_FILTER}"
    "${PATCHED_TEXTURE_FILTER}"
    TEXTURE_CONTENT
    "${TEXTURE_CONTENT}"
)
file(WRITE "${TEXTURE_SOURCE}" "${TEXTURE_CONTENT}")
