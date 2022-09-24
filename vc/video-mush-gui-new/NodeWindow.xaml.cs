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
using System.Windows.Shapes;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using System.Globalization;
using System.IO;
using System.Threading;
using System.Collections.ObjectModel;

namespace video_mush_gui_new
{
    /// <summary>
    /// Interaction logic for NodeWindow.xaml
    /// </summary>
    public partial class NodeWindow : Window
    {

        [DllImport("mush-core", CharSet = CharSet.Ansi)]
        public static extern bool getLog(StringBuilder output, UInt64 size);
        [DllImport("mush-core")]
        public static extern void endLog();


        [DllImport("mush-core")]
        public static extern UInt32 node_get_total_node_count();
        [DllImport("mush-core")]
        public static extern UInt32 node_get_total_processor_count();
        [DllImport("mush-core")]
        public static extern UInt32 node_get_processor_node_count(UInt32 processor_id);
        [DllImport("mush-core")]
        public static extern void node_get_all_processors([In, Out] UInt32[] processors, UInt32 count);

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct node_info
        {
            public UInt32 _id;
            //public UInt32 _name_mem_length;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 255)]
            public string _name;
            [MarshalAs(UnmanagedType.U1)]
            public bool _is_inited;
            [MarshalAs(UnmanagedType.U1)]
            public bool _is_gui;


            public UInt32 id
            {
                get { return _id; }
                set { _id = value; }
            }
            /*public UInt32 name_mem_length
            {
                get { return _name_mem_length; }
                set { _name_mem_length = value; }
            }*/
            public string name
            {
                get { return _name; }
                set { _name = value; }
            }
            public bool is_inited
            {
                get { return _is_inited; }
                set { _is_inited = value; }
            }
            public bool is_gui
            {
                get { return _is_gui; }
                set { _is_gui = value; }
            }
        }

        [DllImport("mush-core")]
        public static extern bool node_get_all_nodes([In, Out] node_info[] output, UInt32 count);
        [DllImport("mush-core")]
        public static extern bool node_get_all_nodes_from_processor(UInt32 processor_id, [In, Out] UInt32[] output, UInt32 count);
        [DllImport("mush-core")]
        public static extern UInt32 node_get_node_type_count();

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct node_type_storage
        {
            public UInt32 _type;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 255)]
            public string _name;


