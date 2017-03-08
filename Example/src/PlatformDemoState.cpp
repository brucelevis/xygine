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

#include <PlatformDemoState.hpp>
#include <PlatformPlayerController.hpp>
#include <PlatformBackground.hpp>

#include <xygine/Reports.hpp>
#include <xygine/Entity.hpp>
#include <xygine/Command.hpp>

#include <xygine/App.hpp>
#include <xygine/Log.hpp>
#include <xygine/Console.hpp>
#include <xygine/util/Random.hpp>

#include <xygine/components/AnimatedDrawable.hpp>
#include <xygine/components/QuadTreeComponent.hpp>
#include <xygine/components/PointLight.hpp>
#include <xygine/components/Camera.hpp>

#include <xygine/physics/RigidBody.hpp>
#include <xygine/physics/CollisionCircleShape.hpp>
#include <xygine/physics/CollisionEdgeShape.hpp>
#include <xygine/physics/CollisionRectangleShape.hpp>

#include <xygine/components/MeshDrawable.hpp>
#include <xygine/components/Model.hpp>
#include <xygine/mesh/shaders/ForwardLighting.hpp>
#include <xygine/mesh/SubMesh.hpp>
#include <xygine/mesh/CubeBuilder.hpp>
#include <xygine/mesh/IQMBuilder.hpp>
#include <xygine/mesh/QuadBuilder.hpp>
#include <xygine/mesh/SphereBuilder.hpp>

#include <xygine/shaders/NormalMapped.hpp>

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

    enum PlatformShaderId
    {
        SpecularSmooth2D
    };

    Plat::PlayerController* playerController = nullptr;
    sf::Uint8 playerInput = 0;
    bool showDebug = false;
    xy::Camera* playerCamera = nullptr;
}

PlatformDemoState::PlatformDemoState(xy::StateStack& stateStack, Context context)
    : State         (stateStack, context),
    m_messageBus    (context.appInstance.getMessageBus()),
    m_physWorld     (m_messageBus),
    m_scene         (m_messageBus),
    m_meshRenderer  ({ context.appInstance.getVideoSettings().VideoMode.width, context.appInstance.getVideoSettings().VideoMode.height }, m_scene)
{
    launchLoadingScreen();
    xy::Stats::clear();
    m_physWorld.setGravity({ 0.f, 1180.f });
    m_physWorld.setPixelScale(120.f);

    m_scene.setView(context.defaultView);
    m_scene.setAmbientColour({ 76, 70, 72 });
    m_scene.getSkyLight().setIntensity(0.5f);
    m_scene.getSkyLight().setDiffuseColour({ 255, 255, 200 });
    m_scene.getSkyLight().setSpecularColour({ 120, 255, 58 });
    m_scene.getSkyLight().setDirection({ 0.2f, 0.4f, -0.4f });

    m_meshRenderer.setView(context.defaultView);

    cacheMeshes();
    buildTerrain();
    buildPhysics();

    addItems();
    addPlayer();

    auto e = xy::Entity::create(m_messageBus);
    auto md = m_meshRenderer.createDrawable(m_messageBus);
    //md->enableWater(true);
    e->addComponent(md);
    m_scene.addEntity(e, xy::Scene::Layer::FrontFront);

    REPORT("Q", "Show Debug");
    xy::App::setMouseCursorVisible(true);

    context.renderWindow.setTitle("xygine Mesh Rendering Demo");

    quitLoadingScreen();
}

