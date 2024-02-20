using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Collections;
using EOSMovieFixer.Atoms;
using System.Threading;

namespace EOSMovieFixer
{
    public partial class EOSMovieFixer : Form
    {
        InputFile InFile = null;
        ArrayList Atoms = null;
        string Version = "v1.01";
        static EOSMovieFixer Instance = null;

        public EOSMovieFixer()
        {
            InitializeComponent();
            Text += " " + Version;

            Instance = this;
        }

        public static void Log(string msg)
        {
            if (Instance.InvokeRequired)
            {
                Instance.BeginInvoke(new Action(() => { Log(msg); }));
            }
            else
            {
                Instance.txtLog.AppendText(msg + Environment.NewLine);
            }
        }

        private void btnLoad_Click(object sender, EventArgs e)
        {
            OpenFileDialog dlg = new OpenFileDialog();

            if(dlg.ShowDialog() == DialogResult.OK)
            {
                txtLog.Clear();
                treeStructure.Nodes.Clear();

                Log("[i] Loading file...");

                try
                {
                    InFile = new InputFile(dlg.FileName);
                    Log("      Size: 0x" + InFile.Length.ToString("X8"));
                    AtomParser parser = new AtomParser();
                    Atoms = parser.ParseFile(InFile);
                    Log("      Atoms in root: " + Atoms.Count);

                    DisplayTree(Atoms);
                }
                catch (Exception ex)
                {
                    Log("");
                    Log("[E] " + ex.ToString());
                    InFile.Close();

                    btnLoad.Enabled = true;
                    btnPatch.Enabled = false;
                    btnSave.Enabled = false;
                    return;
                }

                btnLoad.Enabled = false;
                btnPatch.Enabled = true;
                btnSave.Enabled = false;
                Log("");
                Log("--------------------------------");
            }
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            SaveFileDialog dlg = new SaveFileDialog();

            if (dlg.ShowDialog() == DialogResult.OK)
            {
                Log("[i] Saving file...");

                OutputFile outFile = new OutputFile(dlg.FileName);
                AtomWriter writer = new AtomWriter();

                btnSave.Enabled = false;
                Thread writerThread = new Thread(() =>
                {
                    try
                    {
                        writer.SaveFile(InFile, outFile, Atoms);
                        Log("[i] Done");
                    }
                    catch (Exception ex)
                    {
                        Log("");
                        Log("[E] " + ex.ToString());
                    }

                    BeginInvoke(new Action(() =>
                    {
                        InFile.Close();
                        outFile.Close();

                        btnLoad.Enabled = true;
                        btnPatch.Enabled = false;
                        btnSave.Enabled = false;
                    }));
                });

                writerThread.Start();
            }
        }

        private void DisplayTree(ArrayList atoms)
        {
            TreeNode root = new TreeNode("original");
            BuildTree(root, atoms);
            treeStructure.Nodes.Add(root);
        }

        private void BuildTree(TreeNode root, ArrayList atoms)
        {
            foreach (var obj in atoms)
            {
                if (obj is ContainerAtom)
                {
                    ContainerAtom container = (ContainerAtom)obj;
                    TreeNode node = new TreeNode(container + " C:" + container.Children.Count);
                    BuildTree(node, ((ContainerAtom)obj).Children);
                    root.Nodes.Add(node);
                }
                else
                {
                    LeafAtom leaf = (LeafAtom)obj;
                    TreeNode node = new TreeNode(leaf + "");
                    root.Nodes.Add(node);
                }
            }
        }

