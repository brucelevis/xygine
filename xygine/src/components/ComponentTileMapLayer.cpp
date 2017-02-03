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

#include <xygine/detail/GLExtensions.hpp>
#include <xygine/detail/GLCheck.hpp>

#include <xygine/ShaderResource.hpp>
#include <xygine/components/TileMapLayer.hpp>
#include <xygine/Entity.hpp>
#include <xygine/Scene.hpp>
#include <xygine/Reports.hpp>
#include <xygine/Resource.hpp>

#include <xygine/shaders/Default.hpp>
#include <xygine/shaders/Tilemap.hpp>

#include <xygine/tilemap/TileLayer.hpp>
#include <xygine/tilemap/ImageLayer.hpp>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <limits>

using namespace xy;

namespace
{
    void setTextureProperties(sf::Texture* texture, const std::vector<std::uint16_t>& data, sf::Uint32 x, sf::Uint32 y)
    {
        glCheck(glBindTexture(GL_TEXTURE_2D, texture->getNativeHandle()));
        glCheck(glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16UI, texture->getSize().x, texture->getSize().y, 0, GL_RG_INTEGER, GL_UNSIGNED_SHORT, 0));
        glCheck(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, x, y, GL_RG_INTEGER, GL_UNSIGNED_SHORT, (void*)data.data()));
        glCheck(glBindTexture(GL_TEXTURE_2D, 0));
    }
}

TileMapLayer::TileMapLayer(MessageBus& mb, const tmx::Map::Key&, const sf::Vector2u& chunkResolution)
    : Component         (mb, this),
    m_chunkResolution   (chunkResolution),
    m_opacity           (1.f),
    m_shader            (nullptr)
{

}

//public
void TileMapLayer::entityUpdate(float)
{
    XY_ASSERT(getEntity(), "entity null");
    Entity& entity = *getEntity();

    auto scene = entity.getScene();
    XY_ASSERT(scene, "why you been updated without scene??");

    auto view = scene->getVisibleArea();

    //build list of chunks to render
    m_drawList.clear();
    for (const auto& chunk : m_chunks)
    {
        if (chunk.bounds.intersects(view))
        {
            m_drawList.push_back(&chunk);
        }
    }
}

