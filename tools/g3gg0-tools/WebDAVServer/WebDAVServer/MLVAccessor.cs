using mlv_view_sharp;
using NAudio.Wave;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media.Imaging;

namespace WebDAVServer
{
    public class MLVAccessor
    {
        public enum eFileType
        {
            Dng,
            Jpg,
            MJpeg,
            Fits,
            Wav,
            Txt,
            Unknown
        }
        private class CachedFile
        {
            public string name;
            public eFileType type;
            public int frameNum;
            public byte[] bufferedData;
            public DateTime lastUseTime;
        }

        private class MLVCachedReader
        {
            public string file;
            public MLVHandler handler;
            public MLVReader reader;
            public DateTime lastUseTime;
            public uint[] frameList;
            public Dictionary<string, CachedFile> cachedFiles = new Dictionary<string, CachedFile>();
            public System.Timers.Timer checkTimer;
        }

        private static Dictionary<string, MLVCachedReader> MLVCache = new Dictionary<string, MLVCachedReader>();
        private static System.Timers.Timer CacheCheckTimer = new System.Timers.Timer();

        static MLVAccessor()
        {
            CacheCheckTimer.Interval = 10 * 1000;
            CacheCheckTimer.Elapsed += (sender, e) =>
            {
                CleanCache();
            };
            CacheCheckTimer.Enabled = true;
        }

        private static long CacheUsage
        {
            get
            {
                long totalSize = 0;

                lock (MLVCache)
                {
                    foreach (var elem in MLVCache)
                    {
                        long size = 0;
                        foreach (var file in elem.Value.cachedFiles.Values)
                        {
                            size += file.bufferedData.Length;
                        }
                        totalSize += size;
                    }
                }
                return totalSize;
            }
        }

        private static void CleanCache()
        {
            /* if memory usage is higher than 800MiB, remove until we use less */
            while (CacheUsage > 500 * 1024 * 1024)
            {
                lock (MLVCache)
                {
                    CachedFile oldestFile = new CachedFile();
                    MLVCachedReader oldestReader = null;
                    oldestFile.lastUseTime = DateTime.MaxValue;

                    foreach (MLVCachedReader reader in MLVCache.Values)
                    {
                        foreach (var file in reader.cachedFiles.Values)
                        {
                            if (file.lastUseTime < oldestFile.lastUseTime)
                            {
                                oldestFile = file;
                                oldestReader = reader;
                            }
                        }
                    }

                    oldestFile.bufferedData = null;
                    oldestReader.cachedFiles.Remove(oldestFile.name);
                }
            }

            ArrayList cachedMlvFiles = new ArrayList();
            foreach (var entry in MLVCache.Values)
            {
                cachedMlvFiles.Add(entry);
            }

            foreach (MLVCachedReader entry in cachedMlvFiles)
            {
                /* too old, remove */
                if ((DateTime.Now - entry.lastUseTime).TotalSeconds >= Program.Instance.Server.Settings.CacheTime)
                {
                    MLVCache.Remove(entry.file);

                    /* remove all files separately */
                    lock (entry.cachedFiles)
                    {
                        ArrayList fileRemoves = new ArrayList();

                        foreach (var file in entry.cachedFiles.Values)
                        {
                            fileRemoves.Add(file);
                        }

                        foreach (CachedFile file in fileRemoves)
                        {
                            entry.cachedFiles.Remove(file.name);
                            file.bufferedData = null;
                        }
                    }
                }
                else
                {
                    /* cleanup cached files every minute */
                    lock (entry.cachedFiles)
                    {
                        ArrayList fileRemoves = new ArrayList();

                        foreach (var file in entry.cachedFiles.Values)
                        {
                            if ((DateTime.Now - file.lastUseTime).TotalSeconds >= Program.Instance.Server.Settings.CacheTime)
                            {
                                fileRemoves.Add(file);
                            }
                        }

                        foreach (CachedFile file in fileRemoves)
                        {
                            entry.cachedFiles.Remove(file.name);
                            file.bufferedData = null;
                        }
                    }
                }
            }

            GC.Collect(2);
        }

