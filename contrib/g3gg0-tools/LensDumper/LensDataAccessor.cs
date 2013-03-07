using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections;
using System.Xml.Serialization;

namespace LensDumper
{
    public class LensDataAccessor
    {
        public class LensDataStructure
        {
            public UInt16 Identifier;
            public UInt16 LensEntryCount;
            public UInt16 HeaderSize;
            public UInt16 LensEntriesReserved;

            public UInt16 Unknown1;
            public UInt16 Unknown2;
            public UInt16 Unknown3;
            public UInt16 Unknown4;
            public UInt16 Unknown5;

            public LensEntry[] LensEntries;
            public LensData[] LensDatas;
        }

        public class LensEntry
        {
            public UInt32 LensId;
            public UInt16 Wide;
            public UInt16 ExtenderType;
            public UInt32 Offset;
        }

        public class LensData
        {
            public UInt16 Unknown1;
            public UInt16 Unknown2;
            public UInt16 FocalLengthMin;
            public UInt16 FocalLengthMax;
            public UInt16 FocusDistMin;

            public ProfileTypeA ProfileTypesA;
            public ProfileTypeB ProfileTypesB;
            public ProfileTypeC ProfileTypesC;
        }

        public class ProfileTypeA
        {
            /* 0x14 bytes */
            [XmlIgnore]
            public byte[] HeaderData;

            [XmlAttribute("header")]
            public string HeaderData_
            {
                get
                {
                    StringBuilder str = new StringBuilder();
                    if (HeaderData != null)
                    {
                        for (int pos = 0; pos < HeaderData.Length; pos++)
                        {
                            str.AppendFormat("{0:X2}", HeaderData[pos]);
                        }
                    }
                    return str.ToString();
                }
                set
                {
                }
            }

            public ProfileTypeAData[] Data;
        }

        public class ProfileTypeB
        {
            /* 0x14 bytes */
            [XmlIgnore]
            public byte[] HeaderData;

            [XmlAttribute("header")]
            public string HeaderData_
            {
                get
                {
                    StringBuilder str = new StringBuilder();
                    if (HeaderData != null)
                    {
                        for (int pos = 0; pos < HeaderData.Length; pos++)
                        {
                            str.AppendFormat("{0:X2}", HeaderData[pos]);
                        }
                    }
                    return str.ToString();
                }
                set
                {
                }
            }

            public ProfileTypeBData[] Data;
        }

        public class ProfileTypeC
        {
            /* 0x14 bytes */
            [XmlIgnore]
            public byte[] HeaderData;

            [XmlAttribute("header")]
            public string HeaderData_
            {
                get
                {
                    StringBuilder str = new StringBuilder();
                    if (HeaderData != null)
                    {
                        for (int pos = 0; pos < HeaderData.Length; pos++)
                        {
                            str.AppendFormat("{0:X2}", HeaderData[pos]);
                        }
                    }
                    return str.ToString();
                }
                set
                {
                }
            }

            public ProfileTypeCData[] Data;
        }

        public class ProfileTypeAData
        {
            [XmlAttribute("focalLength")]
            public UInt16 FocalLength;

            /* 0xC0 bytes */
            [XmlElement("CorrectionData")]
            public CorrectionDataTypeA[] CorrectionData;
        }

        public class ProfileTypeBData
        {
            [XmlAttribute("focalLength")]
            public UInt16 FocalLength;
            /* 0x30 bytes */
            public CorrectionDataTypeB[] CorrectionData;
        }

        public class ProfileTypeCData
        {
            [XmlAttribute("focalLength")]
            public UInt16 FocalLength;

            /* 0x188 bytes */
            [XmlElement("CorrectionData")]
            public CorrectionDataTypeC[] CorrectionData;
        }


        public class CorrectionDataTypeA
        {
            [XmlAttribute("aperture")]
            public UInt16 Aperture;

            [XmlElement("Coeff")]
            public Int16[] Coeff;
        }

        public class CorrectionDataTypeB
        {
            [XmlElement("Coeff")]
            public Int16[] Coeff;
        }

