using mlv_view_sharp;
using NAudio.Wave;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media.Imaging;

namespace WebDAVServer
{
    public class MLVAccessor
    {
        private class MLVCachedReader
        {
            public string file;
            public MLVHandler handler;
            public MLVReader reader;
            public DateTime lastUseTime;
            public uint[] frameList;
        }

        private static Dictionary<string, MLVCachedReader> MLVCache = new Dictionary<string, MLVCachedReader>();

        private static MLVCachedReader GetReader(string mlvFileName)
        {
            lock (MLVCache)
            {
                if (MLVCache.ContainsKey(mlvFileName))
                {
                    MLVCache[mlvFileName].lastUseTime = DateTime.Now;
                    return MLVCache[mlvFileName];
                }

                MLVCachedReader entry = new MLVCachedReader();
                entry.file = mlvFileName;
                entry.handler = new MLVHandler();

                /* for JPG, use medium quality */
                entry.handler.SelectDebayer(1);
                entry.handler.UseCorrectionMatrices = false;
                entry.handler.HighlightRecovery = false;

                /* instanciate a reader */
                entry.reader = new MLVReader(mlvFileName, entry.handler.BlockHandler);
                entry.lastUseTime = DateTime.Now;

                /* and create indices */
                entry.reader.BuildIndex();
                entry.reader.BuildFrameIndex();
                entry.reader.SaveIndex();

                while (entry.reader.ReadBlock())
                {
                    /* read until important blocks are filled and we reached a VIDF */
                    if (entry.handler.VidfHeader.blockSize != 0 && entry.handler.RawiHeader.blockSize != 0 && (entry.handler.WaviHeader.blockSize != 0 || entry.handler.FileHeader.audioClass == 0))
                    {
                        break;
                    }

                    if (entry.reader.CurrentBlockNumber < entry.reader.MaxBlockNumber - 1)
                    {
                        entry.reader.CurrentBlockNumber++;
                    }
                    else
                    {
                        break;
                    }
                }

                /* create a list of all frames in this file */
                int pos = 0;
                entry.frameList = new uint[entry.reader.VidfXrefList.Count];
                foreach (var frameNumber in entry.reader.VidfXrefList.Keys)
                {
                    entry.frameList[pos++] = frameNumber;
                }

                MLVCache.Add(mlvFileName, entry);

                return entry;
            }
        }

        public static BitmapSource BitmapSourceFromBitmap(Bitmap bmp)
        {
            return Imaging.CreateBitmapSourceFromHBitmap(bmp.GetHbitmap(), IntPtr.Zero, Int32Rect.Empty, BitmapSizeOptions.FromEmptyOptions());
        }

        public static Stream GetDataStream(string mlvFileName, string content)
        {
            MLVCachedReader cache = GetReader(mlvFileName);
            string type = GetFileType(content);
            int frame = GetFrameNumber(mlvFileName, content);

            /* read video frame with/out debayering dependig on filetype */
            switch (type)
            {
                case "DNG":
                    cache.handler.DebayeringEnabled = false;
                    break;
                case "JPG":
                    cache.handler.DebayeringEnabled = true;
                    break;
                case "WAV":
                    return GetWaveDataStream(mlvFileName);
            }

            /* seek to the correct block */
            int block = cache.reader.GetVideoFrameBlockNumber((uint)frame);
            if (block < 0)
            {
                return new MemoryStream(new byte[0]);
            }

            /* read it */
            cache.reader.CurrentBlockNumber = block;
            cache.handler.VidfHeader.blockSize = 0;
            cache.reader.ReadBlock();

            /* now the VIDF should be read correctly */
            if (cache.handler.VidfHeader.blockSize == 0)
            {
                return new MemoryStream(new byte[0]);
            }

            switch(type)
            {
                case "DNG":
                    object[] metadata = cache.reader.GetVideoFrameMetadata((uint)frame);
                    return DNGCreator.Create(mlvFileName, cache.handler.VidfHeader, cache.handler.RawPixelData, metadata);

                case "JPG":
                    JpegBitmapEncoder encoder = new JpegBitmapEncoder();
                    Stream mem = new MemoryStream();

                    encoder.QualityLevel = 80;
                    encoder.Frames.Add(BitmapFrame.Create(BitmapSourceFromBitmap(cache.handler.CurrentFrame)));
                    encoder.Save(mem);

                    mem.Seek(0, SeekOrigin.Begin);

                    return mem;
            }

            return new MemoryStream(new byte[0]);
        }

