using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Interop;
using System.Runtime.InteropServices;
using System.Globalization;
using System.IO;
using System.Threading;

namespace video_mush_gui_new
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {

        mush.config config;
        private Thread th, th2, th3;
        private bool closing;

        DataOutput data_output_window;
        ParProperties par_properties_window;
        RadeonProperties radeon_properties_window;
        NodeWindow node_window;

        [DllImport("mush-core", CharSet = CharSet.Ansi)]
        public static extern bool getLog(StringBuilder output, UInt64 size);

        [DllImport("mush-core")]
        public static extern void endLog();

        [DllImport("video-mush")]
        public static extern void videoMushRunAll(IntPtr config);

        [DllImport("video-mush")]
        public static extern void videoMushDestroy();


        [DllImport("video-mush")]
        public static extern void videoMushExecute(int ar_size, [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.U1)] ref bool[] use_outputs);
        [DllImport("video-mush")]
        public static extern UInt32 videoMushGetOutputCount();
        [DllImport("video-mush")]
        public static extern bool videoMushGetOutputName(StringBuilder name, UInt64 size);

        public MainWindow()
        {
            try
            {
                InitializeComponent();
            }
            catch (Exception e)
            {
                System.Console.WriteLine("arrgh");
                System.Console.WriteLine(e.InnerException.Message);
            }

            showGUI.IsChecked = Properties.Settings.Default.showGUI;
            showSIM2.IsChecked = Properties.Settings.Default.showSIM2;
            testcard.IsChecked = Properties.Settings.Default.testcard;
            switch (Properties.Settings.Default.cameraRadio)
            {
                case 0:
                default:
                    radioCanon.IsChecked = true;
                    break;
                case 1:
                    radioFlare.IsChecked = true;
                    break;
            }

            switch (Properties.Settings.Default.mergeRadio)
            {
                case 0:
                default:
                    noMerge.IsChecked = true;
                    break;
                case 1:
                    exposures2.IsChecked = true;
                    break;
                case 2:
                    exposures3.IsChecked = true;
                    break;
                case 3:
                    dualISO.IsChecked = true;
                    break;
            }

            if (Properties.Settings.Default.iso1 != "")
            {
                iso1.Text = Properties.Settings.Default.iso1;
            }
            if (Properties.Settings.Default.iso2 != "")
            {
                iso2.Text = Properties.Settings.Default.iso2;
            }
            if (Properties.Settings.Default.iso3 != "")
            {
                iso3.Text = Properties.Settings.Default.iso3;
            }
            if (Properties.Settings.Default.iso4 != "")
            {
                iso4.Text = Properties.Settings.Default.iso4;
            }

            gohdrExposure.Text = Properties.Settings.Default.exposure.ToString();
           

            cellBorders.IsChecked = Properties.Settings.Default.cellBorders;
            cellCenters.IsChecked = Properties.Settings.Default.cellCenters;
            fillCells.IsChecked = Properties.Settings.Default.fillCells;
            uniqueness.IsChecked = Properties.Settings.Default.uniqueness;
            distribution.IsChecked = Properties.Settings.Default.distribution;
            saliency.IsChecked = Properties.Settings.Default.saliency;

            transFunc.SelectedIndex = Properties.Settings.Default.func;
            useLegacyPQ.IsChecked = Properties.Settings.Default.useLegacyPQ;
            pqBitDepth.Text = Properties.Settings.Default.bitDepth.ToString();

            switch (Properties.Settings.Default.waveformRadio)
            {
                case 0:
                default:
                    radioLuma.IsChecked = true;
                    break;
                case 1:
                    radioRGB.IsChecked = true;
                    break;
                case 2:
                    radioR.IsChecked = true;
                    break;
                case 3:
                    radioG.IsChecked = true;
                    break;
                case 4:
                    radioB.IsChecked = true;
                    break;
            }

            bilateralModeBox.SelectedIndex = Properties.Settings.Default.bilateralMode;

            inputMethod.SelectedIndex = Properties.Settings.Default.inputMethod;
            processMethod.SelectedIndex = Properties.Settings.Default.processMethod;
            outputMethod.SelectedIndex = Properties.Settings.Default.outputMethod;

            openclCPU.IsChecked = Properties.Settings.Default.openclCPU;
            catchExceptions.IsChecked = Properties.Settings.Default.catchExceptions;

            wbBlue.Text = Properties.Settings.Default.wbBlue;
            wbRed.Text = Properties.Settings.Default.wbRed;
            blackPoint.Text = Properties.Settings.Default.blackPoint;
            clampPoint.Text = Properties.Settings.Default.clamp;
            ffmpegFPS.Text = Properties.Settings.Default.fps;
            ffmpegCRF.Text = Properties.Settings.Default.crf;
            
            resizeInput_Checked(null, null);
            resizeOutput_Checked(null, null);
            showGUI_Checked(null, null);
            testcard_Checked(null, null);
            
            if (data_output_window == null)
            {
                data_output_window = new DataOutput();
            }

            if (par_properties_window == null)
            {
                par_properties_window = new ParProperties();
            }

            if (radeon_properties_window == null)
            {
                radeon_properties_window = new RadeonProperties();
            }

            if (node_window == null)
            {
                node_window = new NodeWindow();
            }
            closing = false;
            th2 = new Thread(() => doOutput());
            th2.Start();

            th3 = new Thread(() => doRowOutput());
            th3.Start();
        }

        private void fillPaths(System.Collections.Specialized.StringCollection paths, ComboBox box)
        {
            if (paths.Count > 10)
            {
                int p = paths.Count - 10;
                for (int i = 0; i < p; i++)
                {
                    paths.RemoveAt(0);
                }
            }

            foreach (String path in paths)
            {
                box.Items.Add(path);
            }
        }

        private void encodeButton_Click(object sender, RoutedEventArgs e)
        {
            if (th != null)
            {
                if (th.IsAlive)
                {
                    try
                    {
                        videoMushDestroy();
                    }
                    catch (Exception ex)
                    {
                        AppendText(ex.ToString());
                    }
                    return;
                }
            }
            config = new mush.config();

            Properties.Settings.Default.showGUI = showGUI.IsChecked.Value;
            Properties.Settings.Default.showSIM2 = showSIM2.IsChecked.Value;
            Properties.Settings.Default.testcard = testcard.IsChecked.Value;
            Properties.Settings.Default.openclCPU = openclCPU.IsChecked.Value;

            if (radioCanon.IsChecked == true) {
                Properties.Settings.Default.cameraRadio = 0;
            }
            if (radioFlare.IsChecked == true)
            {
                Properties.Settings.Default.cameraRadio = 1;
            }

            if (noMerge.IsChecked == true)
            {
                Properties.Settings.Default.mergeRadio = 0;
            }
            if (exposures2.IsChecked == true)
            {
                Properties.Settings.Default.mergeRadio = 1;
            }
            if (exposures3.IsChecked == true)
            {
                Properties.Settings.Default.mergeRadio = 2;
            }
            if (dualISO.IsChecked == true)
            {
                Properties.Settings.Default.mergeRadio = 3;
            }

            try
            {
                float.Parse(iso1.Text, CultureInfo.InvariantCulture.NumberFormat);
                Properties.Settings.Default.iso1 = iso1.Text;
            }
            catch (Exception) { Properties.Settings.Default.iso1 = ""; }
            try
            {
                float.Parse(iso2.Text, CultureInfo.InvariantCulture.NumberFormat);
                Properties.Settings.Default.iso2 = iso2.Text;
            }
            catch (Exception) { Properties.Settings.Default.iso2 = ""; }
            try
            {
                float.Parse(iso3.Text, CultureInfo.InvariantCulture.NumberFormat);
                Properties.Settings.Default.iso3 = iso3.Text;
            }
            catch (Exception) { Properties.Settings.Default.iso3 = ""; }
            try
            {
                float.Parse(iso4.Text, CultureInfo.InvariantCulture.NumberFormat);
                Properties.Settings.Default.iso4 = iso4.Text;
            }
            catch (Exception) { Properties.Settings.Default.iso4 = ""; }

            Properties.Settings.Default.exposure = float.Parse(gohdrExposure.Text, CultureInfo.InvariantCulture.NumberFormat);
            //Properties.Settings.Default.gammacorrect = float.Parse(gohdrGamma.Text, CultureInfo.InvariantCulture.NumberFormat);
         
            switch (processMethod.SelectedIndex)
            {
                case 0:
                    break;
                case 1:
                   break;
                case 2:                   
                    Properties.Settings.Default.cellBorders = cellBorders.IsChecked.Value;
                    Properties.Settings.Default.cellCenters = cellCenters.IsChecked.Value;
                    Properties.Settings.Default.fillCells = fillCells.IsChecked.Value;
                    Properties.Settings.Default.uniqueness = uniqueness.IsChecked.Value;
                    Properties.Settings.Default.distribution = distribution.IsChecked.Value;
                    Properties.Settings.Default.saliency = saliency.IsChecked.Value;

                    break;
                case 3:
                   if (radioLuma.IsChecked == true)
                    {
                        Properties.Settings.Default.waveformRadio = 0;
                    }
                    if (radioRGB.IsChecked == true)
                    {
                        Properties.Settings.Default.waveformRadio = 1;
                    }
                    if (radioR.IsChecked == true)
                    {
                        Properties.Settings.Default.waveformRadio = 2;
                    }
                    if (radioG.IsChecked == true)
                    {
                        Properties.Settings.Default.waveformRadio = 3;
                    }
                    if (radioB.IsChecked == true)
                    {
                        Properties.Settings.Default.waveformRadio = 4;
                    }
                    break;
                case 5:
                    Properties.Settings.Default.func = transFunc.SelectedIndex;
                    Properties.Settings.Default.bitDepth = uint.Parse(pqBitDepth.Text, CultureInfo.InvariantCulture.NumberFormat);
                    Properties.Settings.Default.useLegacyPQ = useLegacyPQ.IsChecked.Value;
                    break;
            }

            Properties.Settings.Default.inputMethod = inputMethod.SelectedIndex;
            Properties.Settings.Default.processMethod = processMethod.SelectedIndex;
            Properties.Settings.Default.outputMethod = outputMethod.SelectedIndex;

            Properties.Settings.Default.bilateralMode = bilateralModeBox.SelectedIndex;
            
            Properties.Settings.Default.catchExceptions = catchExceptions.IsChecked.Value;


            Properties.Settings.Default.wbBlue = wbBlue.Text;

            Properties.Settings.Default.wbRed = wbRed.Text;
            Properties.Settings.Default.blackPoint = blackPoint.Text;
            Properties.Settings.Default.clamp = clampPoint.Text;

            Properties.Settings.Default.fps = ffmpegFPS.Text;
            Properties.Settings.Default.crf = ffmpegCRF.Text;

            Properties.Settings.Default.Save();

            config.defaults();
            config.gui.show_gui = showGUI.IsChecked.Value;
            config.gui.sim2preview = showSIM2.IsChecked.Value;
            config.openclCPU = openclCPU.IsChecked.Value;
            config.gui.fullscreen = fullscreen.IsChecked.Value;

            config.catch_exceptions = catchExceptions.IsChecked.Value;
            
            inputOptions();
            outputOptions();

            runProcess(processOptions(), catchExceptions.IsChecked.Value);
        }

        public void runProcess(mush.processEngine processEngine, bool catch_exception)
        {
            data_output_window.AddRow(new mush.MetricValue[] { });

            var multiDraw = radeon_properties_window.multiDraw.IsChecked.Value;
            th = new Thread(() => catchRunAll(processEngine, catch_exception, multiDraw));
            th.Start();
            /*
            th2 = new Thread(() => doOutput());
            th2.Start();

            th3 = new Thread(() => doRowOutput());
            th3.Start();
            */
        }

        public void catchRunAll(mush.processEngine processEngine, bool catch_exception, bool multiDraw)
        {
            //this.Dispatcher.Invoke((Action)(() => { encodeButton.Content = "Stop"; }));
            this.config.process = processEngine;
            
            
            int new_config_size = Marshal.SizeOf(typeof(mush.config));

            IntPtr new_config_ptr = Marshal.AllocCoTaskMem(new_config_size);

            // Marshaling to ptr
            Marshal.StructureToPtr(this.config, new_config_ptr, false);
            
            if (catch_exception == true)
            {
                try
                {

                    if (this.config.process == mush.processEngine.amd && multiDraw)
                    {

                        this.config.radeonConfig.stereo_displacement = true;
                        this.config.radeonConfig.camera_type = mush.cameraType.perspective;
                        Marshal.StructureToPtr(this.config, new_config_ptr, false);
                        execute(new_config_ptr);

                        this.config.radeonConfig.stereo_displacement = false;
                        this.config.radeonConfig.camera_type = mush.cameraType.perspective;
                        this.config.output.outputName = System.IO.Path.GetFileNameWithoutExtension(this.config.output.outputName) + " left" + System.IO.Path.GetExtension(this.config.output.outputName);

                        Marshal.StructureToPtr(this.config, new_config_ptr, false);
                        execute(new_config_ptr);

                        this.config.radeonConfig.stereo_displacement = true;
                        this.config.radeonConfig.camera_type = mush.cameraType.sphericalEquirectangular;
                        this.config.output.outputName = System.IO.Path.GetFileNameWithoutExtension(this.config.output.outputName) + " right 360" + System.IO.Path.GetExtension(this.config.output.outputName);

                        Marshal.StructureToPtr(this.config, new_config_ptr, false);
                        execute(new_config_ptr);

                        this.config.radeonConfig.stereo_displacement = false;
                        this.config.radeonConfig.camera_type = mush.cameraType.sphericalEquirectangular;
                        this.config.output.outputName = System.IO.Path.GetFileNameWithoutExtension(this.config.output.outputName) + " left 360" + System.IO.Path.GetExtension(this.config.output.outputName);

                        Marshal.StructureToPtr(this.config, new_config_ptr, false);
                        execute(new_config_ptr);
                    } else
                    {
                        execute(new_config_ptr);
                    }
                }
                catch (Exception e)
                {
                    AppendText(e.ToString() + '\n');
                }
            } else {
                execute(new_config_ptr);
            }
            
            // Marshaling back
            //new_config = (mush.config)Marshal.PtrToStructure(new_config_ptr, typeof(mush.config));
            
            // Freeing the "content" of the marshaled struct (the marshaled 
            // string in this case)
            Marshal.DestroyStructure(new_config_ptr, typeof(mush.config));

            // Freeing the memory allocated for the struct object (the 
            // "container")
            Marshal.FreeCoTaskMem(new_config_ptr);

        }

        private void execute(IntPtr new_config_ptr)
        {
            videoMushRunAll(new_config_ptr);

            UInt32 output_count = videoMushGetOutputCount();
            bool[] use_outputs = new bool[output_count];
            AppendText("Outputs: \n");
            for (int i = 0; i < output_count; i++)
            {
                use_outputs[i] = true;
                StringBuilder name = new StringBuilder(4096);
                videoMushGetOutputName(name, 4096);
                AppendText(name.ToString() + '\n');
            }
           
            videoMushExecute(use_outputs.Length, ref use_outputs);
        }

        private bool isClosing()
        {
            return closing;
        }

        public void doOutput()
        {
            while (!isClosing())
            {
                StringBuilder output = new StringBuilder(4096);
                getLog(output, 4096);
                String str = output.ToString();
                if (str.Length > 0)
                {
                    AppendText(str + "\n");
                }
            }
            StringBuilder output2 = new StringBuilder(4096);
            while (getLog(output2, 4096))
            {
                String str = output2.ToString();
                if (str.Length > 0)
                {
                    AppendText(str + "\n");
                }
            }
            if (!this.Dispatcher.HasShutdownStarted)
            {
                {
                    //this.Dispatcher.Invoke((Action)(() => { encodeButton.Content = "Start"; }));
                    endLog();
                }
            }
        }

        public void doRowOutput()
        {
            while (!isClosing())
            {
                UInt32 names_size = 0;
                if (DataOutput.newRowNames(ref names_size))
                {
                    StringBuilder[] name_strings = new StringBuilder[names_size];
                    for (UInt32 i = 0; i < names_size; ++i) {
                        name_strings[i] = new StringBuilder(128);
                        DataOutput.getRowName(name_strings[i], i, 128);
                    }

                    data_output_window.AddRowNames(name_strings);
                }
                UInt32 row_size = DataOutput.getRowCount();
                if (row_size > 0)
                {
                    mush.MetricValue[] arr = new mush.MetricValue[row_size];
                    DataOutput.getRow(arr, row_size);
                    data_output_window.AddRow(arr);
                }
            }
            UInt32 last_row_size = DataOutput.getRowCount();
            while (last_row_size > 0)
            {
                mush.MetricValue[] arr = new mush.MetricValue[last_row_size];
                DataOutput.getRow(arr, last_row_size);
                data_output_window.AddRow(arr);

                last_row_size = DataOutput.getRowCount();
            }
            DataOutput.endRow();
        }


        delegate void AppendTextCallback(string text);

        private void AppendText(string text)
        {
            
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.

            if (!this.logText.Dispatcher.CheckAccess())
            {
                AppendTextCallback d = new AppendTextCallback(AppendText);
                this.Dispatcher.Invoke(d, new object[] { text });
            }
            else
            {
                this.logText.AppendText(text);
                this.logText.SelectionStart = logText.Text.Length;
                this.logText.ScrollToEnd();
            }
        }

        private void inputOptions()
        {

            if (resizeInput.IsChecked == true)
            {
                config.input.resize = true;
                try
                {
                    config.input.resize_width = uint.Parse(inputWidth.Text, CultureInfo.InvariantCulture.NumberFormat);
                }
                catch (Exception)
                {
                    config.input.resize_width = 1280;
                }
                try
                {
                    config.input.resize_height = uint.Parse(inputHeight.Text, CultureInfo.InvariantCulture.NumberFormat);
                }
                catch (Exception)
                {
                    config.input.resize_height = 720;
                }
            }

            if (noMerge.IsChecked == true)
            {
                config.input.exposures = 1;
            }
            if (exposures2.IsChecked == true)
            {
                config.input.exposures = 2;
            }
            if (exposures3.IsChecked == true)
            {
                config.input.exposures = 3;
            }
            if (dualISO.IsChecked == true)
            {
                config.input.dualISO = true;
            }
                try
                {
                    config.input.dual_iso_comp_factor = float.Parse(iso1.Text, CultureInfo.InvariantCulture.NumberFormat);
                }
                catch (Exception)
                {
                    config.input.dual_iso_comp_factor = 1.0f;
                }

                try
                {
                    config.input.frame_skip = uint.Parse(iso2.Text, CultureInfo.InvariantCulture.NumberFormat);
                } catch (Exception)
                {
                    config.input.frame_skip = 0;
                }

                float red = 1.0f, blue = 1.0f;
                try
                {
                    red = float.Parse(wbRed.Text, CultureInfo.InvariantCulture.NumberFormat);
                }
                catch (Exception)
                {
                    red = 2.0f;
                }
                try
                {
                    blue = float.Parse(wbBlue.Text, CultureInfo.InvariantCulture.NumberFormat);
                }
                catch (Exception)
                {
                    blue = 1.5f;
                }

                config.input.whitePoint[0] = red;
                config.input.whitePoint[1] = 1.0f;
                config.input.whitePoint[2] = blue;

            

            try
            {
                config.input.blackPoint = int.Parse(blackPoint.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception)
            {
                config.input.blackPoint = 0;
            }

            if (testcard.IsChecked == true)
            {
                config.input.inputEngine = mush.inputEngine.testCardInput;
            }
            else
            {
                switch (inputMethod.SelectedIndex)
                {
                    case 0:
                        config.input.inputEngine = mush.inputEngine.folderInput;
                        config.input.inputPath = folderPath.Text;
                        config.input.secondInputPath = secondFolderPath.Text;

                        if (loopCheck.IsChecked == true)
                        {
                            config.input.loopFrames = true;
                        }
                        break;
                    case 1:
                        config.input.inputEngine = mush.inputEngine.videoInput;
                        config.input.inputPath = videoPath.Text;
                        config.input.secondInputPath = videoPathSecond.Text;

                        if (videoLoopCheck.IsChecked == true)
                        {
                            config.input.loopFrames = true;
                        }
                        break;
                    case 2:
                        config.input.inputEngine = mush.inputEngine.singleEXRInput;
                        config.input.inputPath = stillPath.Text;
                        config.input.secondInputPath = secondStillPath.Text;
                        config.input.thirdInputPath = thirdStillPath.Text;
                        config.input.fourthInputPath = fourthStillPath.Text;

                        break;
                    case 3:
                        if (radioCanon.IsChecked == true)
                        {
                            config.input.inputEngine = mush.inputEngine.canonInput;
                            if (config.input.exposures == 1)
                            {
                                config.output.fps = 30.0f;
                            }
                            if (config.input.exposures == 2)
                            {
                                config.output.fps = 15.0f;
                            }
                            if (config.input.exposures == 3)
                            {
                                config.output.fps = 10.0f;
                            }
                        }
                        if (radioFlare.IsChecked == true)
                        {
                            config.input.inputEngine = mush.inputEngine.flare10Input;
                            if (config.input.exposures == 1)
                            {
                                config.output.fps = 30.0f;
                            }
                            if (config.input.exposures == 2)
                            {
                                config.output.fps = 15.0f;
                            }
                            if (config.input.exposures == 3)
                            {
                                config.output.fps = 10.0f;
                            }
                            config.input.inputWidth = 1920;
                            config.input.inputHeight = 1080;
                        }
                        break;
                    case 4:
                        config.input.inputEngine = mush.inputEngine.noInput;
                        break;
                }
            }


            try
            {
                config.isoArray[0] = float.Parse(iso1.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { config.isoArray[0] = 0.0f; }
            try
            {
                config.isoArray[1] = float.Parse(iso2.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { config.isoArray[1] = 0.0f; }
            try
            {
                config.isoArray[2] = float.Parse(iso3.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { config.isoArray[2] = 0.0f; }
            try
            {
                config.isoArray[3] = float.Parse(iso4.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { config.isoArray[3] = 0.0f; }

            switch (inputTransfer.SelectedIndex)
            {
                case 0:
                    config.input.func = mush.transfer.linear;
                    break;
                case 1:
                    config.input.func = mush.transfer.srgb;
                    break;
                case 2:
                    config.input.func = mush.transfer.pq;
                    break;
                case 3:
                    config.input.func = mush.transfer.g8;
                    break;
                case 4:
                    config.input.func = mush.transfer.logc;
                    break;
                case 5:
                    config.input.func = mush.transfer.rec709;
                    break;
                case 6:
                    config.input.func = mush.transfer.gamma;
                    config.input.gammacorrect = 1.8f;
                    break;
                case 7:
                    config.input.func = mush.transfer.gamma;
                    config.input.gammacorrect = 2.2f;
                    break;
                case 8:
                    config.input.func = mush.transfer.gamma;
                    config.input.gammacorrect = 2.4f;
                    break;
            }

            try
            {
                config.input.fps = float.Parse(inputFPS.Text, CultureInfo.InvariantCulture.NumberFormat);
            }
            catch (Exception) { config.input.fps = 25.0f; }

            config.input.lock_fps = lockInputFPS.IsChecked.Value;
        }

        private mush.processEngine processOptions()
        {

            cameraOptions();
            config.parConfig = par_properties_window.get_config_struct();
            mush.processEngine processEngine = mush.processEngine.waveform;
            switch (processMethod.SelectedIndex)
            {
                case 0:
                    processEngine = genericOptions();
                    break;
                case 1:
                    processEngine = mush.processEngine.none;
                    break;
                case 2:
                    processEngine = mush.processEngine.slic;
                    slicOptions();
                    break;
                case 3:
                    processEngine = mush.processEngine.waveform;
                    waveformOptions();
                    break;
                case 4:
                    processEngine = mush.processEngine.fixedBitDepth;
                    switch (transFunc.SelectedIndex)
                    {
                        case 0:
                            config.func = mush.transfer.g8;
                            break;
                        case 1:
                            config.func = mush.transfer.pq;
                            break;
                        case 2:
                            config.func = mush.transfer.linear;
                            break;
                    }
                    config.output.yuvBitDepth = uint.Parse(pqBitDepth.Text, CultureInfo.InvariantCulture.NumberFormat);
                    config.output.pqLegacy = useLegacyPQ.IsChecked.Value;
                    break;
                case 5:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.text;

                    //byte[] utf16bytes = Encoding.Default.GetBytes(textBox.Text);
                    //byte[] utf8bytes = Encoding.Convert(Encoding.Unicode, Encoding.UTF8, utf16bytes);
                    byte[] utf8bytes = Encoding.UTF8.GetBytes(textBox.Text);

                    config.generatorConfig.text_output_string = Marshal.AllocHGlobal(utf8bytes.Length);
                    Marshal.Copy(utf8bytes, 0, config.generatorConfig.text_output_string, utf8bytes.Length);


                    // Marshal.FreeHGlobal(config.generatorConfig.text_output_string); could we leak any harder

                    try
                    {
                        config.input.inputWidth = uint.Parse(inputWidth.Text, CultureInfo.InvariantCulture.NumberFormat);
                    }
                    catch (Exception)
                    {
                        config.input.inputWidth = 1280;
                    }
                    try
                    {
                        config.input.inputHeight = uint.Parse(inputHeight.Text, CultureInfo.InvariantCulture.NumberFormat);
                    }
                    catch (Exception)
                    {
                        config.input.inputHeight = 720;
                    }
                    break;
                case 6:
                    processEngine = mush.processEngine.par;

                    if (par_properties_window.parOculusCheck.IsChecked == true)
                    {
                        processEngine = mush.processEngine.oculusDraw;
                        config.parConfig = par_properties_window.get_config_struct();
                        config.oculusConfig.source = mush.oculus_draw_source.par;
                    }

                    config.parConfig.camera_config = config.cameraConfig;
                    break;
                case 7:
                    processEngine = mush.processEngine.amd;


                    config.radeonConfig = radeon_properties_window.get_config_struct(config.cameraConfig);
                    config.radeonConfig.auto_camera_frame_offset = config.output.count_from;

                    break;
                case 8:
                    processEngine = mush.processEngine.metrics;
                    config.metric_path = metricComparePath.Text;
                    break;

                case 9:
                    processEngine = mush.processEngine.motionExplorer;
                    config.motionExplorer.follow_stereo = reprojFollow.IsChecked.Value;
                    config.motionExplorer.spherical = (reprojCamera.SelectedIndex > 0);
                    break;
            }

            config.parConfig.camera_config = config.cameraConfig;
            if (config.cameraConfig.equirectangular)
            {
                config.parConfig.camera_type = mush.par_camera_type.spherical;
                config.radeonConfig.camera_type = mush.cameraType.sphericalEquirectangular;
            }

            return processEngine;
        }

        private mush.processEngine genericOptions() {
            config.input.darken = float.Parse(gohdrExposure.Text, CultureInfo.InvariantCulture.NumberFormat);
            mush.processEngine processEngine;
            switch (genericCombo.SelectedIndex)
            {
                default:
                case 0:
                    processEngine = mush.processEngine.none;
                    break;
                case 2:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.sand;
                    break;
                case 3:
                    processEngine = mush.processEngine.trayrace;
                    break;
                case 4:
                    processEngine = mush.processEngine.generic;
                    config.genericChoice = mush.genericProcess.barcode;
                    break;
                case 5:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.sphere;
                    config.input.inputWidth = 1920;
                    config.input.inputHeight = 1080;
                    break;
                case 6:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.oculusVideo;
                    config.input.inputWidth = 1280;
                    config.input.inputHeight = 720;
                    //config.metric_path = metricComparePath.Text;
                    break;
                case 7:
                    processEngine = mush.processEngine.oculusDraw;
                    config.oculusConfig.source = mush.oculus_draw_source.direct;
                    break;
                case 8:
                    processEngine = mush.processEngine.generic;
                    config.genericChoice = mush.genericProcess.fisheye2equirectangular;
                    break;
                case 9:
                    processEngine = mush.processEngine.generic;
                    config.genericChoice = mush.genericProcess.sobel;
                    break;
                case 10:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.motionReprojection;
                    break;
                case 11:
                    processEngine = mush.processEngine.motionExplorer;
                    break;
                case 12:
                    processEngine = mush.processEngine.waveform;
                    break;
                case 13:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.anaglyph;
                    config.metric_path = metricComparePath.Text;
                    break;
                case 14:
                    processEngine = mush.processEngine.generic;
                    config.genericChoice = mush.genericProcess.videoAverage;
                    break;
                case 15:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.mse;
                    break;
                case 16:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.sbsPack;
                    break;
                case 17:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.sbsUnpack;
                    break;
                case 18:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.raster;
                    config.radeonConfig = radeon_properties_window.get_config_struct(config.cameraConfig);
                    break;
                case 19:
                    processEngine = mush.processEngine.sand;
                    config.generatorConfig.type = mush.generatorProcess.motionGenerator;
                    break;
            }

            return processEngine;
        }

        private void cameraOptions()
        {
            float.TryParse(generic_camera_x.Text, out config.cameraConfig.position_x);
            float.TryParse(generic_camera_y.Text, out config.cameraConfig.position_y);
            float.TryParse(generic_camera_z.Text, out config.cameraConfig.position_z);
            float.TryParse(generic_camera_theta.Text, out config.cameraConfig.position_theta);
            float.TryParse(generic_camera_phi.Text, out config.cameraConfig.position_phi);

            config.cameraConfig.model_path = genericCameraModelPath.Text;
            config.cameraConfig.load_path = genericCameraLoadPath.Text;
            config.cameraConfig.save_path = genericCameraSavePath.Text;

            config.cameraConfig.autocam = genericAutoCam.IsChecked.Value;
            config.cameraConfig.display_cam = genericDisplayCam.IsChecked.Value;

            int.TryParse(genericCameraWidth.Text, out config.cameraConfig.width);
            int.TryParse(genericCameraHeight.Text, out config.cameraConfig.height);
            float.TryParse(genericCameraSpeed.Text, out config.cameraConfig.speed);
            float.TryParse(genericCameraFov.Text, out config.cameraConfig.fov);

            float.TryParse(genericModelScale.Text, out config.cameraConfig.model_scaling);

            config.cameraConfig.stereo = genericStereo.IsChecked.Value;
            config.cameraConfig.equirectangular = genericSpherical.IsChecked.Value;
        }

        private void slicOptions()
        {
            config.input.darken = float.Parse(gohdrExposure.Text, CultureInfo.InvariantCulture.NumberFormat);

            config.slicConfig.slicDrawBorders = cellBorders.IsChecked.Value;
            config.slicConfig.slicDrawCenters = cellCenters.IsChecked.Value;
            config.slicConfig.slicDrawDistribution = distribution.IsChecked.Value;
            config.slicConfig.slicDrawSaliency = saliency.IsChecked.Value;
            config.slicConfig.slicDrawUniqueness = uniqueness.IsChecked.Value;
            config.slicConfig.slicFillCells = fillCells.IsChecked.Value;
        }

        private void waveformOptions()
        {
            config.input.darken = float.Parse(gohdrExposure.Text, CultureInfo.InvariantCulture.NumberFormat);

            if (radioLuma.IsChecked == true)
            {
                config.waveformConfig.waveformMode = mush.waveformMode.luma;
            }
            if (radioRGB.IsChecked == true)
            {
                config.waveformConfig.waveformMode = mush.waveformMode.rgb;
            }
            if (radioR.IsChecked == true)
            {
                config.waveformConfig.waveformMode = mush.waveformMode.r;
            }
            if (radioG.IsChecked == true)
            {
                config.waveformConfig.waveformMode = mush.waveformMode.g;
            }
            if (radioB.IsChecked == true)
            {
                config.waveformConfig.waveformMode = mush.waveformMode.b;
            }
        }

        private void outputOptions()
        {
            switch (outputMethod.SelectedIndex)
            {
                case 0:
                    config.output.outputEngine = mush.outputEngine.libavformatOutput;

                    try
                    {
                        FileInfo outp = new FileInfo(ffmpegPath.Text);
                        config.output.outputName = outp.Name;
                        config.output.outputPath = outp.Directory.FullName;
                    }
                    catch (System.ArgumentException e)
                    {
                        config.output.outputName = "";
                        config.output.outputPath = "";
                    }

                    switch (ffmpegTransfer.SelectedIndex)
                    {
                        case 0:
                            config.output.func = mush.transfer.srgb;
                            break;
                        case 1:
                            config.output.func = mush.transfer.g8;
                            break;
                        case 2:
                            config.output.func = mush.transfer.pq;
                            break;
                        case 3:
                            config.output.func = mush.transfer.linear;
                            break;
                    }

                    switch (ffmpegEncoder.SelectedIndex)
                    {
                        case 0:
                            config.output.encodeEngine = mush.encodeEngine.x264;
                            break;
                        case 1:
                            config.output.encodeEngine = mush.encodeEngine.x265;
                            break;
                        case 2:
                            config.output.encodeEngine = mush.encodeEngine.vpx;
                            break;
                        case 3:
                            config.output.encodeEngine = mush.encodeEngine.prores;
                            break;
                    }
                    try
                    {
                        config.output.fps = float.Parse(ffmpegFPS.Text, CultureInfo.InvariantCulture.NumberFormat);
                    } catch (Exception)
                    {
                        config.output.fps = 25.0f;
                    }
                    try
                    {
                        config.output.crf = uint.Parse(ffmpegCRF.Text, CultureInfo.InvariantCulture.NumberFormat);
                    }
                    catch (Exception)
                    {
                        config.output.crf = 24;
                    }
                    //config.outputConfig.useHLS = true;
                    //config.outputConfig.zerolatency = true;
                    break;
                case 1:
                    config.output.outputEngine = mush.outputEngine.noOutput;
                    switch (imageOutputType.SelectedIndex)
                    {
                        case 0: // EXR
                            config.output.encodeEngine = mush.encodeEngine.exr;

                            try
                            {
                                config.output.count_from = int.Parse(exrCountOffset.Text, CultureInfo.InvariantCulture.NumberFormat);
                            }
                             catch (Exception)
                            {
                                config.output.count_from = 0;
                            }


                            break;
                        case 1: // GIF89a
                            config.output.encodeEngine = mush.encodeEngine.gif;
                            break;
                    }

                    try
                    {
                        config.output.fps = float.Parse(imageFPS.Text, CultureInfo.InvariantCulture.NumberFormat);
                    }
                    catch (Exception)
                    {
                        config.output.fps = 25.0f;
                    }

                    try
                    {
                        FileInfo outpEXR = new FileInfo(exrPath.Text);
                        config.output.outputName = outpEXR.Name;
                        config.output.outputPath = outpEXR.Directory.FullName;
                    }
                    catch (System.ArgumentException e)
                    {
                        config.output.outputName = "";
                        config.output.outputPath = "";
                    }

                    break;
                case 2:
                    config.output.outputEngine = mush.outputEngine.noOutput;
                    config.output.encodeEngine = mush.encodeEngine.none;

                    break;

            }
        }

        private void outputMethod_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

        }

        private void resizeInput_Checked(object sender, RoutedEventArgs e)
        {
                if (resizeInput.IsChecked == true)
                {
                    inputWidth.IsEnabled = true;
                    inputHeight.IsEnabled = true;
                }
                else
                {
                    inputWidth.IsEnabled = false;
                    inputHeight.IsEnabled = false;
                }
        }

        private void resizeOutput_Checked(object sender, RoutedEventArgs e)
        {
                if (resizeOutput.IsChecked == true)
                {
                    outputWidth.IsEnabled = true;
                    outputHeight.IsEnabled = true;
                }
                else
                {
                    outputWidth.IsEnabled = false;
                    outputHeight.IsEnabled = false;
                }
            }

        private void showGUI_Checked(object sender, RoutedEventArgs e)
        {
            if (showGUI.IsChecked == true)
            {
                showSIM2.IsEnabled = true;
            } else
            {
                showSIM2.IsEnabled = false;
            }
        }

        private void testcard_Checked(object sender, RoutedEventArgs e)
        {
            if (testcard.IsChecked == true)
            {
                inputMethod.IsEnabled = false;
            } else
            {
                inputMethod.IsEnabled = true;
            }
        }

        private void button_Click(object sender, RoutedEventArgs e)
        {
            data_output_window.Show();
            data_output_window.Focus();
        }

        private void parButtonClock(object sender, RoutedEventArgs e)
        {

            par_properties_window.Show();
            par_properties_window.Focus();
        }

        private void radeonButtonClock(object sender, RoutedEventArgs e)
        {

            radeon_properties_window.Show();
            radeon_properties_window.Focus();
        }

        private void nodeButton_Click(object sender, RoutedEventArgs e)
        {
            node_window.Show();
            node_window.Focus();
        }

        private void ffmpegEncoder_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

        }

        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            closing = true;
            data_output_window.OverrideClose();
            data_output_window = null;
            par_properties_window.OverrideClose();
            par_properties_window = null;
            radeon_properties_window.OverrideClose();
            radeon_properties_window = null;
            node_window.OverrideClose();
            node_window = null;
        }

    }
}
public class ComboBoxItem
{
    public string Value;
    public string Text;

    public ComboBoxItem(string val, string text)
    {
        Value = val;
        Text = text;
    }

    public override string ToString()
    {
        return Text;
    }
}