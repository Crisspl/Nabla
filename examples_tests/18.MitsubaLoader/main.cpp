#define _IRR_STATIC_LIB_
#include <irrlicht.h>

#include "../../ext/ScreenShot/ScreenShot.h"

#include "../common/QToQuitEventReceiver.h"

#include "../3rdparty/portable-file-dialogs/portable-file-dialogs.h"
#include "../../ext/MitsubaLoader/CMitsubaLoader.h"
#include <irr/video/IGPUVirtualTexture.h>

#define USE_ENVMAP

using namespace irr;
using namespace core;

constexpr const char* GLSL_COMPUTE_LIGHTING =
R"(
#define _IRR_COMPUTE_LIGHTING_DEFINED_

#include <irr/builtin/glsl/format/decode.glsl>
#include <irr/builtin/glsl/random/xoroshiro.glsl>

struct SLight
{
	vec3 position;
	vec3 intensity;
};

layout (set = 2, binding = 0, std430) readonly restrict buffer Lights
{
	SLight lights[];
};
layout(set = 2, binding = 1) uniform sampler2D envMap;
layout(set = 2, binding = 2) uniform usamplerBuffer sampleSequence;
layout(set = 2, binding = 3) uniform usampler2D scramblebuf;

vec2 rand2d(in uint _sample, inout irr_glsl_xoroshiro64star_state_t scramble_state)
{
	uvec2 seqVal = texelFetch(sampleSequence,int(_sample)).xy;
	seqVal ^= uvec2(irr_glsl_xoroshiro64star(scramble_state),irr_glsl_xoroshiro64star(scramble_state));
    return vec2(seqVal)*uintBitsToFloat(0x2f800004u);
}

vec2 SampleSphericalMap(in vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= irr_glsl_RECIPROCAL_PI*0.5;
    uv += 0.5; 
    return uv;
}

vec3 irr_computeLighting(inout irr_glsl_IsotropicViewSurfaceInteraction out_interaction, in mat2 dUV)
{
	irr_glsl_xoroshiro64star_state_t scramble_start_state = textureLod(scramblebuf,gl_FragCoord.xy/VIEWPORT_SZ,0).rg;

	vec3 emissive = irr_glsl_decodeRGB19E7(InstData.data[InstanceIndex].emissive);

	vec3 color = vec3(0.0);

#ifdef USE_ENVMAP
	instr_stream_t gcs = getGenChoiceStream();
	for (int i = 0; i < SAMPLE_COUNT; ++i)
	{
		irr_glsl_xoroshiro64star_state_t scramble_state = scramble_start_state;

		instr_stream_t gcs = getGenChoiceStream();
		instr_stream_t rnps = getRemAndPdfStream();

		vec2 rand = rand2d(i,scramble_state);//TODO has to be 3d
		float pdf;
		runGenerateAndRemainderStream(gcs, rnps, rand, pdf);

		vec2 uv = SampleSphericalMap(L);
		color += rem*textureLod(envMap, uv, 0.0).xyz;
	}
	color /= float(SAMPLE_COUNT);
#endif

	irr_glsl_BSDFIsotropicParams params;
	for (int i = 0; i < LIGHT_COUNT; ++i)
	{
		SLight l = lights[i];
		vec3 L = l.position-WorldPos;
		params.L = L;
		const float intensityScale = LIGHT_INTENSITY_SCALE;//ehh might want to render to hdr fbo and do tonemapping
		color += irr_bsdf_cos_eval(params, out_interaction, dUV)*l.intensity*intensityScale / dot(L,L);
	}

	return color+emissive;
}
)";
static core::smart_refctd_ptr<asset::ICPUSpecializedShader> createModifiedFragShader(const asset::ICPUSpecializedShader* _fs, uint32_t viewport_w, uint32_t viewport_h, uint32_t lightCnt, uint32_t smplCnt, float intensityScale)
{
    const asset::ICPUShader* unspec = _fs->getUnspecialized();
    assert(unspec->containsGLSL());

    std::string glsl = reinterpret_cast<const char*>( unspec->getSPVorGLSL()->getPointer() );
	std::string extra = "\n#define VIEWPORT_SZ vec2(" + std::to_string(viewport_w)+","+std::to_string(viewport_h)+")" +
		"\n#define LIGHT_COUNT " + std::to_string(lightCnt) +
		"\n#define SAMPLE_COUNT " + std::to_string(smplCnt) +
		"\n#define LIGHT_INTENSITY_SCALE " + std::to_string(intensityScale) +
#ifdef USE_ENVMAP
		"\n#define USE_ENVMAP" +
#endif
		GLSL_COMPUTE_LIGHTING;

    glsl.insert(glsl.find("#ifndef _IRR_COMPUTE_LIGHTING_DEFINED_"), extra);

    //auto* f = fopen("fs.glsl","w");
    //fwrite(glsl.c_str(), 1, glsl.size(), f);
    //fclose(f);

    auto unspecNew = core::make_smart_refctd_ptr<asset::ICPUShader>(glsl.c_str());
    auto specinfo = _fs->getSpecializationInfo();//intentional copy
    auto fsNew = core::make_smart_refctd_ptr<asset::ICPUSpecializedShader>(std::move(unspecNew), std::move(specinfo));

    return fsNew;
}