        private static Stream GetWaveDataStream(string mlvFileName)
        {
            try
            {
                MLVCachedReader cache = GetReader(mlvFileName);
                Stream mem = new MemoryStream();
                MLVTypes.mlv_wavi_hdr_t header = cache.handler.WaviHeader;

                WaveFormat fmt = new WaveFormat((int)header.samplingRate, header.bitsPerSample, header.channels);
                cache.handler.WaveProvider = new BufferedWaveProvider(fmt);
                cache.handler.WaveProvider.BufferLength = 20 * 1024 * 1024;

                WaveFileWriter wr = new WaveFileWriter(mem, fmt);

                foreach (var frame in cache.reader.AudfXrefList.Keys)
                {
                    /* seek to the correct block */
                    cache.reader.CurrentBlockNumber = cache.reader.GetAudioFrameBlockNumber(frame);
                    cache.handler.AudfHeader.blockSize = 0;
                    cache.reader.ReadBlock();

                    /* now the VIDF should be read correctly */
                    if (cache.handler.AudfHeader.blockSize == 0)
                    {
                        return mem;
                    }

                    /* save into wave file stream */
                    byte[] data = new byte[cache.handler.WaveProvider.BufferedBytes];
                    cache.handler.WaveProvider.Read(data, 0, data.Length);
                    wr.Write(data, 0, data.Length);
                }

                mem.Seek(0, SeekOrigin.Begin);

                return mem;
            }
            catch(Exception e)
            {
                return new MemoryStream();
            }
        }

        internal static bool HasAudio(string mlvFileName)
        {
            MLVCachedReader cache = GetReader(mlvFileName);

            return cache.handler.FileHeader.audioClass != 0;
        }

        private static string GetFileType(string content)
        {
            string[] splits = content.Split('.');
            if (splits.Length < 1)
            {
                return "";
            }

            return splits[splits.Length - 1].ToUpper();
        }

        internal static uint[] GetFrameNumbers(string mlvFileName)
        {
            MLVCachedReader cache = GetReader(mlvFileName);

            return cache.frameList;
        }

        internal static bool FileExists(string mlvFileName, string content)
        {
            int frame = GetFrameNumber(mlvFileName, content);

            if(frame < 0)
            {
                return false;
            }

            switch(GetFileType(content))
            {
                case "DNG":
                case "JPG":
                case "WAV":
                    break;

                default:
                    return false;
            }

            return true;
        }

        internal static DateTime GetFileDate(string mlvFileName, string content)
        {
            MLVCachedReader cache = GetReader(mlvFileName);
            int frame = GetFrameNumber(mlvFileName, content);

            if(frame < 0)
            {
                return DateTime.MinValue;
            }

            if(cache.handler.RtciHeader.timestamp != 0)
            {
                DateTime fileDate = cache.handler.ParseRtci(cache.handler.RtciHeader);

                if (!cache.reader.VidfXrefList.ContainsKey((uint)frame) || cache.reader.VidfXrefList[(uint)frame].timestamp == 0)
                {
                    return DateTime.Now;
                }

                ulong offsetInUsec = cache.reader.VidfXrefList[(uint)frame].timestamp - cache.handler.RtciHeader.timestamp;

                DateTime date = DateTime.MinValue.AddMilliseconds((fileDate - DateTime.MinValue).TotalMilliseconds + (offsetInUsec / 1000));
                
                return date;
            }
            return DateTime.Now;
        }

        internal static DateTime GetFileDateUtc(string mlvFileName, string content)
        {
            return GetFileDate(mlvFileName, content).ToUniversalTime();
        }

        internal static long GetFileSize(string mlvFileName, string content)
        {
            string type = GetFileType(content);

            switch (type)
            {
                case "DNG":
                    return 20000000;
                case "JPG":
                    return 100000;
            }

            return 0;
        }

        internal static int GetFrameNumber(string mlvFileName, string content)
        {
            string[] splits = content.Split('.');

            if (splits.Length < 1)
            {
                return -1;
            }

            string frameNumbers = splits[0];
            uint frame = 0;

            if (!uint.TryParse(frameNumbers, out frame))
            {
                return -1;
            }

            return (int)frame;
        }
    }
}
