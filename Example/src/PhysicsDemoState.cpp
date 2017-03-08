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

#include <PhysicsDemoState.hpp>
#include <PhysicsDemoInputController.hpp>
#include <PhysicsDemoLine.hpp>
#include <CommandIds.hpp>

#include <xygine/Reports.hpp>
#include <xygine/Entity.hpp>
#include <xygine/Command.hpp>
#include <xygine/components/AudioSource.hpp>
#include <xygine/components/AnimatedDrawable.hpp>
#include <xygine/components/PointLight.hpp>

#include <xygine/App.hpp>
#include <xygine/Log.hpp>
#include <xygine/util/Math.hpp>
#include <xygine/util/Vector.hpp>
#include <xygine/util/Random.hpp>

#include <xygine/shaders/NormalMapped.hpp>
#include <xygine/shaders/Misc.hpp>
#include <xygine/postprocess/ChromeAb.hpp>

#include <xygine/physics/RigidBody.hpp>
#include <xygine/physics/CollisionCircleShape.hpp>
#include <xygine/physics/CollisionEdgeShape.hpp>
#include <xygine/physics/CollisionPolygonShape.hpp>
#include <xygine/physics/CollisionRectangleShape.hpp>
#include <xygine/physics/JointFriction.hpp>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
    const sf::Keyboard::Key upKey = sf::Keyboard::W;
    const sf::Keyboard::Key downKey = sf::Keyboard::S;
    const sf::Keyboard::Key leftKey = sf::Keyboard::A;
    const sf::Keyboard::Key rightKey = sf::Keyboard::D;
    const sf::Keyboard::Key fireKey = sf::Keyboard::Space;

    const float joyDeadZone = 25.f;
    const float joyMaxAxis = 100.f;

    bool drawOverlay = false;

    sf::Shader* shader = nullptr;
}

PhysicsDemoState::PhysicsDemoState(xy::StateStack& stateStack, Context context)
    : State         (stateStack, context),
    m_physWorld     (context.appInstance.getMessageBus()),
    m_messageBus    (context.appInstance.getMessageBus()),
    m_scene         (m_messageBus)
{
    launchLoadingScreen();
    xy::Stats::clear();
    m_scene.setView(context.defaultView);
    //m_scene.drawDebug(true);
    //auto pp = xy::PostProcess::create<xy::PostChromeAb>();
    //m_scene.addPostProcess(pp);
    m_scene.setClearColour(sf::Color(0u, 0u, 10u));

    //preload shaders
    m_shaderResource.preload(PhysicsShaderId::NormalMapTextured, xy::Shader::NormalMapped::vertex, NORMAL_FRAGMENT_TEXTURED);
    m_shaderResource.preload(PhysicsShaderId::NormalMapTexturedSpecular, xy::Shader::NormalMapped::vertex, NORMAL_FRAGMENT_TEXTURED_SPECULAR);
    m_shaderResource.preload(PhysicsShaderId::ReflectionMap, xy::Shader::Default::vertex, xy::Shader::ReflectionMap::fragment);
    m_shaderResource.get(PhysicsShaderId::ReflectionMap).setUniform("u_reflectionMap", m_textureResource.get("assets/images/physics demo/table_reflection.png"));

    shader = &m_shaderResource.get(PhysicsShaderId::NormalMapTextured);

    //scale a 1200px table image to 2.7 metres
    m_physWorld.setPixelScale(444.5f);
    m_physWorld.setGravity({});
    createBodies();
    addLights();

    context.renderWindow.setTitle("xygine Physics Demo");

    xy::App::setMouseCursorVisible(true);
    quitLoadingScreen();

    REPORT("Q", "Toggle overlay");
}

bool PhysicsDemoState::update(float dt)
{
    //const auto& rw = getContext().renderWindow;
    //auto mousePos = rw.mapPixelToCoords(sf::Mouse::getPosition(rw));
    
    m_scene.update(dt);
    
    auto lights = m_scene.getVisibleLights(m_scene.getVisibleArea());
    auto i = 0;
    for (; i < lights.size() && i < xy::Shader::NormalMapped::MaxPointLights; ++i)
    {
        auto light = lights[i];
        if (light)
        {
            const std::string idx = std::to_string(i);

            auto pos = light->getWorldPosition();
            shader->setUniform("u_pointLightPositions[" + std::to_string(i) + "]", pos);
            shader->setUniform("u_pointLights[" + idx + "].intensity", light->getIntensity());
            shader->setUniform("u_pointLights[" + idx + "].diffuseColour", sf::Glsl::Vec4(light->getDiffuseColour()));
            shader->setUniform("u_pointLights[" + idx + "].specularColour", sf::Glsl::Vec4(light->getSpecularColour()));
            shader->setUniform("u_pointLights[" + idx + "].inverseRange", light->getInverseRange());
        }
    }
    //switch off inactive lights
    for (; i < xy::Shader::NormalMapped::MaxPointLights; ++i)
    {
        shader->setUniform("u_pointLights[" + std::to_string(i) + "].intensity", 0.f);
    }

    return true;
}