        public static string Statistics
        {
            get
            {
                StringBuilder ret = new StringBuilder();
                long totalSize = 0;
                try
                {
                    lock (MLVCache)
                    {
                        ret.AppendLine("  MLV readers: " + MLVCache.Count);

                        foreach (var elem in MLVCache)
                        {
                            long size = 0;
                            foreach (var file in elem.Value.cachedFiles.Values)
                            {
                                size += file.bufferedData.Length;
                            }
                            totalSize += size;

                            double sizeScaled = (size / 1024) / 1024.0f;
                            ret.AppendLine("    " + elem.Value.file + ": Cached " + elem.Value.cachedFiles.Count + " files, " + sizeScaled.ToString("0.00") + " MiB");
                        }
                    }
                }
                catch (Exception e)
                {
                }

                double totalSizeScaled = (totalSize / 1024) / 1024.0f;
                ret.AppendLine("    Cached toal: " + totalSizeScaled.ToString("0.00") + " MiB");
                return ret.ToString();
            }
        }

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
                entry.handler.DebayeringEnabled = false;
                entry.handler.UseCorrectionMatrices = false;
                entry.handler.HighlightRecovery = false;

                /* instanciate a reader */
                if (mlvFileName.ToLower().EndsWith(".mlv"))
                {
                    entry.reader = new MLVReader(mlvFileName, entry.handler.BlockHandler);
                }
                else if (mlvFileName.ToLower().EndsWith(".raw"))
                {
                    entry.reader = new RAWReader(mlvFileName, entry.handler.BlockHandler);
                }
                entry.lastUseTime = DateTime.Now;

                /* and create indices */
                entry.reader.CurrentBlockNumber = 0;
                entry.reader.BuildIndex();
                entry.reader.BuildFrameIndex();
                entry.reader.SaveIndex();

