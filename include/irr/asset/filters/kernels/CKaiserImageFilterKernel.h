// Copyright (C) 2020- Mateusz 'DevSH' Kielan
// This file is part of the "IrrlichtBAW" engine.
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_C_KAISER_IMAGE_FILTER_KERNEL_H_INCLUDED__
#define __IRR_C_KAISER_IMAGE_FILTER_KERNEL_H_INCLUDED__


#include "irr/asset/filters/kernels/IImageFilterKernel.h"

#include <ratio>

namespace irr
{
namespace asset
{

// Kaiser filter, basically a windowed sinc, can be configured to different Kaiser window widths
template<uint32_t support=3u>
class CKaiserImageFilterKernel : public CFloatingPointIsotropicSeparableImageFilterKernelBase<CKaiserImageFilterKernel<support>,std::ratio<support,1> >
{
		using Base = CFloatingPointIsotropicSeparableImageFilterKernelBase<CKaiserImageFilterKernel<support>,std::ratio<support,1> >;

	public:
		// important constant, do not touch, do not tweak
		_IRR_STATIC_INLINE_CONSTEXPR float alpha = 3.f;

		inline float weight(float x, int32_t channel) const
		{
			if (Base::inDomain(x))
			{
				const auto PI = core::PI<float>();
				return core::sinc(x*PI)*core::KaiserWindow(x,alpha,Base::isotropic_support);
			}
			return 0.f;
		}

		_IRR_STATIC_INLINE_CONSTEXPR bool has_derivative = true;
		inline float d_weight(float x, int32_t channel) const
		{
			if (Base::inDomain(x))
			{
				const auto PIx = core::PI<float>()*x;
				float f = core::sinc(PIx);
				float df = core::PI<float>()*core::d_sinc(PIx);
				float g = core::KaiserWindow(x, alpha, Base::isotropic_support);
				float dg = core::d_KaiserWindow(x, alpha, Base::isotropic_support);
				return df*g+f*dg;
			}
			return 0.f;
		}
};

} // end namespace asset
} // end namespace irr

#endif