void PhysicsDemoState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
    rw.setView(getContext().defaultView);
    if(drawOverlay) rw.draw(m_physWorld);
}

bool PhysicsDemoState::handleEvent(const sf::Event& evt)
{
    switch (evt.type)
    {
    case sf::Event::MouseButtonPressed:
        if (evt.mouseButton.button == sf::Mouse::Left)
        {
            xy::Command cmd;
            cmd.category = PhysicsCommandId::CueBall;
            cmd.action = [](xy::Entity& entity, float)
            {
                entity.getComponent<PhysDemo::PlayerController>()->startInput();
            };
            m_scene.sendCommand(cmd);
        }
        break;
    case sf::Event::MouseButtonReleased:
        if (evt.mouseButton.button == sf::Mouse::Left)
        {
            xy::Command cmd;
            cmd.category = PhysicsCommandId::CueBall;
            cmd.action = [](xy::Entity& entity, float)
            {
                entity.getComponent<PhysDemo::PlayerController>()->endInput();
            };
            m_scene.sendCommand(cmd);
        }
        break;
    case sf::Event::KeyPressed:
        switch (evt.key.code)
        {
        case upKey:
            
            break;
        case downKey:
            
            break;
        case leftKey:
            
            break;
        case rightKey:
            
            break;
        case fireKey:
            
            break;
        default: break;
        }
        break;
    case sf::Event::KeyReleased:
        switch (evt.key.code)
        {
        case sf::Keyboard::Escape:
        case sf::Keyboard::BackSpace:
            requestStackPop();
            requestStackPush(States::ID::MenuMain);
            break;
        case sf::Keyboard::P:
            //requestStackPush(States::ID::MenuPaused);
            break;
        case upKey:

            break;
        case downKey:

            break;
        case leftKey:
            
            break;
        case rightKey:
            
            break;
        case fireKey:

            break;
        case sf::Keyboard::Q:
            drawOverlay = !drawOverlay;
        default: break;
        }
        break;
    case sf::Event::JoystickButtonPressed:
        switch (evt.joystickButton.button)
        {
        case 0:
            
            break;
        default: break;
        }
        break;
    case sf::Event::JoystickButtonReleased:

        switch (evt.joystickButton.button)
        {
        case 0:
            
            break;
        case 7:

            break;
        default: break;
        }
        break;
    default: break;
    }

    return true;
}

void PhysicsDemoState::handleMessage(const xy::Message& msg)
{ 
    m_scene.handleMessage(msg);
}

