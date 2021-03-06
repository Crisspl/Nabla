#ifndef _IRR_BUILTIN_COMMON_H_INCLUDED_
#define _IRR_BUILTIN_COMMON_H_INCLUDED_

#include "BuildConfigOptions.h"

#include <cstring>

namespace irr
{
namespace builtin
{

// if you attempt to use this without `IRR_EMBED_BUILTIN_RESOURCES_` CMake option, you will get loads of undefined references
template<typename StringUniqueLiteralType>
const std::pair<const uint8_t*, size_t> get_resource();

// if you attempt to use this without `IRR_EMBED_BUILTIN_RESOURCES_` CMake option, this will always return `{nullptr,0ull}`
std::pair<const uint8_t*,size_t> get_resource_runtime(const std::string&);

}
}

#ifndef _IRR_EMBED_BUILTIN_RESOURCES_
// will only be available in the app or library using irrlicht
#if defined(_IRR_STATIC_LIB_) || !defined(IRRLICHT_EXPORTS)

#define _IRR_BUILTIN_PATH_AVAILABLE
namespace irr
{
namespace builtin
{

// TODO: move this compile time string stuff somewhere relevant
namespace impl
{
    template<std::size_t Pos, std::size_t Len, std::size_t N>
    constexpr std::size_t substrlen()
    {
        constexpr auto PossibleLen = Pos<N ? (N-Pos):0u;
        return Len<PossibleLen ? Len:PossibleLen;
    }

    template <typename C, std::size_t N, std::size_t...Is>
    constexpr std::array<C,sizeof...(Is)+1ull> truncate(const C* s, std::index_sequence<Is...>)
    {
	    return {(Is < N ? s[Is] : 0)..., 0};
    }
}

template<typename C, std::size_t N>
constexpr std::size_t strlen(const C(&s)[N])
{
    return N;
}

template<std::size_t Pos, std::size_t Len, typename C, std::size_t N>
constexpr std::array<C,impl::substrlen<Pos,Len,N>()+1ull> substr(const C(&s)[N])
{
    constexpr auto len = impl::substrlen<Pos,Len,N>();
    return impl::truncate<C,len>(s+Pos,std::make_index_sequence<len>{});
}

inline std::string getBuiltinResourcesDirectoryPath()
{
    return builtin::substr<0ull,builtin::strlen(__FILE__)-builtin::strlen("common.h")>(__FILE__).data();
}

}
}
#endif
#endif

#endif