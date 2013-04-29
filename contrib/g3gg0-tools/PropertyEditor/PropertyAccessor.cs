using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections;
using System.Xml.Serialization;

namespace PropertyEditor
{
    public class PropertyAccessor
    {
        public class Property
        {
            [XmlIgnore]
            public UInt32 Id;
            [XmlIgnore]
            public UInt32 Length;
            [XmlIgnore]
            public byte[] Data;

            [XmlAttribute("Id")]
            public string Id_
            {
                get
                {
                    return Id.ToString("X8");
                }
                set
                {
                    Id = UInt32.Parse(value, System.Globalization.NumberStyles.HexNumber);
                }
            }

            [XmlAttribute("Length")]
            public string Length_
            {
                get
                {
                    return Length.ToString("X8");
                }
                set
                {
                    Length = UInt32.Parse(value, System.Globalization.NumberStyles.HexNumber);
                }
            }

            [XmlElement("Data")]
            public string Data_
            {
                get
                {
                    StringBuilder str = new StringBuilder();
                    if (Data != null)
                    {
                        for (int pos = 0; pos < Data.Length; pos++)
                        {
                            str.AppendFormat("{0:X2}", Data[pos]);
                        }
                    }
                    return str.ToString();
                }
                set
                {
                }
            }

            public string String;
        }


        [XmlInclude(typeof(Property))]
        public class PropertySubBlock
        {
            [XmlIgnore]
            public UInt32 ValidFlag;
            [XmlIgnore]
            public UInt32 Id;
            [XmlIgnore]
            public UInt32 Unknown;
            [XmlIgnore]
            public UInt32 TotalLength;

            [XmlAttribute("ValidFlag")]
            public string ValidFlag_
            {
                get
                {
                    return ValidFlag.ToString("X8");
                }
                set
                {
                    ValidFlag = UInt32.Parse(value, System.Globalization.NumberStyles.HexNumber);
                }
            }

            [XmlAttribute("Id")]
            public string Id_
            {
                get
                {
                    return Id.ToString("X8");
                }
                set
                {
                    Id = UInt32.Parse(value, System.Globalization.NumberStyles.HexNumber);
                }
            }

            [XmlAttribute("TotalLength")]
            public string TotalLength_
            {
                get
                {
                    return TotalLength.ToString("X8");
                }
                set
                {
                    TotalLength = UInt32.Parse(value, System.Globalization.NumberStyles.HexNumber);
                }
            }

            public Property[] Properties;
        }

        [XmlInclude(typeof(PropertySubBlock))]
        public class PropertyBlock
        {
            [XmlIgnore]
            public UInt32 ValidFlag;
            [XmlIgnore]
            public UInt32 TotalLength;
            [XmlIgnore]
            public UInt32 Unknown;

            [XmlAttribute("ValidFlag")]
            public string ValidFlag_
            {
                get
                {
                    return ValidFlag.ToString("X8");
                }
                set
                {
                    ValidFlag = UInt32.Parse(value, System.Globalization.NumberStyles.HexNumber);
                }
            }

            [XmlAttribute("Unknown")]
            public string Unknown_
            {
                get
                {
                    return Unknown.ToString("X8");
                }
                set
                {
                    Unknown = UInt32.Parse(value, System.Globalization.NumberStyles.HexNumber);
                }
            }

            [XmlAttribute("TotalLength")]
            public string TotalLength_
            {
                get
                {
                    return TotalLength.ToString("X8");
                }
                set
                {
                    TotalLength = UInt32.Parse(value, System.Globalization.NumberStyles.HexNumber);
                }
            }

            public PropertySubBlock[] SubBlocks;
        }

        public static PropertyBlock[] ParsePropertyBlocks(byte[] buffer, ulong offset, ulong blockSize, ulong maxBlocks)
        {
            ArrayList blockList = new ArrayList();
            ulong blockNum = 0;

            while (blockNum < maxBlocks || maxBlocks == 0)
            {
                ulong currentOffset = offset + blockNum * blockSize;
                PropertyBlock block = new PropertyBlock();

                block.ValidFlag = GetUInt32(buffer, currentOffset);
                block.TotalLength = GetUInt32(buffer, currentOffset + 4);
                block.Unknown = GetUInt32(buffer, currentOffset + 8);

                if (blockSize == 0)
                {
                    if (block.TotalLength < 0x1000)
                    {
                        blockSize = 0x1000;
                    }
                    else if (block.TotalLength < 0x2000)
                    {
                        blockSize = 0x2000;
                    }
                    else if (block.TotalLength < 0x4000)
                    {
                        blockSize = 0x4000;
                    }
                    else if (block.TotalLength < 0x8000)
                    {
                        blockSize = 0x8000;
                    }
                    else
                    {
                        Log.WriteLine("[E] Failed to autodetect block size. Dumping only until valid block reached.");
                        blockSize = block.TotalLength;
                        maxBlocks = 0;
                    }
                    Log.WriteLine("[I] Autodetected block size as " + blockSize.ToString("X8"));
                }

                Log.WriteLine("[Block: {0:X2}, Length: {1:X8}, Flag: {2:X8}] [Base: {3:X8}]", blockNum, block.TotalLength, block.ValidFlag, currentOffset);

                if (block.ValidFlag != 0xFFFFFFFF)
                {
                    bool invalid = false;

                    if (maxBlocks == 0)
                    {
                        if (block.ValidFlag == 0x0000FFFF)
                        {
                            Console.Out.WriteLine("  [I] Detected valid block while maxBlocks set to zero. Finishing.");
                            maxBlocks = blockNum;
                        }
                        else if (block.ValidFlag == 0x00FFFFFF)
                        {
                        }
                        else if (block.ValidFlag == 0x000000FF)
                        {
                        }
                        else if (block.ValidFlag == 0x00000000)
                        {
                        }
                        else
                        {
                            Console.Out.WriteLine("  [I] Detected invalid block while maxBlocks set to zero. Finishing.");
                            maxBlocks = blockNum;
                            invalid = true;
                        }
                    }
                    if ((block.TotalLength > 0x0C) && (block.TotalLength <= blockSize) && !invalid)
                    {
                        block.SubBlocks = ParseSubBlocks(buffer, currentOffset, 0x0C, block.TotalLength - 0x0C);
                    }
                    else if(!invalid)
                    {
                        Console.Out.WriteLine("  [E] Invalid block size");
                    }
                }
                else
                {
                    Console.Out.WriteLine("  [I] Block empty");
                }
                blockList.Add(block);

                blockNum++;
            }

            return (PropertyBlock[])blockList.ToArray(typeof(PropertyBlock));
        }

