// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_COMPILE_CONFIG_H_INCLUDED__
#define __IRR_COMPILE_CONFIG_H_INCLUDED__

//! Irrlicht SDK Version
#define IRRLICHTBAW_VERSION_MAJOR 0
#define IRRLICHTBAW_VERSION_MINOR 3
#define IRRLICHTBAW_VERSION_REVISION 0
// This flag will be defined only in SVN, the official release code will have
// it undefined
//#define IRRLICHT_VERSION_SVN -alpha
#define IRRLICHTBAW_SDK_VERSION "0.3.0-beta2"

#define NEW_MESHES

#include <stdio.h> // TODO: Although included elsewhere this is required at least for mingw

//! Passing defines to the compiler which have NO in front of the _IRR definename is an alternative
//! way which can be used to disable defines (instead of outcommenting them in this header).
//! So defines can be controlled from Makefiles or Projectfiles which allows building
//! different library versions without having to change the sources.
//! Example: NO_IRR_COMPILE_WITH_X11_ would disable X11


//! Uncomment this line to compile with the SDL device
//#define _IRR_COMPILE_WITH_SDL_DEVICE_
#ifdef NO_IRR_COMPILE_WITH_SDL_DEVICE_
#undef _IRR_COMPILE_WITH_SDL_DEVICE_
#endif

// this actually includes file depending on build type (Debug/Release)
#include "BuildConfigOptions.h"

#define NEW_SHADERS 1

#ifdef _IRR_TARGET_ARCH_ARM_
#   define __IRR_COMPILE_WITH_ARM_SIMD_ // NEON
#else // target arch x86
#   define __IRR_COMPILE_WITH_SSE3
#   define __IRR_COMPILE_WITH_X86_SIMD_ // SSE 4.2 
#   include <immintrin.h>
#endif


#if defined(_IRR_SERVER_)
#   define NO_IRR_COMPILE_WITH_VULKAN_
#   define NO_IRR_COMPILE_WITH_OPENGL_
#endif

#ifdef NO_IRR_COMPILE_WITH_OPENGL_
#   undef _IRR_COMPILE_WITH_OPENGL_
#endif

// The Windows platform and API support SDL and WINDOW device
#if defined(_IRR_PLATFORM_WINDOWS_)
#   define _IRR_WINDOWS_API_
#   define _IRR_COMPILE_WITH_WINDOWS_DEVICE_
#   if defined(_MSC_VER) && (_MSC_VER < 1300)
#       error "Only Microsoft Visual Studio 7.0 and later are supported."
#   endif
#endif

#if defined(_IRR_PLATFORM_LINUX_)
#   define _IRR_POSIX_API_
#   define _IRR_COMPILE_WITH_X11_DEVICE_
#endif

#ifdef _IRR_SERVER_
#   define NO_IRR_LINUX_X11_RANDR_
#endif

#ifdef _MSC_VER
#	define _ENABLE_EXTENDED_ALIGNED_STORAGE
#endif

//! VidMode is ANCIENT
//#define NO_IRR_LINUX_X11_VIDMODE_

//! On some Linux systems the XF86 vidmode extension or X11 RandR are missing. Use these flags
//! to remove the dependencies such that Irrlicht will compile on those systems, too.
//! If you don't need colored cursors you can also disable the Xcursor extension
#if defined(_IRR_PLATFORM_LINUX_) && defined(_IRR_COMPILE_WITH_X11_)
#   define _IRR_LINUX_X11_VIDMODE_
#   define _IRR_LINUX_X11_RANDR_
#   ifdef NO_IRR_LINUX_X11_VIDMODE_
#       undef _IRR_LINUX_X11_VIDMODE_
#   endif
#   ifdef NO_IRR_LINUX_X11_RANDR_
#       undef _IRR_LINUX_X11_RANDR_
#   endif
#endif

//! Define _IRR_COMPILE_WITH_X11_ to compile the Irrlicht engine with X11 support.
/** If you do not wish the engine to be compiled with X11, comment this
define out. */
// Only used in LinuxDevice.
///#ifndef _IRR_SERVER_
#define _IRR_COMPILE_WITH_X11_
///#endif
#ifdef NO_IRR_COMPILE_WITH_X11_
#   undef _IRR_COMPILE_WITH_X11_
#endif

