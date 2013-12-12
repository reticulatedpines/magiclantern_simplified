using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Runtime.InteropServices;
using System.Collections;
using System.Windows.Forms;


namespace mlv_view_sharp
{
    public delegate void MLVBlockHandler(string type, object data, byte[] raw_data, int raw_pos, int raw_length);


    public class MLVReader
    {
        protected BinaryReader[] Reader = null;
        protected MLVBlockHandler Handler = null;
        protected xrefEntry[] BlockIndex = null;

        protected Dictionary<uint, xrefEntry> FrameXrefList = null;

        public string LastType = "";

        /* made public to show debug information */
        public string[] FileNames = null;
        public int FileNum = 0;
        public long FilePos = 0;


        public uint TotalFrameCount = 0;
        public uint FrameRedundantErrors = 0;
        public uint FrameMissingErrors = 0;
        public uint FrameErrors
        {
            get
            {
                return FrameRedundantErrors + FrameMissingErrors;
            }
        }

        public int CurrentBlockNumber = 0;
        public int MaxBlockNumber
        {
            get
            {
                return BlockIndex.Length - 1;
            }
        }


        public MLVReader()
        {
        }

        public MLVReader(string fileName, MLVBlockHandler handler)
        {
            /* ensure that we load the main file first */
            fileName = fileName.Substring(0, fileName.Length - 2) + "LV";

            /* load files and build internal index */
            OpenFiles(fileName);

            if (FileNames == null)
            {
                throw new ArgumentException();
            }

            BuildIndex();

            Handler = handler;
        }

        private bool FilesValid()
        {
            MLVTypes.mlv_file_hdr_t mainFileHeader;
            mainFileHeader.fileGuid = 0;

            for (int fileNum = 0; fileNum < Reader.Length; fileNum++)
            {
                byte[] buf = new byte[16];

                Reader[fileNum].BaseStream.Position = 0;

                /* read MLV block header */
                if (Reader[fileNum].Read(buf, 0, 16) != 16)
                {
                    break;
                }

                string type = Encoding.UTF8.GetString(buf, 0, 4);
                if (type != "MLVI")
                {
                    MessageBox.Show("File '" + FileNames[fileNum] + "' has a invalid header.");
                    return false;
                }

                /* seems to be a valid header, proceed */
                UInt32 length = BitConverter.ToUInt32(buf, 4);
                UInt64 timestamp = BitConverter.ToUInt64(buf, 8);

                /* resize buffer to the block size */
                Array.Resize<byte>(ref buf, (int)length);

                /* now read the rest of the block */
                if (Reader[fileNum].Read(buf, 16, (int)length - 16) != (int)length - 16)
                {
                    MessageBox.Show("File '" + FileNames[fileNum] + "' has a invalid header.");
                    return false;
                }

                MLVTypes.mlv_file_hdr_t hdr = (MLVTypes.mlv_file_hdr_t)MLVTypes.ToStruct(type, buf);

                if (hdr.versionString != "v2.0")
                {
                    MessageBox.Show("File '" + FileNames[fileNum] + "' has a invalid version '" + hdr.versionString + "'.");
                    return false;
                }

                if (fileNum == 0)
                {
                    mainFileHeader = hdr;
                }
                else
                {
                    if (mainFileHeader.fileGuid != hdr.fileGuid)
                    {
                        MessageBox.Show("File '" + FileNames[fileNum] + "' has a different GUID from the main .MLV. Please delete or rename it to fix that issue.");
                        return false;
                    }
                }
            }

            return true;
        }

        protected void OpenFiles(string fileName)
        {
            if(!File.Exists(fileName))
            {
                return;
            }

            int fileNum = 1;
            string chunkName = "";

            /* go up to file M99 */
            while (fileNum <= 100)
            {
                chunkName = GetChunkName(fileName, fileNum - 1);
                if (!File.Exists(chunkName))
                {
                    break;
                }
                fileNum++;
            }

            /* initialize structure and load files */
            Reader = new BinaryReader[fileNum];
            FileNames = new string[fileNum];

            FileNames[0] = fileName;
            for (int file = 1; file < fileNum; file++)
            {
                FileNames[file] = GetChunkName(fileName, file - 1);
            }

            /* open files now */
            for(int file = 0; file < fileNum; file++)
            {
                Reader[file] = new BinaryReader(File.Open(FileNames[file], FileMode.Open, FileAccess.Read, FileShare.ReadWrite));
            }
        }

        protected string GetChunkName(string fileName, int fileNum)
        {
            return fileName.Substring(0, fileName.Length - 2) + fileNum.ToString("D2");
        }


        protected class xrefEntry
        {
            public UInt64 timestamp;
            public long position;
            public int fileNumber;
        }