        private void btnPatch_Click(object sender, EventArgs e)
        {
            Log("");
            Log("[i] Locating sections");

            try
            {

                ArrayList stcoAtoms = new ArrayList();
                ArrayList stcoChunks = new ArrayList();

                Atom mdat = FindAtom(Atoms, "mdat");
                if (mdat != null)
                {
                    Log("      [mdat] at: 0x" + mdat.PayloadFileOffset.ToString("X8"));
                }
                else
                {
                    Log("      [mdat] not found");
                }

                Atom stco = null;
                int num = 0;
                do
                {
                    stco = FindAtom(Atoms, "stco");

                    if (stco != null)
                    {
                        Log("      [stco#" + num + "] at: 0x" + stco.PayloadFileOffset.ToString("X8"));
                        Log("        Extracting...");

                        ArrayList chunkOffsets = StcoExtractOffsets(stco, mdat);

                        Log("      Extending [stco#" + num + "] to [co64]...");
                        StcoExtend(stco, mdat, chunkOffsets);

                        stcoAtoms.Add(stco);
                        stcoChunks.Add(chunkOffsets);
                        num++;
                    }
                } while (stco != null);

                Log("");
                Log("[i] Removing filler atoms...");
                CleanupAtoms(Atoms);

                Log("");
                Log("[i] Updating atom sizes...");
                UInt64 size = CalculateSizes(Atoms, 0, true);
                Log("      (total size: 0x" + size.ToString("X8") + ")");

                Log("");
                Log("[i] Updating new [co64]...");
                /* now update chunk pointers */
                for (num = 0; num < stcoAtoms.Count; num++)
                {
                    stco = (Atom)stcoAtoms[num];
                    ArrayList chunkOffsets = (ArrayList)stcoChunks[num];
                    Log("      Updating [co64#" + num + "] (" + chunkOffsets.Count + " chunks)...");
                    StcoUpdate(stco, mdat, chunkOffsets);
                }
                Log("");
                Log("--------------------------------");
            }
            catch (Exception ex)
            {
                Log("");
                Log("[E] " + ex.ToString());
            }

            TreeNode root = new TreeNode("patched");
            BuildTree(root, Atoms);
            treeStructure.Nodes.Add(root);

            btnLoad.Enabled = false;
            btnPatch.Enabled = false;
            btnSave.Enabled = true;
        }

        private void CleanupAtoms(ArrayList atoms)
        {
            ArrayList deleted = new ArrayList();

            foreach (var obj in atoms)
            {
                Atom atom = (Atom)obj;

                if (atom.Type == "skip" || atom.Type == "free")
                {
                    Log("      Deleting [" + atom.Type + "]");
                    deleted.Add(atom);
                }

                if (obj is ContainerAtom)
                {
                    CleanupAtoms(((ContainerAtom)obj).Children);
                }
            }

            foreach (var obj in deleted)
            {
                atoms.Remove(obj);
            }
        }

        private int TreeDepth = 0;
        private UInt64 CalculateSizes(ArrayList atoms, UInt64 outFileOffset, bool absolute)
        {
            UInt64 totalSize = 0;

            string tabs = "";
            for (int pos = 0; pos < TreeDepth; pos++)
            {
                tabs += "  ";
            }

            foreach (var obj in atoms)
            {
                if (obj is ContainerAtom)
                {
                    ContainerAtom container = (ContainerAtom)obj;

                    if (absolute)
                    {
                        Log(tabs + "  [" + container.Type + "] size: 0x" + container.TotalLength.ToString("X8") + " bytes");
                    }

                    /* run first with absolute file offset set to zero, as it is not clear yet if we have 8 or 16 byte header */
                    UInt64 containerSize = CalculateSizes(container.Children, 0, false);

                    if ((containerSize + 8) > 0xFFFFFFFF)
                    {
                        container.HeaderLength = 16;
                    }
                    else
                    {
                        container.HeaderLength = 8;
                    }
                    container.TotalLength = container.HeaderLength + containerSize;
                    container.HeaderFileOffset = outFileOffset + totalSize;

                    /* run again with the real absolute file offset */
                    if (absolute)
                    {
                        TreeDepth++;
                        CalculateSizes(container.Children, container.PayloadFileOffset, true);
                        TreeDepth--;
                    }

                    /* now we are done with that container */
                    totalSize += container.TotalLength;

                }
                else
                {
                    LeafAtom leaf = (LeafAtom)obj;
                    UInt64 contentSize = leaf.PayloadLength;

                    /* replace content with our patched? */
                    if (leaf.PayloadData != null)
                    {
                        contentSize = (UInt64)leaf.PayloadData.Length;
                        if (absolute)
                        {
                            Log(tabs + "      [" + leaf.Type + "] Replacing data (0x" + leaf.OriginalPayloadLength.ToString("X8") + " bytes) with 0x" + contentSize.ToString("X8") + " bytes");
                        }
                    }

                    if ((contentSize + 8) > 0xFFFFFFFF)
                    {
                        leaf.HeaderLength = 16;
                    }
                    else
                    {
                        leaf.HeaderLength = 8;
                    }

                    leaf.TotalLength = leaf.HeaderLength + contentSize;
                    leaf.HeaderFileOffset = outFileOffset + totalSize;

                    totalSize += leaf.TotalLength;

                    if (absolute)
                    {
                        Log(tabs + "      [" + leaf.Type + "] size: 0x" + leaf.TotalLength.ToString("X8") + " pos: 0x" + leaf.HeaderFileOffset.ToString("X8"));
                    }
                }
            }

            return totalSize;
        }