bool PlatformDemoState::update(float dt)
{    
    playerController->applyInput(playerInput);

    m_scene.update(dt);
    m_meshRenderer.update();

    //update lighting
    auto& shader = m_shaderResource.get(PlatformShaderId::SpecularSmooth2D);
    //shader.setUniform("u_ambientColour", sf::Glsl::Vec4(m_scene.getAmbientColour()));
    auto lights = m_scene.getVisibleLights(m_scene.getVisibleArea());
    auto i = 0u;
    for (; i < lights.size() && i < xy::Shader::NormalMapped::MaxPointLights; ++i)
    {
        auto light = lights[i];
        if (light)
        {
            const std::string idx = std::to_string(i);
            
            auto pos = light->getWorldPosition();
            shader.setUniform("u_pointLightPositions[" + std::to_string(i) + "]", pos);
            shader.setUniform("u_pointLights[" + idx + "].intensity", light->getIntensity());
            shader.setUniform("u_pointLights[" + idx + "].diffuseColour", sf::Glsl::Vec4(light->getDiffuseColour()));
            shader.setUniform("u_pointLights[" + idx + "].specularColour", sf::Glsl::Vec4(light->getSpecularColour()));
            shader.setUniform("u_pointLights[" + idx + "].inverseRange", light->getInverseRange());
        }
    }
    //switch off inactive lights
    for (; i < xy::Shader::NormalMapped::MaxPointLights; ++i)
    {
        shader.setUniform("u_pointLights[" + std::to_string(i) + "].intensity", 0.f);
    }
    
    //update skylight
    const auto& skyLight = m_scene.getSkyLight();
    shader.setUniform("u_directionalLight.diffuseColour", sf::Glsl::Vec4(skyLight.getDiffuseColour()));
    shader.setUniform("u_directionalLight.specularColour", sf::Glsl::Vec4(skyLight.getSpecularColour()));
    shader.setUniform("u_directionalLight.intensity", skyLight.getIntensity());
    shader.setUniform("u_directionaLightDirection", skyLight.getDirection());

    return true;
}

void PlatformDemoState::draw()
{   
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);    

    //rw.draw(m_meshRenderer);

    if (showDebug)
    {
        rw.setView(m_scene.getView());
        rw.draw(m_physWorld);
    }
}