        public class CorrectionDataTypeC
        {
            [XmlAttribute("aperture")]
            public UInt16 Aperture;

            [XmlElement("Coeff")]
            public Int16[] Coeff;
        }

        public static LensDataStructure ParseLensData(byte[] buffer)
        {
            ulong offset = 0;
            LensDataStructure data = new LensDataStructure();
            
            data.Identifier = GetUInt16(buffer, 0x00);
            data.LensEntryCount = GetUInt16(buffer, 0x02);
            data.HeaderSize = GetUInt16(buffer, 0x04);
            data.LensEntriesReserved = GetUInt16(buffer, 0x06);
            data.Unknown1 = GetUInt16(buffer, 0x10);
            data.Unknown2 = GetUInt16(buffer, 0x12);
            data.Unknown3 = GetUInt16(buffer, 0x14);
            data.Unknown4 = GetUInt16(buffer, 0x16);
            data.Unknown5 = GetUInt16(buffer, 0x18);

            offset += data.HeaderSize;

            Log.WriteLine("  Parsing " + data.LensEntryCount + " index entries");
            data.LensEntries = new LensEntry[data.LensEntryCount];
            for (ulong entry = 0; entry < data.LensEntryCount; entry++)
            {
                LensEntry lensEntry = new LensEntry();

                lensEntry.LensId = GetUInt16(buffer, offset + entry * 0x10 + 0x00);
                lensEntry.Wide = GetUInt16(buffer, offset + entry * 0x10 + 0x02);
                lensEntry.ExtenderType = GetUInt16(buffer, offset + entry * 0x10 + 0x04);
                lensEntry.Offset = GetUInt32(buffer, offset + entry * 0x10 + 0x0C);

                Log.WriteLine("  Lens #{0} has ID 0x{1:X8} [Wide: {2}, ExtenderType: {3}]", entry, lensEntry.LensId, lensEntry.Wide, lensEntry.ExtenderType);
                data.LensEntries[entry] = lensEntry;
            }

            offset += (ulong)data.LensEntriesReserved * 0x10;

            Log.WriteLine("  Parsing " + data.LensEntryCount + " lens datas");
            data.LensDatas = new LensData[data.LensEntryCount];
            for (ulong entry = 0; entry < data.LensEntryCount; entry++)
            {
                LensEntry lensEntry = data.LensEntries[entry];

                data.LensDatas[entry] = ParseLensCorrection(buffer, offset + lensEntry.Offset);
            }

            return data;
        }

        private static LensData ParseLensCorrection(byte[] buffer, ulong offset)
        {
            LensData data = new LensData();

            data.Unknown1 = GetUInt16(buffer, offset + 0x00);
            data.Unknown2 = GetUInt16(buffer, offset + 0x02);
            data.FocalLengthMin = GetUInt16(buffer, offset + 0x04);
            data.FocalLengthMax = GetUInt16(buffer, offset + 0x06);
            data.FocusDistMin = GetUInt16(buffer, offset + 0x08);

            if (data.FocalLengthMax != data.FocalLengthMin)
            {
                Log.WriteLine("    Parsing lens {0}-{1} mm (min focus {2} mm) lens data [{3}, {4}]", data.FocalLengthMin, data.FocalLengthMax, data.FocusDistMin, data.Unknown1, data.Unknown2);
            }
            else
            {
                Log.WriteLine("    Parsing lens {0} mm (min focus {2} mm) lens data [{3}, {4}]", data.FocalLengthMin, data.FocalLengthMax, data.FocusDistMin, data.Unknown1, data.Unknown2);
            }

            /* this header */
            offset += 0x20;

            Log.WriteLine("      Reading profile type A");
            data.ProfileTypesA = ParseLensCorrectionTypeA(buffer, ref offset);
            if (GetUInt32(buffer, offset) != 0x44444444)
            {
                Log.WriteLine("      [E] End token at offset 0x{0:X8} is not 0x44444444", offset);
                return null;
            }
            offset += 0x04;
            Log.WriteLine("      Reading profile type B");
            data.ProfileTypesB = ParseLensCorrectionTypeB(buffer, ref offset);
            if (GetUInt32(buffer, offset) != 0x33333333)
            {
                Log.WriteLine("      [E] End token at offset 0x{0:X8} is not 0x33333333", offset);
                return null;
            }
            offset += 0x04;
            Log.WriteLine("      Reading profile type C");
            data.ProfileTypesC = ParseLensCorrectionTypeC(buffer, ref offset);
            if (GetUInt32(buffer, offset) != 0x22222222)
            {
                Log.WriteLine("      [E] End token at offset 0x{0:X8} is not 0x22222222", offset);
                return null;
            }

            return data;
        }

