/*********************************************************************
Matt Marchant 2014 - 2017
http://trederia.blogspot.com

xygine - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#ifndef XYT_ATLAS_HPP_
#define XYT_ATLAS_HPP_

#include <xygine/components/Component.hpp>

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace xy
{
    class TextureResource;
}

class AtlasWindow final : public xy::Component, public sf::Drawable
{
public:
    AtlasWindow(xy::MessageBus&, xy::TextureResource&);
    ~AtlasWindow() = default;

    xy::Component::Type type() const override { return xy::Component::Type::Drawable; }
    void entityUpdate(float) override;

    void setSpriteSheet(const std::string&);

    const sf::Texture& getTexture() const { return m_renderTexture.getTexture(); }

private:
    xy::TextureResource& m_textureResource;

    sf::RenderTexture m_renderTexture;
    sf::Sprite m_previewSprite;
    
    void draw(sf::RenderTarget&, sf::RenderStates) const override;
};

#endif //XYT_ATLAS_HPP_