bool PlatformDemoState::handleEvent(const sf::Event& evt)
{
    switch (evt.type)
    {
    case sf::Event::MouseButtonReleased:
    {
        //const auto& rw = getContext().renderWindow;
        //auto mousePos = rw.mapPixelToCoords(sf::Mouse::getPosition(rw));
        
    }
        break;
    case sf::Event::KeyPressed:
        switch (evt.key.code)
        {
        case upKey:
            playerInput |= Plat::Jump;
            break;
        case downKey:
            
            break;
        case leftKey:
            playerInput |= Plat::Left;
            break;
        case rightKey:
            playerInput |= Plat::Right;
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
            playerInput &= ~Plat::Jump;
            break;
        case downKey:
            
            break;
        case leftKey:
            playerInput &= ~Plat::Left;
            break;
        case rightKey:
            playerInput &= ~Plat::Right;
            break;
        case fireKey:

            break;
        case sf::Keyboard::Q:
            showDebug = !showDebug;
            m_scene.drawDebug(showDebug);
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

void PlatformDemoState::handleMessage(const xy::Message& msg)
{   
    m_scene.handleMessage(msg);
    m_meshRenderer.handleMessage(msg);    
    
    if (msg.id == xy::Message::UIMessage)
    {
        const auto& msgData = msg.getData<xy::Message::UIEvent>();
        switch (msgData.type)
        {
        default: break;
        case xy::Message::UIEvent::ResizedWindow:
            //m_meshRenderer.setView(getContext().defaultView);
            //m_scene.setView(getContext().defaultView);
        {
            auto v = playerCamera->getView();
            v.setViewport(getContext().defaultView.getViewport());
            playerCamera->setView(v);
        }
            break;
        }
    }
}

//private
void PlatformDemoState::cacheMeshes()
{
    xy::CubeBuilder cb(100.f);
    m_meshRenderer.loadModel(MeshID::Cube, cb);

    xy::IQMBuilder ib("assets/models/mrfixit.iqm");
    m_meshRenderer.loadModel(MeshID::Fixit, ib);

    xy::IQMBuilder ib2("assets/models/platform_01.iqm");
    m_meshRenderer.loadModel(MeshID::Platform, ib2);

    xy::IQMBuilder ib3("assets/models/batcat.iqm");
    m_meshRenderer.loadModel(MeshID::Batcat, ib3);

    xy::QuadBuilder qb({ 1500.f, 550.f });
    m_meshRenderer.loadModel(MeshID::Quad, qb);

    xy::SphereBuilder sb(34.f, 6u);
    m_meshRenderer.loadModel(MeshID::Sphere, sb);

    auto& demoMaterial = m_meshRenderer.addMaterial(MatId::Demo, xy::Material::TexturedBumped, true);
    demoMaterial.addProperty({ "u_diffuseMap", m_textureResource.get("assets/images/platform/cube_diffuse.png") });
    demoMaterial.addProperty({ "u_normalMap", m_textureResource.get("assets/images/platform/cube_normal.png") });
    demoMaterial.addProperty({ "u_maskMap", m_textureResource.get("assets/images/platform/cube_mask.png") });
    demoMaterial.getRenderPass(xy::RenderPass::ID::ShadowMap)->setCullFace(xy::CullFace::Front);
  
    auto& fixitMaterialBody = m_meshRenderer.addMaterial(MatId::MrFixitBody, xy::Material::TexturedSkinnedBumped, true);
    fixitMaterialBody.addProperty({ "u_diffuseMap", m_textureResource.get("assets/images/fixit/fixitBody.png") });
    fixitMaterialBody.addProperty({ "u_normalMap", m_textureResource.get("assets/images/fixit/fixitBody_normal.png") });
    fixitMaterialBody.addProperty({ "u_maskMap", m_textureResource.get("assets/images/fixit/fixitBody_mask.tga") });
    fixitMaterialBody.getRenderPass(xy::RenderPass::ID::ShadowMap)->setCullFace(xy::CullFace::Front);

    auto& fixitMaterialHead = m_meshRenderer.addMaterial(MatId::MrFixitHead, xy::Material::TexturedSkinnedBumped, true);
    fixitMaterialHead.addProperty({ "u_diffuseMap", m_textureResource.get("assets/images/fixit/fixitHead.png") });
    fixitMaterialHead.addProperty({ "u_normalMap", m_textureResource.get("assets/images/fixit/fixitHead_normal.png") });
    fixitMaterialHead.addProperty({ "u_maskMap", m_textureResource.get("assets/images/fixit/fixitHead_mask.png") });
    fixitMaterialHead.getRenderPass(xy::RenderPass::ID::ShadowMap)->setCullFace(xy::CullFace::Front);

    auto& platformMaterial01 = m_meshRenderer.addMaterial(MatId::Platform01, xy::Material::Textured, true, true);
    platformMaterial01.addProperty({ "u_diffuseMap", m_textureResource.get("assets/images/platform/plat_01.png") });


    auto& platformMaterial04 = m_meshRenderer.addMaterial(MatId::Platform04, xy::Material::Textured, true, true);
    platformMaterial04.addProperty({ "u_diffuseMap", m_textureResource.get("assets/images/platform/plat_04.png") });

    auto& catMat = m_meshRenderer.addMaterial(MatId::BatcatMat, xy::Material::TexturedSkinnedBumped, true);
    auto& tex = m_textureResource.get("assets/images/platform/batcat_diffuse.png");
    tex.setRepeated(true);
    catMat.addProperty({ "u_diffuseMap", tex });
    auto& tex2 = m_textureResource.get("assets/images/platform/batcat_normal.png");
    tex2.setRepeated(true);
    catMat.addProperty({ "u_normalMap", tex2 });
    auto& tex3 = m_textureResource.get("assets/images/platform/batcat_mask.png");
    tex3.setRepeated(true);
    catMat.addProperty({ "u_maskMap", tex3 });
    catMat.getRenderPass(xy::RenderPass::ID::ShadowMap)->setCullFace(xy::CullFace::Front);


    m_textureResource.setFallbackColour(sf::Color(230, 120, 0));
    auto& sphereMat = m_meshRenderer.addMaterial(MatId::SphereTest, xy::Material::TexturedBumped, true);
    sphereMat.addProperty({ "u_diffuseMap", m_textureResource.get("assets/images/sphere_test.png") });
    sphereMat.addProperty({ "u_normalMap", m_textureResource.get("assets/images/sphere_normal.png") });
    sphereMat.addProperty({ "u_maskMap", m_textureResource.get("I don't want a mask texture!!") });
    sphereMat.getRenderPass(xy::RenderPass::ID::ShadowMap)->setCullFace(xy::CullFace::Front);

    auto& lightMaterial = m_meshRenderer.addMaterial(MatId::LightSource, xy::Material::Coloured);
    lightMaterial.addProperty({ "u_colour", sf::Color(135, 135, 80) });
    lightMaterial.addProperty({ "u_maskColour", sf::Color::Blue });

    auto light = m_scene.createComponent<xy::PointLight>(m_messageBus, 800.f, 500.f, sf::Color(255, 255, 100));
    light->setDepth(400.f);
    light->enableShadowCasting(true);

    auto model = m_meshRenderer.createModel(MeshID::Cube, m_messageBus);
    model->setPosition({ 0.f, 0.f, light->getWorldPosition().z });
    model->setSubMaterial(lightMaterial, 0);

    auto entity = xy::Entity::create(m_messageBus);
    entity->setPosition(xy::DefaultSceneSize / 2.f);
    entity->setScale(0.25f, 0.25f);
    entity->addComponent(light);
    entity->addComponent(model);
    m_scene.addEntity(entity, xy::Scene::Layer::FrontFront);


    //---------------
    light = m_scene.createComponent<xy::PointLight>(m_messageBus, 600.f, 500.f, sf::Color(255, 255, 100));
    light->setDepth(300.f);
    light->setIntensity(2.5f);
    light->enableShadowCasting(true);

    model = m_meshRenderer.createModel(MeshID::Cube, m_messageBus);
    model->setPosition({ 0.f, 0.f, light->getWorldPosition().z });
    model->setSubMaterial(lightMaterial, 0);

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(2000.f, 200.f);
    entity->setScale(0.35f, 0.35f);
    entity->addComponent(light);
    entity->addComponent(model);
    m_scene.addEntity(entity, xy::Scene::Layer::FrontFront);
}

void PlatformDemoState::buildTerrain()
{
    m_shaderResource.preload(PlatformShaderId::SpecularSmooth2D, xy::Shader::NormalMapped::vertex, NORMAL_FRAGMENT_TEXTURED);
    m_textureResource.setFallbackColour({ 127, 127, 255 });
    const auto& normalTexture = m_textureResource.get("normalFallback");

    auto background = m_scene.createComponent<Plat::Background>(m_messageBus, m_textureResource);
    background->setAmbientColour(m_scene.getAmbientColour());
    auto entity = xy::Entity::create(m_messageBus);
    entity->addComponent(background);
    m_scene.addEntity(entity, xy::Scene::Layer::BackRear);

    auto drawable = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    drawable->setTexture(m_textureResource.get("assets/images/platform/left_edge.png"));
    drawable->setNormalMap(normalTexture);
    drawable->setShader(m_shaderResource.get(PlatformShaderId::SpecularSmooth2D));

    entity = xy::Entity::create(m_messageBus);
    entity->addComponent(drawable);

    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);
    //-------------------------
    drawable = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    drawable->setTexture(m_textureResource.get("assets/images/platform/ground_section.png"));
    drawable->setNormalMap(normalTexture);
    drawable->setShader(m_shaderResource.get(PlatformShaderId::SpecularSmooth2D));

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(256.f, 1080.f - 128.f);
    entity->addComponent(drawable);

    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);
    //-------------------------
    drawable = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    drawable->setTexture(m_textureResource.get("assets/images/platform/ground_section.png"));
    drawable->setNormalMap(normalTexture);
    drawable->setShader(m_shaderResource.get(PlatformShaderId::SpecularSmooth2D));

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(1024.f, 1080.f - 128.f);
    entity->addComponent(drawable);

    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);
    //-------------------------
    drawable = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    drawable->setTexture(m_textureResource.get("assets/images/platform/ground_section.png"));
    drawable->setNormalMap(normalTexture);
    drawable->setShader(m_shaderResource.get(PlatformShaderId::SpecularSmooth2D));

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(1792.f, 1080.f - 128.f);
    entity->addComponent(drawable);

    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);
    //-------------------------
    drawable = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    drawable->setTexture(m_textureResource.get("assets/images/platform/right_edge.png"));
    drawable->setNormalMap(normalTexture);
    drawable->setShader(m_shaderResource.get(PlatformShaderId::SpecularSmooth2D));

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(2560.f, 0.f);
    entity->addComponent(drawable);

    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);
    //-------------------------
    auto model = m_meshRenderer.createModel(MeshID::Platform, m_messageBus);
    model->rotate(xy::Model::Axis::X, 90.f);
    model->rotate(xy::Model::Axis::Z, 180.f);
    model->setPosition({ 384.f, 158.f, 0.f });
    model->setBaseMaterial(m_meshRenderer.getMaterial(MatId::Platform01));

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(400.f, 700.f);
    entity->addComponent(model);

    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);
    //-------------------------
    drawable = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    drawable->setTexture(m_textureResource.get("assets/images/platform/plat_03.png"));
    drawable->setNormalMap(normalTexture);
    drawable->setShader(m_shaderResource.get(PlatformShaderId::SpecularSmooth2D));

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(2000.f, 550.f);
    entity->addComponent(drawable);

    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);
    //-------------------------
    drawable = m_scene.createComponent<xy::AnimatedDrawable>(m_messageBus);
    drawable->setTexture(m_textureResource.get("assets/images/platform/plat_02.png"));
    drawable->setNormalMap(normalTexture);
    drawable->setShader(m_shaderResource.get(PlatformShaderId::SpecularSmooth2D));

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(1670.f, 450.f);
    entity->addComponent(drawable);

    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);
    //-------------------------

    model = m_meshRenderer.createModel(MeshID::Platform, m_messageBus);
    model->rotate(xy::Model::Axis::X, 90.f);
    model->rotate(xy::Model::Axis::Z, 180.f);
    model->setPosition({ 384.f, 158.f, 0.f });
    model->setBaseMaterial(m_meshRenderer.getMaterial(MatId::Platform04));

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(1210.f, 600.f);
    entity->addComponent(model);

    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);

    //------some shadow receivers-------//
    model = m_meshRenderer.createModel(MeshID::Quad, m_messageBus);
    model->setPosition({ 0.f, 0.f, -250.f });

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(1000.f, 702.f);
    entity->addComponent(model);
    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);

    model = m_meshRenderer.createModel(MeshID::Quad, m_messageBus);
    model->rotate(xy::Model::Axis::X, 90.f);

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(1000.f, 952.f);
    entity->addComponent(model);
    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);
}

