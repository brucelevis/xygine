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

#include <xygine/ui/Container.hpp>
#include <xygine/MessageBus.hpp>
#include <xygine/util/Position.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

using namespace xy;
using namespace UI;

namespace
{
    const float deadzone = 40.f;
}

Container::Container(xy::MessageBus& mb)
    : m_messageBus  (mb),
    m_selectedIndex (0),
    m_background    (xy::DefaultSceneSize)
{
    m_background.setFillColor(sf::Color::Transparent);
}

//public
void Container::addControl(Control::Ptr c)
{    
    m_controls.push_back(c);

    if (!hasSelection() && c->selectable())
        select(m_controls.size() - 1);
}

bool Container::selectable() const
{
    return false;
}

void Container::handleEvent(const sf::Event& e, const sf::Vector2f& mousePos)
{
    if (!visible()) return;
    
    auto mPos = getTransform().getInverse().transformPoint(mousePos);

    //pass event to selected control
    if (hasSelection() && m_controls[m_selectedIndex]->active())
    {
        m_controls[m_selectedIndex]->handleEvent(e, mPos);
    }
    //keyboard input
    else if (e.type == sf::Event::KeyReleased)
    {
        if (e.key.code == sf::Keyboard::Up
            || e.key.code == sf::Keyboard::Left)
        {
            selectPrevious();
        }
        else if (e.key.code == sf::Keyboard::Down
            || e.key.code == sf::Keyboard::Right)
        {
            selectNext();
        }
        else if (e.key.code == sf::Keyboard::Return/* ||
            e.key.code == sf::Keyboard::Space*/)
        {
            if (hasSelection())
            {
                m_controls[m_selectedIndex]->activate();

                auto msg = m_messageBus.post<Message::UIEvent>(Message::UIMessage);
                msg->type = Message::UIEvent::SelectionActivated;
            }
        }
    }
    //controller navigation
    else if (e.type == sf::Event::JoystickMoved)
    {
        if (e.joystickMove.axis == sf::Joystick::Axis::PovY)
        {
            if (e.joystickMove.position > deadzone)
            {
                selectPrevious();
            }
            else if (e.joystickMove.position < -deadzone)
            {
                selectNext();
            }
        }//axis is inverse so needs its own branch :/
        else if (e.joystickMove.axis == sf::Joystick::Axis::PovX)
        {
            if (e.joystickMove.position > deadzone)
            {
                selectNext();
            }
            else if (e.joystickMove.position < -deadzone)
            {
                selectPrevious();
            }
        }
    }
    //controller button
    else if (e.type == sf::Event::JoystickButtonReleased)
    {
        //TODO check button mapping (A and START on xbawx)
        if (e.joystickButton.button == 0 || e.joystickButton.button == 7)
        {
            if (hasSelection())
            {
                m_controls[m_selectedIndex]->activate();

                auto msg = m_messageBus.post<Message::UIEvent>(Message::UIMessage);
                msg->type = Message::UIEvent::SelectionActivated;
            }
        }
    }
    //mouse input
    else if (e.type == sf::Event::MouseMoved)
    {
        for (auto i = 0u; i < m_controls.size(); ++i)
        {
            if (m_controls[i]->contains(mPos))
            {
                if (m_selectedIndex != i && m_controls[i]->selectable())
                {
                    //if (m_controls[i]->selectable())
                    {
                        //deselect existing only when selecting new
                        //and there's more than one to deselect
                        if (m_controls.size() > 1)
                        {
                            for (auto& c : m_controls)
                            {
                                c->deselect();
                            }
                        }

                        m_controls[i]->select();
                        m_selectedIndex = i;

                        auto msg = m_messageBus.post<xy::Message::UIEvent>(xy::Message::UIMessage);
                        msg->type = xy::Message::UIEvent::SelectionChanged;
                    }
                }
            }
        }
    }
    else if (e.type == sf::Event::MouseButtonPressed
        && e.mouseButton.button == sf::Mouse::Left)
    {
        if (hasSelection())
        {
            if (m_controls[m_selectedIndex]->contains(mPos))
            {
                m_controls[m_selectedIndex]->activate();

                auto msg = m_messageBus.post<Message::UIEvent>(Message::UIMessage);
                msg->type = Message::UIEvent::SelectionActivated;
            }
        }
    }
}

void Container::update(float dt)
{
    for (auto& c : m_controls)
        c->update(dt);
}

void Container::setBackgroundColour(const sf::Color& colour)
{
    m_background.setFillColor(colour);
}

void Container::setBackgroundTexture(const sf::Texture& t, bool centre)
{
    m_background.setTexture(&t);
    m_background.setFillColor(sf::Color::White);
    auto size = sf::Vector2f(t.getSize());
    m_background.setSize(size);

    if (centre)
    {
        xy::Util::Position::centreOrigin(m_background);
    }
}

//private
bool Container::hasSelection() const
{
    return m_controls.empty() ? false : (m_selectedIndex >= 0);
}

void Container::select(Index index)
{
    if (m_controls[index]->selectable())
    {
        if (hasSelection())
            m_controls[m_selectedIndex]->deselect();

        m_controls[index]->select();
        m_selectedIndex = index;

        auto msg = m_messageBus.post<xy::Message::UIEvent>(xy::Message::UIMessage);
        msg->type = xy::Message::UIEvent::SelectionChanged;
    }
}

void Container::selectNext()
{
    if (!hasSelection()) return;

    auto next = m_selectedIndex;

    do
    {
        next = (next + 1) % m_controls.size();
    } while (!m_controls[next]->selectable());

    select(next);
}

void Container::selectPrevious()
{
    if (!hasSelection()) return;

    auto prev = m_selectedIndex;

    do
    {
        prev = (prev + m_controls.size() - 1) % m_controls.size();
    } while (!m_controls[prev]->selectable());

    select(prev);
}

void Container::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    if (!visible()) return;

    states.transform *= getTransform();
    rt.draw(m_background, states);
    for (const auto& c : m_controls)
    {
        rt.draw(*c, states);
    }
}