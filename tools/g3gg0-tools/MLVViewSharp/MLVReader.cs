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
        public xrefEntry[] BlockIndex = null;

        public Dictionary<uint, frameXrefEntry> VidfXrefList = new Dictionary<uint,frameXrefEntry>();
        public Dictionary<uint, frameXrefEntry> AudfXrefList = new Dictionary<uint,frameXrefEntry>();

        public string LastType = "";

        /* made public to show debug information */
        public string IndexName = null;
        public string[] FileNames = null;
        public int FileNum = 0;
        public long FilePos = 0;

        public uint HighestVideoFrameNumber = 0;
        public uint TotalVideoFrameCount = 0;
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
                throw new FileNotFoundException("File '" + fileName + "' does not exist.");
            }

            UpdateIndex();

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
            IndexName = fileName.ToUpper().Replace(".MLV", ".IDX");

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

        
        public class xrefEntry
        {
            public UInt64 timestamp;
            public long position;
            public int size;
            public int fileNumber;
            public string type;
        }

        public class frameXrefEntry
        {
            public object[] metadata;
            public UInt64 timestamp;
            public int blockIndexPos;
        }

        protected byte[] ReadBlockData(BinaryReader reader)
        {
            long pos = reader.BaseStream.Position;

            /* read block header */
            byte[] buf = new byte[8];

            if (reader.Read(buf, 0, buf.Length) != buf.Length)
            {
                throw new Exception("Failed to read file");
            }

            UInt32 length = BitConverter.ToUInt32(buf, 4);

            /* resize buffer to the block size */
            Array.Resize<byte>(ref buf, (int)length);

            /* now read the block */
            reader.BaseStream.Position = pos;
            if (reader.Read(buf, 0, buf.Length) != buf.Length)
            {
                throw new Exception("Failed to read file");
            }

            return buf;
        }

        protected MLVTypes.mlv_file_hdr_t ReadMainHeader()
        {
            Reader[0].BaseStream.Position = 0;
            byte[] block = ReadBlockData(Reader[0]);

            return (MLVTypes.mlv_file_hdr_t)MLVTypes.ToStruct("MLVI", block);
        }

        private void UpdateIndex()
        {
            if (File.Exists(IndexName))
            {
                try
                {
                    LoadIndex();
                    BuildFrameIndex();
                    return;
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.ToString());
                }
            }

            BuildIndex();
            BuildFrameIndex();
            SaveIndex();
        }


        private void LoadIndex()
        {
            BinaryReader index = new BinaryReader(File.Open(IndexName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite));

            /* read MLV block header */
            byte[] mlvBlockBuf = ReadBlockData(index);

            MLVTypes.mlv_file_hdr_t fileHeader = (MLVTypes.mlv_file_hdr_t)MLVTypes.ToStruct("MLVI", mlvBlockBuf);
            MLVTypes.mlv_file_hdr_t mainHeader = ReadMainHeader();

            if (mainHeader.fileGuid != fileHeader.fileGuid)
            {
                throw new Exception("GUID mismatch");
            }

            byte[] xrefBlockBuf = ReadBlockData(index);
            MLVTypes.mlv_xref_hdr_t xrefHeader = (MLVTypes.mlv_xref_hdr_t)MLVTypes.ToStruct("XREF", xrefBlockBuf);

            BlockIndex = new xrefEntry[xrefHeader.entryCount];
            int entrySize = Marshal.SizeOf(new MLVTypes.mlv_xref_t());

            for (int pos = 0; pos < xrefHeader.entryCount; pos++)
            {
                MLVTypes.mlv_xref_t xrefEntry = (MLVTypes.mlv_xref_t)MLVTypes.ToStruct("XREF_ENTRY", xrefBlockBuf, Marshal.SizeOf(xrefHeader) + pos * entrySize);

                BlockIndex[pos] = new xrefEntry();
                BlockIndex[pos].fileNumber = xrefEntry.fileNumber;
                BlockIndex[pos].position = (long)xrefEntry.frameOffset;
                BlockIndex[pos].size = 0;

                switch(xrefEntry.frameType)
                {
                    case 0:
                        BlockIndex[pos].type = "";
                        break;
                    case 1:
                        BlockIndex[pos].type = "VIDF";
                        break;
                    case 2:
                        BlockIndex[pos].type = "AUDF";
                        break;
                }
            }
        }

        public class MetadataContainer
        {
            private Dictionary<string, object> metadata = new Dictionary<string, object>();

            public void Update(string type, object data)
            {
                if (!metadata.ContainsKey(type))
                {
                    metadata.Add(type, data);
                }
                else
                {
                    metadata[type] = data;
                }
            }
            
            public object[] Metadata
            {
                get
                {
                    return metadata.Values.ToArray<object>();
                }
            }
        }

        public virtual void BuildFrameIndex()
        {
            if (Reader == null)
            {
                return;
            }

            HighestVideoFrameNumber = 0;
            TotalVideoFrameCount = 0;

            Dictionary<uint, frameXrefEntry> vidfXrefList = new Dictionary<uint, frameXrefEntry>();
            Dictionary<uint, frameXrefEntry> audfXrefList = new Dictionary<uint, frameXrefEntry>();
            MetadataContainer metadataContainer = new MetadataContainer();
            uint highestFrameNumber = 0;

            for (int blockIndexPos = 0; blockIndexPos < BlockIndex.Length; blockIndexPos++)            
            {
                var block = BlockIndex[blockIndexPos];

                Reader[block.fileNumber].BaseStream.Position = block.position;

                /* 16 bytes are enough for size, type and timestamp, but we try to read all blocks up to 1k */
                byte[] buf = new byte[1024];

                /* read MLV block header */
                if (Reader[block.fileNumber].Read(buf, 0, 16) != 16)
                {
                    break;
                }

                uint size = BitConverter.ToUInt32(buf, 4);
                string type = Encoding.UTF8.GetString(buf, 0, 4);
                UInt64 timestamp = BitConverter.ToUInt64(buf, 8);

                /* read that block, up to 256 byte */
                Reader[block.fileNumber].BaseStream.Position = block.position;

                /* read MLV block header */
                int readSize = (int)Math.Min(size, 256);
                if (Reader[block.fileNumber].Read(buf, 0, readSize) != readSize)
                {
                    break;
                }

                object blockData = MLVTypes.ToStruct(buf);

                switch (type)
                {
                    case "NULL":
                        continue;

                    case "VIDF":
                        {
                            MLVTypes.mlv_vidf_hdr_t header = (MLVTypes.mlv_vidf_hdr_t)blockData;
                            if (!vidfXrefList.ContainsKey(header.frameNumber))
                            {
                                frameXrefEntry entry = new frameXrefEntry();
                                entry.blockIndexPos = blockIndexPos;
                                entry.metadata = metadataContainer.Metadata;
                                entry.timestamp = timestamp;
                                vidfXrefList.Add(header.frameNumber, entry);
                            }
                            else
                            {
                                FrameRedundantErrors++;
                            }
                            highestFrameNumber = Math.Max(highestFrameNumber, header.frameNumber);
                        }
                        break;

                    case "AUDF":
                        {
                            MLVTypes.mlv_audf_hdr_t header = (MLVTypes.mlv_audf_hdr_t)blockData;
                            if (!audfXrefList.ContainsKey(header.frameNumber))
                            {
                                frameXrefEntry entry = new frameXrefEntry();
                                entry.blockIndexPos = blockIndexPos;
                                entry.metadata = metadataContainer.Metadata;
                                entry.timestamp = timestamp;
                                audfXrefList.Add(header.frameNumber, entry);
                            }
                            else
                            {
                                FrameRedundantErrors++;
                            }
                        }
                        break;

                    default:
                        metadataContainer.Update(type, blockData);
                        break;
                }
            }

            /* count the number of missing video frames */
            uint curFrame = 0;
            foreach (var elem in vidfXrefList.OrderBy(elem => elem.Key))
            {
                if (elem.Key != curFrame)
                {
                    curFrame = elem.Key;
                    FrameMissingErrors++;
                }
                curFrame++;
            }

            VidfXrefList = vidfXrefList;
            AudfXrefList = audfXrefList;
            TotalVideoFrameCount = (uint)vidfXrefList.Count;
            HighestVideoFrameNumber = highestFrameNumber;
        }


        public virtual void BuildIndex()
        {
            if (Reader == null)
            {
                return;
            }

            ArrayList list = new ArrayList();

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

                    if (size < 0x10 || size > 50 * 1024 * 1024)
                    {
                        MessageBox.Show("File '" + FileNames[fileNum] + "' has a an invalid block at offset 0x" + offset.ToString("X8") + ".");
                        break;
                    }

                    /* just skip NULL blocks */
                    if (type == "NULL")
                    {
                        Reader[fileNum].BaseStream.Position = offset + size;
                        continue;
                    }

                    /* create xref entry */
                    xrefEntry xref = new xrefEntry();

                    xref.fileNumber = fileNum;
                    xref.position = offset;
                    xref.size = (int)size;
                    xref.timestamp = BitConverter.ToUInt64(buf, 8);
                    xref.type = type;

                    if (type == "MLVI")
                    {
                        /* at this position there is the file version string */
                        xref.timestamp = 0;
                    }
                    if (type == "VIDF" && xref.timestamp == 0)
                    {
                        /* at this position there is the file version string */
                        xref.timestamp = 0xDEADBEEF;
                    }

                    list.Add(xref);

                    /* skip block */
                    Reader[fileNum].BaseStream.Position = offset + size;
                }
            }

            /* update global indices */
            BlockIndex = ((xrefEntry[])list.ToArray(typeof(xrefEntry))).OrderBy(x => x.timestamp).ToArray<xrefEntry>();
        }

        public virtual void SaveIndex()
        {
            if (BlockIndex == null || BlockIndex.Length == 0)
            {
                return;
            }

            MLVTypes.mlv_file_hdr_t fileHeader = ReadMainHeader();
            MLVTypes.mlv_xref_hdr_t xrefHdr = new MLVTypes.mlv_xref_hdr_t();

            /* update MLVI header */
            fileHeader.blockSize = (uint)Marshal.SizeOf(fileHeader);
            fileHeader.videoFrameCount = 0;
            fileHeader.audioFrameCount = 0;
            fileHeader.fileNum = (ushort)FileNames.Length;

            /* create XREF block */
            xrefHdr.blockType = "XREF";
            xrefHdr.blockSize = (uint)(Marshal.SizeOf(xrefHdr) + BlockIndex.Length * Marshal.SizeOf(new MLVTypes.mlv_xref_t()));
            xrefHdr.timestamp = 1;
            xrefHdr.frameType = 3; /* video+audio */
            xrefHdr.entryCount = (uint)BlockIndex.Length;

            /* open file */
            BinaryWriter writer = new BinaryWriter(new FileStream(IndexName, FileMode.Create, FileAccess.Write));

            writer.Write(MLVTypes.ToByteArray(fileHeader));
            writer.Write(MLVTypes.ToByteArray(xrefHdr));

            foreach (var entry in BlockIndex)
            {
                MLVTypes.mlv_xref_t xrefEntry = new MLVTypes.mlv_xref_t();

                xrefEntry.fileNumber = (ushort)entry.fileNumber;
                xrefEntry.frameOffset = (ulong)entry.position;
                xrefEntry.empty = 0;
                switch(entry.type)
                {
                    case "VIDF":
                        xrefEntry.frameType = 1;
                        break;
                    case "AUDF":
                        xrefEntry.frameType = 2;
                        break;
                    default:
                        xrefEntry.frameType = 0;
                        break;
                }

                writer.Write(MLVTypes.ToByteArray(xrefEntry));
            }

            writer.Close();
        }

        public virtual object[] GetVideoFrameMetadata(uint frameNumber)
        {
            try
            {
                if (!VidfXrefList.ContainsKey(frameNumber))
                {
                    return null;
                }
                frameXrefEntry xref = VidfXrefList[frameNumber];
                return xref.metadata;
            }
            catch (Exception e)
            {
                e.ToString();
            }

            return null;
        }

        public virtual object[] GetAudioFrameMetadata(uint frameNumber)
        {
            try
            {
                if (!AudfXrefList.ContainsKey(frameNumber))
                {
                    return null;
                }
                frameXrefEntry xref = AudfXrefList[frameNumber];
                return xref.metadata;
            }
            catch (Exception e)
            {
                e.ToString();
            }

            return null;
        }

        public virtual int GetVideoFrameBlockNumber(uint frameNumber)
        {
            try
            {
                if(!VidfXrefList.ContainsKey(frameNumber))
                {
                    return -1;
                }
                frameXrefEntry xref = VidfXrefList[frameNumber];
                return xref.blockIndexPos;
            }
            catch (Exception e)
            {
                e.ToString();
            }

            return -1;
        }

        public virtual int GetAudioFrameBlockNumber(uint frameNumber)
        {
            try
            {
                if (!AudfXrefList.ContainsKey(frameNumber))
                {
                    return -1;
                }
                frameXrefEntry xref = AudfXrefList[frameNumber];
                return xref.blockIndexPos;
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

            FileNum = BlockIndex[CurrentBlockNumber].fileNumber;
            FilePos = BlockIndex[CurrentBlockNumber].position;
            int size = Math.Max(BlockIndex[CurrentBlockNumber].size, 16);
            byte[] buf = new byte[size];

            /* seek to current block pos */
            Reader[FileNum].BaseStream.Position = FilePos;

            /* if there are not enough blocks anymore */
            if (Reader[FileNum].BaseStream.Position >= Reader[FileNum].BaseStream.Length - 16)
            {
                return false;
            }

            /* read MLV block header */
            if (Reader[FileNum].Read(buf, 0, size) != size)
            {
                return false;
            }

            string type = Encoding.UTF8.GetString(buf, 0, 4);
            UInt32 length = BitConverter.ToUInt32(buf, 4);
            UInt64 timestamp = BitConverter.ToUInt64(buf, 8);

            /* oh, size mismatch. boo. */
            if (size != length)
            {
                /* resize buffer to the block size */
                Array.Resize<byte>(ref buf, (int)length);

                /* now read the rest of the block */
                Reader[FileNum].BaseStream.Position = FilePos;
                if (Reader[FileNum].Read(buf, 0, (int)length) != (int)length)
                {
                    return false;
                }
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
