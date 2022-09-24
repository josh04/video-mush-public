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
using System.IO;

namespace video_mush_gui_new
{
    /// <summary>
    /// Interaction logic for UserControl1.xaml
    /// </summary>
    public partial class trackingComboBox : UserControl
    {
        public String Text
        {
            get
            {
                return box.Text;
            }
        }
        private String propertyName;
        public String PropertyName
        {
            get
            {
                return this.propertyName;
            }

            set
            {

                this.propertyName = value;
                this.SettingKey = value + "_collection";
                this.SettingPosKey = value + "_position";

                if (!Properties.Settings.Default.Properties.Cast<System.Configuration.SettingsProperty>().Any(prop => prop.Name == SettingKey))
                {

                    System.Configuration.SettingsProperty property = new System.Configuration.SettingsProperty(SettingKey);
                    property.DefaultValue = null;
                    property.IsReadOnly = false;
                    property.SerializeAs = System.Configuration.SettingsSerializeAs.Xml;
                    property.PropertyType = typeof(System.Collections.Specialized.StringCollection);
                    property.Provider = Properties.Settings.Default.Providers["LocalFileSettingsProvider"];
                    property.Attributes.Add(typeof(System.Configuration.UserScopedSettingAttribute), new System.Configuration.UserScopedSettingAttribute());

                    Properties.Settings.Default.Properties.Add(property);

                    System.Configuration.SettingsProperty property2 = new System.Configuration.SettingsProperty(SettingPosKey);
                    property2.DefaultValue = 0;
                    property2.IsReadOnly = false;
                    property2.SerializeAs = System.Configuration.SettingsSerializeAs.String;
                    property2.PropertyType = typeof(int);
                    property2.Provider = Properties.Settings.Default.Providers["LocalFileSettingsProvider"];
                    property2.Attributes.Add(typeof(System.Configuration.UserScopedSettingAttribute), new System.Configuration.UserScopedSettingAttribute());

                    Properties.Settings.Default.Properties.Add(property2);
                    Properties.Settings.Default.Reload();

                }

                if (Properties.Settings.Default[SettingKey] == null)
                {
                    Properties.Settings.Default[SettingKey] = new System.Collections.Specialized.StringCollection();
                    Properties.Settings.Default.Save();
                }

                fillPaths((System.Collections.Specialized.StringCollection)Properties.Settings.Default[SettingKey], box);

                box.SelectedIndex = (int)Properties.Settings.Default[SettingPosKey];
            }
        }

        private String SettingKey;
        private String SettingPosKey;

        public trackingComboBox()
        {
            wantsFolder = false;
            isSave = false;
            extension = "";
            InitializeComponent();
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

        private void commonFileDialogMaker(ComboBox combo, System.Collections.Specialized.StringCollection props, UInt32 chooseNum, bool wantsFolder, String extension, bool isSave)
        {

            if (combo.SelectedIndex == chooseNum)
            {
                Microsoft.WindowsAPICodePack.Dialogs.CommonFileDialog dialog;

                if (isSave == true)
                {
                    dialog = new Microsoft.WindowsAPICodePack.Dialogs.CommonSaveFileDialog();
                }
                else
                {
                    dialog = new Microsoft.WindowsAPICodePack.Dialogs.CommonOpenFileDialog();
                    ((Microsoft.WindowsAPICodePack.Dialogs.CommonOpenFileDialog)dialog).IsFolderPicker = wantsFolder;
                }

                dialog.DefaultExtension = extension;

                Microsoft.WindowsAPICodePack.Dialogs.CommonFileDialogResult ans = dialog.ShowDialog();

                // Process open file dialog box results 
                if (ans == Microsoft.WindowsAPICodePack.Dialogs.CommonFileDialogResult.Ok)
                {
                    String val;
                    if (wantsFolder == true)
                    {
                        val = Directory.Exists(dialog.FileName) ? dialog.FileName : System.IO.Path.GetDirectoryName(dialog.FileName);
                    }
                    else
                    {
                        val = dialog.FileName;
                    }
                    ComboBoxItem itm = new ComboBoxItem(val, val);
                    combo.Items.Add(itm);
                    combo.SelectedItem = itm;

                    if (props != null)
                    {
                        if (!props.Contains(val))
                        {
                            props.Add(val);
                            Properties.Settings.Default.Save();
                        }
                    }
                }
                else
                {
                    combo.SelectedIndex = 0;
                }
            }
        }

        public bool isSave { get; set; }
        public bool wantsFolder { get; set; }
        public String extension { get; set; }

        private void box_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (SettingKey != null)
            {
                // Configure open file dialog box 
                commonFileDialogMaker((ComboBox)sender, (System.Collections.Specialized.StringCollection)Properties.Settings.Default[SettingKey], 1, wantsFolder, extension, isSave);
                Properties.Settings.Default[SettingPosKey] = box.SelectedIndex;
            }
            else
            {
                commonFileDialogMaker((ComboBox)sender, null, 1, wantsFolder, extension, isSave);
            }
            
        }
    }
}
