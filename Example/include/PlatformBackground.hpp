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

#ifndef PLATFORM_BACKGROUND_HPP_
#define PLATFORM_BACKGROUND_HPP_

#include <xygine/components/Component.hpp>

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include <array>

namespace xy
{
    class TextureResource;
}

namespace Plat
{
    class Background final : public xy::Component, public sf::Drawable
    {
    public:
        Background(xy::MessageBus&, xy::TextureResource&);
        ~Background() = default;

        xy::Component::Type type() const override { return xy::Component::Type::Drawable; }
        void entityUpdate(float) override;
        sf::FloatRect globalBounds() const override { return{ {0.f, 0.f}, xy::DefaultSceneSize }; }

        void setAmbientColour(const sf::Color&);

    private:
        std::array<sf::Vertex, 4u> m_nearVertices;
        std::array<sf::Vertex, 4u> m_farVertices;

        sf::Texture* m_nearTexture;
        sf::Texture* m_farTexture;

        void draw(sf::RenderTarget&, sf::RenderStates) const override;
    };
}

#endif //PLATFORM_BACKGROUND_HPP_