//private
void PhysicsDemoState::createBodies()
{
    auto tableEntity = xy::Entity::create(m_messageBus);
    tableEntity->setWorldPosition({ 360.f, 211.f });
    auto tableBody = m_scene.createComponent<xy::Physics::RigidBody>(m_messageBus, xy::Physics::BodyType::Static);

    //cushions
    std::vector<sf::Vector2f> points = 
    {
        { 90.f, 46.f },
        { 574.f, 46.f },
        { 568.f, 61.f },
        { 103.f, 61.f }
    };
    xy::Physics::CollisionPolygonShape cushion(points);
    cushion.setRestitution(0.98f);
    cushion.setDensity(30.f);
    cushion.setFriction(0.1f);
    tableBody->addCollisionShape(cushion);

    points =
    {
        { 627.f, 46.f },
        { 1110.f, 46.f },
        { 1097.f, 61.f },
        { 632.f, 61.f }
    };
    cushion.setPoints(points);
    tableBody->addCollisionShape(cushion);

    points =
    {
        { 627.f, 614.f },
        { 1110.f, 614.f },
        { 1097.f, 599.f },
        { 632.f, 599.f }
    };
    cushion.setPoints(points);
    tableBody->addCollisionShape(cushion);

    points =
    {
        { 90.f, 614.f },
        { 574.f, 614.f },
        { 568.f, 599.f },
        { 103.f, 599.f }
    };
    cushion.setPoints(points);
    tableBody->addCollisionShape(cushion);

    points =
    {
        { 45.f, 89.f },
        { 60.f, 103.f },
        { 60.f, 558.f },
        { 45.f, 572.f }
    };
    cushion.setPoints(points);
    tableBody->addCollisionShape(cushion);

    points =
    {
        { 1155.f, 89.f },
        { 1142.f, 103.f },
        { 1142.f, 558.f },
        { 1155.f, 572.f }
    };
    cushion.setPoints(points);
    tableBody->addCollisionShape(cushion);

    //pockets - use force affectors on pocket sensors to draw to middle
    points =
    {
        { 45.f, 89.f },
        { 30.f, 64.f },
        { 30.f, 50.f },
        { 50.f, 30.f },
        { 64.f, 30.f },
        { 90.f, 46.f }
    };
    xy::Physics::CollisionEdgeShape pocket(points);
    pocket.setDensity(10.f);
    pocket.setRestitution(0.3f);
    tableBody->addCollisionShape(pocket);

    points =
    {
        { 574.f, 46.f },
        { 583.f, 26.f },
        { 600.f, 20.f },
        { 617.f, 26.f },
        { 627.f, 46.f }
    };
    pocket.setPoints(points);
    tableBody->addCollisionShape(pocket);

    points =
    {
        { 1110.f, 46.f },
        { 1136.f, 30.f },
        { 1150.f, 30.f },
        { 1172.f, 50.f },
        { 1172.f, 64.f },
        { 1155.f, 89.f }
    };
    pocket.setPoints(points);
    tableBody->addCollisionShape(pocket);

    points =
    {
        { 1155.f, 572.f },
        { 1172.f, 594.f },
        { 1172.f, 608.f },
        { 1150.f, 632.f },
        { 1136.f, 632.f },
        { 1110.f, 614.f }
    };
    pocket.setPoints(points);
    tableBody->addCollisionShape(pocket);

    points =
    {
        { 627.f, 614.f },
        { 617.f, 632.f },
        { 600.f, 638.f },
        { 583.f, 632.f },
        { 574.f, 614.f }
    };
    pocket.setPoints(points);
    tableBody->addCollisionShape(pocket);

    points =
    {
        { 90.f, 614.f },
        { 64.f, 632.f },
        { 50.f, 632.f },
        { 28.f, 608.f },
        { 28.f, 594.f },
        { 45.f, 572.f }
    };
    pocket.setPoints(points);
    tableBody->addCollisionShape(pocket);

    tableEntity->addComponent(tableBody);

    //image
    auto drawable = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    drawable->setTexture(m_textureResource.get("assets/images/physics demo/table.png"));
    drawable->setNormalMap(m_textureResource.get("assets/images/physics demo/table_normal.png"));
    drawable->setShader(m_shaderResource.get(PhysicsShaderId::NormalMapTextured));
    tableEntity->addComponent(drawable);

    m_scene.addEntity(tableEntity, xy::Scene::Layer::BackMiddle);

    //add balls
    auto cueball = addBall({ 1260.f, 560.f });
    auto cbEntity = m_scene.findEntity(cueball->getParentUID());
    cbEntity->addCommandCategories(PhysicsCommandId::CueBall);
    auto playerController = std::make_unique<PhysDemo::PlayerController>(m_messageBus);
    cbEntity->addComponent(playerController);
    auto ls = std::make_unique<PhysDemo::LineDrawable>(m_messageBus);
    cbEntity->addComponent(ls);
    cbEntity->getComponent<xy::AnimatedDrawable>()->setColour(sf::Color::White);
    auto as = m_scene.createComponent<xy::AudioSource>(m_messageBus, m_soundResource);
    as->setSound("assets/sound/cuetip.wav");
    as->setName("tip_sound");
    cbEntity->addComponent(as);
    //auto light = m_scene.createComponent<xy::PointLight>(m_messageBus, 200.f, 16.f, sf::Color::Blue);
    //light->setDepth(70.f);
    //cbEntity->addComponent(light);

    const float spacingY = 27.f;
    const float halfY = spacingY / 2.f;
    const float spacingX = std::sqrt((spacingY * spacingY) - (halfY * halfY));

    sf::Vector2f startPos(757.3f, 552.7f);
    for (auto x = 0; x < 5; ++x)
    {
        for (auto y = 0; y < x; ++y)
        {
            addBall(startPos);
            startPos.y += spacingY;
        }       
        startPos.x -= spacingX;
        startPos.y -= ((x * spacingY) + halfY);
    }
}

void PhysicsDemoState::addLights()
{
    auto light = m_scene.createComponent<xy::PointLight>(m_messageBus, 1200.f, 220.f/*, sf::Color::Blue*/);
    light->setDepth(600.f);
    //light->setIntensity(1.5f);

    auto entity = xy::Entity::create(m_messageBus);
    entity->setPosition(xy::DefaultSceneSize / 2.f);
    entity->addComponent(light);
    m_scene.addEntity(entity, xy::Scene::Layer::FrontFront);
}

