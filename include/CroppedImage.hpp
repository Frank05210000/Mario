#ifndef CROPPED_IMAGE_HPP
#define CROPPED_IMAGE_HPP

#include "Core/Drawable.hpp"
#include "Core/Program.hpp"
#include "Core/Texture.hpp"
#include "Core/UniformBuffer.hpp"
#include "Core/VertexArray.hpp"

class CroppedImage final : public Core::Drawable {
public:
    CroppedImage(const std::string& filepath, const SDL_Rect& cropRect);

    void Draw(const Core::Matrices& data) override;
    glm::vec2 GetSize() const override { return m_Size; }

private:
    static std::shared_ptr<SDL_Surface> LoadSurface(const std::string& filepath);
    static std::shared_ptr<SDL_Surface> CropSurface(const std::shared_ptr<SDL_Surface>& source,
                                                    const SDL_Rect& cropRect);
    static void InitProgram();
    static void InitVertexArray();

    static constexpr int kUniformSurfaceLocation = 0;

    static std::unique_ptr<Core::Program> s_Program;
    static std::unique_ptr<Core::VertexArray> s_VertexArray;

    std::unique_ptr<Core::UniformBuffer<Core::Matrices>> m_UniformBuffer;
    std::unique_ptr<Core::Texture> m_Texture;
    glm::vec2 m_Size {};
};

#endif
