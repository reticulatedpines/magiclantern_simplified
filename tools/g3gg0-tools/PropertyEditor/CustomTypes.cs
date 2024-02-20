using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.Serialization;
using System.Security.Permissions;
using System.Xml.Serialization;

namespace PropertyEditor
{
    public struct UInt32Hex : IComparable, IFormattable, IComparable<uint>, IEquatable<uint>
    {
        private UInt32 Value;

        public static implicit operator string(UInt32Hex val)
        {
            return val.Value.ToString("X8");
        }

        public static implicit operator UInt32(UInt32Hex val)
        {
            return val.Value;
        }

        public static implicit operator UInt32Hex(string str)
        {
            UInt32Hex ret = new UInt32Hex();

            ret.Value = UInt32.Parse(str, System.Globalization.NumberStyles.HexNumber);
            return ret;
        }

        public static implicit operator UInt32Hex(UInt32 val)
        {
            UInt32Hex ret = new UInt32Hex();

            ret.Value = val;
            return ret;
        }




        #region IFormattable Member

        public string ToString(string format, IFormatProvider formatProvider)
        {
            return Value.ToString("X8");
        }

        #endregion

        #region IComparable Member

        public int CompareTo(UInt32Hex other)
        {
            return Value.CompareTo(other.Value);
        }

        public int CompareTo(UInt32 other)
        {
            return Value.CompareTo(other);
        }

        public int CompareTo(object other)
        {
            return Value.CompareTo(other);
        }

        #endregion

        #region IEquatable<uint> Member

        public bool Equals(uint other)
        {
            return Value.Equals(other);
        }

        #endregion

        #region IXmlSerializable Member

        public System.Xml.Schema.XmlSchema GetSchema()
        {
            throw new NotImplementedException();
        }

        public void ReadXml(System.Xml.XmlReader reader)
        {
            Value = UInt32.Parse(reader.ReadString(), System.Globalization.NumberStyles.HexNumber);
        }

        public void WriteXml(System.Xml.XmlWriter writer)
        {
            writer.WriteString(Value.ToString("X8"));
        }

        #endregion

        #region IXmlTextParser Member

        public bool Normalized
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public System.Xml.WhitespaceHandling WhitespaceHandling
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        #endregion
    }


}
