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

#include <ParticleDemoState.hpp>
#include <CaveDemoDrawable.hpp>

#include <xygine/Reports.hpp>
#include <xygine/Entity.hpp>
#include <xygine/Command.hpp>

#include <xygine/App.hpp>
#include <xygine/Log.hpp>

#include <xygine/components/ParticleController.hpp>
#include <xygine/components/AnimatedDrawable.hpp>
#include <xygine/components/QuadTreeComponent.hpp>
#include <xygine/components/PointLight.hpp>
#include <xygine/physics/RigidBody.hpp>
#include <xygine/physics/CollisionCircleShape.hpp>
#include <xygine/physics/CollisionEdgeShape.hpp>
#include <xygine/postprocess/Bloom.hpp>
#include <xygine/postprocess/ChromeAb.hpp>
#include <xygine/shaders/NormalMapped.hpp>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <TimedDestruction.hpp>

namespace
{
    const sf::Keyboard::Key upKey = sf::Keyboard::W;
    const sf::Keyboard::Key downKey = sf::Keyboard::S;
    const sf::Keyboard::Key leftKey = sf::Keyboard::A;
    const sf::Keyboard::Key rightKey = sf::Keyboard::D;
    const sf::Keyboard::Key fireKey = sf::Keyboard::Space;

    const float joyDeadZone = 25.f;
    const float joyMaxAxis = 100.f;

    enum ParticleType
    {
        Bubbles = 0,
        Explosion,
        FairyDust,
        Fire,
        Count
    };

    enum ParticleShaderId
    {
        NormalMapTextured = 1,
        NormalMapTexturedSpecular
    };

    sf::Uint64 controllerId = 0;

    bool debugDraw = false;

    sf::Shader* shader;
}

ParticleDemoState::ParticleDemoState(xy::StateStack& stateStack, Context context)
    : State         (stateStack, context),
    m_messageBus    (context.appInstance.getMessageBus()),
    m_physWorld     (m_messageBus),
    m_scene         (m_messageBus)    
{
    launchLoadingScreen();
    xy::Stats::clear();
    m_physWorld.setGravity({ 0.f, 980.f });
    m_physWorld.setPixelScale(100.f);

    m_scene.setView(context.defaultView);

    //xy::PostProcess::Ptr pp = xy::PostProcess::create<xy::PostBloom>();
    //m_scene.addPostProcess(pp);
    auto pp = xy::PostProcess::create<xy::PostChromeAb>(/*true*/);
    m_scene.addPostProcess(pp);
    m_scene.setClearColour({ 0u, 0u, 20u });

    m_shaderResource.preload(ParticleShaderId::NormalMapTexturedSpecular, xy::Shader::NormalMapped::vertex, NORMAL_FRAGMENT_TEXTURED_SPECULAR);
    //m_shaderResource.preload(ParticleShaderId::NormalMapTextured, xy::Shader::NormalMapped::vertex, NORMAL_FRAGMENT_TEXTURED);
    shader = &m_shaderResource.get(ParticleShaderId::NormalMapTexturedSpecular);

    auto& skyLight = m_scene.getSkyLight();
    skyLight.setDiffuseColour({255, 255, 220});
    skyLight.setSpecularColour(sf::Color::Green);
    skyLight.setIntensity(0.5f);
    skyLight.setDirection({ 1.f, 1.f, -1.f });
    shader->setUniform("u_directionalLight.diffuseColour", sf::Glsl::Vec4(skyLight.getDiffuseColour()));
    shader->setUniform("u_directionalLight.specularColour", sf::Glsl::Vec4(skyLight.getSpecularColour()));
    shader->setUniform("u_directionalLight.intensity", skyLight.getIntensity());
    shader->setUniform("u_directionaLightDirection", skyLight.getDirection());

    setupParticles();
    buildTerrain();

    xy::App::setMouseCursorVisible(true);

    context.renderWindow.setTitle("xygine Particle Demo");

    quitLoadingScreen();
}

