using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace mush
{
    /* output engine */
    public enum outputEngine
    {
        //		screenOutput,
        libavformatOutput,
        noOutput
    };

    public enum chromaSubsample
    {
        yuv420p,
        yuv422p,
        yuv444p
    };

    public enum transfer
    {
        pq,
        g8,
        srgb,
        linear,
        logc,
        gamma,
        rec709
    };
    public enum encodeEngine
    {
        x264,
        x265,
        nvenc,
        exr,
        yuvRaw,
        vpx,
        prores,
        gif,
        none
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct outputConfigStruct
    {
        public void defaults()
        {
            outputEngine = outputEngine.noOutput;
            encodeEngine = encodeEngine.none;

            bitrate = 60000000;
            outputBuffers = 2;
            outputName = "output";
            outputPath = "../output/";
            h264Profile = "high";
            h264Preset = "faster";
            height = 1440;
            width = 1280;
            fps = 24.0f;

            yuvBitDepth = 10;
            yuvMax = 10000.0f;
            pqLegacy = false;

            func = transfer.g8;
            zerolatency = false;
            crf = 14;

            overrideSize = false;

            use_auto_decode = false;

            liveStream.defaults();

            frame_skip_interval = 0;

            chromaSubsample = chromaSubsample.yuv420p;

            count_from = 0;
        }

        public outputEngine outputEngine;

        public encodeEngine encodeEngine;

        public float fps; // fps for x264 and vlc

        public UInt32 bitrate;
        public UInt32 outputBuffers;
        public string outputPath;
        public string outputName;

        public string h264Profile;
        public string h264Preset;

        public UInt32 height; // for [REDACTED], regular for tonemap
        public UInt32 width; // for [REDACTED], regular for tonemap

        public UInt32 yuvBitDepth;
        public float yuvMax;
        [MarshalAs(UnmanagedType.U1)]
        public bool yuvDryRun;
        [MarshalAs(UnmanagedType.U1)]
        public bool pqLegacy;

        public transfer func;
        [MarshalAs(UnmanagedType.U1)]
        public bool zerolatency;
        public UInt32 crf;
        [MarshalAs(UnmanagedType.U1)]
        public bool overrideSize;

        [MarshalAs(UnmanagedType.U1)]
        public bool use_auto_decode;

        public struct liveStreamStruct
        {
            public void defaults()
            {
                enabled = false;
                webroot = "";
                webaddress = "";
                streamname = "";

                num_files = 5;
                file_length = 5;
                wrap_after = 5;
            }
            [MarshalAs(UnmanagedType.U1)]
            public bool enabled;
            public string webroot;
            public string webaddress;
            public string streamname;
            public int num_files;
            public int file_length;
            public int wrap_after;
        }
        public liveStreamStruct liveStream;

        public UInt32 frame_skip_interval;

        public chromaSubsample chromaSubsample;

        public int count_from;
    }

}
