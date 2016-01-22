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

//static functions for cross platform file handling

#ifndef XY_FILE_SYS_HPP_
#define XY_FILE_SYS_HPP_

#include <string>
#include <vector>

namespace xy
{
    /*!
    \brief Utilities for reading files from the current file system
    */
    class FileSystem final
    {
    public:
        /*!
        \brief Lists all the files in the given directory
        */
        static std::vector<std::string> listFiles(std::string path);
        /*!
        \brief Attempts to return a string containing the file extension
        of a given path, including the period (.)
        */
        static std::string getFileExtension(const std::string& path);
        /*!
        \brief Attempts to return the name of a file at the end of
        a given file path
        */
        static std::string getFileName(const std::string& path);

    private:

    };
}
#endif //XY_FILE_SYS_HPP_