bool ParticleDemoState::update(float dt)
{    
    m_scene.update(dt);

    //update lighting
    auto lights = m_scene.getVisibleLights(m_scene.getVisibleArea());
    auto i = 0u;
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

void ParticleDemoState::draw()
{   
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
    rw.setView(getContext().defaultView); //need to restore this to return correct mouse position
    //rw.draw(m_physWorld);
}

bool ParticleDemoState::handleEvent(const sf::Event& evt)
{
    switch (evt.type)
    {
    case sf::Event::MouseButtonReleased:
    {
        const auto& rw = getContext().renderWindow;
        auto mousePos = rw.mapPixelToCoords(sf::Mouse::getPosition(rw));
        
        spawnThing(mousePos);
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
            debugDraw = !debugDraw;
            m_scene.drawDebug(debugDraw);
            break;
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

void ParticleDemoState::handleMessage(const xy::Message& msg)
{ 
    m_scene.handleMessage(msg);
}

//private
void ParticleDemoState::setupParticles()
{
    auto particleController = m_scene.createComponent<xy::ParticleController>(m_messageBus);
    auto entity = xy::Entity::create(m_messageBus);
    
    m_particleDef.loadFromFile("assets/particles/explosion.xyp", m_textureResource);
    particleController->addDefinition(ParticleType::Explosion, m_particleDef);

    auto pc = entity->addComponent(particleController);

    m_particleDef.loadFromFile("assets/particles/bubbles.xyp", m_textureResource);
    pc->addDefinition(ParticleType::Bubbles, m_particleDef);

    m_particleDef.loadFromFile("assets/particles/fire.xyp", m_textureResource);
    pc->addDefinition(ParticleType::Fire, m_particleDef);

    m_particleDef.loadFromFile("assets/particles/fairydust.xyp", m_textureResource);
    pc->addDefinition(ParticleType::FairyDust, m_particleDef);

    xy::Component::MessageHandler mh;
    mh.action = [](xy::Component* c, const xy::Message& msg) 
    {
        auto& msgData = msg.getData<xy::Message::EntityEvent>();
        if (msgData.action == xy::Message::EntityEvent::Destroyed)
        {
            auto controller = dynamic_cast<xy::ParticleController*>(c);
            controller->fire(ParticleType::Explosion, msgData.entity->getWorldPosition());
        }
    };
    mh.id = xy::Message::EntityMessage;
    pc->addMessageHandler(mh);

    controllerId = entity->getUID();
    m_scene.addEntity(entity, xy::Scene::Layer::FrontFront);
}

void ParticleDemoState::buildTerrain()
{
    auto ent = xy::Entity::create(m_messageBus);

    /*auto ad = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    ad->setTexture(m_textureResource.get("assets/images/cave/background.png"));
    ad->setNormalMap(m_textureResource.get("assets/images/cave/background_normal.png"));
    ad->setShader(m_shaderResource.get(ParticleShaderId::NormalMapTexturedSpecular));
    ad->setScale(1.6f, 1.6f);
    ent->addComponent(ad);*/

    auto cd = std::make_unique<CaveDemo::CaveDrawable>(m_messageBus);
    ent->move((sf::Vector2f(1920.f, 1080.f) - cd->getSize()) / 2.f);
    //ent->move(100.f, 100.f);
    auto cave = ent->addComponent(cd);
    cave->setTexture(m_textureResource.get("assets/images/cave/diffuse.png"));
    cave->setNormalMap(m_textureResource.get("assets/images/cave/normal.png"));
    cave->setMaskMap(m_textureResource.get("assets/images/cave/mask.png"));
    cave->setShader(&m_shaderResource.get(ParticleShaderId::NormalMapTexturedSpecular));
    
    //get edges to add to physworld
    const auto& edges = cave->getEdges();
    REPORT("edge count", std::to_string(edges.size()));
    auto rb = m_scene.createComponent<xy::Physics::RigidBody>(m_messageBus, xy::Physics::BodyType::Static);
    for (const auto& e : edges)
    {
        xy::Physics::CollisionEdgeShape es(e);
        rb->addCollisionShape(es);
    }
    ent->addComponent(rb);

    m_scene.addEntity(ent, xy::Scene::Layer::BackRear);
}

void ParticleDemoState::spawnThing(const sf::Vector2f& position)
{
    auto ps = m_particleDef.createSystem(m_scene, m_messageBus);
    ps->setLifetimeVariance(0.5f);
    ps->start(m_particleDef.releaseCount, m_particleDef.delay, m_particleDef.duration);

    auto physBody = m_scene.createComponent<xy::Physics::RigidBody>(m_messageBus, xy::Physics::BodyType::Dynamic);
    xy::Physics::CollisionCircleShape cs(10.f);
    cs.setRestitution(0.99f);
    cs.setDensity(20.f);
    physBody->addCollisionShape(cs);

    auto td = m_scene.createComponent<TimedDestruction>(m_messageBus);

    auto dwbl = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    dwbl->setTexture(m_textureResource.get("assets/images/physics demo/ball.png"));
    //dwbl->setNormalMap(m_textureResource.get("assets/images/physics demo/ball_normal.png"));
    //dwbl->setShader(m_shaderResource.get(ParticleShaderId::NormalMapTexturedSpecular));
    auto size = dwbl->getFrameSize();
    dwbl->setOrigin({ size.x / 2.f, size.y / 2.f });
    dwbl->setColour({ 198u, 200u, 250u });

    auto light = m_scene.createComponent<xy::PointLight>(m_messageBus, 100.f, size.x / 2.f, sf::Color(198u, 200u, 250u), sf::Color(128u, 128u, 255u));
    light->setDepth(300.f);
    light->setRange(750.f);
    //light->setIntensity(0.9f);

    auto entity = xy::Entity::create(m_messageBus);
    entity->setWorldPosition(position);
    entity->addComponent(ps);
    entity->addComponent(physBody);
    entity->addComponent(td);
    entity->addComponent(dwbl);
    entity->addComponent(light);

    m_scene.addEntity(entity, xy::Scene::Layer::FrontMiddle);
}