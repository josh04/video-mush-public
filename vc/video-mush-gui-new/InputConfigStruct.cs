using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace mush
{
    public enum rawCameraType
    {
        mk2 = 0,
        mk3 = 1
    }
    public enum bmModes
    {
        m720p60,
        m1080p24,
        m1080p25,
        m1080p30
    };
    public enum input_pix_fmt
    {
        char_4channel,
        half_4channel,
        float_4channel
    };
    public enum filetype
    {
        pfmFiletype,
        rawFiletype,
        exrFiletype,
        mergeTiffFiletype,
        tiffFiletype,
        detectFiletype
    };
    public enum inputEngine
    {
        folderInput,
        stdinInput,
        videoInput,
        rgb16bitInput,
        externalInput,
        flareInput,
        fastEXRInput,
        udpInput,
        flare10Input,
        testCardInput,
        jpegInput,
        canonInput,
        yuv10bitInput,
        singleEXRInput,
        mlvRawInput,
        noInput
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct inputConfigStruct
    {
        public void defaults()
        {
            inputEngine = inputEngine.folderInput;

            inputPath = "e:/resources/MorganLovers";
            filetype = filetype.detectFiletype;

            inputBuffers = 3;
            exposures = 1;

            inputWidth = 1280;
            inputHeight = 720;
            inputSize = 1;

            noflip = false;


            input_pix_fmt = input_pix_fmt.float_4channel;

            flareCOMPort = "COM4";
            testCardPath = "resources/TestCard.exr";
            resourceDir = "";
            //testMode = false;

            legacyIsoArray = new float[4];
            loopFrames = false;
            bmMode = bmModes.m1080p30;
            func = transfer.linear;

            whitePoint = new float[4];
            whitePoint[0] = 1.392498f;
            whitePoint[1] = 1.0f;
            whitePoint[2] = 2.375114f;
            whitePoint[3] = 1.0f;

            blackPoint = 0;
            dualISO = false;
            dual_iso_comp_factor = 3.0f;
            camera_type = rawCameraType.mk3;
            frame_skip = 0;
            raw_clamp = 1.0f;

            resize = false;
            resize_width = 1280;
            resize_height = 720;
            gammacorrect = 1.0f;
            darken = 0.0f;

            doubleHeightFrame = false;

            lock_fps = false;
            fps = 25.0f;

            secondInputPath = "";
            thirdInputPath = "";
            fourthInputPath = "";
        }

        public inputEngine inputEngine;

        public string inputPath;
        filetype filetype;
        public int inputBuffers; // number of input buffers. no idea why you'd change it.
        public int exposures; // a sub-property of mergeEngine. make sure they match!

        public UInt32 inputWidth; // width. will be set automatically if possible
        public UInt32 inputHeight; // height, ditto
        public UInt32 inputSize; // size of pixel in ints

        [MarshalAs(UnmanagedType.U1)]
        public bool noflip; // if your two-canon system is the right way up
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] legacyIsoArray; // iso array. ascending!

        public string flareCOMPort; // COM port for the flare

        public string testCardPath;
        public string resourceDir;

        public input_pix_fmt input_pix_fmt;

        [MarshalAs(UnmanagedType.U1)]
        bool doubleHeightFrame; // inputting a double height [REDACTED] frame? who cares!

        //[MarshalAs(UnmanagedType.U1)]
        //public bool testMode; // Make the preprocessor spew out the last frame if the new frame isn't timely, so you can test real-time effects with a slow frame grabber;

        [MarshalAs(UnmanagedType.U1)]
        public bool loopFrames;

        public bmModes bmMode;

        public transfer func;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] whitePoint; // iso array. ascending!
        public int blackPoint;

        [MarshalAs(UnmanagedType.U1)]
        public bool dualISO;

        public float dual_iso_comp_factor;
        public rawCameraType camera_type;

        public UInt32 frame_skip;
        public float raw_clamp;

        [MarshalAs(UnmanagedType.U1)]
        public bool resize;
        public UInt32 resize_width;
        public UInt32 resize_height;
        public float gammacorrect; // gamma to correct for
        public float darken; // exposure modifier

        public bool lock_fps;
        public float fps;

        public string secondInputPath;
        public string thirdInputPath;
        public string fourthInputPath;
    }
}
