using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.IO;

namespace mush
{


    public enum MetricValueType
    {
        i,
		f
    };

    [StructLayout(LayoutKind.Explicit)]
    public struct MetricValue
    {
        [FieldOffset(0)]
        public MetricValueType t;
        [FieldOffset(4)]
        public int integer;
        [FieldOffset(4)]
        public float floating_point;
        [FieldOffset(8)]
        public UInt32 frame;
        [FieldOffset(12)]
        [MarshalAs(UnmanagedType.U1)]
        public bool is_average;
        [FieldOffset(13)]
        [MarshalAs(UnmanagedType.U1)]
        public bool is_null;
    };

    public enum genericProcess
    {
        barcode,
        displaceChannel,
        rotateImage,
        falseColour,
        bt709luminance,
        tonemap,
        colourBilateral,
        laplace,
        debayer,
        tape,
        fisheye2equirectangular,
        sobel,
        videoAverage
    };

    public enum generatorProcess
    {
        sand,
        text,
        sphere,
        oculusVideo,
        oculusDraw,
        motionReprojection,
        anaglyph,
        motionGenerator,
        sbsPack,
        sbsUnpack,
        mse,
        raster
    };

    public enum processEngine
    {
        tonemapCompress,
        slic,
        waveform,
        sand,
        fixedBitDepth,
        nuHDR,
        generic,
        debayer,
        trayrace,
        par,
        amd,
        oculusDraw,
        motionExplorer,
        metrics,

        none = 999
    };

    public enum waveformMode {
        luma,
        rgb,
        r,
        g,
        b
    };

    public enum bilateralMode
    {
        bilateral,
        bilateralOpt,
        bilinear,
        off
    };
    
    public enum fbdOutputs {
        decoded,
        chromaSwap,
        banding,
        falseColour,
        switcher
    };

    public enum oculus_draw_source
    {
        par,
        amd,
        direct
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct config
    {
        public void defaults()
        {

            process = processEngine.none;
            
            resourceDir = "./";

            openclCPU = false;

            isoArray = new float[4];
			
			output.defaults();
            
			input.defaults();
			input.resourceDir = resourceDir;

            gui.defaults();
            
            slicConfig.defaults();
            waveformConfig.defaults();

            func = transfer.g8;
            genericChoice = genericProcess.barcode;

            demoMode = false;
            fbd_output = fbdOutputs.switcher;
            catch_exceptions = true;

            trayraceConfig.defaults();
            generatorConfig.defaults();
            parConfig.defaults();
            radeonConfig.defaults();

            metric_path = "";

            oculusConfig.defaults();
            cameraConfig.defaults();
        }

        public processEngine process;
        public inputConfigStruct input;
        public outputConfigStruct output;
        public guiConfigStruct gui;
		
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
		public float[] isoArray; // iso array. ascending!

		public string resourceDir;

        public transfer func; // for tf testing stuff
        public genericProcess genericChoice;

        [MarshalAs(UnmanagedType.U1)]
        public bool catch_exceptions;
        [MarshalAs(UnmanagedType.U1)]
        public bool openclCPU;

        [MarshalAs(UnmanagedType.U1)]
        public bool demoMode;

        public fbdOutputs fbd_output;

        [StructLayout(LayoutKind.Sequential)]
        public struct slicConfigStruct {
            public void defaults() {
                slicDrawBorders = false;
                slicDrawCenters = false;
                slicFillCells = true;
                slicDrawUniqueness = false;
                slicDrawDistribution = false;
                slicDrawSaliency = false;
            }
        [MarshalAs(UnmanagedType.U1)]
        public bool slicDrawBorders;
        [MarshalAs(UnmanagedType.U1)]
        public bool slicDrawCenters;
        [MarshalAs(UnmanagedType.U1)]
        public bool slicFillCells;
        [MarshalAs(UnmanagedType.U1)]
        public bool slicDrawUniqueness;
        [MarshalAs(UnmanagedType.U1)]
        public bool slicDrawDistribution;
        [MarshalAs(UnmanagedType.U1)]
        public bool slicDrawSaliency;
        } 
        
        public slicConfigStruct slicConfig;
        
        [StructLayout(LayoutKind.Sequential)]
        public struct waveformConfigStruct {
            public void defaults() {
                waveformMode = waveformMode.luma;
            }
            public waveformMode waveformMode;
        }
        public waveformConfigStruct waveformConfig;

        public struct trayraceConfigStruct
        {
            public void defaults()
            {
                viewportPath = "";
                depthPath = "";
                normalsPath = "";
                motionPath = "";
            }

            public string viewportPath;
            public string depthPath;
            public string normalsPath;
            public string motionPath;
        }

        public trayraceConfigStruct trayraceConfig;

        public struct generatorProcessStruct
        {
            public void defaults()
            {
                type = generatorProcess.sand;
                //text_output_string = "Once upon a time...";

                bg_colour = new float[4];
                bg_colour[0] = 1.0f;
                bg_colour[1] = 0.5f;
                bg_colour[2] = 0.5f;
                bg_colour[3] = 1.0f;


                text_colour = new float[4];
                text_colour[0] = 0.9f;
                text_colour[1] = 0.9f;
                text_colour[2] = 0.9f;
                text_colour[3] = 1.0f;
            }

            public generatorProcess type;
            
            public IntPtr text_output_string;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public float[] bg_colour;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public float[] text_colour;
        }
        public generatorProcessStruct generatorConfig;
        
        public parConfigStruct parConfig;
        public radeonConfigStruct radeonConfig;

        public string metric_path;

        public struct oculusConfigStruct
        {
            public void defaults()
            {
                source = oculus_draw_source.direct;
            }

            public oculus_draw_source source;
        }
        public oculusConfigStruct oculusConfig;
        public cameraConfigStruct cameraConfig;

        public struct motionExplorerConfigStruct
        {
            public void defaults()
            {
                follow_stereo = false;
                spherical = false;
            }

            [MarshalAs(UnmanagedType.U1)]
            public bool follow_stereo;
            [MarshalAs(UnmanagedType.U1)]
            public bool spherical;
        };
        public motionExplorerConfigStruct motionExplorer;
    };

}