void PlatformDemoState::buildPhysics()
{
    auto groundBody = m_scene.createComponent<xy::Physics::RigidBody>(m_messageBus, xy::Physics::BodyType::Static);
    
    xy::Physics::CollisionRectangleShape rectShape({ 128.f, 1080.f });
    groundBody->addCollisionShape(rectShape);
    rectShape.setRect({ 128.f, 1080.f - 128.f, 2560.f, 128.f });
    groundBody->addCollisionShape(rectShape);
    rectShape.setRect({ 2688.f, 0.f, 128.f, 1080.f });
    groundBody->addCollisionShape(rectShape);
    rectShape.setRect({ 128.f, -20.f, 2560.f, 20.f });
    groundBody->addCollisionShape(rectShape);

    //add platforms
    std::vector<sf::Vector2f> points = 
    {
        {420.f, 800.f},
        {400.f, 765.f},
        {784.f, 765.f},
        {768.f, 800.f}
    };
    xy::Physics::CollisionEdgeShape ce(points);
    groundBody->addCollisionShape(ce);
    
    points =
    {
        { 2020.f, 639.f },
        { 2000.f, 604.f },
        { 2384.f, 604.f },
        { 2364.f, 639.f }
    };
    ce.setPoints(points);
    groundBody->addCollisionShape(ce);

    points =
    {
        { 1690.f, 533.f },
        { 1670.f, 497.f },
        { 2054.f, 497.f },
        { 2034.f, 533.f }
    };
    ce.setPoints(points);
    groundBody->addCollisionShape(ce);

    points =
    {
        { 1230.f, 700.f },
        { 1210.f, 665.f },
        { 1584.f, 665.f },
        { 1564.f, 700.f }
    };
    ce.setPoints(points);
    groundBody->addCollisionShape(ce);

    auto entity = xy::Entity::create(m_messageBus);
    entity->addComponent(groundBody);

    m_scene.addEntity(entity, xy::Scene::Layer::BackFront);
}

