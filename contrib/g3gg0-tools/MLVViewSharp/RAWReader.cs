using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using uint8_t = System.Byte;
using uint16_t = System.UInt16;
using uint32_t = System.UInt32;
using uint64_t = System.UInt64;
using int8_t = System.Byte;
using int16_t = System.Int16;
using int32_t = System.Int32;
using int64_t = System.Int64;
using System.Runtime.InteropServices;
using System.Collections;
using System.IO;

namespace mlv_view_sharp
{
    /* file footer data */
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    struct lv_rec_file_footer_t
    {
        [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 4)]
        public byte[] blockTypeData;
        public string magic
        {
            get
            {
                return Encoding.ASCII.GetString(blockTypeData);
            }
            set
            {
                blockTypeData = Encoding.ASCII.GetBytes(value);
            }
        }
        public int16_t xRes;
        public int16_t yRes;
        public int32_t frameSize;
        public int32_t frameCount;
        public int32_t frameSkip;
        public int32_t sourceFpsx1000;
        public int32_t reserved3;
        public int32_t reserved4;
        public MLVTypes.raw_info raw_info;
    }

    internal static class RAWHelper
    {
        internal static T ReadStruct<T>(this byte[] buffer)
            where T : struct
        {
            GCHandle handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            T result = (T)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(T));
            handle.Free();
            return result;
        }
    }

    public class RAWReader : MLVReader
    {
        private MLVTypes.mlv_file_hdr_t FileHeader;
        private MLVTypes.mlv_rawi_hdr_t RawiHeader;
        private MLVTypes.mlv_vidf_hdr_t VidfHeader;
        private lv_rec_file_footer_t Footer;
        private byte[] FrameBuffer;

        public RAWReader()
        {
        }

        public RAWReader(string fileName, MLVBlockHandler handler)
        {
            Handler = handler;

            /* ensure that we load the main file first */
            fileName = fileName.Substring(0, fileName.Length - 2) + "AW";

            /* load files and build internal index */
            OpenFiles(fileName);

            if (FileNames == null)
            {
                throw new FileNotFoundException("File '" + fileName + "' does not exist.");
            }

            RawiHeader = ReadFooter();
            BuildIndex();

            /* fill with dummy values */
            FileHeader.fileMagic = "MLVI";
            FileHeader.audioClass = 0;
            FileHeader.videoClass = 1;
            FileHeader.blockSize = (uint)Marshal.SizeOf(FileHeader);
        }

        public override void BuildIndex()
        {
            ArrayList list = new ArrayList();
            Dictionary<uint, xrefEntry> frameXrefList = new Dictionary<uint, xrefEntry>();
            long currentFileOffset = 0;
            int fileNum = 0;
            int block = 0;

            while (fileNum < Reader.Length)
            {
                /* create xref entry */
                xrefEntry xref = new xrefEntry();

                xref.fileNumber = fileNum;
                xref.position = currentFileOffset;
                xref.size = 0;
                xref.timestamp = (ulong)(block * (1000000000.0d / Footer.sourceFpsx1000));

                list.Add(xref);

                frameXrefList.Add((uint)block, xref);

                /* increment position */
                block++;
                currentFileOffset += Footer.frameSize;

                /* if the read goes beyond the file end, go to next file */
                if (currentFileOffset > Reader[fileNum].BaseStream.Length)
                {
                    currentFileOffset -= Reader[fileNum].BaseStream.Length;
                    fileNum++;
                }

                /* the last block isnt a frame, but the footer. so skip that one. */
                if ((fileNum == Reader.Length - 1) && (Reader[fileNum].BaseStream.Length - currentFileOffset < Footer.frameSize))
                {
                    break;
                }
            }

            BlockIndex = ((xrefEntry[])list.ToArray(typeof(xrefEntry))).OrderBy(x => x.timestamp).ToArray<xrefEntry>();

            FileHeader.videoFrameCount = (uint)BlockIndex.Length;
        }

        public override void BuildFrameIndex()
        {
            Dictionary<uint, frameXrefEntry> vidfXrefList = new Dictionary<uint, frameXrefEntry>();

            for (int blockIndexPos = 0; blockIndexPos < BlockIndex.Length; blockIndexPos++)
            {
                var block = BlockIndex[blockIndexPos];

                if (!vidfXrefList.ContainsKey((uint)blockIndexPos))
                {
                    frameXrefEntry entry = new frameXrefEntry();
                    entry.blockIndexPos = blockIndexPos;
                    entry.metadata = new object[] { FileHeader, RawiHeader };
                    entry.timestamp = (ulong)(blockIndexPos + 1);
                    vidfXrefList.Add((uint)blockIndexPos, entry);
                }
            }

            VidfXrefList = vidfXrefList;
        }

        public override void SaveIndex()
        {
        }

        private MLVTypes.mlv_rawi_hdr_t ReadFooter()
        {
            MLVTypes.mlv_rawi_hdr_t rawi = new MLVTypes.mlv_rawi_hdr_t();
            int fileNum = FileNames.Length - 1;
            int headerSize = Marshal.SizeOf(typeof(lv_rec_file_footer_t));
            byte[] buf = new byte[headerSize];

            Reader[fileNum].BaseStream.Position = Reader[fileNum].BaseStream.Length - headerSize;

            if (Reader[fileNum].Read(buf, 0, headerSize) != headerSize)
            {
                throw new ArgumentException();
            }

            Reader[fileNum].BaseStream.Position = 0;

            string type = Encoding.UTF8.GetString(buf, 0, 4);
            if (type != "RAWM")
            {
                throw new ArgumentException();
            }

            Footer = RAWHelper.ReadStruct<lv_rec_file_footer_t>(buf);

            /* now forge necessary information */
            rawi.raw_info = Footer.raw_info;
            rawi.xRes = (ushort)Footer.xRes;
            rawi.yRes = (ushort)Footer.yRes;

            FrameBuffer = new byte[Footer.frameSize];

            rawi.blockType = "RAWI";
            rawi.blockSize = (uint)Marshal.SizeOf(rawi);

            return rawi;
        }


        public override bool ReadBlock()
        {
            if (Reader == null)
            {
                return false;
            }

            if (CurrentBlockNumber == 0)
            {
                Handler("MLVI", FileHeader, FrameBuffer, 0, 0);
                Handler("RAWI", RawiHeader, FrameBuffer, 0, 0);
            }

            int fileNum = BlockIndex[CurrentBlockNumber].fileNumber;
            long filePos = BlockIndex[CurrentBlockNumber].position;

            /* seek to current block pos */
            Reader[fileNum].BaseStream.Position = filePos;

            int read = Reader[fileNum].Read(FrameBuffer, 0, Footer.frameSize);
            if (read != Footer.frameSize)
            {
                if (fileNum >= Reader.Length || read < 0)
                {
                    return false;
                }

                /* second part from the frame is in the next file */
                fileNum++;
                Reader[fileNum].BaseStream.Position = 0;
                if (Reader[fileNum].Read(FrameBuffer, read, Footer.frameSize - read) != Footer.frameSize - read)
                {
                    return false;
                }
            }

            VidfHeader.blockType = "VIDF";
            VidfHeader.frameSpace = 0;
            VidfHeader.frameNumber = (uint)CurrentBlockNumber;
            VidfHeader.blockSize = (uint)Marshal.SizeOf(VidfHeader);

            Handler("VIDF", VidfHeader, FrameBuffer, 0, Footer.frameSize);

            LastType = "VIDF";

            return true;
        }
    }
}
