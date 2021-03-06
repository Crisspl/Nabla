#ifndef __IRR_C_BUILTIN_INCLUDER_H_INCLUDED__
#define __IRR_C_BUILTIN_INCLUDER_H_INCLUDED__

#include "irr/asset/IIncluder.h"
#include "CObjectCache.h"

#include "irr/asset/IGLSLEmbeddedIncludeLoader.h"

namespace irr
{
namespace asset
{

class CBuiltinIncluder : public IIncluder
{
        core::smart_refctd_ptr<IGLSLEmbeddedIncludeLoader> m_default;

        using LoadersContainer = core::CMultiObjectCache<std::string, IBuiltinIncludeLoader*>;
        LoadersContainer m_loaders;

        static void loaderGrab(IBuiltinIncludeLoader* _ldr) { _ldr->grab(); }
        static void loaderDrop(IBuiltinIncludeLoader* _ldr) { _ldr->drop(); }

    public:
        CBuiltinIncluder(io::IFileSystem* fs) : m_default(core::make_smart_refctd_ptr<IGLSLEmbeddedIncludeLoader>(fs)), m_loaders(&loaderGrab, &loaderDrop)
        {
            m_searchDirectories.emplace_back("/");
        }

        //! No-op, cannot add search dirs to includer of builtins
        void addSearchDirectory(const std::string& _searchDir) override {}

        void addBuiltinLoader(core::smart_refctd_ptr<IBuiltinIncludeLoader>&& _loader)
        {
            using namespace std::string_literals;
            if (!_loader)
                return;

            m_loaders.insert(std::string(IIncludeHandler::BUILTIN_PREFIX) + _loader->getVirtualDirectoryName(), _loader.get());
        }

    protected:
        std::string getInclude_internal(const std::string& _path) const override
        {
            if (!IIncludeHandler::isBuiltinPath(_path))
                return {};

            const std::string relativePath = _path.substr(strlen(IIncludeHandler::BUILTIN_PREFIX), std::string::npos); // builtin loaders take path relative to PREFIX
            std::string path = _path.substr(0, _path.find_last_of('/')+1);

            std::string res;
            while (path != IIncludeHandler::BUILTIN_PREFIX) // going up the directory tree
            {
                auto capableLoadersRng = m_loaders.findRange(path);
                for (auto& loader : capableLoadersRng)
                {
                    if (!(res = loader.second->getBuiltinInclude(relativePath)).empty())
                        return res;
                }
                if (path.size()==0ull)
                    break;
                path.back() = 'x'; // get rid of trailing slash...
                path.erase(path.find_last_of('/')+1, std::string::npos); // ...and find the one before
            }
            return m_default->getBuiltinInclude(relativePath);
        }
};

}
}

#endif//__IRR_C_BUILTIN_INCLUDER_H_INCLUDED__