void PlatformDemoState::addItems()
{
    std::function<void(const sf::Vector2f&)> createBox = 
        [this](const sf::Vector2f& position)
    {
        auto body = m_scene.createComponent<xy::Physics::RigidBody>(m_messageBus, xy::Physics::BodyType::Dynamic);
        xy::Physics::CollisionRectangleShape cs({ 100.f, 100.f }, { -50.f, -50.f });
        cs.setDensity(11.f);
        cs.setFriction(0.7f);
        body->addCollisionShape(cs);

        auto model = m_meshRenderer.createModel(MeshID::Cube, m_messageBus);
        model->setBaseMaterial(m_meshRenderer.getMaterial(MatId::Demo));

        auto ent = xy::Entity::create(m_messageBus);
        ent->setPosition(position);
        ent->addComponent(body);
        ent->addComponent(model);

        m_scene.addEntity(ent, xy::Scene::BackFront);
    };


    std::function<void(const sf::Vector2f&)> createBall = 
        [this](const sf::Vector2f& position)
    {
        auto body = m_scene.createComponent<xy::Physics::RigidBody>(m_messageBus, xy::Physics::BodyType::Dynamic);
        xy::Physics::CollisionCircleShape cs(34.f);
        cs.setDensity(4.f);
        cs.setFriction(0.2f);
        cs.setRestitution(0.8f);
        body->addCollisionShape(cs);

        auto model = m_meshRenderer.createModel(MeshID::Sphere, m_messageBus);
        model->setBaseMaterial(m_meshRenderer.getMaterial(MatId::SphereTest));

        auto ent = xy::Entity::create(m_messageBus);
        ent->setPosition(position);
        ent->addComponent(body);
        ent->addComponent(model);

        m_scene.addEntity(ent, xy::Scene::BackFront);
    };

    for (auto i = 0u; i < 5u; ++i)
    {
        createBox({ xy::Util::Random::value(20.f, 2500.f), 20.f });
        createBall({ xy::Util::Random::value(20.f, 2500.f), 20.f });
    }

}

