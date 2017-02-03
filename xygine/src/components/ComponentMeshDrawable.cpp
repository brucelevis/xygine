/*********************************************************************
� Matt Marchant 2014 - 2017
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

#include <xygine/components/MeshDrawable.hpp>
#include <xygine/Entity.hpp>
#include <xygine/Scene.hpp>

using namespace xy;

MeshDrawable::MeshDrawable(MessageBus& mb, MeshRenderer& mr, const MeshRenderer::Lock&)
    : Component     (mb, this),
    m_meshRenderer  (mr),
    m_waterShader   (nullptr)
{
    auto size = mr.m_gBuffer.getSize();

    m_renderTexture.create(size.x, size.y, true);
    m_sprite.setTexture(m_renderTexture.getTexture());
    m_sprite.setOrigin(sf::Vector2f(size) / 2.f);
    m_sprite.setScale(xy::DefaultSceneSize.x / size.x, xy::DefaultSceneSize.y / size.y);
    mr.setView(sf::View({}, xy::DefaultSceneSize));

    xy::Component::MessageHandler mh;
    mh.id = xy::Message::UIMessage;
    mh.action = [this, &mr](xy::Component*, const xy::Message& msg)
    {
        const auto& data = msg.getData<xy::Message::UIEvent>();
        switch (data.type)
        {
        default: break;
        case xy::Message::UIEvent::ResizedWindow:
        {
            std::int32_t resolution = static_cast<std::int32_t>(data.value);
            auto width = resolution >> 16;
            auto height = resolution & 0xffff;

            if (width > height)
            {
                height = (width / 16) * 9;
            }
            else
            {
                width = (height / 9) * 16;
            }
            m_renderTexture.create(static_cast<unsigned>(width), static_cast<unsigned>(height), true);
            //remember to reset the sprite's sub-rect!
            m_sprite.setTexture(m_renderTexture.getTexture(), true);
            m_sprite.setOrigin(width / 2.f, height / 2.f);
            m_sprite.setScale(xy::DefaultSceneSize.x / width, xy::DefaultSceneSize.y / height);
        }
            break;
        }
    };
    addMessageHandler(mh);
}

//public
void MeshDrawable::entityUpdate(float)
{
    XY_ASSERT(getEntity(), "entity null");
    Entity& e = *getEntity();
    m_sprite.setPosition(e.getScene()->getView().getCenter());
}

void MeshDrawable::enableWater(bool enabled)
{
    m_waterShader = (enabled) ? &m_meshRenderer.m_waterShader : nullptr;

    if (enabled)
    {
        m_waterShader->setUniform("u_diffuseMap", m_renderTexture.getTexture());
    }
}

void MeshDrawable::setWaterLevel(float level)
{
    enableWater(true);
    m_waterShader->setUniform("u_waterLevel", level);
}

void MeshDrawable::setWaterColour(const sf::Color& colour)
{
    enableWater(true);
    m_waterShader->setUniform("u_waterColour", sf::Glsl::Vec4(colour));
}

void MeshDrawable::setWaterLightInfluence(float amount)
{
    XY_ASSERT(amount >= 0.f && amount <= 1.f, "value must be 0.0 - 1.0");
    enableWater(true);
    m_waterShader->setUniform("u_lightInfluence", amount);
}

//private
void MeshDrawable::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    m_renderTexture.clear(sf::Color::Transparent);
    m_renderTexture.draw(m_meshRenderer);
    m_renderTexture.display();

    states.shader = m_waterShader;

    rt.draw(m_sprite, states);
}