// Copyright (C) 2020- Mateusz 'DevSH' Kielan
// This file is part of the "IrrlichtBAW" engine.
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_COMMON_IMAGE_FILTER_KERNELS_H_INCLUDED__
#define __IRR_COMMON_IMAGE_FILTER_KERNELS_H_INCLUDED__


#include "irr/asset/filters/kernels/IImageFilterKernel.h"

#include <ratio>

namespace irr
{
namespace asset
{

// base class for all kernels that require the pixels and arithmetic to be done in precise floats
class CFloatingPointOnlyImageFilterKernelBase
{
	public:
		using value_type = double; // should probably allot `float`s at some point

		static inline bool validate(ICPUImage* inImage, ICPUImage* outImage)
		{
			const auto& inParams = inImage->getCreationParameters();
			const auto& outParams = inImage->getCreationParameters();
			return !(isIntegerFormat(inParams.format)||isIntegerFormat(outParams.format));
		}

	protected:
		CFloatingPointOnlyImageFilterKernelBase() {}
};

// base class for all kernels which can be separated into axis-aligned passes
class CSeparableImageFilterKernelBase
{
	public:
		_IRR_STATIC_INLINE_CONSTEXPR bool is_separable = true;

	protected:
		CSeparableImageFilterKernelBase() {}
};

// base class for all kernels that require pixels and arithmetic to be done in precise floats AND are separable AND have the same kernel function in each dimension AND have a rational support
template<class CRTP>
class CFloatingPointSeparableImageFilterKernelBase : public CImageFilterKernel<CRTP,typename CFloatingPointOnlyImageFilterKernelBase::value_type>, public CFloatingPointOnlyImageFilterKernelBase, public CSeparableImageFilterKernelBase
{
	public:
		using value_type = typename CFloatingPointOnlyImageFilterKernelBase::value_type;
	private:
		using StaticPolymorphicBase = CImageFilterKernel<CRTP,value_type>;

	public:
		CFloatingPointSeparableImageFilterKernelBase(float _negative_support, float _positive_support) : StaticPolymorphicBase({_negative_support,_negative_support,_negative_support},{_positive_support,_positive_support,_positive_support}) {}
		
		// no special user data by default
		inline const IImageFilterKernel::UserData* getUserData() const { return nullptr; }

		//
		template<class PreFilter, class PostFilter>
		struct sample_functor_t
		{
				sample_functor_t(const CRTP* __this, PreFilter& _preFilter, PostFilter& _postFilter) :
					_this(__this), preFilter(_preFilter), postFilter(_postFilter) {}

				inline void operator()(value_type* windowSample, core::vectorSIMDf& relativePos, const core::vectorSIMDi32& globalTexelCoord, const IImageFilterKernel::UserData* userData);

			private:
				const CRTP* _this;
				PreFilter& preFilter;
				PostFilter& postFilter;
		};

		// this is the function that must be defined for each kernel
		template<class PreFilter, class PostFilter>
		inline auto create_sample_functor_t(PreFilter& preFilter, PostFilter& postFilter) const
		{
			return sample_functor_t(static_cast<const CRTP*>(this),preFilter,postFilter);
		}

		// derived classes must declare this
		inline float weight(float x, int32_t channel) const
		{
			return static_cast<const CRTP*>(this)->weight(x,channel);
		}

	protected:
		// utility function so we dont evaluate `weight` function in children outside the support and just are able to return 0.f
		inline bool inDomain(float x) const
		{
			return (-x)<negative_support.x && x<positive_support.x;
		}
};


// base class for all kernels that have the same support in each dimension AND have a rational support
// there's nothing special about having a rational support, we just use that type because there's no possibility of using `float` as a template parameter in C++
template<class Support=std::ratio<1,1> >
class CIsotropicImageFilterKernelBase
{
	public:
		using isotropic_support_as_ratio = Support;
	protected:
		_IRR_STATIC_INLINE_CONSTEXPR float isotropic_support = float(Support::num)/float(Support::den);
};

// same as CFloatingPointSeparableImageFilterKernelBase but with added constraint that support is symmetric around the orign
template<class CRTP,class Support=std::ratio<1,1> >
class CFloatingPointIsotropicSeparableImageFilterKernelBase :	public CFloatingPointSeparableImageFilterKernelBase<CFloatingPointIsotropicSeparableImageFilterKernelBase<CRTP,Support>>,
																public CIsotropicImageFilterKernelBase<Support>
{
		using Base = CFloatingPointSeparableImageFilterKernelBase<CFloatingPointIsotropicSeparableImageFilterKernelBase<CRTP,Support>>;

	public:
		CFloatingPointIsotropicSeparableImageFilterKernelBase() : Base(isotropic_support,isotropic_support) {}

		// need this declared to forward
		inline float weight(float x, int32_t channel) const
		{
			return static_cast<const CRTP*>(this)->weight(x, channel);
		}
};

} // end namespace asset
} // end namespace irr

#endif