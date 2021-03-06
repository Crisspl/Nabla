#ifndef __IRR_C_INCLUDE_HANDLER_H_INCLUDED__
#define __IRR_C_INCLUDE_HANDLER_H_INCLUDED__

#include "irr/asset/IIncludeHandler.h"

#include "CFilesystemIncluder.h"
#include "CBuiltinIncluder.h"

namespace irr
{
namespace asset
{

class CIncludeHandler : public IIncludeHandler
{
    using SmartRefctIncluderPtr = core::smart_refctd_ptr<IIncluder>;

    core::vector<SmartRefctIncluderPtr> m_includers;

    enum E_INCLUDER_INDEX
    {
        EII_FILESYSTEM = 0,
        EII_BUILTIN = 1
    };

public:
    CIncludeHandler(io::IFileSystem* _filesystem)
    {
        // TODO It has to be reworked in the future
        m_includers.emplace_back(core::make_smart_refctd_ptr<CFilesystemIncluder>(_filesystem));
        m_includers.emplace_back(core::make_smart_refctd_ptr<CBuiltinIncluder>(_filesystem));
    }

    std::string getIncludeStandard(const std::string& _path) const override
    {
        return getIncluderDependentOnPath(_path)->getIncludeStandard(_path);
    }

    std::string getIncludeRelative(const std::string& _path, const std::string& _workingDirectory) const override
    {
        return getIncluderDependentOnPath(_path)->getIncludeRelative(_path, _workingDirectory);
    }

    void addBuiltinIncludeLoader(core::smart_refctd_ptr<IBuiltinIncludeLoader>&& _inclLoader) override
    {
        static_cast<CBuiltinIncluder*>(m_includers[EII_BUILTIN].get())->addBuiltinLoader(std::move(_inclLoader));
    }

private:
    const IIncluder* getIncluderDependentOnPath(const std::string& _path) const
    {
        return (isBuiltinPath(_path) ? m_includers[EII_BUILTIN].get() : m_includers[EII_FILESYSTEM].get());
    }
};

}
}

#endif//__IRR_C_INCLUDE_HANDLER_H_INCLUDED__
