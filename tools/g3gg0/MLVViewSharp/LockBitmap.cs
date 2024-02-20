

/* from http://www.codeproject.com/Tips/240428/Work-with-bitmap-faster-with-Csharp */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

namespace mlv_view_sharp
{
    public class LockBitmap
    {
        Bitmap source = null;
        IntPtr Iptr = IntPtr.Zero;
        BitmapData bitmapData = null;
        Rectangle Rect = new Rectangle();

        public byte[] Pixels { get; set; }
        public int Depth { get; private set; }
        public int Width { get; private set; }
        public int Height { get; private set; }

        public LockBitmap(Bitmap source)
        {
            this.source = source;

            // Get width and height of bitmap
            Width = source.Width;
            Height = source.Height;

            // get total locked pixels count
            int PixelCount = Width * Height;

            // Create rectangle to lock
            Rect = new Rectangle(0, 0, Width, Height);

            // get source bitmap pixel format size
            Depth = System.Drawing.Bitmap.GetPixelFormatSize(source.PixelFormat);

            // Check if bpp (Bits Per Pixel) is 8, 24, or 32
            if (Depth != 8 && Depth != 24 && Depth != 32)
            {
                throw new ArgumentException("Only 8, 24 and 32 bpp images are supported.");
            }

            // create byte array to copy pixel values
            int step = Depth / 8;
            Pixels = new byte[PixelCount * step];
        }

        /// <summary>
        /// Lock bitmap data
        /// </summary>
        public void LockBits()
        {
            try
            {
                bitmapData = source.LockBits(Rect, ImageLockMode.ReadWrite, source.PixelFormat);
                Iptr = bitmapData.Scan0;

                // Copy data from pointer to array
                //Marshal.Copy(Iptr, Pixels, 0, Pixels.Length);
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }

        /// <summary>
        /// Unlock bitmap data
        /// </summary>
        public void UnlockBits()
        {
            try
            {
                Marshal.Copy(Pixels, 0, Iptr, Pixels.Length);
                source.UnlockBits(bitmapData);
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }
    }
}