        private ArrayList StcoExtractOffsets(Atom stco, Atom mdat)
        {
            ArrayList offsetsInMdat = new ArrayList();
            UInt64 lastOffset = 0;
            UInt64 overflow = 0;
            int entry = 0;

            for (UInt64 pos = 8; pos < stco.PayloadLength; pos += 4)
            {
                UInt64 offset = InFile.ReadUInt32(stco.PayloadFileOffset + pos) + overflow;
                if (lastOffset > offset)
                {
                    offset += 0x0100000000;
                    overflow += 0x0100000000;
                }
                lastOffset = offset;
                if ((lastOffset < mdat.PayloadFileOffset) || ((lastOffset - mdat.PayloadFileOffset) >= mdat.PayloadLength))
                {
                    throw new Exception("Offset not pointing into 'mdat'");
                }

                UInt64 relativeOffset = (UInt64)(lastOffset - mdat.PayloadFileOffset);
                if (entry < 5)
                {
                    Log("        abs: 0x" + offset.ToString("X08") + " -> rel: 0x" + relativeOffset.ToString("X08"));
                }
                offsetsInMdat.Add(relativeOffset);
                entry++;
            }

            Log("        ...");

            UInt32 entries = InFile.ReadUInt32(stco.PayloadFileOffset + 4);
            Log("        (" + offsetsInMdat.Count + " chunks)");

            if (entries != offsetsInMdat.Count)
            {
                throw new Exception("Offset count invalid");
            }

            return offsetsInMdat;
        }

        private void StcoExtend(Atom stco, Atom mdat, ArrayList offsetsInMdat)
        {
            byte[] newPayload = new byte[2 * 4 + offsetsInMdat.Count * 8];

            /* read 8 bytes - version, flags and entry count */
            InFile.ReadBytes(stco.PayloadFileOffset, newPayload, 8);

            /* apply changes */
            stco.Type = "co64";
            stco.PayloadData = newPayload;
        }

        private void StcoUpdate(Atom stco, Atom mdat, ArrayList offsetsInMdat)
        {
            for (int entry = 0; entry < offsetsInMdat.Count; entry++)
            {
                UInt64 relativeOffset = (UInt64)offsetsInMdat[entry];
                UInt64 offset = relativeOffset + mdat.PayloadFileOffset;
                if (entry < 5)
                {
                    Log("        rel: 0x" + relativeOffset.ToString("X08") + " -> abs: 0x" + offset.ToString("X08"));
                }

                byte[] a64 = BitConverter.GetBytes(offset);
                Array.Reverse(a64);
                Array.Copy(a64, 0, stco.PayloadData, 2 * 4 + entry * 8, 8);
            }
            Log("        ...");
        }

        private Atom FindAtom(ArrayList atoms, string name)
        {
            foreach (var obj in atoms)
            {
                if (((Atom)obj).Type == name)
                {
                    return (Atom)obj;
                }

                if (obj is ContainerAtom)
                {
                    ContainerAtom container = (ContainerAtom)obj;
                    Atom sub = FindAtom(((ContainerAtom)obj).Children, name);

                    if (sub != null)
                    {
                        return sub;
                    }
                }
            }

            return null;
        }
    }
}