//! Define _IRR_COMPILE_WITH_OPENSSL_ to enable compiling the engine using libjpeg.
/** This enables the engine to read and write encrypted BAW format files.
If you comment this out, the engine will no longer read or write encrypted .baw files! */
#define _IRR_COMPILE_WITH_OPENSSL_
#ifdef NO_IRR_COMPILE_WITH_OPENSSL_
#undef _IRR_COMPILE_WITH_OPENSSL_
#endif

//! Define _IRR_COMPILE_WITH_JPEGLIB_ to enable compiling the engine using libjpeg.
/** This enables the engine to read jpeg images. If you comment this out,
the engine will no longer read .jpeg images. */
#define _IRR_COMPILE_WITH_LIBJPEG_
#ifdef NO_IRR_COMPILE_WITH_LIBJPEG_
#undef _IRR_COMPILE_WITH_LIBJPEG_
#endif

//! Define _IRR_COMPILE_WITH_LIBPNG_ to enable compiling the engine using libpng.
/** This enables the engine to read png images. If you comment this out,
the engine will no longer read .png images. */
#define _IRR_COMPILE_WITH_LIBPNG_
#ifdef NO_IRR_COMPILE_WITH_LIBPNG_
#undef _IRR_COMPILE_WITH_LIBPNG_
#endif

//! Uncomment the following line if you want to ignore the deprecated warnings
//#define IGNORE_DEPRECATED_WARNING

//! Define __IRR_COMPILE_WITH_ZIP_ARCHIVE_LOADER_ if you want to open ZIP and GZIP archives
/** ZIP reading has several more options below to configure. */
#define __IRR_COMPILE_WITH_ZIP_ARCHIVE_LOADER_
#ifdef NO__IRR_COMPILE_WITH_ZIP_ARCHIVE_LOADER_
#undef __IRR_COMPILE_WITH_ZIP_ARCHIVE_LOADER_
#endif
#ifdef __IRR_COMPILE_WITH_ZIP_ARCHIVE_LOADER_
//! Define _IRR_COMPILE_WITH_ZLIB_ to enable compiling the engine using zlib.
/** This enables the engine to read from compressed .zip archives. If you
disable this feature, the engine can still read archives, but only uncompressed
ones. */
#define _IRR_COMPILE_WITH_ZLIB_
#ifdef NO_IRR_COMPILE_WITH_ZLIB_
#undef _IRR_COMPILE_WITH_ZLIB_
#endif
//! Define _IRR_COMPILE_WITH_ZIP_ENCRYPTION_ if you want to read AES-encrypted ZIP archives
#define _IRR_COMPILE_WITH_ZIP_ENCRYPTION_
#ifdef NO_IRR_COMPILE_WITH_ZIP_ENCRYPTION_
#undef _IRR_COMPILE_WITH_ZIP_ENCRYPTION_
#endif
//! Define _IRR_COMPILE_WITH_BZIP2_ if you want to support bzip2 compressed zip archives
/** bzip2 is superior to the original zip file compression modes, but requires
a certain amount of memory for decompression and adds several files to the
library. */
#define _IRR_COMPILE_WITH_BZIP2_
#ifdef NO_IRR_COMPILE_WITH_BZIP2_
#undef _IRR_COMPILE_WITH_BZIP2_
#endif
//! Define _IRR_COMPILE_WITH_LZMA_ if you want to use LZMA compressed zip files.
/** LZMA is a very efficient compression code, known from 7zip. Irrlicht
currently only supports zip archives, though. */
//#define _IRR_COMPILE_WITH_LZMA_
#ifdef NO_IRR_COMPILE_WITH_LZMA_
#undef _IRR_COMPILE_WITH_LZMA_
#endif
#endif

