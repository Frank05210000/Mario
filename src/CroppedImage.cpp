#include "CroppedImage.hpp"

#include <filesystem>

#include "Core/TextureUtils.hpp"
#include "Util/Logger.hpp"

#include "Util/MissingTexture.hpp"

namespace {
std::filesystem::path GetShaderPath(const std::string& filename) {
    return std::filesystem::path(RESOURCE_DIR).parent_path() / "PTSD" / "assets" / "shaders" / filename;
}
}

std::shared_ptr<SDL_Surface> CroppedImage::LoadSurface(const std::string& filepath) {
    auto surface = std::shared_ptr<SDL_Surface>(IMG_Load(filepath.c_str()), SDL_FreeSurface);
    if (surface == nullptr) {
        surface = {GetMissingTextureSDLSurface(), SDL_FreeSurface};
        LOG_ERROR("Failed to load image: '{}'", filepath);
        LOG_ERROR("{}", IMG_GetError());
    }
    return surface;
}

std::shared_ptr<SDL_Surface> CroppedImage::CropSurface(
    const std::shared_ptr<SDL_Surface>& source,
    const SDL_Rect& cropRect) {
    auto* rawSurface = SDL_CreateRGBSurfaceWithFormat(
        0,
        cropRect.w,
        cropRect.h,
        source->format->BitsPerPixel,
        source->format->format);
    auto cropped = std::shared_ptr<SDL_Surface>(rawSurface, SDL_FreeSurface);

    if (cropped == nullptr) {
        throw std::runtime_error("Failed to create cropped SDL surface");
    }

    if (SDL_BlitSurface(source.get(), &cropRect, cropped.get(), nullptr) != 0) {
        throw std::runtime_error("Failed to crop SDL surface");
    }

    return cropped;
}

CroppedImage::CroppedImage(const std::string& filepath, const SDL_Rect& cropRect) {
    if (s_Program == nullptr) {
        InitProgram();
    }
    if (s_VertexArray == nullptr) {
        InitVertexArray();
    }

    m_UniformBuffer = std::make_unique<Core::UniformBuffer<Core::Matrices>>(
        *s_Program,
        "Matrices",
        0);

    auto sourceSurface = LoadSurface(filepath);
    auto croppedSurface = CropSurface(sourceSurface, cropRect);

    m_Texture = std::make_unique<Core::Texture>(
        Core::SdlFormatToGlFormat(croppedSurface->format->format),
        croppedSurface->w,
        croppedSurface->h,
        croppedSurface->pixels);
    m_Size = {static_cast<float>(croppedSurface->w), static_cast<float>(croppedSurface->h)};
}

void CroppedImage::Draw(const Core::Matrices& data) {
    m_UniformBuffer->SetData(0, data);

    m_Texture->Bind(kUniformSurfaceLocation);
    s_Program->Bind();
    s_Program->Validate();

    s_VertexArray->Bind();
    s_VertexArray->DrawTriangles();
}

void CroppedImage::InitProgram() {
    s_Program = std::make_unique<Core::Program>(
        GetShaderPath("Base.vert").string(),
        GetShaderPath("Base.frag").string());
    s_Program->Bind();

    const GLint location = glGetUniformLocation(s_Program->GetId(), "surface");
    glUniform1i(location, kUniformSurfaceLocation);
}

void CroppedImage::InitVertexArray() {
    s_VertexArray = std::make_unique<Core::VertexArray>();

    s_VertexArray->AddVertexBuffer(std::make_unique<Core::VertexBuffer>(
        std::vector<float> {
            -0.5F, 0.5F,
            -0.5F, -0.5F,
            0.5F, -0.5F,
            0.5F, 0.5F,
        },
        2));

    s_VertexArray->AddVertexBuffer(std::make_unique<Core::VertexBuffer>(
        std::vector<float> {
            0.0F, 0.0F,
            0.0F, 1.0F,
            1.0F, 1.0F,
            1.0F, 0.0F,
        },
        2));

    s_VertexArray->SetIndexBuffer(
        std::make_unique<Core::IndexBuffer>(std::vector<unsigned int> {
            0, 1, 2,
            0, 2, 3,
        }));
}

std::unique_ptr<Core::Program> CroppedImage::s_Program = nullptr;
std::unique_ptr<Core::VertexArray> CroppedImage::s_VertexArray = nullptr;