        public static PropertySubBlock[] ParseSubBlocks(byte[] buffer, ulong offset, ulong startPos, ulong maxLength)
        {
            ArrayList subBlockList = new ArrayList();
            ulong currentOffset = offset + startPos;
            ulong endOffset = offset + startPos + maxLength;
            ulong blockNum = 0;

            while (currentOffset + 0x0C < endOffset)
            {
                PropertySubBlock subBlock = new PropertySubBlock();

                subBlock.ValidFlag = GetUInt32(buffer, currentOffset);
                subBlock.Id = GetUInt32(buffer, currentOffset + 4);
                if (subBlock.Id == 0x00FF0000)
                {
                    UInt32 propTypes = GetUInt32(buffer, currentOffset + 8);
                    UInt32 remainSize = GetUInt32(buffer, currentOffset + 0x0C);
                    UInt32 unknown = GetUInt32(buffer, currentOffset + 0x10);
                    Console.Out.WriteLine("  [I] Advanced subblock. [Prop: {0:X8}, Remain: {1:X8}, Unk: {2:X8}", propTypes, remainSize, unknown);
                    currentOffset += 0x0C;
                }

                subBlock.TotalLength = GetUInt32(buffer, currentOffset + 8);

                Log.WriteLine("  [SubBlock: {0:X2}, Length: {1:X8}, Flag: {2:X8}] [Base: {3:X8}]", blockNum, subBlock.TotalLength, subBlock.ValidFlag, currentOffset);

                if ((subBlock.TotalLength > 0x0C) && (currentOffset + subBlock.TotalLength <= endOffset))
                {
                    subBlock.Properties = ParseProperties(buffer, currentOffset, 0x0C, subBlock.TotalLength - 0x0C);
                }
                else
                {
                    Console.Out.WriteLine("    [E] Invalid subblock size");
                }
                subBlockList.Add(subBlock);

                currentOffset += subBlock.TotalLength;
                blockNum++;
            }

            return (PropertySubBlock[])subBlockList.ToArray(typeof(PropertySubBlock));
        }

        public static Property[] ParseProperties(byte[] buffer, ulong offset, ulong startPos, ulong maxLength)
        {
            ArrayList propList = new ArrayList();
            ulong currentOffset = offset + startPos;
            ulong endOffset = offset + startPos + maxLength;

            while (currentOffset + 8 < endOffset)
            {
                UInt32 propertyId = GetUInt32(buffer, currentOffset);
                UInt32 propertyLength = GetUInt32(buffer, currentOffset + 4);

                Log.WriteLine("    [Property: {0:X8}, Length: {1:X8}] [Base: {2:X8}]", propertyId, propertyLength, currentOffset);

                Property prop = new Property();
                prop.Id = propertyId;

                if ((propertyLength > 0x08) && (currentOffset + propertyLength <= endOffset))
                {
                    prop.Length = propertyLength - 8;
                    prop.Data = new byte[prop.Length];
                    
                    Array.Copy(buffer, (int)currentOffset + 8, prop.Data, 0, prop.Data.Length);

                    prop.String = ByteArrayToString(prop.Data);
                }
                else
                {
                    Console.Out.WriteLine("      [E] Property length invalid");
                }
                propList.Add(prop);

                currentOffset += propertyLength;
            }

            return (Property[])propList.ToArray(typeof(Property));
        }

        /* helper functions */
        private static UInt32 GetUInt32(byte[] buffer, ulong offset)
        {
            UInt32 ret = (UInt32)GetUInt16(buffer, offset) | ((UInt32)GetUInt16(buffer, offset + 2)) << 16;
            return ret;
        }

        private static UInt32 GetUInt16(byte[] buffer, ulong offset)
        {
            UInt32 ret = (UInt32)GetUInt8(buffer, offset) | ((UInt32)GetUInt8(buffer, offset + 1)) << 8;
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