//! Define __IRR_COMPILE_WITH_MOUNT_ARCHIVE_LOADER_ if you want to mount folders as archives
#define __IRR_COMPILE_WITH_MOUNT_ARCHIVE_LOADER_
#ifdef NO__IRR_COMPILE_WITH_MOUNT_ARCHIVE_LOADER_
#undef __IRR_COMPILE_WITH_MOUNT_ARCHIVE_LOADER_
#endif
//! Define __IRR_COMPILE_WITH_PAK_ARCHIVE_LOADER_ if you want to open ID software PAK archives
#define __IRR_COMPILE_WITH_PAK_ARCHIVE_LOADER_
#ifdef NO__IRR_COMPILE_WITH_PAK_ARCHIVE_LOADER_
#undef __IRR_COMPILE_WITH_PAK_ARCHIVE_LOADER_
#endif
//! Define __IRR_COMPILE_WITH_TAR_ARCHIVE_LOADER_ if you want to open TAR archives
#define __IRR_COMPILE_WITH_TAR_ARCHIVE_LOADER_
#ifdef NO__IRR_COMPILE_WITH_TAR_ARCHIVE_LOADER_
#undef __IRR_COMPILE_WITH_TAR_ARCHIVE_LOADER_
#endif

// Some cleanup and standard stuff

#ifdef _IRR_WINDOWS_API_

// To build Irrlicht as a static library, you must define _IRR_STATIC_LIB_ in both the
// Irrlicht build, *and* in the user application, before #including <irrlicht.h>
#ifndef _IRR_STATIC_LIB_
#ifdef IRRLICHT_EXPORTS
#define IRRLICHT_API __declspec(dllexport)
#else
#define IRRLICHT_API __declspec(dllimport)
#endif // IRRLICHT_EXPORT
#else
#define IRRLICHT_API
#endif // _IRR_STATIC_LIB_

// Declare the calling convention.
#if defined(_STDCALL_SUPPORTED)
#define IRRCALLCONV __stdcall
#else
#define IRRCALLCONV __cdecl
#endif // STDCALL_SUPPORTED

#else // _IRR_WINDOWS_API_

// Force symbol export in shared libraries built with gcc.
#if (__GNUC__ >= 4) && !defined(_IRR_STATIC_LIB_) && defined(IRRLICHT_EXPORTS)
#define IRRLICHT_API __attribute__ ((visibility("default")))
#else
#define IRRLICHT_API
#endif

#define IRRCALLCONV

#endif // _IRR_WINDOWS_API_

#ifndef _IRR_WINDOWS_API_
#   undef _IRR_WCHAR_FILESYSTEM
#endif


#define _IRR_BAW_FORMAT_VERSION 3

//! @see @ref CBlobsLoadingManager
#define _IRR_ADD_BLOB_SUPPORT(BlobClassName, EnumValue, Function, ...) \
case asset::Blob::EnumValue:\
	return asset::BlobClassName::Function(__VA_ARGS__);

//! Used inside CBlobsLoadingManager. Adds support of given blob types.
// @crisspl / @Anastazluk fix this shit for new pipeline !!!
#define _IRR_SUPPORTED_BLOBS(Function, ...) \
_IRR_ADD_BLOB_SUPPORT(RawBufferBlobV3, EBT_RAW_DATA_BUFFER, Function, __VA_ARGS__)\
_IRR_ADD_BLOB_SUPPORT(TexturePathBlobV3, EBT_TEXTURE_PATH, Function, __VA_ARGS__)\
_IRR_ADD_BLOB_SUPPORT(MeshBlobV3, EBT_MESH, Function, __VA_ARGS__)\
_IRR_ADD_BLOB_SUPPORT(SkinnedMeshBlobV3, EBT_SKINNED_MESH, Function, __VA_ARGS__)\
_IRR_ADD_BLOB_SUPPORT(MeshBufferBlobV3, EBT_MESH_BUFFER, Function, __VA_ARGS__)\
_IRR_ADD_BLOB_SUPPORT(SkinnedMeshBufferBlobV3, EBT_SKINNED_MESH_BUFFER, Function, __VA_ARGS__)\
_IRR_ADD_BLOB_SUPPORT(MeshDataFormatDescBlobV3, EBT_DATA_FORMAT_DESC, Function, __VA_ARGS__)\
_IRR_ADD_BLOB_SUPPORT(FinalBoneHierarchyBlobV3, EBT_FINAL_BONE_HIERARCHY, Function, __VA_ARGS__)

#endif // __IRR_COMPILE_CONFIG_H_INCLUDED__