void TileMapLayer::setTileData(const tmx::TileLayer* layer, const std::vector<tmx::Tileset>& tileSets, const tmx::Map& map, TextureResource& tr, ShaderResource& sr)
{
    XY_ASSERT(layer, "not a valid layer");
    
    const auto& tileIDs = layer->getTiles();
    std::uint32_t maxID = std::numeric_limits<std::uint32_t>::max();
    std::vector<const tmx::Tileset*> usedTileSets;

    //discover which tilesets are used in the layer
    for (auto i = tileSets.rbegin(); i != tileSets.rend(); ++i)
    {
        for (const auto& tile : tileIDs)
        {
            if (tile.ID >= i->getFirstGID() && tile.ID < maxID)
            {
                usedTileSets.push_back(&(*i));
                break;
            }
        }
        maxID = i->getFirstGID();
    }

    //divide layer into chunks
    const auto bounds = map.getBounds();
    sf::Uint32 chunkX = static_cast<sf::Uint32>(std::ceil(bounds.width / m_chunkResolution.x));
    sf::Uint32 chunkY = static_cast<sf::Uint32>(std::ceil(bounds.height / m_chunkResolution.y));
    sf::Vector2f floatRes(m_chunkResolution);

    sf::Uint32 tileXCount = m_chunkResolution.x / map.getTileSize().x;
    sf::Uint32 tileYCount = m_chunkResolution.y / map.getTileSize().y;
    sf::Uint32 rowCount = static_cast<sf::Uint32>(bounds.width / map.getTileSize().x);

    for (auto y = 0u; y < chunkY; ++y)
    {
        for (auto x = 0u; x < chunkX; ++x)
        {
            //add a new chunk
            m_chunks.emplace_back();
            auto& chunk = m_chunks.back();
            chunk.bounds = { x * floatRes.x, y * floatRes.y, floatRes.x, floatRes.y };
            chunk.bounds.left += layer->getOffset().x;
            chunk.bounds.top += layer->getOffset().y;

            float right = std::min(chunk.bounds.left + floatRes.x, map.getBounds().width + map.getBounds().left);
            float bottom = std::min(chunk.bounds.top + floatRes.y, map.getBounds().height + map.getBounds().top);

            chunk.vertices =
            {
                sf::Vertex(sf::Vector2f(chunk.bounds.left, chunk.bounds.top), sf::Vector2f()),
                sf::Vertex(sf::Vector2f(right, chunk.bounds.top), sf::Vector2f(right - chunk.bounds.left, 0.f)),
                sf::Vertex(sf::Vector2f(right, bottom), { right - chunk.bounds.left, bottom - chunk.bounds.top }),
                sf::Vertex(sf::Vector2f(chunk.bounds.left, bottom), sf::Vector2f(0.f, bottom - chunk.bounds.top))
            };
            
            //check each used tileset, and if it's used by this chunk create a lookup texture
            for (const auto ts : usedTileSets)
            {
                if (ts->getTileCount() == 0)
                {
                    Logger::log(ts->getName() + " missing tile count property. This tileset will not be drawn", Logger::Type::Warning);
                }
                
                std::vector<std::uint16_t> pixelData;
                bool tsUsed = false;

                auto chunkSize = rowCount * tileYCount;
                auto chunkStart = (y * (tileYCount * rowCount)) + (x * tileXCount); //find starting tile of this chunk
                
                for (auto rowPos = chunkStart; rowPos < chunkStart + chunkSize; rowPos += rowCount)
                {
                    auto colStart = rowPos;
                    for (auto colPos = colStart; colPos < colStart + tileXCount; ++colPos)
                    {
                        if (colPos < tileIDs.size() && tileIDs[colPos].ID >= ts->getFirstGID() 
                            && tileIDs[colPos].ID < (ts->getFirstGID() + ts->getTileCount()))
                        {
                            //ID belongs to this tile set
                            tsUsed = true;
                            pixelData.push_back(static_cast<std::uint16_t>((tileIDs[colPos].ID - ts->getFirstGID()) + 1)); //red channel - making sure to index relative to the tileset
                            pixelData.push_back(static_cast<std::uint16_t>(tileIDs[colPos].flipFlags)); //green channel
                        }
                        else
                        {
                            //pad with empty space else array will be incorrectly aligned
                            pixelData.push_back(0);
                            pixelData.push_back(0);
                        }
                    }
                }

                //if this tileset is used create the texture data for it
                if (tsUsed)
                {
                    chunk.tileData.emplace_back();
                    auto& tileData = chunk.tileData.back();
                    tileData.lookupTexture = std::make_unique<sf::Texture>();
                    tileData.lookupTexture->create(m_chunkResolution.x, m_chunkResolution.y);
                    setTextureProperties(tileData.lookupTexture.get(), pixelData, tileXCount, tileYCount);

                    //load / assign tileset texture
                    //TODO shader will need to know tileset data such as margin
                    //and offset. Could use a struct uniform ideally
                    tr.setFallbackColour(sf::Color::Cyan);
                    tileData.tileTexture = &tr.get(ts->getImagePath());
                    
                    //set the transparency colour
                    if (ts->hasTransparency())
                    {
                        sf::Image img = tileData.tileTexture->copyToImage();
                        img.createMaskFromColor(ts->getTransparencyColour());
                        tileData.tileTexture->update(img);
                    }
                    tileData.tileCount.x = tileData.tileTexture->getSize().x / ts->getTileSize().x;
                    tileData.tileCount.y = tileData.tileTexture->getSize().y / ts->getTileSize().y;
                    tileData.tileScale.x = static_cast<float>(map.getTileSize().x) / ts->getTileSize().x;
                    tileData.tileScale.y = static_cast<float>(map.getTileSize().y) / ts->getTileSize().y;
                }
            }

            //if this chunk ended up empty, remove it (seems wasteful to process in the first place, but ultimately improves drawing)
            if (chunk.tileData.empty()) m_chunks.pop_back();
        }
    }

    m_opacity = layer->getOpacity();
    m_globalBounds = map.getBounds();
    m_tileSize = map.getTileSize();

    m_shader = &sr.get(Shader::Tilemap);
    XY_ASSERT(m_shader, "failed to load tilemap shader");

    m_renderFunc = [this](sf::RenderTarget& rt, sf::RenderStates states)
    {
        //set states shader to tilemap shader
        states.shader = m_shader;

        //set shader uniforms
        m_shader->setUniform("u_tileSize", sf::Glsl::Vec2(m_tileSize));
        m_shader->setUniform("u_opacity", m_opacity);

        //draw each texture in current list
        for (const auto chunk : m_drawList)
        {
            rt.draw(*chunk, states);
        }
    };
}

void TileMapLayer::setImageData(const tmx::ImageLayer* layer, const tmx::Map&, TextureResource& tr)
{
    auto& texture = tr.get(layer->getImagePath());   
    if (layer->hasTransparency())
    {
        sf::Image img = texture.copyToImage();
        img.createMaskFromColor(layer->getTransparencyColour());
        texture.update(img);
    }
    m_imageSprite.setTexture(texture);
    m_imageSprite.setPosition(sf::Vector2f(layer->getOffset()));

    sf::Color colour = sf::Color::White;
    colour.a = static_cast<sf::Uint8>(255.f * layer->getOpacity());
    m_imageSprite.setColor(colour);

    m_renderFunc = [this](sf::RenderTarget& rt, sf::RenderStates states)
    {
        rt.draw(m_imageSprite);
    };
}

//private
void TileMapLayer::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    m_renderFunc(rt, states);
}

void TileMapLayer::Chunk::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    for (const auto& td : tileData)
    {
        //set shader texture uniforms
        sf::Shader* shader = const_cast<sf::Shader*>(states.shader);
        shader->setUniform("u_lookup", *td.lookupTexture);
        shader->setUniform("u_tileMap", *td.tileTexture);
        shader->setUniform("u_tilesetCount", sf::Glsl::Vec2(td.tileCount));
        shader->setUniform("u_tilesetScale", td.tileScale);

        states.texture = td.lookupTexture.get();
        rt.draw(vertices.data(), vertices.size(), sf::Quads, states);
    }
}