                while (entry.reader.ReadBlock())
                {
                    /* read until important blocks are filled and we reached a VIDF */
                    if (entry.handler.VidfHeader.blockSize != 0)
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

        public static byte[] GetDataStream(string mlvFileName, string content)
        {
            return GetDataStream(mlvFileName, content, Program.Instance.Server.Settings.PrefetchCount);
        }

        public static byte[] GetDataStream(string mlvFileName, string content, int prefetchCount)
        {
            MLVCachedReader cache = GetReader(mlvFileName);
            eFileType type = GetFileType(content);

            lock (cache.cachedFiles)
            {
                if (cache.cachedFiles.ContainsKey(content))
                {
                    PrefetchNext(mlvFileName, content, prefetchCount - 1);

                    cache.cachedFiles[content].lastUseTime = DateTime.Now;

                    return cache.cachedFiles[content].bufferedData;
                }
            }

            switch(type)
            {
                case eFileType.Txt:
                    string info = GetInfoFields(mlvFileName).Aggregate<string>(addString);
                    return ASCIIEncoding.ASCII.GetBytes(info);

                case eFileType.Wav:
                    return GetWaveDataStream(mlvFileName);

                case eFileType.MJpeg:
                    return GetMJpegDataStream(mlvFileName);
            }

            int frame = GetFrameNumber(mlvFileName, content);

            /* read video frame with/out debayering dependig on filetype */
            switch (type)
            {
                case eFileType.Dng:
                case eFileType.Fits:
                    cache.handler.DebayeringEnabled = false;
                    break;
                case eFileType.Jpg:
                    cache.handler.DebayeringEnabled = true;
                    break;
            }

            byte[] data = null;
            Bitmap currentFrame = null;
            MLVTypes.mlv_vidf_hdr_t vidfHeader = new MLVTypes.mlv_vidf_hdr_t();

            /* seek to the correct block */
            int block = cache.reader.GetVideoFrameBlockNumber((uint)frame);
            if (block < 0)
            {
                throw new FileNotFoundException("Requested video frame " + frame + " but thats not in the file index");
            }

            /* ensure that multiple threads dont conflict */
            lock (cache)
            {
                /* read it */
                cache.reader.CurrentBlockNumber = block;
                cache.handler.VidfHeader.blockSize = 0;
                cache.reader.ReadBlock();

                /* now the VIDF should be read correctly */
                if (cache.handler.VidfHeader.blockSize == 0)
                {
                    throw new InvalidDataException("Requested video frame " + frame + " but the index points us wrong");
                }

                /* get all data we need */
                switch (type)
                {
                    case eFileType.Dng:
                    case eFileType.Fits:
                        data = cache.handler.RawPixelData;
                        vidfHeader = cache.handler.VidfHeader;
                        break;

                    case eFileType.Jpg:
                        if (cache.handler.CurrentFrame != null)
                        {
                            currentFrame = new Bitmap(cache.handler.CurrentFrame);
                        }
                        break;
                }
            }

            /* process it */
            switch(type)
            {
                case eFileType.Fits:
                    {
                        object[] metadata = cache.reader.GetVideoFrameMetadata((uint)frame);
                        byte[] stream = FITSCreator.Create(mlvFileName, vidfHeader, data, metadata);

                        PrefetchSave(mlvFileName, content, eFileType.Dng, frame, stream);
                        PrefetchNext(mlvFileName, content, prefetchCount);

                        return stream;
                    }

                case eFileType.Dng:
                    {
                        object[] metadata = cache.reader.GetVideoFrameMetadata((uint)frame);
                        byte[] stream = DNGCreator.Create(mlvFileName, vidfHeader, data, metadata);

                        PrefetchSave(mlvFileName, content, eFileType.Dng, frame, stream);
                        PrefetchNext(mlvFileName, content, prefetchCount);

                        return stream;
                    }

                case eFileType.Jpg:
                    {
                        JpegBitmapEncoder encoder = new JpegBitmapEncoder();
                        Stream stream = new MemoryStream();

                        encoder.QualityLevel = 90;
                        if (currentFrame != null)
                        {
                            encoder.Frames.Add(BitmapFrame.Create(BitmapSourceFromBitmap(currentFrame)));
                            encoder.Save(stream);

                            stream.Seek(0, SeekOrigin.Begin);
                        }
                        stream.Seek(0, SeekOrigin.Begin);

                        byte[] buffer = new byte[stream.Length];
                        stream.Read(buffer, 0, buffer.Length);
                        stream.Close();

                        PrefetchSave(mlvFileName, content, eFileType.Jpg, frame, buffer);
                        PrefetchNext(mlvFileName, content, prefetchCount);

                        return buffer;
                    }
            }

            throw new FileNotFoundException("Requested frame " + frame + " of type " + type + " but we dont support that");
        }

        private static byte[] GetMJpegDataStream(string mlvFileName)
        {
            Stream stream = new MemoryStream();


            uint[] frames = GetFrameNumbers(mlvFileName);

            foreach (uint frame in frames)
            {
                byte[] frameData = GetDataStream(mlvFileName, frame.ToString("000000") + ".JPG");
                if (frameData != null)
                {
                    JpegBitmapEncoder encoder = new JpegBitmapEncoder();
                    encoder.QualityLevel = 90;
                    encoder.Frames.Add(BitmapFrame.Create(new MemoryStream(frameData)));
                    encoder.Save(stream);
                }
            }

            stream.Seek(0, SeekOrigin.Begin);

            byte[] buffer = new byte[stream.Length];
            stream.Read(buffer, 0, buffer.Length);

            stream.Close();

            return buffer;
        }

        private static void PrefetchSave(string mlvFileName, string content, eFileType type, int frame, byte[] stream)
        {
            MLVCachedReader cache = GetReader(mlvFileName);

            CleanCache();

            lock (cache.cachedFiles)
            {
                if (!cache.cachedFiles.ContainsKey(content))
                {
                    CachedFile cachedFile = new CachedFile();

                    cachedFile.lastUseTime = DateTime.Now;
                    cachedFile.name = content;
                    cachedFile.bufferedData = stream;
                    cachedFile.type = type;
                    cachedFile.frameNum = frame;

                    cache.cachedFiles.Add(content, cachedFile);
                }
            }
        }

        private static void PrefetchNext(string mlvFileName, string content, int count)
        {
            MLVCachedReader cache = GetReader(mlvFileName);
            string nextContent = "";
            int currentFrame = 0;
            eFileType type = eFileType.Unknown;

            if (count <= 0)
            {
                return;
            }

            lock (cache.cachedFiles)
            {
                if (!cache.cachedFiles.ContainsKey(content))
                {
                    return;
                }

                type = cache.cachedFiles[content].type;
                currentFrame = cache.cachedFiles[content].frameNum;
            }

            Thread fetchThread = new Thread(() =>
            {
                foreach (uint frame in GetFrameNumbers(mlvFileName))
                {
                    if (frame > currentFrame && count > 0)
                    {
                        nextContent = (currentFrame + 1).ToString("000000") + GetExtension(type);
                        try
                        {
                            GetDataStream(mlvFileName, nextContent, 0);
                            count--;
                        }
                        catch (FileNotFoundException e)
                        {
                        }
                        catch (Exception e)
                        {
                        }
                        currentFrame++;
                    }
                }
            });
            fetchThread.Name = "Prefetch: " + mlvFileName + " " + content;
            fetchThread.Start();

        }


        private static byte[] GetWaveDataStream(string mlvFileName)
        {
            MLVCachedReader cache = GetReader(mlvFileName);
            Stream mem = new MemoryStream();
            MLVTypes.mlv_wavi_hdr_t header = cache.handler.WaviHeader;

            WaveFormat fmt = new WaveFormat((int)header.samplingRate, header.bitsPerSample, header.channels);
            cache.handler.WaveProvider = new BufferedWaveProvider(fmt);
            cache.handler.WaveProvider.BufferLength = 20 * 1024 * 1024;

            WaveFileWriter wr = new WaveFileWriter(mem, fmt);

            lock (cache)
            {
                foreach (var frame in cache.reader.AudfXrefList.Keys)
                {
                    /* seek to the correct block */
                    cache.reader.CurrentBlockNumber = cache.reader.GetAudioFrameBlockNumber(frame);
                    cache.handler.AudfHeader.blockSize = 0;
                    cache.reader.ReadBlock();

                    /* now the VIDF should be read correctly */
                    if (cache.handler.AudfHeader.blockSize == 0)
                    {
                        throw new InvalidDataException("Requested audio frame " + frame + " but the index points us wrong");
                    }

                    /* save into wave file stream */
                    byte[] data = new byte[cache.handler.WaveProvider.BufferedBytes];
                    cache.handler.WaveProvider.Read(data, 0, data.Length);
                    wr.Write(data, 0, data.Length);
                }
            }

            mem.Seek(0, SeekOrigin.Begin);

            byte[] buffer = new byte[mem.Length];
            mem.Read(buffer, 0, buffer.Length);

            mem.Close();
            wr.Close();

            return buffer;
        }

        internal static bool HasAudio(string mlvFileName)
        {
            MLVCachedReader cache = GetReader(mlvFileName);

            if(cache.handler.WaviHeader.blockSize == 0)
            {
                return false;
            }

            return cache.handler.FileHeader.audioClass != 0;
        }

        private static string GetExtension(eFileType type)
        {
            switch (type)
            {
                case eFileType.Dng:
                    return ".DNG";
                case eFileType.Fits:
                    return ".FITS";
                case eFileType.Wav:
                    return ".WAV";
                case eFileType.MJpeg:
                    return ".MJPEG";
                case eFileType.Jpg:
                    return ".JPG";
                case eFileType.Txt:
                    return ".TXT";
            }
            return "";
        }

        private static eFileType GetFileType(string content)
        {
            string[] splits = content.Split('.');
            if (splits.Length < 1)
            {
                return eFileType.Unknown;
            }

            switch(splits[splits.Length - 1].ToUpper())
            {
                case "FITS":
                case "FIT":
                case "FTS":
                    return eFileType.Fits;
                case "DNG":
                    return eFileType.Dng;
                case "JPG":
                    return eFileType.Jpg;
                case "MJPEG":
                    return eFileType.MJpeg;
                case "WAV":
                    return eFileType.Wav;
                case "TXT":
                    return eFileType.Txt;
            }

            return eFileType.Unknown;
        }

        internal static uint[] GetFrameNumbers(string mlvFileName)
        {
            MLVCachedReader cache = GetReader(mlvFileName);

            return cache.frameList;
        }

        internal static bool FileExists(string mlvFileName, string content)
        {
            switch (GetFileType(content))
            {
                case eFileType.Txt:
                    return (content.ToLower() == ".txt");

                case eFileType.Wav:
                    return HasAudio(mlvFileName) && (content.ToLower() == ".wav");
            }

            int frame = GetFrameNumber(mlvFileName, content);

            switch (GetFileType(content))
            {
                case eFileType.Dng:
                case eFileType.Jpg:
                case eFileType.Fits:
                    return (frame >= 0);
            }

            return false;
        }

        internal static DateTime GetFileDate(string mlvFileName, string content)
        {
            MLVCachedReader cache = GetReader(mlvFileName);
            DateTime fileDate = cache.handler.ParseRtci(cache.handler.RtciHeader);

            switch (GetFileType(content))
            {
                case eFileType.Txt:
                case eFileType.Wav:
                case eFileType.MJpeg:
                    return fileDate;
            }

            int frame = GetFrameNumber(mlvFileName, content);
            if(frame < 0)
            {
                return DateTime.MinValue;
            }

            if(cache.handler.RtciHeader.timestamp != 0)
            {
                if (!cache.reader.VidfXrefList.ContainsKey((uint)frame) || cache.reader.VidfXrefList[(uint)frame].timestamp == 0)
                {
                    return DateTime.Now;
                }

                long offsetInUsec = (long)cache.reader.VidfXrefList[(uint)frame].timestamp - (long)cache.handler.RtciHeader.timestamp;
                if(offsetInUsec < 0)
                {
                    return DateTime.Now;
                }

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
            switch (GetFileType(content))
            {
                case eFileType.Txt:
                    return GetInfoFields(mlvFileName).Aggregate<string>(addString).Length;

                case eFileType.Wav:
                    if (!HasAudio(mlvFileName))
                    {
                        return 0;
                    }
                    return GetWaveDataStream(mlvFileName).Length;
            }

            MLVCachedReader cache = GetReader(mlvFileName);
            int frame = GetFrameNumber(mlvFileName, content);
            object[] metadata = cache.reader.GetVideoFrameMetadata((uint)frame);

            switch (GetFileType(content))
            {
                case eFileType.Dng:
                    return DNGCreator.GetSize(mlvFileName, cache.handler.VidfHeader, cache.handler.RawPixelData, metadata);
                case eFileType.Fits:
                    return 20000000;//GetDataStream(mlvFileName, content, 0).Length;

                case eFileType.Jpg:
                    return 100000;
                case eFileType.MJpeg:
                    return 1000000;
            }

            return 0;
        }

        private static string addString(string arg1, string arg2)
        {
            return arg1 + Environment.NewLine + arg2;
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

        internal static string[] GetInfoFields(string mlvFileName)
        {
            ArrayList infos = new ArrayList();
            MLVCachedReader cache = GetReader(mlvFileName);

            infos.Add("Frames: Video " + cache.handler.FileHeader.videoFrameCount + ", Audio " + cache.handler.FileHeader.audioFrameCount);
            if (cache.handler.FileHeader.videoFrameCount != cache.reader.VidfXrefList.Count || cache.handler.FileHeader.audioFrameCount != cache.reader.AudfXrefList.Count)
            {
                infos.Add("Blocks: Video " + cache.reader.VidfXrefList.Count + ", Audio " + cache.reader.AudfXrefList.Count);
            }

            if(cache.handler.InfoString.Length > 0)
            {
                string[] infoFields = cache.handler.InfoString.Split(';');

                foreach (string info in infoFields)
                {
                    if (info.Trim().Length > 0)
                    {
                        infos.Add(info.Trim());
                    }
                }
            }
            if (cache.handler.LensHeader.blockSize > 0)
            {
                infos.Add("Lens: " + cache.handler.LensHeader.lensName + " at " + cache.handler.LensHeader.focalLength + " mm" + " (" + cache.handler.LensHeader.focalDist + " mm) f/" + ((float)cache.handler.LensHeader.aperture / 100).ToString("0.00"));
            }
            if (cache.handler.IdntHeader.blockSize > 0)
            {
                string serHex = cache.handler.IdntHeader.cameraSerial;
                ulong serial = 0;
                ulong.TryParse(serHex, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out serial);

                infos.Add("Camera: " + cache.handler.IdntHeader.cameraName.Trim() + ", Serial #" + serial);
            }
            if (cache.handler.ExpoHeader.blockSize > 0)
            {
                infos.Add("Exposure: ISO" + cache.handler.ExpoHeader.isoValue + " 1/" + (1000000.0f / cache.handler.ExpoHeader.shutterValue).ToString("0") + " s");
            }
            if (cache.handler.StylHeader.blockSize > 0)
            {
                infos.Add("Style: " + cache.handler.StylHeader.picStyleName);
            }
            if (cache.handler.WbalHeader.blockSize > 0)
            {
                infos.Add("White: White balance mode " + cache.handler.WbalHeader.wb_mode);
                infos.Add("White: Color temperature " + cache.handler.WbalHeader.kelvin);
            }

            return (string[])infos.ToArray(typeof(string));
        }
    }
}