        private static ProfileTypeC ParseLensCorrectionTypeC(byte[] buffer, ref ulong offset)
        {
            ProfileTypeC data = new ProfileTypeC();

            UInt16[] focalLengths = new UInt16[4];
            focalLengths[0] = GetUInt16(buffer, offset + 0x00);
            focalLengths[1] = GetUInt16(buffer, offset + 0x02);
            focalLengths[2] = GetUInt16(buffer, offset + 0x04);
            focalLengths[3] = GetUInt16(buffer, offset + 0x06);
            offset += 0x08;

            data.HeaderData = new byte[0x14];
            Array.Copy(buffer, (int)offset, data.HeaderData, 0, 0x14);
            offset += 0x14;

            data.Data = new ProfileTypeCData[4];
            for (int pos = 0; pos < 4; pos++)
            {
                data.Data[pos] = ParseLensCorrectionTypeCData(buffer, ref offset);
                data.Data[pos].FocalLength = focalLengths[pos];
            }

            return data;
        }

        private static ProfileTypeB ParseLensCorrectionTypeB(byte[] buffer, ref ulong offset)
        {
            ProfileTypeB data = new ProfileTypeB();

            /* zero header */
            offset += 0x10;

            UInt16[] focalLengths = new UInt16[4];
            focalLengths[0] = GetUInt16(buffer, offset + 0x00);
            focalLengths[1] = GetUInt16(buffer, offset + 0x02);
            focalLengths[2] = GetUInt16(buffer, offset + 0x04);
            focalLengths[3] = GetUInt16(buffer, offset + 0x06);
            offset += 0x08;

            data.HeaderData = new byte[0x14];
            Array.Copy(buffer, (int)offset, data.HeaderData, 0, 0x14);
            offset += 0x14;

            data.Data = new ProfileTypeBData[4];
            for (int pos = 0; pos < 4; pos++)
            {
                data.Data[pos] = ParseLensCorrectionTypeBData(buffer, ref offset);
                data.Data[pos].FocalLength = focalLengths[pos];
            }

            return data;
        }

        private static ProfileTypeA ParseLensCorrectionTypeA(byte[] buffer, ref ulong offset)
        {
            ProfileTypeA data = new ProfileTypeA();

            UInt16[] focalLengths = new UInt16[4];
            focalLengths[0] = GetUInt16(buffer, offset + 0x00);
            focalLengths[1] = GetUInt16(buffer, offset + 0x02);
            focalLengths[2] = GetUInt16(buffer, offset + 0x04);
            focalLengths[3] = GetUInt16(buffer, offset + 0x06);
            offset += 0x08;

            data.HeaderData = new byte[0x14];
            Array.Copy(buffer, (int)offset, data.HeaderData, 0, 0x14);
            offset += 0x14;

            data.Data = new ProfileTypeAData[4];
            for (int pos = 0; pos < 4; pos++)
            {
                data.Data[pos] = ParseLensCorrectionTypeAData(buffer, ref offset);
                data.Data[pos].FocalLength = focalLengths[pos];
            }

            return data;
        }