            public UInt32 type {
                get { return _type; }
                set { _type = value; }
            }
            public string name
            {
                get { return _name; }
                set { _name = value; }
            }
        }

        [DllImport("mush-core")]
        public static extern void node_get_all_node_types([In, Out] node_type_storage[] output, UInt32 count);


        [DllImport("mush-core")]
        public static extern UInt32 node_get_linked_node_count(UInt32 node_id);
        [DllImport("mush-core")]
        public static extern bool node_get_linked_nodes(UInt32 node_id, [In, Out] UInt32[] output, UInt32 count);

        [DllImport("mush-core")]
        public static extern UInt32 node_create_processor();
        [DllImport("mush-core")]
        public static extern UInt32 node_add_new_node(UInt32 node_type_id);
        [DllImport("mush-core")]
        public static extern void node_add_node_to_processor(UInt32 processor_id, UInt32 node_id);
        [DllImport("mush-core")]
        public static extern UInt32 node_add_new_node_to_processor(UInt32 processor_id, UInt32 node_type_id);
        [DllImport("mush-core")]
        public static extern void node_add_node_to_link(UInt32 node_id, UInt32 node_id_to_link);

        [DllImport("mush-core")]
        public static extern void node_get_node_name(UInt32 node_id, StringBuilder output, UInt32 count);

        [DllImport("mush-core")]
        public static extern void node_run_inits();
        [DllImport("mush-core")]
        public static extern void node_create_gui();
        [DllImport("mush-core")]
        public static extern void node_update_gui();
        [DllImport("mush-core")]
        public static extern void node_add_to_gui(UInt32 node_id);
        [DllImport("mush-core")]
        public static extern void node_process(UInt32 node_id);
        [DllImport("mush-core")]
        public static extern bool node_is_inited(UInt32 node_id);


        [StructLayout(LayoutKind.Sequential)]
        public struct vnode_add_new_input_return
        {
            public UInt32 processor_id;
            public UInt32 i_id;
            public UInt32 pre_id;
        }

        [DllImport("video-mush")]
        public static extern vnode_add_new_input_return vnode_add_new_input(mush.inputConfigStruct input_config);
        [DllImport("video-mush")]
        public static extern bool vnode_spawn_input_thread(UInt32 input);
        [DllImport("video-mush")]
        public static extern void vnode_create_dx_context(IntPtr hwnd);
        [DllImport("video-mush")]
        public static extern bool vnode_has_dx_context();
        [DllImport("video-mush")]
        public static extern IntPtr vnode_get_dx_texture();

        ObservableCollection<node_type_storage> _node_types;

        public struct node_name {
            public UInt32 id { get; set; }
            public string name { get; set; }
        }
        ObservableCollection<node_name> _node_names;
        ObservableCollection<node_info> _all_nodes;

        public NodeWindow()
        {
            _node_types = new ObservableCollection<node_type_storage>();
            _node_names = new ObservableCollection<node_name>();
            _all_nodes = new ObservableCollection<node_info>();
            //_node_types[2] = "kay";
            Resources["types"] = _node_types;
            Resources["node_names"] = _node_names;
            Resources["all_nodes"] = _all_nodes;
            InitializeComponent();

        }
        
        bool override_closed = false;

        public void OverrideClose()
        {
            override_closed = true;
            Close();
        }

        // Disclaimer, untested! 
        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            if (!override_closed)
            {
                e.Cancel = true;  // cancels the window close    
                this.Hide();      // Programmatically hides the window
            }
            else
            {
                closing = true;
                if (gui_thread != null)
                {
                    if (gui_thread.IsAlive)
                    {
                        gui_thread.Join();
                    }
                }
            }
        }

        private void addNode_Click(object sender, RoutedEventArgs e)
        {
            if (typeBox.SelectedItem != null)
            {
                UInt32 node_id = node_add_new_node((uint)typeBox.SelectedValue);
            }
            refresh();
        }

        private void addProcessor_Click(object sender, RoutedEventArgs e)
        {
            node_create_processor();
            refresh();
        }

        private void addInput_Click(object sender, RoutedEventArgs e)
        {
            mush.inputConfigStruct config = new mush.inputConfigStruct();
            config.defaults();
            config.inputEngine = mush.inputEngine.testCardInput;
            vnode_add_new_input(config);

            refresh();
        }
        private void dxButton_Click(object sender, RoutedEventArgs e)
        {
            Window window = Window.GetWindow(this);
            var wih = new WindowInteropHelper(window);
            IntPtr hWnd = wih.Handle;

            vnode_create_dx_context(hWnd);

            IntPtr ptr = vnode_get_dx_texture();
            //dxImage.Width = 512;
            //dxImage.Height = 512;
            //displayImage.Width = 256;
            //displayImage.Height = 256;

            dxImage.Lock();
            // Repeatedly calling SetBackBuffer with the same IntPtr is 
            // a no-op. There is no performance penalty.
            dxImage.SetBackBuffer(D3DResourceType.IDirect3DSurface9, ptr);
            //HRESULT.Check(Render());
            dxImage.AddDirtyRect(new Int32Rect(0, 0, (int)dxImage.Width, (int)dxImage.Height));
            dxImage.Unlock();

            refresh();
        }
        private void getProcessors_Click(object sender, RoutedEventArgs e)
        {

            UInt32 processor_count = node_get_total_processor_count();
            UInt32[] processor_array = new UInt32[processor_count];
            node_get_all_processors(processor_array, processor_count);

            var sel_ind2 = processorBox.SelectedIndex;
            processorBox.Items.Clear();
            foreach (var p in processor_array)
            {
                processorBox.Items.Add(p.ToString());
            }
            processorBox.SelectedIndex = sel_ind2;


            UInt32 all_node_count = node_get_total_node_count();
            node_info[] all_node_array = new node_info[all_node_count];
            node_get_all_nodes(all_node_array, all_node_count);

            var sel_ind = allNodeBox.SelectedIndex;
            //allNodeBox.Items.Clear();

            _node_names.Clear();
            _all_nodes.Clear();

            foreach (var p in all_node_array)
            {
                //allNodeBox.Items.Add(p.ToString());


                var node_n = new node_name();
                node_n.id = p.id;

                //StringBuilder name_string = new StringBuilder(255);
                //node_get_node_name(p, name_string, 255);
                node_n.name = p.name; //name_string.ToString();
                _node_names.Add(node_n);

                _all_nodes.Add(p);
            }
            allNodeBox.SelectedIndex = sel_ind;


            //refresh();
        }

        public void refresh()
        {
            UInt32 node_count = node_get_total_node_count();
            UInt32 processor_count = node_get_total_processor_count();
            bool has_dx = vnode_has_dx_context();

            nodesCount.Text = node_count.ToString();
            processorsCount.Text = processor_count.ToString();
            dxStatus.Text = has_dx.ToString();
            getProcessors_Click(null, null);
            processorBox_SelectionChanged(processorBox, null);
            nodeBox_SelectionChanged(nodeBox, null);
            allNodeBox_SelectionChanged(allNodeBox, null);

            getNodeTypes();
        }

        private void getNodeTypes()
        {


            UInt32 type_count = node_get_node_type_count();
            node_type_storage[] type_array = new node_type_storage[type_count];
            /*for (int i = 0; i < type_count; i++)
            {
                type_array[i].name = new string(' ', 255);
            }*/
            node_get_all_node_types(type_array, type_count);
            
            var sel_ind = typeBox.SelectedIndex;
            //typeBox.Items.Clear();
            _node_types.Clear();
            foreach (var p in type_array)
            {
                _node_types.Add(p);
            }
            //typeBox.GetBindingExpression(ListBox.ItemsSourceProperty).UpdateTarget();
            //typeBox.ItemsSource = null;
           // typeBox.ItemsSource = _node_types;
            typeBox.SelectedIndex = sel_ind;
            
        }

        private void processorBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if ((sender as ListBox).SelectedItem != null)
            {
                UInt32 processor_id = uint.Parse(((sender as ListBox).SelectedItem as string));
                UInt32 node_count = node_get_processor_node_count(processor_id);
                UInt32[] node_array = new UInt32[node_count];
                bool success = node_get_all_nodes_from_processor(processor_id, node_array, node_count);

                var sel_ind = nodeBox.SelectedIndex;
                nodeBox.Items.Clear();
                foreach (var p in node_array)
                {
                    nodeBox.Items.Add(p.ToString());
                }
                nodeBox.SelectedIndex = sel_ind;

            }
        }
        private void nodeBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if ((sender as ListBox).SelectedItem != null)
            {
                UInt32 node_id = uint.Parse(((sender as ListBox).SelectedItem as string));
                UInt32 link_count = node_get_linked_node_count(node_id);
                UInt32[] link_array = new UInt32[link_count];
                bool success = node_get_linked_nodes(node_id, link_array, link_count);

                var sel_ind = linkBox.SelectedIndex;
                linkBox.Items.Clear();
                foreach (var p in link_array)
                {
                    linkBox.Items.Add(p.ToString());
                }
                linkBox.SelectedIndex = sel_ind;

            }
        }

        private void allNodeBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if ((sender as ListBox).SelectedItem != null)
            {
                UInt32 node_id = (uint)(sender as ListBox).SelectedValue;
                UInt32 link_count = node_get_linked_node_count(node_id);
                UInt32[] link_array = new UInt32[link_count];
                bool success = node_get_linked_nodes(node_id, link_array, link_count);

                var sel_ind = linkBox2.SelectedIndex;
                linkBox2.Items.Clear();
                foreach (var p in link_array)
                {
                    linkBox2.Items.Add(p.ToString());
                }
                linkBox2.SelectedIndex = sel_ind;

            }
        }

        private void addLink_Click(object sender, RoutedEventArgs e)
        {
            if (nodeBox.SelectedItem != null && allNodeBox.SelectedItem != null)
            {
                node_add_node_to_link(UInt32.Parse(nodeBox.SelectedItem as string), (uint)allNodeBox.SelectedValue);
            }
            refresh();
        }

        private void addNodeTo_Click(object sender, RoutedEventArgs e)
        {
            if (processorBox.SelectedItem != null && allNodeBox.SelectedItem != null)
            {
                node_add_node_to_processor(UInt32.Parse(processorBox.SelectedItem as string), (uint)allNodeBox.SelectedValue);
            }
            refresh();
        }

        private void runInits_Click(object sender, RoutedEventArgs e)
        {
            node_run_inits();
        }

        private void guiInit_Click(object sender, RoutedEventArgs e)
        {

            gui_thread = new Thread(() => updateGUI());
            gui_thread.Start();
        }

        private Thread gui_thread;
        private bool closing = false;

        private bool isClosing()
        {
            return closing;
        }

        private void updateGUI()
        {
            node_create_gui();
            while (!isClosing())
            {
                node_update_gui();
            }
        }

        private void addGui_Click(object sender, RoutedEventArgs e)
        {
            if (allNodeBox.SelectedItem != null)
            {
                node_add_to_gui((uint)allNodeBox.SelectedValue);
            }
            refresh();
        }

        private void spawnInput_Click(object sender, RoutedEventArgs e)
        {
            if (allNodeBox.SelectedItem != null)
            {
                vnode_spawn_input_thread((uint)allNodeBox.SelectedValue);
            }
        }

        private void process_Click(object sender, RoutedEventArgs e)
        {
            if (allNodeBox.SelectedItem != null)
            {
                node_process((uint)allNodeBox.SelectedValue);
            }
        }
    }
}
