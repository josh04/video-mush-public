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
using System.Globalization;
using System.IO;
using System.Threading;
using System.Data;

namespace video_mush_gui_new
{
    /// <summary>
    /// Interaction logic for DataOutput.xaml
    /// </summary>
    public partial class DataOutput : Window
    {

        public DataTable data_table { get; set; }

        [DllImport("mush-core", CharSet = CharSet.Ansi)]
        public static extern UInt32 getRowCount();

        [DllImport("mush-core", CharSet = CharSet.Ansi)]
        public static extern bool getRow([MarshalAs(UnmanagedType.LPArray)] [Out] mush.MetricValue[] output, UInt32 size);

        [DllImport("mush-core", CharSet = CharSet.Ansi)]
        public static extern bool endRow();


        [DllImport("mush-core", CharSet = CharSet.Ansi)]
        public static extern bool newRowNames(ref UInt32 count);

        [DllImport("mush-core", CharSet = CharSet.Ansi)]
        public static extern void getRowName(StringBuilder name, UInt32 index, UInt64 max_string_length);
        


        public DataOutput()
        {
            InitializeComponent();

            data_table = new DataTable();
            dataGrid.DataContext = data_table.DefaultView;
        }

        public void reset()
        {
            data_table = null;
            data_table = new DataTable();
            dataGrid.DataContext = data_table.DefaultView;
        }

        UInt32 row_primary = 0;
        UInt32 col_primary = 0;


        delegate void AddRowNamesCallback(StringBuilder[] names);


        public void AddRowNames(StringBuilder[] names)
        {
            if (!this.dataGrid.Dispatcher.CheckAccess())
            {
                AddRowNamesCallback d = new AddRowNamesCallback(AddRowNames);
                this.Dispatcher.Invoke(d, new object[] { names });
            }
            else
            {
                bool edited_columns = false;

                while (names.Length > data_table.Columns.Count)
                {
                    //data_table.Columns.Add();
                    data_table.Columns.Add(new DataColumn("crud " + col_primary.ToString()));
                    /*DataGridTextColumn textColumn = new DataGridTextColumn();
                    textColumn.Header = "crud " + col_primary.ToString();
                    textColumn.Binding = new Binding("crud " + col_primary.ToString());
                    //dataGrid.Columns.Add(textColumn); */
                    col_primary++;

                    edited_columns = true;
                }

                //data_table.Columns[0].ColumnName = "Frame";

                for (int i = 0; i < names.Length; i++)
                {
                    data_table.Columns[i].ColumnName = names[i].ToString();
                }
                
                dataGrid.DataContext = null;
                dataGrid.DataContext = data_table.DefaultView;
                
            }
        }

        delegate void AddRowCallback(mush.MetricValue[] row);

        public void AddRow(mush.MetricValue[] row)
        {
            if (!this.dataGrid.Dispatcher.CheckAccess())
            {
                AddRowCallback d = new AddRowCallback(AddRow);
                this.Dispatcher.Invoke(d, new object[] { row });
            }
            else
            {
                List<object> object_floats = new List<object> { /* row_primary */ };
                //object_floats.AddRange(row.Cast<object>().ToList());

                foreach (mush.MetricValue m in row)
                {
                    switch (m.t)
                    {
                        case mush.MetricValueType.f:
                            object_floats.Add(m.floating_point);
                            break;
                        case mush.MetricValueType.i:
                            object_floats.Add(m.integer);
                            break;
                    }
                }
                if (row.Length > 0)
                {
                    if (row[0].is_average == true)
                    {
                        object_floats[0] = "Avg. " + (row[0].integer).ToString() + " Frames";
                    }
                }

                /*for (int i = 0; i < row_primary; ++i)
                {
                    object_floats.Add(row_primary);
                }*/

                bool edited_columns = false;
                while (object_floats.Count > data_table.Columns.Count)
                {
                    //data_table.Columns.Add();
                    data_table.Columns.Add(new DataColumn("crud " + col_primary.ToString()));
                    /*DataGridTextColumn textColumn = new DataGridTextColumn();
                    textColumn.Header = "crud " + col_primary.ToString();
                    textColumn.Binding = new Binding("crud " + col_primary.ToString());
                    //dataGrid.Columns.Add(textColumn); */
                    col_primary++;

                    edited_columns = true;
                }

                data_table.BeginLoadData();
                data_table.LoadDataRow(object_floats.ToArray<object>(), true);
                data_table.EndLoadData();
                
                row_primary++;
                /*
                foreach (DataRow r in data_table.Rows)
                {
                    Console.WriteLine();
                    for (int x = 0; x < data_table.Columns.Count; x++)
                    {
                        Console.Write(r[x].ToString() + " ");
                    }
                }
                */
                if (edited_columns)
                {
                    dataGrid.DataContext = null;
                    dataGrid.DataContext = data_table.DefaultView;
                }
            }
        }

        public void Save()
        {
            var path = saveFileDialog();
            if (path.Length > 0)
            {
                StringBuilder sb = new StringBuilder();

                IEnumerable<string> columnNames = data_table.Columns.Cast<DataColumn>().
                                                  Select(column => column.ColumnName);
                sb.AppendLine(string.Join(",", columnNames));

                foreach (DataRow row in data_table.Rows)
                {
                    IEnumerable<string> fields = row.ItemArray.Select(field => field.ToString());
                    sb.AppendLine(string.Join(",", fields));
                }

                File.WriteAllText(path, sb.ToString());
            }
        }


        private String saveFileDialog()
        {
            
            Microsoft.WindowsAPICodePack.Dialogs.CommonFileDialog dialog;
            dialog = new Microsoft.WindowsAPICodePack.Dialogs.CommonSaveFileDialog();
            dialog.DefaultExtension = "csv";

            Microsoft.WindowsAPICodePack.Dialogs.CommonFileDialogResult ans = dialog.ShowDialog();

            // Process open file dialog box results 
            if (ans == Microsoft.WindowsAPICodePack.Dialogs.CommonFileDialogResult.Ok)
            {
                String val = dialog.FileName;
                return val;
            }

            return "";
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
        }

        private void button_Click(object sender, RoutedEventArgs e)
        {
            Save();
        }
    }
}
