#ifndef __IRR_ASSET_I_BUFFER_H_INCLUDED__
#define __IRR_ASSET_I_BUFFER_H_INCLUDED__

#include "irr/core/IBuffer.h"
#include "irr/asset/IDescriptor.h"

namespace irr
{
namespace asset
{

class IBuffer : public core::IBuffer, public IDescriptor
{
	public:
		E_CATEGORY getTypeCategory() const override { return EC_BUFFER; }

	protected:
		IBuffer() = default;
		virtual ~IBuffer() = default;
};

template<class BufferType>
struct SBufferBinding
{
	uint64_t offset = 0ull;
	core::smart_refctd_ptr<BufferType> buffer = nullptr;

	inline bool operator==(const SBufferBinding<BufferType>& rhs) const { return buffer==rhs.buffer && offset==rhs.offset; }
	inline bool operator!=(const SBufferBinding<BufferType>& rhs) const { return !operator==(rhs); }
};

template<typename BufferType>
struct SBufferRange
{
	bool isValid() const
	{
		return buffer && (offset+size<=buffer->getSize());
	}

	size_t offset = 0ull;
	size_t size = 0ull;
	core::smart_refctd_ptr<BufferType> buffer = nullptr;
};

}
}

#endif//__IRR_ASSET_I_BUFFER_H_INCLUDED__