void PlatformDemoState::addPlayer()
{
    auto body = m_scene.createComponent<xy::Physics::RigidBody>(m_messageBus, xy::Physics::BodyType::Dynamic);
    xy::Physics::CollisionRectangleShape cs({ 120.f, 240.f });
    cs.setFriction(0.6f);
    cs.setDensity(0.9f);

    body->fixedRotation(true);
    body->addCollisionShape(cs);

    auto controller = m_scene.createComponent<Plat::PlayerController>(m_messageBus);

    auto camera = m_scene.createComponent<xy::Camera>(m_messageBus, getContext().defaultView);
    camera->lockTransform(xy::Camera::TransformLock::AxisY);
    camera->lockBounds({ 0.f,0.f, 2816.f, 1080.f });

    auto model = m_meshRenderer.createModel(MeshID::Batcat, m_messageBus);
    model->setBaseMaterial(m_meshRenderer.getMaterial(MatId::BatcatMat));
    model->rotate(xy::Model::Axis::X, 90.f);
    model->rotate(xy::Model::Axis::Z, -90.f);
    model->setPosition({ 60.f, 240.f, 0.f });
    model->playAnimation(1, 0.2f);

    auto entity = xy::Entity::create(m_messageBus);
    entity->setPosition(960.f, 540.f);
    entity->addComponent(body);
    entity->addComponent(model);
    playerController = entity->addComponent(controller);
    playerCamera = entity->addComponent(camera);
    m_scene.setActiveCamera(playerCamera);
    m_scene.addEntity(entity, xy::Scene::Layer::FrontMiddle);

    body = m_scene.createComponent<xy::Physics::RigidBody>(m_messageBus, xy::Physics::BodyType::Dynamic);
    body->fixedRotation(true);
    cs.setRect({ 200.f, 300.f });
    body->addCollisionShape(cs);
    cs.setRect({ 80.f, 80.f }, { 60.f, -80.f });
    body->addCollisionShape(cs);

    model = m_meshRenderer.createModel(MeshID::Fixit, m_messageBus);
    model->setSubMaterial(m_meshRenderer.getMaterial(MatId::MrFixitBody), 0);
    model->setSubMaterial(m_meshRenderer.getMaterial(MatId::MrFixitHead), 1);
    model->rotate(xy::Model::Axis::X, 90.f);
    model->rotate(xy::Model::Axis::Z, 90.f);
    model->setScale({ 50.f, 50.f, 50.f });
    model->setPosition({ 100.f, 300.f, 0.f });

    entity = xy::Entity::create(m_messageBus);
    entity->setPosition(1320.f, 40.f);
    entity->addComponent(model);
    entity->addComponent(body);
    m_scene.addEntity(entity, xy::Scene::Layer::FrontMiddle);
}