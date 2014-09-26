using mlv_view_sharp;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;



namespace WebDAVServer
{
    public class DNGCreator
    {
        public static double ProcessingTime;
        //all the mlv block headers corresponding to a particular frame, needed to generate a DNG for that frame
        [StructLayout(LayoutKind.Sequential, Pack = 1), Serializable]
        public struct frame_headers
        {
            public uint fileNumber;
            public ulong position;
            public MLVTypes.mlv_vidf_hdr_t vidf_hdr;
            public MLVTypes.mlv_file_hdr_t file_hdr;
            public MLVTypes.mlv_rtci_hdr_t rtci_hdr;
            public MLVTypes.mlv_idnt_hdr_t idnt_hdr;
            public MLVTypes.mlv_rawi_hdr_t rawi_hdr;
            public MLVTypes.mlv_expo_hdr_t expo_hdr;
            public MLVTypes.mlv_lens_hdr_t lens_hdr;
            public MLVTypes.mlv_wbal_hdr_t wbal_hdr;
        };

        [DllImport("MlvFsDng.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern uint dng_get_header_data(ref frame_headers frame_headers, byte[] output_buffer, uint offset, uint max_size);
        [DllImport("MlvFsDng.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern uint dng_get_header_size(ref frame_headers frame_headers);
        [DllImport("MlvFsDng.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern uint dng_get_image_data(ref frame_headers frame_headers, byte[] packed_buffer, byte[] output_buffer, int offset, uint max_size);
        [DllImport("MlvFsDng.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern uint dng_get_image_size(ref frame_headers frame_headers);
        [DllImport("MlvFsDng.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern uint dng_get_size(ref frame_headers frame_headers);

        internal static uint GetSize(string mlvFileName, MLVTypes.mlv_vidf_hdr_t vidfHeader, byte[] pixelData, object[] metadata)
        {
            frame_headers dngData = CreateDngData(vidfHeader, metadata);

            uint headerSize = dng_get_header_size(ref dngData);
            uint imageSize = dng_get_image_size(ref dngData);
            byte[] headerData = new byte[headerSize];
            uint headerSizeReal = dng_get_header_data(ref dngData, headerData, 0, headerSize);

            uint totalSize = headerSizeReal + imageSize;

            return totalSize;
        }

        internal static Stream Create(string mlvFileName, MLVTypes.mlv_vidf_hdr_t vidfHeader, byte[] inData, object[] metadata)
        {
            frame_headers dngData = CreateDngData(vidfHeader, metadata);

            try
            {
                uint headerSize = dng_get_header_size(ref dngData);
                uint imageSize = dng_get_image_size(ref dngData);
                byte[] headerData = new byte[headerSize];
                byte[] imageData = new byte[imageSize];

                uint headerSizeReal = dng_get_header_data(ref dngData, headerData, 0, headerSize);

                DateTime start = DateTime.Now;
                uint dataSizeReal = dng_get_image_data(ref dngData, inData, imageData, 0, imageSize);
                ProcessingTime = (DateTime.Now - start).TotalMilliseconds;

                /* now assemble header and image data */
                byte[] dngFileData = new byte[headerSizeReal + dataSizeReal];

                Array.Copy(headerData, 0, dngFileData, 0, headerSizeReal);
                Array.Copy(imageData, 0, dngFileData, headerSizeReal, dataSizeReal);

                return new MemoryStream(dngFileData);
            }
            catch(Exception e)
            {
                return new MemoryStream();
            }
        }

        private static frame_headers CreateDngData(MLVTypes.mlv_vidf_hdr_t vidfHeader, object[] metadata)
        {
            frame_headers dngData = new frame_headers();

            foreach (dynamic obj in metadata)
            {
                if (obj.GetType() == typeof(MLVTypes.mlv_file_hdr_t))
                {
                    dngData.file_hdr = obj;
                }
                else
                {
                    switch ((String)obj.blockType)
                    {
                        case "RTCI":
                            dngData.rtci_hdr = obj;
                            break;
                        case "IDNT":
                            dngData.idnt_hdr = obj;
                            break;
                        case "RAWI":
                            dngData.rawi_hdr = obj;
                            break;
                        case "EXPO":
                            dngData.expo_hdr = obj;
                            break;
                        case "LENS":
                            dngData.lens_hdr = obj;
                            break;
                        case "WBAL":
                            dngData.wbal_hdr = obj;
                            break;
                    }
                }
            }
            dngData.vidf_hdr = vidfHeader;

            return dngData;
        }
    }
}