static auto createGPUImageView(const std::string& path, asset::IAssetManager* am, video::IVideoDriver* driver)
{
	using namespace asset;
	using namespace video;

	IAssetLoader::SAssetLoadParams lp(0ull, nullptr, IAssetLoader::ECF_DONT_CACHE_REFERENCES);
	auto cpuTexture = am->getAsset(path, lp);
	auto cpuTextureContents = cpuTexture.getContents();

	auto asset = *cpuTextureContents.begin();
	auto cpuimg = core::smart_refctd_ptr_static_cast<asset::ICPUImage>(asset);

	IGPUImageView::SCreationParams viewParams;
	viewParams.flags = static_cast<IGPUImageView::E_CREATE_FLAGS>(0u);
	viewParams.image = driver->getGPUObjectsFromAssets(&cpuimg.get(),&cpuimg.get()+1u)->front();
	viewParams.format = viewParams.image->getCreationParameters().format;
	viewParams.viewType = IImageView<IGPUImage>::ET_2D;
	viewParams.subresourceRange.baseArrayLayer = 0u;
	viewParams.subresourceRange.layerCount = 1u;
	viewParams.subresourceRange.baseMipLevel = 0u;
	viewParams.subresourceRange.levelCount = viewParams.image->getCreationParameters().mipLevels;

	auto gpuImageView = driver->createGPUImageView(std::move(viewParams));

	return gpuImageView;
};

#include "irr/irrpack.h"
//std430-compatible
struct SLight
{
	core::vectorSIMDf position;
	core::vectorSIMDf intensity;
} PACK_STRUCT;
#include "irr/irrunpack.h"

