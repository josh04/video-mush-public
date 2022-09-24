#include <boost/format.hpp>
extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include <NVEncodeAPI.h>

#include "nvEncStream.hpp"
#include "ldrRingBuffer.hpp"
#include "metadataRingBuffer.hpp"

#ifdef _WIN32
extern void SetThreadName(const char* threadName);
#endif

void nvEncStream::init(std::shared_ptr<ldrRingBuffer> output, std::shared_ptr<metadataRingBuffer> metadata, const mush::core::outputConfigStruct &outputConfig) {
	this->output = output;
	this->metadata = metadata;
	width = outputConfig.width;
	height = outputConfig.height;
	bitrate = outputConfig.bitrate;
	fps = outputConfig.fps;

	filename = outputConfig.outputName;
	path = outputConfig.outputPath;

	unsigned int deviceID = 0;

	CUresult        cuResult = CUDA_SUCCESS;
	CUcontext       cuContextCurr;
	CUdevice cuDevice = 0;
	char gpu_name[100];
	int  deviceCount = 0;
	int  SMminor = 0, SMmajor = 0;

	// CUDA interfaces
	cuResult = cuInit(0);

	if (cuResult != CUDA_SUCCESS)
	{
		putLog("CUDA failed entirely.");
		return;
	}

	cuDeviceGetCount(&deviceCount);

	if (deviceCount == 0)
	{
		putLog("No CUDA devices.");
		return;
	}
	else
	{
		for (int currentDevice = 0; currentDevice < deviceCount; currentDevice++)
		{
			cuDeviceGet(&cuDevice, currentDevice);
			cuDeviceGetName(gpu_name, 100, cuDevice);
			cuDeviceComputeCapability(&SMmajor, &SMminor, currentDevice);
		}
	}

	// If dev is negative value, we clamp to 0
	if (deviceID < 0)
		deviceID = 0;

	if (deviceID >(unsigned int)deviceCount - 1)
	{
		putLog("CUDA Device not valid.");
		return;
	}

	// Now we get the actual device
	cuDeviceGet(&cuDevice, deviceID);
	cuDeviceGetName(gpu_name, 100, cuDevice);
	cuDeviceComputeCapability(&SMmajor, &SMminor, deviceID);

	if (((SMmajor << 4) + SMminor) < 0x30)
	{
		putLog("CUDA Device does not support NVENC.");
		return;
	}

	cuCtxCreate(&m_cuContext, 0, cuDevice);
	cuCtxPopCurrent(&cuContextCurr);

	static std::once_flag initFlag;
	std::call_once(initFlag, []() { av_register_all(); });

	av_log_set_callback([](void * ptr, int level, const char* szFmt, va_list varg){ if (level < av_log_get_level()) { int pre = 1; char line[1024]; av_log_format_line(ptr, level, szFmt, varg, line, 1024, &pre); putLog(std::string(line)); }});
}