namespace
{
    const float maxVelocity = 400000.f;
    void cushionImpactAction(xy::Component* component, const xy::Message& msg)
    {
        auto& msgData = msg.getData<xy::Message::PhysicsEvent>();

        xy::AudioSource* as = dynamic_cast<xy::AudioSource*>(component);
        
        if (msgData.event == xy::Message::PhysicsEvent::BeginContact)
        {
            //if neither parent ents our ours, ignore
            if (msgData.contact->getCollisionShapeA()->getRigidBody()->getParentUID() != as->getParentUID()
                && msgData.contact->getCollisionShapeB()->getRigidBody()->getParentUID() != as->getParentUID())
            {
                return;
            }

            //if one of the shapes is not a ball it must be a cushion
            if (msgData.contact->getCollisionShapeA()->type() != xy::Physics::CollisionShape::Type::Circle
                || msgData.contact->getCollisionShapeB()->type() != xy::Physics::CollisionShape::Type::Circle)
            {
                //get velocity and set sound volume
                auto vel = (msgData.contact->getCollisionShapeA()->getRigidBody()->getParentUID() == as->getParentUID()) ?
                    msgData.contact->getCollisionShapeA()->getRigidBody()->getLinearVelocity()
                    :
                    msgData.contact->getCollisionShapeB()->getRigidBody()->getLinearVelocity();

                const float len = xy::Util::Vector::lengthSquared(vel);
                const float vol = xy::Util::Math::clamp(len / maxVelocity, 0.1f, 1.f);
                as->setVolume(vol);
                as->play();
            }
        }
    }

    void ballImpactAction(xy::Component* component, const xy::Message& msg)
    {
        auto& msgData = msg.getData<xy::Message::PhysicsEvent>();

        xy::AudioSource* as = dynamic_cast<xy::AudioSource*>(component);

        if (msgData.event == xy::Message::PhysicsEvent::BeginContact)
        {
            //if entity of first parent isn't ours, skip. We only want to play the sound once in each contact
            if (msgData.contact->getCollisionShapeA()->getRigidBody()->getParentUID() != as->getParentUID())
            {
                return;
            }

            //if we have ball/ball collision
            if (msgData.contact->getCollisionShapeA()->type() == xy::Physics::CollisionShape::Type::Circle
                && msgData.contact->getCollisionShapeB()->type() == xy::Physics::CollisionShape::Type::Circle)
            {
                //get velocity and set sound volume
                const auto vel = xy::Util::Vector::lengthSquared(msgData.contact->getCollisionShapeA()->getRigidBody()->getLinearVelocity());
                const float vol = xy::Util::Math::clamp(vel / maxVelocity, 0.1f, 1.f);
                as->setVolume(vol);
                as->play();
            }
        }
    }
}

xy::Physics::RigidBody* PhysicsDemoState::addBall(const sf::Vector2f& position)
{   
    auto ballEntity = xy::Entity::create(m_messageBus);
    ballEntity->setWorldPosition(position);
    auto ballBody = m_scene.createComponent<xy::Physics::RigidBody>(m_messageBus, xy::Physics::BodyType::Dynamic);
    ballBody->setAngularDamping(0.6f);
    ballBody->setLinearDamping(0.75f);
    ballBody->isBullet(true);

    xy::Physics::CollisionCircleShape ballShape(12.7f);
    ballShape.setDensity(10.f);
    ballShape.setRestitution(0.99f);
    ballShape.setFriction(0.05f);
    ballBody->addCollisionShape(ballShape);

    auto b = ballEntity->addComponent(ballBody);

    //ball / ball sound
    auto soundSource = m_scene.createComponent<xy::AudioSource>(m_messageBus, m_soundResource);
    soundSource->setSound("assets/sound/ball.wav");
    xy::Component::MessageHandler ballImpactHandler;
    ballImpactHandler.action = std::bind(ballImpactAction, std::placeholders::_1, std::placeholders::_2);
    ballImpactHandler.id = xy::Message::PhysicsMessage;
    soundSource->addMessageHandler(ballImpactHandler);
    ballEntity->addComponent(soundSource);

    //ball / cushion sound
    soundSource = m_scene.createComponent<xy::AudioSource>(m_messageBus, m_soundResource);
    soundSource->setSound("assets/sound/cushion.wav");
    ballImpactHandler.action = std::bind(cushionImpactAction, std::placeholders::_1, std::placeholders::_2);
    soundSource->addMessageHandler(ballImpactHandler);
    ballEntity->addComponent(soundSource);

    auto drawable = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    drawable->setColour(sf::Color::Blue);
    drawable->setTexture(m_textureResource.get("assets/images/physics demo/ball.png"));
    drawable->setNormalMap(m_textureResource.get("assets/images/physics demo/ball_normal.png"));
    auto size = drawable->getTexture()->getSize();
    drawable->setOrigin({ size.x / 2.f, size.y / 2.f });
    drawable->setShader(m_shaderResource.get(PhysicsShaderId::NormalMapTexturedSpecular));
    ballEntity->addComponent(drawable);

    m_scene.addEntity(ballEntity, xy::Scene::Layer::BackMiddle);

    return b;
}