int main()
{
	// create device with full flexibility over creation parameters
	// you can add more parameters if desired, check irr::SIrrlichtCreationParameters
	irr::SIrrlichtCreationParameters params;
	params.Bits = 24; //may have to set to 32bit for some platforms
	params.ZBufferBits = 24; //we'd like 32bit here
	params.DriverType = video::EDT_NULL;
	params.Fullscreen = false;
	params.Vsync = false;
	params.Doublebuffer = true;
	params.Stencilbuffer = false; //! This will not even be a choice soon

	//
	asset::SAssetBundle meshes;
	core::smart_refctd_ptr<ext::MitsubaLoader::CGlobalMitsubaMetadata> globalMeta;
	{
		auto device = createDeviceEx(params);


		io::IFileSystem* fs = device->getFileSystem();
		asset::IAssetManager* am = device->getAssetManager();
		asset::CQuantNormalCache* qnc = am->getMeshManipulator()->getQuantNormalCache();

		am->addAssetLoader(core::make_smart_refctd_ptr<irr::ext::MitsubaLoader::CSerializedLoader>(am));
		am->addAssetLoader(core::make_smart_refctd_ptr<irr::ext::MitsubaLoader::CMitsubaLoader>(am));

		std::string filePath = "../../media/mitsuba/staircase2.zip";
	//#define MITSUBA_LOADER_TESTS
	#ifndef MITSUBA_LOADER_TESTS
		pfd::message("Choose file to load", "Choose mitsuba XML file to load or ZIP containing an XML. \nIf you cancel or choosen file fails to load staircase will be loaded.", pfd::choice::ok);
		pfd::open_file file("Choose XML or ZIP file", "../../media/mitsuba", { "ZIP files (.zip)", "*.zip", "XML files (.xml)", "*.xml"});
		if (!file.result().empty())
			filePath = file.result()[0];
	#endif
		if (core::hasFileExtension(io::path(filePath.c_str()), "zip", "ZIP"))
		{
			io::IFileArchive* arch = nullptr;
			device->getFileSystem()->addFileArchive(filePath.c_str(),io::EFAT_ZIP,"",&arch);
			if (!arch)
				device->getFileSystem()->addFileArchive("../../media/mitsuba/staircase2.zip", io::EFAT_ZIP, "", &arch);
			if (!arch)
				return 2;

			auto flist = arch->getFileList();
			if (!flist)
				return 3;
			auto files = flist->getFiles();

			for (auto it=files.begin(); it!=files.end(); )
			{
				if (core::hasFileExtension(it->FullName, "xml", "XML"))
					it++;
				else
					it = files.erase(it);
			}
			if (files.size() == 0u)
				return 4;

			std::cout << "Choose File (0-" << files.size() - 1ull << "):" << std::endl;
			for (auto i = 0u; i < files.size(); i++)
				std::cout << i << ": " << files[i].FullName.c_str() << std::endl;
			uint32_t chosen = 0;
	#ifndef MITSUBA_LOADER_TESTS
			std::cin >> chosen;
	#endif
			if (chosen >= files.size())
				chosen = 0u;

			filePath = files[chosen].FullName.c_str();
		}

		//! read cache results -- speeds up mesh generation
		//qnc->loadNormalQuantCacheFromFile<asset::E_QUANT_NORM_CACHE_TYPE::Q_2_10_10_10>(fs, "../../tmp/normalCache101010.sse", true);
		//! load the mitsuba scene
		meshes = am->getAsset(filePath, {});
		//! cache results -- speeds up mesh generation on second run
		qnc->saveCacheToFile(asset::CQuantNormalCache::E_CACHE_TYPE::ECT_2_10_10_10, fs, "../../tmp/normalCache101010.sse");
		
		auto contents = meshes.getContents();
		if (contents.begin()>=contents.end())
			return 2;

		auto firstmesh = *contents.begin();
		if (!firstmesh)
			return 3;

		auto meta = firstmesh->getMetadata();
		if (!meta)
			return 4;
		assert(core::strcmpi(meta->getLoaderName(),ext::MitsubaLoader::IMitsubaMetadata::LoaderName) == 0);
		globalMeta = static_cast<ext::MitsubaLoader::IMeshMetadata*>(meta)->globalMetadata;
	}


	// recreate wth resolution
	params.WindowSize = dimension2d<uint32_t>(1280, 720);
	// set resolution
	if (globalMeta->sensors.size())
	{
		const auto& film = globalMeta->sensors.front().film;
		params.WindowSize.Width = film.width;
		params.WindowSize.Height = film.height;
	}
	params.DriverType = video::EDT_OPENGL;
	auto device = createDeviceEx(params);

	if (!device)
		return 1; // could not create selected driver.


	scene::ISceneManager* smgr = device->getSceneManager();
	QToQuitEventReceiver receiver;
	device->setEventReceiver(&receiver);


	video::IVideoDriver* driver = device->getVideoDriver();
	asset::IAssetManager* am = device->getAssetManager();

	core::smart_refctd_ptr<asset::ICPUDescriptorSetLayout> ds2layout;
	{
		asset::ICPUDescriptorSetLayout::SBinding bnd[4];
		bnd[0].binding = 0u;
		bnd[0].count = 1u;
		bnd[0].samplers = nullptr;
		bnd[0].stageFlags = asset::ISpecializedShader::ESS_FRAGMENT;
		bnd[0].type = asset::EDT_STORAGE_BUFFER;

		using namespace asset;
		ISampler::SParams samplerParams = { ISampler::ETC_CLAMP_TO_EDGE, ISampler::ETC_CLAMP_TO_EDGE, ISampler::ETC_CLAMP_TO_EDGE, ISampler::ETBC_FLOAT_OPAQUE_BLACK, ISampler::ETF_LINEAR, ISampler::ETF_LINEAR, ISampler::ESMM_LINEAR, 0u, false, ECO_ALWAYS };
		auto smplr = core::make_smart_refctd_ptr<asset::ICPUSampler>(samplerParams);
		bnd[1].binding = 1u;
		bnd[1].count = 1u;
		bnd[1].samplers = &smplr;
		bnd[1].stageFlags = asset::ISpecializedShader::ESS_FRAGMENT;
		bnd[1].type = asset::EDT_COMBINED_IMAGE_SAMPLER;

		bnd[2].binding = 2u;
		bnd[2].count = 1u;
		bnd[2].samplers = nullptr;
		bnd[2].stageFlags = asset::ISpecializedShader::ESS_FRAGMENT;
		bnd[2].type = asset::EDT_UNIFORM_TEXEL_BUFFER;

		samplerParams = { ISampler::ETC_CLAMP_TO_EDGE, ISampler::ETC_CLAMP_TO_EDGE, ISampler::ETC_CLAMP_TO_EDGE, ISampler::ETBC_INT_OPAQUE_BLACK, ISampler::ETF_NEAREST, ISampler::ETF_NEAREST, ISampler::ESMM_NEAREST, 0u, false, ECO_ALWAYS };
		auto smplr_int = core::make_smart_refctd_ptr<asset::ICPUSampler>(samplerParams);
		bnd[3].binding = 3u;
		bnd[3].count = 1u;
		bnd[3].samplers = &smplr_int;
		bnd[3].stageFlags = asset::ISpecializedShader::ESS_FRAGMENT;
		bnd[3].type = asset::EDT_COMBINED_IMAGE_SAMPLER;

		ds2layout = core::make_smart_refctd_ptr<asset::ICPUDescriptorSetLayout>(bnd, bnd + 4);
	}

	//gather all meshes into core::vector and modify their pipelines
	core::vector<core::smart_refctd_ptr<asset::ICPUMesh>> cpumeshes;
	cpumeshes.reserve(meshes.getSize());
	for (auto it = meshes.getContents().begin(); it != meshes.getContents().end(); ++it)
	{
		cpumeshes.push_back(core::smart_refctd_ptr_static_cast<asset::ICPUMesh>(std::move(*it)));
	}

	core::smart_refctd_ptr<asset::ICPUDescriptorSet> cpuds0;
	{
		auto* meta = static_cast<ext::MitsubaLoader::CMitsubaPipelineMetadata*>(cpumeshes[0]->getMeshBuffer(0)->getPipeline()->getMetadata());
		cpuds0 = core::smart_refctd_ptr<asset::ICPUDescriptorSet>(meta->getDescriptorSet());
	}

	//all pipelines have the same metadata
	auto* pipelineMetadata = static_cast<const asset::IPipelineMetadata*>(cpumeshes.front()->getMeshBuffer(0u)->getPipeline()->getMetadata());

    asset::ICPUDescriptorSetLayout* ds1layout = cpumeshes.front()->getMeshBuffer(0u)->getPipeline()->getLayout()->getDescriptorSetLayout(1u);
    uint32_t ds1UboBinding = 0u;
    for (const auto& bnd : ds1layout->getBindings())
        if (bnd.type==asset::EDT_UNIFORM_BUFFER)
        {
            ds1UboBinding = bnd.binding;
            break;
        }


	//point lights
	core::vector<SLight> lights;
	core::vector<const ext::MitsubaLoader::IMeshMetadata*> meshmetas;
	meshmetas.reserve(cpumeshes.size());
	for (const auto& cpumesh : cpumeshes)
	{
		meshmetas.push_back(static_cast<const ext::MitsubaLoader::IMeshMetadata*>(cpumesh->getMetadata()));
		const auto& instances = meshmetas.back()->getInstances();

		auto computeAreaAndAvgPos = [](asset::ICPUMeshBuffer* mb, const core::matrix3x4SIMD& tform, core::vectorSIMDf& _outAvgPos) {
			uint32_t triCount = 0u;
			asset::IMeshManipulator::getPolyCount(triCount, mb);
			assert(triCount>0u);

			_outAvgPos = core::vectorSIMDf(0.f);

			core::vectorSIMDf v[3];
			for (uint32_t i = 0u; i < triCount; ++i)
			{
				auto triIxs = asset::IMeshManipulator::getTriangleIndices(mb, i);
				for (uint32_t j = 0u; j < 3u; ++j) {
					v[j] = mb->getPosition(triIxs[j]);
					_outAvgPos += v[j];
				}
			}
			core::vectorSIMDf differentialElementCrossProdcut = core::cross(v[1]-v[0], v[2]-v[0]);
			core::matrix3x4SIMD tformCofactors;
			{
				auto tmp4 = irr::core::matrix4SIMD(tform.getSub3x3TransposeCofactors());
				tformCofactors = core::transpose(tmp4).extractSub3x4();
			}
			tformCofactors.mulSub3x3WithNx1(differentialElementCrossProdcut);

			//outputs position in model space
			_outAvgPos /= static_cast<float>(triCount)*3.f;

			return 0.5f*core::length(differentialElementCrossProdcut).x;
		};
		for (const auto& inst : instances)
		{
			if (inst.emitter.type==ext::MitsubaLoader::CElementEmitter::AREA)
			{
				core::vectorSIMDf pos;
				assert(cpumesh->getMeshBufferCount()==1u);
				const float area = computeAreaAndAvgPos(cpumesh->getMeshBuffer(0), inst.tform, pos);
				assert(area>0.f);
				inst.tform.pseudoMulWith4x1(pos);

				SLight l;
				l.intensity = inst.emitter.area.radiance*area*2.f*core::PI<float>();
				l.position = pos;

				lights.push_back(l);
			}
		}
	}

	constexpr uint32_t ENVMAP_SAMPLE_COUNT = 64u;
	constexpr float LIGHT_INTENSITY_SCALE = 0.01f;

	core::unordered_set<const asset::ICPURenderpassIndependentPipeline*> modifiedPipelines;
	core::unordered_map<core::smart_refctd_ptr<asset::ICPUSpecializedShader>, core::smart_refctd_ptr<asset::ICPUSpecializedShader>> modifiedShaders;
	for (auto& mesh : cpumeshes)
	{
		//modify pipeline layouts with our custom DS2 layout (DS2 will be used for lights buffer)
		for (uint32_t i = 0u; i < mesh->getMeshBufferCount(); ++i)
		{
			auto* pipeline = mesh->getMeshBuffer(i)->getPipeline();
			if (modifiedPipelines.find(pipeline) == modifiedPipelines.end())
			{
				//if (!pipeline->getLayout()->getDescriptorSetLayout(2u))
				pipeline->getLayout()->setDescriptorSetLayout(2u, core::smart_refctd_ptr(ds2layout));
				auto* fs = pipeline->getShaderAtStage(asset::ICPUSpecializedShader::ESS_FRAGMENT);
				auto found = modifiedShaders.find(core::smart_refctd_ptr<asset::ICPUSpecializedShader>(fs));
				if (found != modifiedShaders.end())
					pipeline->setShaderAtStage(asset::ICPUSpecializedShader::ESS_FRAGMENT, found->second.get());
				else {
					auto newfs = createModifiedFragShader(fs, params.WindowSize.Width, params.WindowSize.Height, lights.size(), ENVMAP_SAMPLE_COUNT, LIGHT_INTENSITY_SCALE);
					modifiedShaders.insert({ core::smart_refctd_ptr<asset::ICPUSpecializedShader>(fs),newfs });
					pipeline->setShaderAtStage(asset::ICPUSpecializedShader::ESS_FRAGMENT, newfs.get());
				}
				modifiedPipelines.insert(pipeline);
			}
		}
	}
	modifiedShaders.clear();

	core::aabbox3df sceneBound;
	auto gpumeshes = driver->getGPUObjectsFromAssets(cpumeshes.data(), cpumeshes.data()+cpumeshes.size());
	{
		auto metait = meshmetas.begin();
		for (auto gpuit = gpumeshes->begin(); gpuit != gpumeshes->end(); gpuit++, metait++)
		{
			auto* meta = *metait;
			const auto* meshmeta = static_cast<const ext::MitsubaLoader::IMeshMetadata*>(meta);
			const auto& instances = meshmeta->getInstances();

			auto bb = (*gpuit)->getBoundingBox();
			for (const auto& inst : instances)
			{
				sceneBound.addInternalBox(core::transformBoxEx(bb, inst.tform));
			}
		}
	}

	auto gpuVT = core::make_smart_refctd_ptr<video::IGPUVirtualTexture>(driver, globalMeta->VT.get());
	auto gpuds0 = driver->getGPUObjectsFromAssets(&cpuds0.get(), &cpuds0.get()+1)->front();
	{
		auto count = gpuVT->getDescriptorSetWrites(nullptr, nullptr, nullptr);

		auto writes = core::make_refctd_dynamic_array<core::smart_refctd_dynamic_array<video::IGPUDescriptorSet::SWriteDescriptorSet>>(count.first);
		auto info = core::make_refctd_dynamic_array<core::smart_refctd_dynamic_array<video::IGPUDescriptorSet::SDescriptorInfo>>(count.second);

		gpuVT->getDescriptorSetWrites(writes->data(), info->data(), gpuds0.get());

		driver->updateDescriptorSets(writes->size(), writes->data(), 0u, nullptr);
	}

    auto gpuds1layout = driver->getGPUObjectsFromAssets(&ds1layout, &ds1layout+1)->front();

    auto gpuubo = driver->createDeviceLocalGPUBufferOnDedMem(sizeof(asset::SBasicViewParameters));
    auto gpuds1 = driver->createGPUDescriptorSet(std::move(gpuds1layout));
    {
        video::IGPUDescriptorSet::SWriteDescriptorSet write;
        write.dstSet = gpuds1.get();
        write.binding = ds1UboBinding;
        write.count = 1u;
        write.arrayElement = 0u;
        write.descriptorType = asset::EDT_UNIFORM_BUFFER;
        video::IGPUDescriptorSet::SDescriptorInfo info;
        {
            info.desc = gpuubo;
            info.buffer.offset = 0ull;
            info.buffer.size = gpuubo->getSize();
        }
        write.info = &info;
        driver->updateDescriptorSets(1u, &write, 0u, nullptr);
    }

	smart_refctd_ptr<video::IGPUBufferView> gpuSequenceBufferView;
	{
		constexpr uint32_t MaxSamples = ENVMAP_SAMPLE_COUNT;

		auto sampleSequence = core::make_smart_refctd_ptr<asset::ICPUBuffer>(sizeof(uint32_t) * MaxSamples*2u);

		core::OwenSampler sampler(1u, 0xdeadbeefu);

		auto out = reinterpret_cast<uint32_t*>(sampleSequence->getPointer());
		for (uint32_t i = 0; i < MaxSamples*2u; i++)
		{
			out[i] = sampler.sample(i&1u, i>>1);
		}
		auto gpuSequenceBuffer = driver->createFilledDeviceLocalGPUBufferOnDedMem(sampleSequence->getSize(), sampleSequence->getPointer());
		gpuSequenceBufferView = driver->createGPUBufferView(gpuSequenceBuffer.get(), asset::EF_R32G32_UINT);
	}

	smart_refctd_ptr<video::IGPUImageView> gpuScrambleImageView;
	{
		video::IGPUImage::SCreationParams imgParams;
		imgParams.flags = static_cast<asset::IImage::E_CREATE_FLAGS>(0u);
		imgParams.type = asset::IImage::ET_2D;
		imgParams.format = asset::EF_R32G32_UINT;
		imgParams.extent = {params.WindowSize.Width,params.WindowSize.Height,1u};
		imgParams.mipLevels = 1u;
		imgParams.arrayLayers = 1u;
		imgParams.samples = asset::IImage::ESCF_1_BIT;

		video::IGPUImage::SBufferCopy region;
		region.imageExtent = imgParams.extent;
		region.imageSubresource.layerCount = 1u;

		constexpr auto ScrambleStateChannels = 2u;
		const auto renderPixelCount = imgParams.extent.width*imgParams.extent.height;
		core::vector<uint32_t> random(renderPixelCount*ScrambleStateChannels);
		{
			core::RandomSampler rng(0xbadc0ffeu);
			for (auto& pixel : random)
				pixel = rng.nextSample();
		}
		auto buffer = driver->createFilledDeviceLocalGPUBufferOnDedMem(random.size()*sizeof(uint32_t),random.data());

		video::IGPUImageView::SCreationParams viewParams;
		viewParams.flags = static_cast<video::IGPUImageView::E_CREATE_FLAGS>(0u);
		viewParams.image = driver->createFilledDeviceLocalGPUImageOnDedMem(std::move(imgParams),buffer.get(),1u,&region);
		viewParams.viewType = video::IGPUImageView::ET_2D;
		viewParams.format = asset::EF_R32G32_UINT;
		viewParams.subresourceRange.levelCount = 1u;
		viewParams.subresourceRange.layerCount = 1u;
		gpuScrambleImageView = driver->createGPUImageView(std::move(viewParams));
	}

	auto gpuds2layout = driver->getGPUObjectsFromAssets(&ds2layout.get(), &ds2layout.get()+1)->front();
	auto gpuds2 = driver->createGPUDescriptorSet(std::move(gpuds2layout));
	{
		video::IGPUDescriptorSet::SDescriptorInfo info[4];
		video::IGPUDescriptorSet::SWriteDescriptorSet w[4];
		w[0].arrayElement = 0u;
		w[0].binding = 0u;
		w[0].count = 1u;
		w[0].descriptorType = asset::EDT_STORAGE_BUFFER;
		w[0].dstSet = gpuds2.get();
		w[0].info = info+0;
		auto lightsBuf = driver->createFilledDeviceLocalGPUBufferOnDedMem(lights.size()*sizeof(SLight), lights.data());
		info[0].buffer.offset = 0u;
		info[0].buffer.size = lightsBuf->getSize();
		info[0].desc = std::move(lightsBuf);

		w[1].arrayElement = 0u;
		w[1].binding = 1u;
		w[1].count = 1u;
		w[1].descriptorType = asset::EDT_COMBINED_IMAGE_SAMPLER;
		w[1].dstSet = gpuds2.get();
		w[1].info = info+1;
		auto gpuEnvmapImageView = createGPUImageView("../../media/envmap/envmap_0.exr", am, driver);
		info[1].image.imageLayout = asset::EIL_UNDEFINED;
		info[1].image.sampler = nullptr;
		info[1].desc = std::move(gpuEnvmapImageView);

		w[2].arrayElement = 0u;
		w[2].binding = 2u;
		w[2].count = 1u;
		w[2].descriptorType = asset::EDT_UNIFORM_TEXEL_BUFFER;
		w[2].dstSet = gpuds2.get();
		w[2].info = info+2;
		info[2].desc = gpuSequenceBufferView;

		w[3].arrayElement = 0u;
		w[3].binding = 3u;
		w[3].count = 1u;
		w[3].descriptorType = asset::EDT_COMBINED_IMAGE_SAMPLER;
		w[3].dstSet = gpuds2.get();
		w[3].info = info+3;
		info[3].image.imageLayout = asset::EIL_UNDEFINED;
		info[3].image.sampler = nullptr;//imm sampler is present
		info[3].desc = gpuScrambleImageView;

		driver->updateDescriptorSets(4u, w, 0u, nullptr);
	}

	// camera and viewport
	scene::ICameraSceneNode* camera = nullptr;
	core::recti viewport(core::position2di(0,0), core::position2di(params.WindowSize.Width,params.WindowSize.Height));

	auto isOkSensorType = [](const ext::MitsubaLoader::CElementSensor& sensor) -> bool {
		return sensor.type==ext::MitsubaLoader::CElementSensor::Type::PERSPECTIVE || sensor.type==ext::MitsubaLoader::CElementSensor::Type::THINLENS;
	};
//#define TESTING
#ifdef TESTING
	if (0)
#else
	if (globalMeta->sensors.size() && isOkSensorType(globalMeta->sensors.front()))
#endif
	{
		const auto& sensor = globalMeta->sensors.front();
		const auto& film = sensor.film;
		viewport = core::recti(core::position2di(film.cropOffsetX,film.cropOffsetY), core::position2di(film.cropWidth,film.cropHeight));

		auto extent = sceneBound.getExtent();
		camera = smgr->addCameraSceneNodeFPS(nullptr,100.f,core::min(extent.X,extent.Y,extent.Z)*0.0001f);
		// need to extract individual components
		bool leftHandedCamera = false;
		{
			auto relativeTransform = sensor.transform.matrix.extractSub3x4();
			if (relativeTransform.getPseudoDeterminant().x < 0.f)
				leftHandedCamera = true;

			auto pos = relativeTransform.getTranslation();
			camera->setPosition(pos.getAsVector3df());

			auto tpose = core::transpose(sensor.transform.matrix);
			auto up = tpose.rows[1];
			core::vectorSIMDf view = tpose.rows[2];
			auto target = view+pos;

			camera->setTarget(target.getAsVector3df());
			if (core::dot(core::normalize(core::cross(camera->getUpVector(),view)),core::cross(up,view)).x<0.99f)
				camera->setUpVector(up);
		}

		const ext::MitsubaLoader::CElementSensor::PerspectivePinhole* persp = nullptr;
		switch (sensor.type)
		{
			case ext::MitsubaLoader::CElementSensor::Type::PERSPECTIVE:
				persp = &sensor.perspective;
				break;
			case ext::MitsubaLoader::CElementSensor::Type::THINLENS:
				persp = &sensor.thinlens;
				break;
			default:
				assert(false);
				break;
		}
		float realFoVDegrees;
		auto width = viewport.getWidth();
		auto height = viewport.getHeight();
		float aspectRatio = float(width) / float(height);
		auto convertFromXFoV = [=](float fov) -> float
		{
			float aspectX = tan(core::radians(fov)*0.5f);
			return core::degrees(atan(aspectX/aspectRatio)*2.f);
		};
		switch (persp->fovAxis)
		{
			case ext::MitsubaLoader::CElementSensor::PerspectivePinhole::FOVAxis::X:
				realFoVDegrees = convertFromXFoV(persp->fov);
				break;
			case ext::MitsubaLoader::CElementSensor::PerspectivePinhole::FOVAxis::Y:
				realFoVDegrees = persp->fov;
				break;
			case ext::MitsubaLoader::CElementSensor::PerspectivePinhole::FOVAxis::DIAGONAL:
				{
					float aspectDiag = tan(core::radians(persp->fov)*0.5f);
					float aspectY = aspectDiag/core::sqrt(1.f+aspectRatio*aspectRatio);
					realFoVDegrees = core::degrees(atan(aspectY)*2.f);
				}
				break;
			case ext::MitsubaLoader::CElementSensor::PerspectivePinhole::FOVAxis::SMALLER:
				if (width < height)
					realFoVDegrees = convertFromXFoV(persp->fov);
				else
					realFoVDegrees = persp->fov;
				break;
			case ext::MitsubaLoader::CElementSensor::PerspectivePinhole::FOVAxis::LARGER:
				if (width < height)
					realFoVDegrees = persp->fov;
				else
					realFoVDegrees = convertFromXFoV(persp->fov);
				break;
			default:
				realFoVDegrees = NAN;
				assert(false);
				break;
		}
		if (leftHandedCamera)
			camera->setProjectionMatrix(core::matrix4SIMD::buildProjectionMatrixPerspectiveFovLH(core::radians(realFoVDegrees), aspectRatio, persp->nearClip, persp->farClip));
		else
			camera->setProjectionMatrix(core::matrix4SIMD::buildProjectionMatrixPerspectiveFovRH(core::radians(realFoVDegrees), aspectRatio, persp->nearClip, persp->farClip));
	}
	else
		camera = smgr->addCameraSceneNodeFPS(0, 100.0f, 0.01f);
	smgr->setActiveCamera(camera);
	device->getCursorControl()->setVisible(false);

	uint64_t lastFPSTime = 0;
	float lastFastestMeshFrameNr = -1.f;

	constexpr uint32_t MAX_INSTANCES = 512u;
	core::vector<core::matrix4SIMD> uboData(MAX_INSTANCES);
	while (device->run() && receiver.keepOpen())
	{
		driver->beginScene(true, true, video::SColor(255, 0, 0, 255));
		driver->setViewPort(viewport);
//#ifdef TESTING
		camera->OnAnimate(std::chrono::duration_cast<std::chrono::milliseconds>(device->getTimer()->getTime()).count());
		camera->render();
//#endif

		asset::SBasicViewParameters uboData;
		//view-projection matrix
		memcpy(uboData.MVP, camera->getConcatenatedMatrix().pointer(), sizeof(core::matrix4SIMD));
		//view matrix
		memcpy(uboData.MV, camera->getViewMatrix().pointer(), sizeof(core::matrix3x4SIMD));
		//writing camera position to 4th column of NormalMat
		uboData.NormalMat[3] = camera->getPosition().X;
		uboData.NormalMat[7] = camera->getPosition().Y;
		uboData.NormalMat[11] = camera->getPosition().Z;
		driver->updateBufferRangeViaStagingBuffer(gpuubo.get(), 0u, sizeof(uboData), &uboData);

		for (uint32_t j = 1u; j < gpumeshes->size(); ++j)
		{
			auto& mesh = (*gpumeshes)[j];

			for (uint32_t i = 0u; i < mesh->getMeshBufferCount(); ++i)
			{
				auto* mb = mesh->getMeshBuffer(i);

				auto* pipeline = mb->getPipeline();
				const video::IGPUDescriptorSet* ds[3]{ gpuds0.get(), gpuds1.get(), gpuds2.get() };
				driver->bindGraphicsPipeline(pipeline);
				driver->bindDescriptorSets(video::EPBP_GRAPHICS, pipeline->getLayout(), 0u, 3u, ds, nullptr);
				driver->pushConstants(pipeline->getLayout(), video::IGPUSpecializedShader::ESS_VERTEX|video::IGPUSpecializedShader::ESS_FRAGMENT, 0u, sizeof(uint32_t), mb->getPushConstantsDataPtr());

				driver->drawMeshBuffer(mb);
			}
		}

		driver->endScene();

		// display frames per second in window title
		uint64_t time = device->getTimer()->getRealTime();
		if (time - lastFPSTime > 1000)
		{
			std::wostringstream str;
			str << L"Mitsuba Loader Demo - Irrlicht Engine [" << driver->getName() << "] FPS:" << driver->getFPS() << " PrimitvesDrawn:" << driver->getPrimitiveCountDrawn();

			device->setWindowCaption(str.str());
			lastFPSTime = time;
		}
	}

	return 0;
}