        private void BuildIndex()
        {
            if (Reader == null)
            {
                return;
            }

            ArrayList list = new ArrayList();
            Dictionary<uint, xrefEntry> frameXrefList = new Dictionary<uint, xrefEntry>();

            TotalFrameCount = 0;

            for(int fileNum = 0; fileNum < Reader.Length; fileNum++)
            {
                Reader[fileNum].BaseStream.Position = 0;

                while (Reader[fileNum].BaseStream.Position < Reader[fileNum].BaseStream.Length - 16)
                {
                    byte[] buf = new byte[16];
                    long offset = Reader[fileNum].BaseStream.Position;

                    /* read MLV block header and seek to next block */
                    if (Reader[fileNum].Read(buf, 0, 16) != 16)
                    {
                        break;
                    }

                    uint size = BitConverter.ToUInt32(buf, 4);
                    string type = Encoding.UTF8.GetString(buf, 0, 4);

                    /* just skip NULL blocks */
                    if (type != "NULL")
                    {
                        /* create xref entry */
                        xrefEntry xref = new xrefEntry();

                        xref.fileNumber = fileNum;
                        xref.position = offset;

                        if (type == "MLVI")
                        {
                            byte[] mlviBuf = new byte[size];

                            /* go back to header start */
                            Reader[fileNum].BaseStream.Position = offset;
                            if (Reader[fileNum].Read(mlviBuf, 0, mlviBuf.Length) != mlviBuf.Length)
                            {
                                break;
                            }

                            /* sum up the frame counts in all chunks */
                            MLVTypes.mlv_file_hdr_t header = (MLVTypes.mlv_file_hdr_t)MLVTypes.ToStruct("MLVI", mlviBuf);
                            TotalFrameCount += header.videoFrameCount;

                            /* at this position there is the file version string */
                            xref.timestamp = 0;
                        }
                        else if (type == "VIDF")
                        {
                            /* hardcoded block size, better to use marshal? */
                            byte[] vidfBuf = new byte[256];

                            /* go back to header start */
                            Reader[fileNum].BaseStream.Position = offset;
                            if (Reader[fileNum].Read(vidfBuf, 0, vidfBuf.Length) != vidfBuf.Length)
                            {
                                break;
                            }

                            MLVTypes.mlv_vidf_hdr_t header = (MLVTypes.mlv_vidf_hdr_t)MLVTypes.ToStruct("VIDF", vidfBuf);

                            if (!frameXrefList.ContainsKey(header.frameNumber))
                            {
                                frameXrefList.Add(header.frameNumber, xref);
                                xref.timestamp = BitConverter.ToUInt64(buf, 8);
                            }
                            else
                            {
                                FrameRedundantErrors++;
                                //MessageBox.Show("File " + FileNames[0] + " contains frame #" + header.frameNumber + " more than once!");
                            }
                        }
                        else
                        {
                            xref.timestamp = BitConverter.ToUInt64(buf, 8);
                        }
                        list.Add(xref);
                    }

                    /* skip block */
                    Reader[fileNum].BaseStream.Position = offset + size;
                }
            }

            /* update global indices */
            BlockIndex = ((xrefEntry[])list.ToArray(typeof(xrefEntry))).OrderBy(x => x.timestamp).ToArray<xrefEntry>();
            FrameXrefList = frameXrefList;

            /* check for non-header blocks with the same timestamp */
            xrefEntry prev = new xrefEntry();
            prev.timestamp = 0;
            prev.fileNumber = 0;

            foreach (var indexEntry in BlockIndex)
            {
                if (indexEntry.position != 0 && indexEntry.timestamp == prev.timestamp)
                {
                    FrameRedundantErrors++;
                }
                prev = indexEntry;
            }

            uint curFrame = 0;
            foreach (var elem in frameXrefList.OrderBy(elem => elem.Key))
            {
                if (elem.Key != curFrame)
                {
                    curFrame = elem.Key;
                    FrameMissingErrors++;
                }
                curFrame++;
            }
        }

        public virtual int GetFrameBlockNumber(uint frameNumber)
        {
            if(frameNumber < 1)
            {
                return 0;
            }

            try
            {
                xrefEntry xref = FrameXrefList[frameNumber - 1];
                int pos = Array.IndexOf(BlockIndex, Array.Find(BlockIndex, elem => elem.timestamp == xref.timestamp)) + 1;

                return Math.Min(pos, BlockIndex.Length - 1);
            }
            catch (Exception e)
            {
                e.ToString();
            }

            return -1;
        }


        public virtual bool ReadBlock()
        {
            if (Reader == null)
            {
                return false;
            }

            byte[] buf = new byte[16];
            FileNum = BlockIndex[CurrentBlockNumber].fileNumber;
            FilePos = BlockIndex[CurrentBlockNumber].position;

            /* seek to current block pos */
            Reader[FileNum].BaseStream.Position = FilePos;

            /* if there are not enough blocks anymore */
            if (Reader[FileNum].BaseStream.Position >= Reader[FileNum].BaseStream.Length - 16)
            {
                return false;
            }

            /* read MLV block header */
            if (Reader[FileNum].Read(buf, 0, 16) != 16)
            {
                return false;
            }

            string type = Encoding.UTF8.GetString(buf, 0, 4);
            UInt32 length = BitConverter.ToUInt32(buf, 4);
            UInt64 timestamp = BitConverter.ToUInt64(buf, 8);

            /* resize buffer to the block size */
            Array.Resize<byte>(ref buf, (int)length);

            /* now read the rest of the block */
            if (Reader[FileNum].Read(buf, 16, (int)length - 16) != (int)length - 16)
            {
                return false;
            }

            var data = MLVTypes.ToStruct(type, buf);
            int headerLength = Marshal.SizeOf(data.GetType());

            Handler(type, data, buf, headerLength, buf.Length - headerLength);

            LastType = type;

            return true;
        }

        public virtual void Close()
        {
            if (Reader == null)
            {
                return;
            }

            for (int fileNum = 0; fileNum < Reader.Length; fileNum++)
            {
                Reader[fileNum].Close();
            }
            Reader = null;
        }
    }
}
