/*********************************************************************
Matt Marchant 2014 - 2016
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

#include <xygine/tilemap/ImageLayer.hpp>
#include <xygine/tilemap/FreeFuncs.hpp>
#include <xygine/parsers/pugixml.hpp>

using namespace xy;
using namespace xy::tmx;

ImageLayer::ImageLayer(const std::string& workingDir)
    : m_workingDir      (workingDir)
{

}

//public
void ImageLayer::parse(const pugi::xml_node& node)
{
    std::string attribName = node.name();
    if (attribName != "imagelayer")
    {
        Logger::log("Node not an image layer, node skipped", Logger::Type::Error);
        return;
    }

    for (const auto& child : node.children())
    {
        attribName = child.name();
        if (attribName == "image")
        {
            attribName = node.attribute("source").as_string();
            if (attribName.empty())
            {
                Logger::log("Image Layer has missing source property", Logger::Type::Warning);
                return;
            }
            m_filePath = resolveFilePath(attribName, m_workingDir);
            if (node.attribute("trans"))
            {
                attribName = node.attribute("trans").as_string();
                m_transparencyColour = colourFromString(attribName);
            }
        }
        else if (attribName == "properties")
        {
            for (const auto& p : child.children())
            {
                addProperty(p);
            }
        }
    }
}