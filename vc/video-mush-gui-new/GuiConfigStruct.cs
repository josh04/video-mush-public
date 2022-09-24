using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace mush
{

    [StructLayout(LayoutKind.Sequential)]
    public struct guiConfigStruct
    {
        public void defaults()
        {
            show_gui = false;
            sim2preview = false;
            subScreenRows = 8;
            exrDir = ".\\EXR\\";
            fullscreen = false;
        }
        [MarshalAs(UnmanagedType.U1)]
        public bool show_gui;
        [MarshalAs(UnmanagedType.U1)]
        public bool sim2preview;
        public UInt32 subScreenRows; // size of pixel in ints
        public string exrDir;
        [MarshalAs(UnmanagedType.U1)]
        public bool fullscreen;

    }
}