        private static ProfileTypeAData ParseLensCorrectionTypeAData(byte[] buffer, ref ulong offset)
        {
            ProfileTypeAData data = new ProfileTypeAData();
            int fields = 0x10;
            UInt16[] fStop = new UInt16[4];

            fStop[0] = GetUInt16(buffer, offset + 0x00);
            fStop[1] = GetUInt16(buffer, offset + 0x02);
            fStop[2] = GetUInt16(buffer, offset + 0x04);
            fStop[3] = GetUInt16(buffer, offset + 0x06);
            offset += 0x08;

            data.CorrectionData = new CorrectionDataTypeA[fields];
            for (int field = 0; field < fields; field++)
            {
                data.CorrectionData[field] = new CorrectionDataTypeA();
                data.CorrectionData[field].Aperture = fStop[field % 4];
                data.CorrectionData[field].Coeff = new Int16[6];
                for (int coeff = 0; coeff < 6; coeff++)
                {
                    Int16 val = (Int16)((Int16)0x2000 - (Int16)GetUInt16(buffer, offset));
                    data.CorrectionData[field].Coeff[coeff] = val;
                    offset += 2;
                }
            }


            return data;
        }

        private static ProfileTypeBData ParseLensCorrectionTypeBData(byte[] buffer, ref ulong offset)
        {
            ProfileTypeBData data = new ProfileTypeBData();
            int fields = 4;

            data.CorrectionData = new CorrectionDataTypeB[fields];
            for (int field = 0; field < fields; field++)
            {
                data.CorrectionData[field] = new CorrectionDataTypeB();
                data.CorrectionData[field].Coeff = new Int16[0x06];
                for (int coeff = 0; coeff < 0x06; coeff++)
                {
                    Int16 val = (Int16)((Int16)GetUInt16(buffer, offset) - (Int16)0x4000);
                    data.CorrectionData[field].Coeff[coeff] = val;
                    offset += 2;
                }
            }

            return data;
        }

        private static ProfileTypeCData ParseLensCorrectionTypeCData(byte[] buffer, ref ulong offset)
        {
            ProfileTypeCData data = new ProfileTypeCData();
            int fields = 16;
            UInt16[] fStop = new UInt16[4];

            fStop[0] = GetUInt16(buffer, offset + 0x00);
            fStop[1] = GetUInt16(buffer, offset + 0x02);
            fStop[2] = GetUInt16(buffer, offset + 0x04);
            fStop[3] = GetUInt16(buffer, offset + 0x06);
            offset += 0x08;

            data.CorrectionData = new CorrectionDataTypeC[fields];
            for (int field = 0; field < fields; field++)
            {
                data.CorrectionData[field] = new CorrectionDataTypeC();
                data.CorrectionData[field].Aperture = fStop[field % 4];
                data.CorrectionData[field].Coeff = new Int16[0x0C];
                for (int coeff = 0; coeff < 0x0C; coeff++)
                {
                    Int16 val = (Int16)((Int16)GetUInt16(buffer, offset) - (Int16)0x4000);
                    data.CorrectionData[field].Coeff[coeff] = val;
                    offset += 2;
                }
            }

            return data;
        }

        /* helper functions */
        private static UInt32 GetUInt32(byte[] buffer, ulong offset)
        {
            UInt32 ret = (UInt32)GetUInt16(buffer, offset) | ((UInt32)GetUInt16(buffer, offset + 2)) << 16;
            return ret;
        }

        private static UInt16 GetUInt16(byte[] buffer, ulong offset)
        {
            UInt16 ret = (UInt16)(GetUInt8(buffer, offset) | (GetUInt8(buffer, offset + 1) << 8));
            return ret;
        }

        private static UInt32 GetUInt8(byte[] buffer, ulong offset)
        {
            return (UInt32)buffer[offset];
        }

        private static string ByteArrayToString(byte[] buffer)
        {
            try
            {
                byte[] buf = new byte[buffer.Length];
                for (int pos = 0; pos < buf.Length; pos++)
                {
                    buf[pos] = buffer[pos];
                    if (buf[pos] < 0x20 || buf[pos] > 0x7E)
                    {
                        buf[pos] = 0x5F;
                    }
                }

                System.Text.ASCIIEncoding enc = new System.Text.ASCIIEncoding();
                return enc.GetString(buf);
            }
            catch (Exception ex)
            {
                return "";
            }
        }
    }
}