void nvEncStream::go() {
#ifdef _WIN32
	SetThreadName("nvenc");
#endif

	NVENCSTATUS nvStatus;

	HINSTANCE m_hinstLib;
#if defined (_WIN32)
	if ((sizeof(void *) != sizeof(DWORD))) // Is64Bit() *sigh*
	{
		m_hinstLib = LoadLibrary("nvEncodeAPI64.dll");
	}
	else
	{
		m_hinstLib = LoadLibrary("nvEncodeAPI.dll");
	}

#else
	m_hinstLib = dlopen(__NVEncodeLibName, RTLD_LAZY);
#endif

#ifdef _WIN32
	typedef NVENCSTATUS(__stdcall *MYPROC)(NV_ENCODE_API_FUNCTION_LIST *);
	MYPROC nvEncodeAPICreateInstance = (MYPROC)GetProcAddress(m_hinstLib, "NvEncodeAPICreateInstance");
#else
	NVENCSTATUS(*MYPROC)(NV_ENCODE_API_FUNCTION_LIST *)nvEncodeAPICreateInstance = (MYPROC)dlsym(m_hinstLib, "NvEncodeAPICreateInstance");
#endif

	auto nvEncApi = new NV_ENCODE_API_FUNCTION_LIST;
	memset(nvEncApi, 0, sizeof(NV_ENCODE_API_FUNCTION_LIST));
	nvEncApi->version = NV_ENCODE_API_FUNCTION_LIST_VER;
	if (nvStatus = nvEncodeAPICreateInstance(nvEncApi)) {
		putLog("NVENC couldn't create instance.");
	}

	void * encoder;
	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params;
	memset(&params, 0, sizeof(NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS));
	params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
	params.apiVersion = NVENCAPI_VERSION;
	params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
	params.device = m_cuContext;
	//		params.reserved1 = 0;
	//		params.reserved2 = NULL;
	nvStatus = nvEncApi->nvEncOpenEncodeSessionEx(&params, &encoder);

	unsigned int encCnt = 0;
	nvStatus = nvEncApi->nvEncGetEncodeGUIDCount(encoder, &encCnt);

	GUID * stEncodeGUIDArray = new GUID[encCnt];
	memset(stEncodeGUIDArray, 0, sizeof(GUID) * encCnt);
	unsigned int uArraysize = 0;
	nvStatus = nvEncApi->nvEncGetEncodeGUIDs(encoder, stEncodeGUIDArray, encCnt, &uArraysize);

	bool foundCodec = false;
	GUID encodeGUID;
	for (unsigned int i = 0; i < uArraysize; i++)
	{
		// check if HW encoder supports the particular codec
		if (compareGUIDs(stEncodeGUIDArray[i], NV_ENC_CODEC_H264_GUID)) {
			foundCodec = true;
			memcpy(&encodeGUID, &stEncodeGUIDArray[i], sizeof(GUID));
			break;
		}
	}

	delete[] stEncodeGUIDArray;

	if (foundCodec == false)
	{
		putLog("No codec");
		return;
	}


	unsigned int preCnt = 0;
	nvStatus = nvEncApi->nvEncGetEncodePresetCount(encoder, encodeGUID, &preCnt);

	GUID * stPresetGUIDArray = new GUID[preCnt];
	memset(stPresetGUIDArray, 0, sizeof(GUID) * preCnt);
	unsigned int presetArraySize = 0;
	nvStatus = nvEncApi->nvEncGetEncodePresetGUIDs(encoder, encodeGUID, stPresetGUIDArray, preCnt, &presetArraySize);

	bool foundPreset = false;
	GUID presetGUID;
	for (unsigned int i = 0; i < presetArraySize; i++)
	{
		// check if HW encoder supports the particular codec
		if (compareGUIDs(stPresetGUIDArray[i], NV_ENC_PRESET_HQ_GUID)) {
			foundPreset = true;
			memcpy(&presetGUID, &stPresetGUIDArray[i], sizeof(GUID));
			break;
		}
	}

	delete[] stPresetGUIDArray;

	if (foundPreset == false)
	{
		putLog("No HQ preset");
		return;
	}

	NV_ENC_PRESET_CONFIG presetConfig;
	memset(&presetConfig, 0, sizeof(NV_ENC_PRESET_CONFIG));
	nvStatus = nvEncApi->nvEncGetEncodePresetConfig(encoder, encodeGUID, presetGUID, &presetConfig);

	unsigned int proCnt = 0;
	nvStatus = nvEncApi->nvEncGetEncodeProfileGUIDCount(encoder, encodeGUID, &proCnt);

	GUID * stProfileGUIDArray = new GUID[proCnt];
	memset(stProfileGUIDArray, 0, sizeof(GUID) * proCnt);
	unsigned int profileArraySize = 0;
	nvStatus = nvEncApi->nvEncGetEncodeProfileGUIDs(encoder, encodeGUID, stProfileGUIDArray, proCnt, &profileArraySize);

	bool foundProfile = false;
	GUID profileGUID;
	for (unsigned int i = 0; i < presetArraySize; i++)
	{
		// check if HW encoder supports the particular codec
		if (compareGUIDs(stProfileGUIDArray[i], NV_ENC_H264_PROFILE_HIGH_GUID)) {
			foundProfile = true;
			memcpy(&profileGUID, &stProfileGUIDArray[i], sizeof(GUID));
			break;
		}
	}

	delete[] stProfileGUIDArray;

	if (foundProfile == false)
	{
		putLog("No High profile");
		return;
	}

	unsigned int fmtCnt = 0;
	nvStatus = nvEncApi->nvEncGetInputFormatCount(encoder, encodeGUID, &fmtCnt);

	NV_ENC_BUFFER_FORMAT * stInputFormatGUIDArray = new NV_ENC_BUFFER_FORMAT[fmtCnt];
	memset(stInputFormatGUIDArray, 0, sizeof(NV_ENC_BUFFER_FORMAT) * fmtCnt);
	unsigned int inputFormatArraySize = 0;
	nvStatus = nvEncApi->nvEncGetInputFormats(encoder, encodeGUID, stInputFormatGUIDArray, fmtCnt, &inputFormatArraySize);

	bool foundInputFormat = false;
	NV_ENC_BUFFER_FORMAT inputFormat;
	for (unsigned int i = 0; i < inputFormatArraySize; i++)
	{
		// check if HW encoder supports the particular codec
		if (stInputFormatGUIDArray[i] == NV_ENC_BUFFER_FORMAT_NV12_TILED64x16) {
			foundInputFormat = true;
			memcpy(&inputFormat, &stInputFormatGUIDArray[i], sizeof(NV_ENC_BUFFER_FORMAT));
			break;
		}
	}
	delete[] stInputFormatGUIDArray;
	//inputFormat = NV_ENC_BUFFER_FORMAT_YUV444_PL;

	if (foundInputFormat == false) {
		putLog("No Input Format");
		return;
	}
	else {
		inputFormat = NV_ENC_BUFFER_FORMAT_NV12_PL;
	};

	//width = 64;
	//height = 16;

	NV_ENC_INITIALIZE_PARAMS initParam;
	memset(&initParam, 0, sizeof(NV_ENC_INITIALIZE_PARAMS));
	initParam.version = NV_ENC_INITIALIZE_PARAMS_VER;
	memcpy(&initParam.encodeGUID, &encodeGUID, sizeof(GUID));
	initParam.encodeWidth = width;
	initParam.encodeHeight = height;
	initParam.darHeight = height;
	initParam.darWidth = width;
	initParam.maxEncodeHeight = height;
	initParam.maxEncodeWidth = width;
	initParam.frameRateDen = 30;
	initParam.frameRateNum = 1;
	initParam.enableEncodeAsync = 0;

	memcpy(&initParam.presetGUID, &presetGUID, sizeof(GUID));
	initParam.enablePTD = 1;

	NV_ENC_CONFIG h264Param;
	memset(&h264Param, 0, sizeof(NV_ENC_CONFIG));
	h264Param.version = NV_ENC_CONFIG_VER;
	h264Param.gopLength = 30;
	h264Param.frameIntervalP = 1;
	h264Param.encodeCodecConfig.h264Config.idrPeriod = 30;
	h264Param.encodeCodecConfig.h264Config.level = 51;
	h264Param.encodeCodecConfig.h264Config.sliceMode = 3;
	h264Param.encodeCodecConfig.h264Config.adaptiveTransformMode = NV_ENC_H264_ADAPTIVE_TRANSFORM_ENABLE;
	h264Param.encodeCodecConfig.h264Config.bdirectMode = NV_ENC_H264_BDIRECT_MODE_DISABLE;

	h264Param.encodeCodecConfig.h264Config.entropyCodingMode = NV_ENC_H264_ENTROPY_CODING_MODE_CABAC;

	h264Param.frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME;

	h264Param.rcParams.initialRCQP.qpIntra = 14;
	h264Param.rcParams.initialRCQP.qpInterP = 14;
	h264Param.rcParams.initialRCQP.qpInterB = 20;
	h264Param.rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;
//	h264Param.rcParams.initialRCQP = h264Param.rcParams.constQP;
	h264Param.rcParams.enableInitialRCQP = 1;
/*	h264Param.rcParams.averageBitRate = 2000000;
	h264Param.rcParams.vbvInitialDelay = 1000000;
	h264Param.rcParams.maxBitRate = 2000000;
	h264Param.rcParams.vbvBufferSize = 2000000;*/

	memcpy(&h264Param.profileGUID, &profileGUID, sizeof(GUID));
	initParam.encodeConfig = &h264Param;
	//		initParam.encodeConfig = NULL;
	nvStatus = nvEncApi->nvEncInitializeEncoder(encoder, &initParam);


	NV_ENC_CREATE_INPUT_BUFFER inputParam;
	memset(&inputParam, 0, sizeof(NV_ENC_CREATE_INPUT_BUFFER));
	inputParam.version = NV_ENC_CREATE_INPUT_BUFFER_VER;
	inputParam.bufferFmt = inputFormat;
	unsigned int padwidth = (width + 31)&~31;
	unsigned int padheight = (height + 31)&~31;
	inputParam.height = padheight;
	inputParam.width = padwidth;
	inputParam.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;

	auto inputBuffers = make_shared<nvEncHelper>();
	int _buffers = 16;
	for (int i = 0; i < _buffers; i++) {
		nvStatus = nvEncApi->nvEncCreateInputBuffer(encoder, &inputParam);
		inputBuffers->add((unsigned char *)inputParam.inputBuffer);
	}

	NV_ENC_CREATE_BITSTREAM_BUFFER outputParam;
	memset(&outputParam, 0, sizeof(NV_ENC_CREATE_BITSTREAM_BUFFER));
	outputParam.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
	outputParam.size = 1024 * 1024;

	auto outputBuffers = make_shared<nvEncHelper>();
	for (int i = 0; i < _buffers; i++) {
		nvStatus = nvEncApi->nvEncCreateBitstreamBuffer(encoder, &outputParam);
		outputBuffers->add((unsigned char *)outputParam.bitstreamBuffer);
	}


	AVFormatContext * _format = nullptr;
	avformat_alloc_output_context2(&_format, NULL, "mp4", NULL);

	std::shared_ptr<AVFormatContext> avFormat(_format, &avformat_free_context);
	AVOutputFormat * outputFormat = avFormat->oformat;
	
	AVCodec * _codec = avcodec_find_encoder(CODEC_ID_H264);
	if (!_codec) {
		fprintf(stderr, "codec not found\n");
		return;
	}

	AVStream * videoStream = avformat_new_stream(avFormat.get(), NULL);
	videoStream->id = avFormat->nb_streams - 1;
	videoStream->codec->flags = CODEC_FLAG_GLOBAL_HEADER;
	//av_dump_format(m_formatContext, 0, filename, 1);

	auto videoContext = videoStream->codec;
	videoContext->codec_type = AVMEDIA_TYPE_VIDEO;
	videoContext->codec_id = AV_CODEC_ID_H264;
	videoContext->bit_rate = 512000;
	videoContext->width = width;
	videoContext->height = height;
	videoContext->time_base.den = 30;
	videoContext->time_base.num = 1;
	videoContext->gop_size = h264Param.gopLength;
	videoContext->pix_fmt = AV_PIX_FMT_YUV420P;

	if (!(outputFormat->flags & AVFMT_NOFILE)) {
		if (avio_open(&avFormat->pb, (path + "/" + filename).c_str(), AVIO_FLAG_WRITE) < 0) {
			return;
		}
	}

	if (avformat_write_header(avFormat.get(), NULL)) {
		return;
	}

	struct SwsContext* convertCtx = sws_getContext(width, height, PIX_FMT_BGR24, width, height, PIX_FMT_NV12, SWS_BILINEAR, NULL, NULL, NULL);
	//struct SwsContext* convertCtx = sws_getContext(width, height, PIX_FMT_BGR24, width, height, PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
	//struct SwsContext* convertCtx2 = sws_getContext(width, height, PIX_FMT_YUV420P, width, height, PIX_FMT_NV12, SWS_BILINEAR, NULL, NULL, NULL);

	//uint8_t * tmp = (uint8_t *)malloc(width * height * 1.5);

	bool incoming = true;
	bool running = true;
	int framecount = 0;


	const int srcstride = width * 3;
	NV_ENC_LOCK_INPUT_BUFFER lockParam;
	memset(&lockParam, 0, sizeof(NV_ENC_LOCK_INPUT_BUFFER));
	lockParam.version = NV_ENC_LOCK_INPUT_BUFFER_VER;
	//FILE* q = fopen("out.h264", "wb");
	bool temp = true;
	while (running && output->good()) {
		int got = 0;
		NVENCSTATUS nvFrameStatus = NV_ENC_ERR_NEED_MORE_INPUT;
		while (nvFrameStatus == NV_ENC_ERR_NEED_MORE_INPUT) {
			uint8_t * ptr = (uint8_t *)output->outLock();
			if (ptr == nullptr) {
				break;
			}

			lockParam.inputBuffer = inputBuffers->inLock();
			if (lockParam.inputBuffer == nullptr) {
				break;
			}

			nvStatus = nvEncApi->nvEncLockInputBuffer(encoder, &lockParam);
			memset((uint8_t *)lockParam.bufferDataPtr, 0, lockParam.pitch*height*1.5);
			//				uint8_t * ptrs[3] = { (uint8_t *)lockParam.bufferDataPtr, (uint8_t *)lockParam.bufferDataPtr + width*height, (uint8_t *)lockParam.bufferDataPtr + 2 * width*height };
			uint8_t * ptrs[3] = { (uint8_t *)lockParam.bufferDataPtr, (uint8_t *)lockParam.bufferDataPtr + lockParam.pitch*padheight, (uint8_t *)lockParam.bufferDataPtr + lockParam.pitch*padheight };

			const int pitches[3] = { lockParam.pitch, lockParam.pitch, lockParam.pitch };
			unsigned int h = sws_scale(convertCtx, &ptr, &srcstride, 0, height, ptrs, pitches);

			nvStatus = nvEncApi->nvEncUnlockInputBuffer(encoder, lockParam.inputBuffer);
			output->outUnlock();

			NV_ENC_PIC_PARAMS encParam;
			memset(&encParam, 0, sizeof(NV_ENC_PIC_PARAMS));
			encParam.version = NV_ENC_PIC_PARAMS_VER;
			encParam.bufferFmt = inputFormat;
			encParam.inputBuffer = lockParam.inputBuffer;
			//				encParam.inputDuration = 33333;
			encParam.inputHeight = padheight;
			encParam.inputWidth = width;
			//				encParam.inputPitch = 1536;
			encParam.rcParams = h264Param.rcParams;
			//				encParam.inputTimeStamp = i;
			encParam.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
			//encParam.pictureType = NV_ENC_PIC_TYPE_IDR;
			encParam.outputBitstream = outputBuffers->inLock();
			encParam.codecPicParams.h264PicParams.sliceMode = 3;

			encParam.inputTimeStamp = framecount + got;

			nvFrameStatus = nvEncApi->nvEncEncodePicture(encoder, &encParam);
			if (nvFrameStatus == NV_ENC_ERR_NEED_MORE_INPUT) {
				std::cout << "bums" << std::endl;
			}
			outputBuffers->inUnlock();
			inputBuffers->inUnlock();
			++got;
		}
		if (nvStatus == NV_ENC_SUCCESS) {
			if (got > 1) {
				std::cout << "FUCK" << std::endl;
			}
			for (int j = 0; j < got; ++j) {
				AVPacket _packet;
				memset(&_packet, 0, sizeof(AVPacket));
				std::shared_ptr<AVPacket> packet(&_packet, &av_free_packet);
				NV_ENC_LOCK_BITSTREAM outLockParam;
				memset(&outLockParam, 0, sizeof(NV_ENC_LOCK_BITSTREAM));
				outLockParam.version = NV_ENC_LOCK_BITSTREAM_VER;
				outLockParam.outputBitstream = outputBuffers->outLock();
				nvStatus = nvEncApi->nvEncLockBitstream(encoder, &outLockParam);
				if (nvStatus != NV_ENC_SUCCESS) {
					std::cout << "FUCK" << std::endl;
				}
				//fwrite(outLockParam.bitstreamBufferPtr, 1, outLockParam.bitstreamSizeInBytes, q);
				packet->size = outLockParam.bitstreamSizeInBytes;
				packet->data = (uint8_t *)outLockParam.bitstreamBufferPtr;
				AVRational av;
				av.den = initParam.frameRateDen;
				av.num = initParam.frameRateNum;
				packet->pts = av_rescale_q_rnd(outLockParam.outputTimeStamp, av, videoStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				packet->dts = av_rescale_q_rnd(outLockParam.outputTimeStamp, av, videoStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				packet->duration = av_rescale_q(outLockParam.outputDuration, av, videoStream->time_base);
				packet->stream_index = videoStream->index;

				//packet->duration = 1;
				packet->stream_index = videoStream->index;
				if (outLockParam.pictureType == NV_ENC_PIC_TYPE_IDR) {
					packet->flags = AV_PKT_FLAG_KEY;
				}

				/* Write the compressed frame to the media file. */
				#ifdef _VIDEO_MUSH_FFMPEG_DEBUG
				std::stringstream strm;
				strm << boost::str(boost::format("%12i ") % i) << "Frame Encoded";
				putLog(strm.str().c_str());
				#endif
				av_interleaved_write_frame(avFormat.get(), packet.get());
				nvStatus = nvEncApi->nvEncUnlockBitstream(encoder, outLockParam.outputBitstream);
				outputBuffers->outUnlock();
				inputBuffers->outLock();
				inputBuffers->outUnlock();
			}
		}
		else {
			if (!incoming) {
				running = false;
			}
		}
		framecount = framecount + got;
	}
	//fclose(q);
	av_write_trailer(avFormat.get());

	std::vector<unsigned char *> imem = inputBuffers->get();

	for (int i = 0; i < _buffers; i++) {
		nvEncApi->nvEncDestroyInputBuffer(encoder, imem[i]);
	}

	std::vector<unsigned char *> omem = outputBuffers->get();

	for (int i = 0; i < 8; i++) {
		nvEncApi->nvEncDestroyBitstreamBuffer(encoder, omem[i]);
	}

	nvEncApi->nvEncDestroyEncoder(encoder);

} 


void nvEncStream::go2() {
	height = height / 2;
#ifdef _WIN32
	SetThreadName("nvenc2stream");
#endif

	NVENCSTATUS nvStatus;

	HINSTANCE m_hinstLib;
#if defined (_WIN32)
	if ((sizeof(void *) != sizeof(DWORD))) // Is64Bit() *sigh*
	{
		m_hinstLib = LoadLibrary("nvEncodeAPI64.dll");
	} else
	{
		m_hinstLib = LoadLibrary("nvEncodeAPI.dll");
	}

#else
	m_hinstLib = dlopen(__NVEncodeLibName, RTLD_LAZY);
#endif

#ifdef _WIN32
	typedef NVENCSTATUS(__stdcall *MYPROC)(NV_ENCODE_API_FUNCTION_LIST *);
	MYPROC nvEncodeAPICreateInstance = (MYPROC)GetProcAddress(m_hinstLib, "NvEncodeAPICreateInstance");
#else
	NVENCSTATUS(*MYPROC)(NV_ENCODE_API_FUNCTION_LIST *)nvEncodeAPICreateInstance = (MYPROC)dlsym(m_hinstLib, "NvEncodeAPICreateInstance");
#endif

	auto nvEncApi = new NV_ENCODE_API_FUNCTION_LIST;
	memset(nvEncApi, 0, sizeof(NV_ENCODE_API_FUNCTION_LIST));
	nvEncApi->version = NV_ENCODE_API_FUNCTION_LIST_VER;
	if (nvStatus = nvEncodeAPICreateInstance(nvEncApi)) {
		putLog("NVENC couldn't create instance.");
	}

	void * encoderLuma;
	void * encoderChroma;
	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params;
	memset(&params, 0, sizeof(NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS));
	params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
	params.apiVersion = NVENCAPI_VERSION;
	params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
	params.device = m_cuContext;
	//		params.reserved1 = 0;
	//		params.reserved2 = NULL;
	nvStatus = nvEncApi->nvEncOpenEncodeSessionEx(&params, &encoderLuma);
	nvStatus = nvEncApi->nvEncOpenEncodeSessionEx(&params, &encoderChroma);

	unsigned int encCnt = 0;
	nvStatus = nvEncApi->nvEncGetEncodeGUIDCount(encoderLuma, &encCnt);

	GUID * stEncodeGUIDArray = new GUID[encCnt];
	memset(stEncodeGUIDArray, 0, sizeof(GUID) * encCnt);
	unsigned int uArraysize = 0;
	nvStatus = nvEncApi->nvEncGetEncodeGUIDs(encoderLuma, stEncodeGUIDArray, encCnt, &uArraysize);

	bool foundCodec = false;
	GUID encodeGUID;
	for (unsigned int i = 0; i < uArraysize; i++)
	{
		// check if HW encoder supports the particular codec
		if (compareGUIDs(stEncodeGUIDArray[i], NV_ENC_CODEC_H264_GUID)) {
			foundCodec = true;
			memcpy(&encodeGUID, &stEncodeGUIDArray[i], sizeof(GUID));
			break;
		}
	}

	delete[] stEncodeGUIDArray;

	if (foundCodec == false)
	{
		putLog("No codec");
		return;
	}


	unsigned int preCnt = 0;
	nvStatus = nvEncApi->nvEncGetEncodePresetCount(encoderLuma, encodeGUID, &preCnt);

	GUID * stPresetGUIDArray = new GUID[preCnt];
	memset(stPresetGUIDArray, 0, sizeof(GUID) * preCnt);
	unsigned int presetArraySize = 0;
	nvStatus = nvEncApi->nvEncGetEncodePresetGUIDs(encoderLuma, encodeGUID, stPresetGUIDArray, preCnt, &presetArraySize);

	bool foundPreset = false;
	GUID presetGUID;
	for (unsigned int i = 0; i < presetArraySize; i++)
	{
		// check if HW encoder supports the particular codec
		if (compareGUIDs(stPresetGUIDArray[i], NV_ENC_PRESET_HQ_GUID)) {
			foundPreset = true;
			memcpy(&presetGUID, &stPresetGUIDArray[i], sizeof(GUID));
			break;
		}
	}

	delete[] stPresetGUIDArray;

	if (foundPreset == false)
	{
		putLog("No HQ preset");
		return;
	}

	NV_ENC_PRESET_CONFIG presetConfig;
	memset(&presetConfig, 0, sizeof(NV_ENC_PRESET_CONFIG));
	nvStatus = nvEncApi->nvEncGetEncodePresetConfig(encoderLuma, encodeGUID, presetGUID, &presetConfig);

	unsigned int proCnt = 0;
	nvStatus = nvEncApi->nvEncGetEncodeProfileGUIDCount(encoderLuma, encodeGUID, &proCnt);

	GUID * stProfileGUIDArray = new GUID[proCnt];
	memset(stProfileGUIDArray, 0, sizeof(GUID) * proCnt);
	unsigned int profileArraySize = 0;
	nvStatus = nvEncApi->nvEncGetEncodeProfileGUIDs(encoderLuma, encodeGUID, stProfileGUIDArray, proCnt, &profileArraySize);

	bool foundProfile = false;
	GUID profileGUID;
	for (unsigned int i = 0; i < presetArraySize; i++)
	{
		// check if HW encoder supports the particular codec
		if (compareGUIDs(stProfileGUIDArray[i], NV_ENC_H264_PROFILE_HIGH_GUID)) {
			foundProfile = true;
			memcpy(&profileGUID, &stProfileGUIDArray[i], sizeof(GUID));
			break;
		}
	}

	delete[] stProfileGUIDArray;

	if (foundProfile == false)
	{
		putLog("No High profile");
		return;
	}

	unsigned int fmtCnt = 0;
	nvStatus = nvEncApi->nvEncGetInputFormatCount(encoderLuma, encodeGUID, &fmtCnt);

	NV_ENC_BUFFER_FORMAT * stInputFormatGUIDArray = new NV_ENC_BUFFER_FORMAT[fmtCnt];
	memset(stInputFormatGUIDArray, 0, sizeof(NV_ENC_BUFFER_FORMAT) * fmtCnt);
	unsigned int inputFormatArraySize = 0;
	nvStatus = nvEncApi->nvEncGetInputFormats(encoderLuma, encodeGUID, stInputFormatGUIDArray, fmtCnt, &inputFormatArraySize);

	bool foundInputFormat = false;
	NV_ENC_BUFFER_FORMAT inputFormat;
	for (unsigned int i = 0; i < inputFormatArraySize; i++)
	{
		// check if HW encoder supports the particular codec
		if (stInputFormatGUIDArray[i] == NV_ENC_BUFFER_FORMAT_NV12_TILED64x16) {
			foundInputFormat = true;
			memcpy(&inputFormat, &stInputFormatGUIDArray[i], sizeof(NV_ENC_BUFFER_FORMAT));
			break;
		}
	}
	delete[] stInputFormatGUIDArray;
	//inputFormat = NV_ENC_BUFFER_FORMAT_YUV444_PL;

	if (foundInputFormat == false) {
		putLog("No Input Format");
		return;
	} else {
		inputFormat = NV_ENC_BUFFER_FORMAT_NV12_PL;
	};

	//width = 64;
	//height = 16;

	NV_ENC_INITIALIZE_PARAMS initParam;
	memset(&initParam, 0, sizeof(NV_ENC_INITIALIZE_PARAMS));
	initParam.version = NV_ENC_INITIALIZE_PARAMS_VER;
	memcpy(&initParam.encodeGUID, &encodeGUID, sizeof(GUID));
	initParam.encodeWidth = width;
	initParam.encodeHeight = height;
	initParam.darHeight = height;
	initParam.darWidth = width;
	initParam.maxEncodeHeight = height;
	initParam.maxEncodeWidth = width;
	initParam.frameRateDen = 30;
	initParam.frameRateNum = 1;
	initParam.enableEncodeAsync = 0;

	memcpy(&initParam.presetGUID, &presetGUID, sizeof(GUID));
	initParam.enablePTD = 1;

	NV_ENC_CONFIG h264Param;
	memset(&h264Param, 0, sizeof(NV_ENC_CONFIG));
	h264Param.version = NV_ENC_CONFIG_VER;
	h264Param.gopLength = 30;
	h264Param.frameIntervalP = 1;
	h264Param.encodeCodecConfig.h264Config.idrPeriod = 30;
	h264Param.encodeCodecConfig.h264Config.level = 51;
	h264Param.encodeCodecConfig.h264Config.sliceMode = 3;
	h264Param.encodeCodecConfig.h264Config.adaptiveTransformMode = NV_ENC_H264_ADAPTIVE_TRANSFORM_ENABLE;
	h264Param.encodeCodecConfig.h264Config.bdirectMode = NV_ENC_H264_BDIRECT_MODE_DISABLE;

	h264Param.encodeCodecConfig.h264Config.entropyCodingMode = NV_ENC_H264_ENTROPY_CODING_MODE_CABAC;

	h264Param.frameFieldMode = NV_ENC_PARAMS_FRAME_FIELD_MODE_FRAME;

	h264Param.rcParams.initialRCQP.qpIntra = 14;
	h264Param.rcParams.initialRCQP.qpInterP = 14;
	h264Param.rcParams.initialRCQP.qpInterB = 20;
	h264Param.rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;
	//	h264Param.rcParams.initialRCQP = h264Param.rcParams.constQP;
	h264Param.rcParams.enableInitialRCQP = 1;
	/*	h264Param.rcParams.averageBitRate = 2000000;
	h264Param.rcParams.vbvInitialDelay = 1000000;
	h264Param.rcParams.maxBitRate = 2000000;
	h264Param.rcParams.vbvBufferSize = 2000000;*/

	memcpy(&h264Param.profileGUID, &profileGUID, sizeof(GUID));
	initParam.encodeConfig = &h264Param;
	//		initParam.encodeConfig = NULL;
	nvStatus = nvEncApi->nvEncInitializeEncoder(encoderLuma, &initParam);
	nvStatus = nvEncApi->nvEncInitializeEncoder(encoderChroma, &initParam);


	NV_ENC_CREATE_INPUT_BUFFER inputParam;
	memset(&inputParam, 0, sizeof(NV_ENC_CREATE_INPUT_BUFFER));
	inputParam.version = NV_ENC_CREATE_INPUT_BUFFER_VER;
	inputParam.bufferFmt = inputFormat;
	unsigned int padwidth = (width + 31)&~31;
	unsigned int padheight = (height + 31)&~31;
	inputParam.height = padheight;
	inputParam.width = padwidth;
	inputParam.memoryHeap = NV_ENC_MEMORY_HEAP_SYSMEM_CACHED;

	auto inputBuffersLuma = make_shared<nvEncHelper>();
	int _buffers = 16;
	for (int i = 0; i < _buffers; i++) {
		nvStatus = nvEncApi->nvEncCreateInputBuffer(encoderLuma, &inputParam);
		inputBuffersLuma->add((unsigned char *)inputParam.inputBuffer);
	}
	auto inputBuffersChroma = make_shared<nvEncHelper>();
	for (int i = 0; i < _buffers; i++) {
		nvStatus = nvEncApi->nvEncCreateInputBuffer(encoderChroma, &inputParam);
		inputBuffersChroma->add((unsigned char *)inputParam.inputBuffer);
	}

	NV_ENC_CREATE_BITSTREAM_BUFFER outputParam;
	memset(&outputParam, 0, sizeof(NV_ENC_CREATE_BITSTREAM_BUFFER));
	outputParam.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
	outputParam.size = 1024 * 1024;

	auto outputBuffersLuma = make_shared<nvEncHelper>();
	for (int i = 0; i < _buffers; i++) {
		nvStatus = nvEncApi->nvEncCreateBitstreamBuffer(encoderLuma, &outputParam);
		outputBuffersLuma->add((unsigned char *)outputParam.bitstreamBuffer);
	}
    outputBuffersLuma->
	auto outputBuffersChroma = make_shared<nvEncHelper>();
	for (int i = 0; i < _buffers; i++) {
		nvStatus = nvEncApi->nvEncCreateBitstreamBuffer(encoderChroma, &outputParam);
		outputBuffersChroma->add((unsigned char *)outputParam.bitstreamBuffer);
	}
    outputBuffersChroma->

	AVFormatContext * _format = nullptr;
	avformat_alloc_output_context2(&_format, NULL, "mp4", NULL);

	std::shared_ptr<AVFormatContext> avFormat(_format, &avformat_free_context);
	AVOutputFormat * outputFormat = avFormat->oformat;

	AVCodec * _codec = avcodec_find_encoder(CODEC_ID_H264);
	if (!_codec) {
		fprintf(stderr, "codec not found\n");
		return;
	}

	AVStream * videoStreamLuma = avformat_new_stream(avFormat.get(), NULL);
	videoStreamLuma->id = avFormat->nb_streams - 1;
	videoStreamLuma->codec->flags = CODEC_FLAG_GLOBAL_HEADER;
	//av_dump_format(m_formatContext, 0, filename, 1);

	auto videoContextLuma = videoStreamLuma->codec;
	videoContextLuma->codec_type = AVMEDIA_TYPE_VIDEO;
	videoContextLuma->codec_id = AV_CODEC_ID_H264;
	videoContextLuma->bit_rate = 512000;
	videoContextLuma->width = width;
	videoContextLuma->height = height;
	videoContextLuma->time_base.den = 30;
	videoContextLuma->time_base.num = 1;
	videoContextLuma->gop_size = h264Param.gopLength;
	videoContextLuma->pix_fmt = AV_PIX_FMT_YUV420P;

	AVStream * videoStreamChroma = avformat_new_stream(avFormat.get(), NULL);
	videoStreamChroma->id = avFormat->nb_streams - 1;
	videoStreamChroma->codec->flags = CODEC_FLAG_GLOBAL_HEADER;
	//av_dump_format(m_formatContext, 0, filename, 1);

	auto videoContextChroma = videoStreamChroma->codec;
	videoContextChroma->codec_type = AVMEDIA_TYPE_VIDEO;
	videoContextChroma->codec_id = AV_CODEC_ID_H264;
	videoContextChroma->bit_rate = 512000;
	videoContextChroma->width = width;
	videoContextChroma->height = height;
	videoContextChroma->time_base.den = 30;
	videoContextChroma->time_base.num = 1;
	videoContextChroma->gop_size = h264Param.gopLength;
	videoContextChroma->pix_fmt = AV_PIX_FMT_YUV420P;


	AVStream * dataStream = avformat_new_stream(avFormat.get(), NULL);
	dataStream->id = avFormat->nb_streams - 1;
	dataStream->codec->flags = CODEC_FLAG_GLOBAL_HEADER;
	//av_dump_format(m_formatContext, 0, filename, 1);

	auto dataContext = dataStream->codec;
	dataContext->codec_type = AVMEDIA_TYPE_DATA;
	dataContext->codec_id = AV_CODEC_ID_MPEG4SYSTEMS;
	dataContext->time_base.den = 30;
	dataContext->time_base.num = 1;

	if (!(outputFormat->flags & AVFMT_NOFILE)) {
		if (avio_open(&avFormat->pb, (path + "/" + filename).c_str(), AVIO_FLAG_WRITE) < 0) {
			return;
		}
	}

	if (avformat_write_header(avFormat.get(), NULL)) {
		return;
	}

	struct SwsContext* convertCtx = sws_getContext(width, height, PIX_FMT_BGR24, width, height, PIX_FMT_NV12, SWS_BILINEAR, NULL, NULL, NULL);
	//struct SwsContext* convertCtx = sws_getContext(width, height, PIX_FMT_BGR24, width, height, PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
	//struct SwsContext* convertCtx2 = sws_getContext(width, height, PIX_FMT_YUV420P, width, height, PIX_FMT_NV12, SWS_BILINEAR, NULL, NULL, NULL);

	//uint8_t * tmp = (uint8_t *)malloc(width * height * 1.5);

	bool incoming = true;
	bool running = true;
	int framecount = 0;


	const int srcstride = width * 3;
	NV_ENC_LOCK_INPUT_BUFFER lockParam;
	memset(&lockParam, 0, sizeof(NV_ENC_LOCK_INPUT_BUFFER));
	lockParam.version = NV_ENC_LOCK_INPUT_BUFFER_VER;
	//FILE* q = fopen("out.h264", "wb");
	bool temp = true;
	while (running && output->good()) {
		int gotLuma = 0;
		int gotChroma = 0;
		NVENCSTATUS nvFrameStatusLuma = NV_ENC_ERR_NEED_MORE_INPUT;
		NVENCSTATUS nvFrameStatusChroma  = NV_ENC_ERR_NEED_MORE_INPUT;
		while (nvFrameStatusLuma == NV_ENC_ERR_NEED_MORE_INPUT || nvFrameStatusChroma == NV_ENC_ERR_NEED_MORE_INPUT) {
			uint8_t * ptr = (uint8_t *)output->outLock();
			if (ptr == nullptr) {
				break;
			}

			lockParam.inputBuffer = inputBuffersLuma->inLock();
			if (lockParam.inputBuffer == nullptr) {
				break;
			}

			nvStatus = nvEncApi->nvEncLockInputBuffer(encoderLuma, &lockParam);
			memset((uint8_t *)lockParam.bufferDataPtr, 0, lockParam.pitch*height*1.5);
			//				uint8_t * ptrs[3] = { (uint8_t *)lockParam.bufferDataPtr, (uint8_t *)lockParam.bufferDataPtr + width*height, (uint8_t *)lockParam.bufferDataPtr + 2 * width*height };
			uint8_t * ptrs[3] = { (uint8_t *)lockParam.bufferDataPtr, (uint8_t *)lockParam.bufferDataPtr + lockParam.pitch*padheight, (uint8_t *)lockParam.bufferDataPtr + lockParam.pitch*padheight };

			const int pitches[3] = { lockParam.pitch, lockParam.pitch, lockParam.pitch };
			unsigned int h = sws_scale(convertCtx, &ptr, &srcstride, 0, height, ptrs, pitches);

			nvStatus = nvEncApi->nvEncUnlockInputBuffer(encoderLuma, lockParam.inputBuffer);

			NV_ENC_PIC_PARAMS encParam;
			memset(&encParam, 0, sizeof(NV_ENC_PIC_PARAMS));
			encParam.version = NV_ENC_PIC_PARAMS_VER;
			encParam.bufferFmt = inputFormat;
			encParam.inputBuffer = lockParam.inputBuffer;
			//				encParam.inputDuration = 33333;
			encParam.inputHeight = padheight;
			encParam.inputWidth = width;
			//				encParam.inputPitch = 1536;
			encParam.rcParams = h264Param.rcParams;
			//				encParam.inputTimeStamp = i;
			encParam.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
			//encParam.pictureType = NV_ENC_PIC_TYPE_IDR;
			encParam.outputBitstream = outputBuffersLuma->inLock();
			encParam.codecPicParams.h264PicParams.sliceMode = 3;

			encParam.inputTimeStamp = framecount + gotLuma;

			nvFrameStatusLuma = nvEncApi->nvEncEncodePicture(encoderLuma, &encParam);
			if (nvFrameStatusLuma == NV_ENC_ERR_NEED_MORE_INPUT) {
				std::cout << "bums" << std::endl;
			}
			outputBuffersLuma->inUnlock();
			inputBuffersLuma->inUnlock();
			++gotLuma;


			lockParam.inputBuffer = inputBuffersChroma->inLock();
			if (lockParam.inputBuffer == nullptr) {
				break;
			}

			nvStatus = nvEncApi->nvEncLockInputBuffer(encoderChroma, &lockParam);
			memset((uint8_t *)lockParam.bufferDataPtr, 0, lockParam.pitch*height*1.5);
			//				uint8_t * ptrs[3] = { (uint8_t *)lockParam.bufferDataPtr, (uint8_t *)lockParam.bufferDataPtr + width*height, (uint8_t *)lockParam.bufferDataPtr + 2 * width*height };
			uint8_t * ptrs2[3] = { (uint8_t *)lockParam.bufferDataPtr, (uint8_t *)lockParam.bufferDataPtr + lockParam.pitch*padheight, (uint8_t *)lockParam.bufferDataPtr + lockParam.pitch*padheight };

			const uint8_t * ptrChr = ptr + srcstride*height;
			const int pitches2[3] = { lockParam.pitch, lockParam.pitch, lockParam.pitch };
			h = sws_scale(convertCtx, &ptrChr, &srcstride, 0, height, ptrs2, pitches2);

			nvStatus = nvEncApi->nvEncUnlockInputBuffer(encoderChroma, lockParam.inputBuffer);

			memset(&encParam, 0, sizeof(NV_ENC_PIC_PARAMS));
			encParam.version = NV_ENC_PIC_PARAMS_VER;
			encParam.bufferFmt = inputFormat;
			encParam.inputBuffer = lockParam.inputBuffer;
			//				encParam.inputDuration = 33333;
			encParam.inputHeight = padheight;
			encParam.inputWidth = width;
			//				encParam.inputPitch = 1536;
			encParam.rcParams = h264Param.rcParams;
			//				encParam.inputTimeStamp = i;
			encParam.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
			//encParam.pictureType = NV_ENC_PIC_TYPE_IDR;
			encParam.outputBitstream = outputBuffersChroma->inLock();
			encParam.codecPicParams.h264PicParams.sliceMode = 3;

			encParam.inputTimeStamp = framecount + gotChroma;

			nvFrameStatusChroma = nvEncApi->nvEncEncodePicture(encoderChroma, &encParam);
			if (nvFrameStatusChroma == NV_ENC_ERR_NEED_MORE_INPUT) {
				std::cout << "bums" << std::endl;
			}
			outputBuffersChroma->inUnlock();
			inputBuffersChroma->inUnlock();
			++gotChroma;

			output->outUnlock();

		}

		if (nvFrameStatusLuma == NV_ENC_SUCCESS) {
			if (gotLuma > 1) {
				std::cout << "FUCK" << std::endl;
			}
			for (int j = 0; j < gotLuma; ++j) {
				AVPacket _packet;
				memset(&_packet, 0, sizeof(AVPacket));
				std::shared_ptr<AVPacket> packet(&_packet, &av_free_packet);
				NV_ENC_LOCK_BITSTREAM outLockParam;
				memset(&outLockParam, 0, sizeof(NV_ENC_LOCK_BITSTREAM));
				outLockParam.version = NV_ENC_LOCK_BITSTREAM_VER;
				outLockParam.outputBitstream = outputBuffersLuma->outLock();
				nvStatus = nvEncApi->nvEncLockBitstream(encoderLuma, &outLockParam);
				if (nvStatus != NV_ENC_SUCCESS) {
					std::cout << "FUCK" << std::endl;
				}
				//fwrite(outLockParam.bitstreamBufferPtr, 1, outLockParam.bitstreamSizeInBytes, q);
				packet->size = outLockParam.bitstreamSizeInBytes;
				packet->data = (uint8_t *)outLockParam.bitstreamBufferPtr;
				AVRational av;
				av.den = initParam.frameRateDen;
				av.num = initParam.frameRateNum;
				packet->pts = av_rescale_q_rnd(outLockParam.outputTimeStamp, av, videoStreamLuma->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				packet->dts = av_rescale_q_rnd(outLockParam.outputTimeStamp, av, videoStreamLuma->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				packet->duration = av_rescale_q(outLockParam.outputDuration, av, videoStreamLuma->time_base);
				packet->stream_index = videoStreamLuma->index;

				//packet->duration = 1;
				packet->stream_index = videoStreamLuma->index;
				if (outLockParam.pictureType == NV_ENC_PIC_TYPE_IDR) {
					packet->flags = AV_PKT_FLAG_KEY;
				}

				/* Write the compressed frame to the media file. */
#ifdef _VIDEO_MUSH_FFMPEG_DEBUG
				std::stringstream strm;
				strm << boost::str(boost::format("%12i ") % i) << "Frame Encoded";
				putLog(strm.str().c_str());
#endif
				av_interleaved_write_frame(avFormat.get(), packet.get());
				nvStatus = nvEncApi->nvEncUnlockBitstream(encoderLuma, outLockParam.outputBitstream);
				outputBuffersLuma->outUnlock();
				inputBuffersLuma->outLock();
				inputBuffersLuma->outUnlock();

				AVPacket _dataPacket;
				memset(&_dataPacket, 0, sizeof(AVPacket));
				std::shared_ptr<AVPacket> dataPacket(&_dataPacket, &av_free_packet);
				float data[4] = {1.0f, 2.0f, 4.0f, 8.0f};
				if (metadata != nullptr) {
/*					std::array<float, 4>& adta = metadata->dataOutLock();
					data[0] = adta[0];
					data[1] = adta[1];
					data[2] = adta[2];
					data[3] = adta[3];
					metadata->outUnlock();*/
				}
				dataPacket->size = sizeof(float)*4;
				dataPacket->data = (uint8_t *)data;
				dataPacket->pts = av_rescale_q_rnd(outLockParam.outputTimeStamp, av, dataStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				dataPacket->dts = av_rescale_q_rnd(outLockParam.outputTimeStamp, av, dataStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				dataPacket->stream_index = dataStream->index;

				av_interleaved_write_frame(avFormat.get(), dataPacket.get());


			}
		}

		if (nvFrameStatusChroma == NV_ENC_SUCCESS) {
			if (gotChroma > 1) {
				std::cout << "FUCK" << std::endl;
			}
			for (int j = 0; j < gotChroma; ++j) {
				AVPacket _packet;
				memset(&_packet, 0, sizeof(AVPacket));
				std::shared_ptr<AVPacket> packet(&_packet, &av_free_packet);
				NV_ENC_LOCK_BITSTREAM outLockParam;
				memset(&outLockParam, 0, sizeof(NV_ENC_LOCK_BITSTREAM));
				outLockParam.version = NV_ENC_LOCK_BITSTREAM_VER;
				outLockParam.outputBitstream = outputBuffersChroma->outLock();
				nvStatus = nvEncApi->nvEncLockBitstream(encoderChroma, &outLockParam);
				if (nvStatus != NV_ENC_SUCCESS) {
					std::cout << "FUCK" << std::endl;
				}
				//fwrite(outLockParam.bitstreamBufferPtr, 1, outLockParam.bitstreamSizeInBytes, q);
				packet->size = outLockParam.bitstreamSizeInBytes;
				packet->data = (uint8_t *)outLockParam.bitstreamBufferPtr;
				AVRational av;
				av.den = initParam.frameRateDen;
				av.num = initParam.frameRateNum;
				packet->pts = av_rescale_q_rnd(outLockParam.outputTimeStamp, av, videoStreamChroma->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				packet->dts = av_rescale_q_rnd(outLockParam.outputTimeStamp, av, videoStreamChroma->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				packet->duration = av_rescale_q(outLockParam.outputDuration, av, videoStreamChroma->time_base);
				packet->stream_index = videoStreamChroma->index;

				//packet->duration = 1;
				packet->stream_index = videoStreamChroma->index;
				if (outLockParam.pictureType == NV_ENC_PIC_TYPE_IDR) {
					packet->flags = AV_PKT_FLAG_KEY;
				}

				/* Write the compressed frame to the media file. */
#ifdef _VIDEO_MUSH_FFMPEG_DEBUG
				std::stringstream strm;
				strm << boost::str(boost::format("%12i ") % i) << "Frame Encoded";
				putLog(strm.str().c_str());
#endif
				av_interleaved_write_frame(avFormat.get(), packet.get());
				nvStatus = nvEncApi->nvEncUnlockBitstream(encoderChroma, outLockParam.outputBitstream);
				outputBuffersChroma->outUnlock();
				inputBuffersChroma->outLock();
				inputBuffersChroma->outUnlock();
			}
		}
		
		if (!gotLuma || !gotChroma) {
			if (!incoming) {
				running = false;
			}
		}
		framecount = framecount + gotLuma;
	}
	//fclose(q);
	av_write_trailer(avFormat.get());

	std::vector<unsigned char *> imemLum = inputBuffersLuma->get();

	for (int i = 0; i < _buffers; i++) {
		nvEncApi->nvEncDestroyInputBuffer(encoderLuma, imemLum[i]);
	}

	std::vector<unsigned char *> omemLum = outputBuffersLuma->get();

	for (int i = 0; i < 8; i++) {
		nvEncApi->nvEncDestroyBitstreamBuffer(encoderLuma, omemLum[i]);
	}

	std::vector<unsigned char *> imemChr = inputBuffersChroma->get();

	for (int i = 0; i < _buffers; i++) {
		nvEncApi->nvEncDestroyInputBuffer(encoderChroma, imemChr[i]);
	}

	std::vector<unsigned char *> omemChr = outputBuffersChroma->get();

	for (int i = 0; i < 8; i++) {
		nvEncApi->nvEncDestroyBitstreamBuffer(encoderChroma, omemChr[i]);
	}
	nvEncApi->nvEncDestroyEncoder(encoderLuma);
	nvEncApi->nvEncDestroyEncoder(encoderChroma);

}
