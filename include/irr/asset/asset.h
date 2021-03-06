#ifndef __IRR_ASSET_H_INCLUDED__
#define __IRR_ASSET_H_INCLUDED__

// dependencies
#include "irr/system/system.h"

// utils
#include "irr/asset/asset_utils.h"

// format
#include "irr/asset/format/EFormat.h"
#include "irr/asset/format/convertColor.h"
#include "irr/asset/format/decodePixels.h"
#include "irr/asset/format/encodePixels.h"

// base
#include "irr/asset/ICPUBuffer.h"
#include "irr/asset/IAsset.h"
#include "irr/asset/IMesh.h"

// images
#include "irr/asset/ICPUImage.h"
#include "irr/asset/ICPUImageView.h"
// filters
#include "irr/asset/filters/CFillImageFilter.h"
#include "irr/asset/filters/CCopyImageFilter.h"
#include "irr/asset/filters/CPaddedCopyImageFilter.h"
#include "irr/asset/filters/CConvertFormatImageFilter.h"
#include "irr/asset/filters/CSwizzleAndConvertImageFilter.h"
#include "irr/asset/filters/CFlattenRegionsImageFilter.h"
#include "irr/asset/filters/CMipMapGenerationImageFilter.h"

// shaders
#include "irr/asset/ShaderRes.h"
#include "irr/asset/IIncluder.h"
#include "irr/asset/IIncludeHandler.h"
#include "irr/asset/IBuiltinIncludeLoader.h"
#include "irr/asset/IGLSLCompiler.h"
#include "irr/asset/ISPIR_VProgram.h"
#include "irr/asset/ICPUShader.h"
#include "irr/asset/ICPUSpecializedShader.h"
#include "irr/asset/CShaderIntrospector.h"
// pipelines
// meshes
#include "irr/asset/ICPUMeshBuffer.h"
#include "irr/asset/ICPUSkinnedMeshBuffer.h"
#include "irr/asset/ICPUMesh.h"
#include "irr/asset/CCPUMesh.h" // refactor
#include "irr/asset/ICPUSkinnedMesh.h"
#include "irr/asset/CCPUSkinnedMesh.h" // refactor
#include "irr/asset/IGeometryCreator.h"
#include "irr/asset/IMeshPacker.h"
// animated
#include "CFinalBoneHierarchy.h"

// manipulation + reflection + introspection
#include "irr/asset/IMeshManipulator.h"

// baw files
#include "irr/asset/bawformat/CBAWFile.h"
#include "irr/asset/bawformat/CBlobsLoadingManager.h"


// importexport
#include "irr/asset/IAssetLoader.h"
#include "irr/asset/IAssetManager.h"
#include "irr/asset/IAssetWriter.h"
#include "irr/asset/COpenEXRImageMetadata.h"
#include "irr/asset/CMTLPipelineMetadata.h"
#include "irr/asset/CPLYPipelineMetadata.h"
#include "irr/asset/CSTLPipelineMetadata.h"

//VT
#include "irr/asset/IVirtualTexture.h"
#include "irr/asset/ICPUVirtualTexture.h"

#endif
