using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MLVViewSharp
{
    public class Matrix
    {
        private float[,] mInnerMatrix;
        private int mRowCount = 0;
        public int RowCount
        {
            get { return mRowCount; }
        }
        private int mColumnCount = 0;
        public int ColumnCount
        {
            get { return mColumnCount; }
        }

        public Matrix()
        {

        }

        public Matrix(float[][] source)
        {
            mRowCount = source.Length;
            mColumnCount = source[0].Length;
            mInnerMatrix = new float[mRowCount, mColumnCount];

            for (int row = 0; row < mRowCount; row++)
            {
                for (int col = 0; col < mColumnCount; col++)
                {
                    mInnerMatrix[row, col] = source[row][col];
                }
            }
        }

        public Matrix(Matrix source)
        {
            mRowCount = source.RowCount;
            mColumnCount = source.ColumnCount;
            mInnerMatrix = new float[mRowCount, mColumnCount];

            for (int row = 0; row < mRowCount; row++)
            {
                for (int col = 0; col < mColumnCount; col++)
                {
                    mInnerMatrix[row, col] = source.mInnerMatrix[row, col];
                }
            }
        }

        public Matrix(int rowCount, int columnCount)
        {
            mRowCount = rowCount;
            mColumnCount = columnCount;
            mInnerMatrix = new float[rowCount, columnCount];
        }

        public Matrix(int rowCount)
        {
            mRowCount = rowCount;
            mColumnCount = 1;
            mInnerMatrix = new float[rowCount, 1];
        }

        public float this[int rowNumber, int columnNumber]
        {
            get
            {
                return mInnerMatrix[rowNumber, columnNumber];
            }
            set
            {
                mInnerMatrix[rowNumber, columnNumber] = value;
            }
        }


        public float this[int rowNumber]
        {
            get
            {
                return mInnerMatrix[rowNumber, 0];
            }
            set
            {
                mInnerMatrix[rowNumber, 0] = value;
            }
        }

        public Matrix Identity()
        {
            Matrix identity = new Matrix(RowCount, ColumnCount);

            for (int row = 0; row < RowCount; row++)
            {
                for (int col = 0; col < ColumnCount; col++)
                {
                    if (row == col)
                    {
                        identity[row, col] = 1;
                    }
                    else
                    {
                        identity[row, col] = 0;
                    }
                }
            }

            return identity;
        }

        public float[] GetRow(int rowIndex)
        {
            float[] rowValues = new float[mColumnCount];
            for (int i = 0; i < mColumnCount; i++)
            {
                rowValues[i] = mInnerMatrix[rowIndex, i];
            }
            return rowValues;
        }
        public void SetRow(int rowIndex, float[] value)
        {
            if (value.Length != mColumnCount)
            {
                throw new Exception("Boyut Uyusmazligi");
            }
            for (int i = 0; i < value.Length; i++)
            {
                mInnerMatrix[rowIndex, i] = value[i];
            }
        }
        public float[] GetColumn(int columnIndex)
        {
            float[] columnValues = new float[mRowCount];
            for (int i = 0; i < mRowCount; i++)
            {
                columnValues[i] = mInnerMatrix[i, columnIndex];
            }
            return columnValues;
        }
        public void SetColumn(int columnIndex, float[] value)
        {
            if (value.Length != mRowCount)
            {
                throw new Exception("Boyut Uyusmazligi");
            }
            for (int i = 0; i < value.Length; i++)
            {
                mInnerMatrix[i, columnIndex] = value[i];
            }
        }


        public static Matrix operator +(Matrix pMatrix1, Matrix pMatrix2)
        {
            if (!(pMatrix1.RowCount == pMatrix2.RowCount && pMatrix1.ColumnCount == pMatrix2.ColumnCount))
            {
                throw new Exception("Boyut Uyusmazligi");
            }
            Matrix returnMatrix = new Matrix(pMatrix1.RowCount, pMatrix2.RowCount);
            for (int i = 0; i < pMatrix1.RowCount; i++)
            {
                for (int j = 0; j < pMatrix1.ColumnCount; j++)
                {
                    returnMatrix[i, j] = pMatrix1[i, j] + pMatrix2[i, j];
                }
            }
            return returnMatrix;
        }
        public static Matrix operator *(float scalarValue, Matrix pMatrix)
        {
            Matrix returnMatrix = new Matrix(pMatrix.RowCount, pMatrix.RowCount);
            for (int i = 0; i < pMatrix.RowCount; i++)
            {
                for (int j = 0; j < pMatrix.ColumnCount; j++)
                {
                    returnMatrix[i, j] = pMatrix[i, j] * scalarValue;
                }
            }
            return returnMatrix;
        }
        public static Matrix operator -(Matrix pMatrix1, Matrix pMatrix2)
        {
            if (!(pMatrix1.RowCount == pMatrix2.RowCount && pMatrix1.ColumnCount == pMatrix2.ColumnCount))
            {
                throw new Exception("Boyut Uyusmazligi");
            }
            return pMatrix1 + (-1 * pMatrix2);
        }
        public static bool operator ==(Matrix pMatrix1, Matrix pMatrix2)
        {
            if ((Object)pMatrix1 == (Object)pMatrix2)
            {
                return true;
            }
            if ((Object)pMatrix1 == null || (Object)pMatrix2 == null)
            {
                return false;
            }
            if (!(pMatrix1.RowCount == pMatrix2.RowCount && pMatrix1.ColumnCount == pMatrix2.ColumnCount))
            {
                //boyut uyusmazligi
                return false;
            }
            for (int i = 0; i < pMatrix1.RowCount; i++)
            {
                for (int j = 0; j < pMatrix1.ColumnCount; j++)
                {
                    if (pMatrix1[i, j] != pMatrix2[i, j])
                    {
                        return false;
                    }
                }
            }
            return true; ;
        }
        public static bool operator !=(Matrix pMatrix1, Matrix pMatrix2)
        {
            return !(pMatrix1 == pMatrix2);
        }
        public static Matrix operator -(Matrix pMatrix)
        {
            return -1 * pMatrix;
        }
        public static Matrix operator ++(Matrix pMatrix)
        {

            for (int i = 0; i < pMatrix.RowCount; i++)
            {
                for (int j = 0; j < pMatrix.ColumnCount; j++)
                {
                    pMatrix[i, j] += 1;
                }
            }
            return pMatrix;
        }
        public static Matrix operator --(Matrix pMatrix)
        {
            for (int i = 0; i < pMatrix.RowCount; i++)
            {
                for (int j = 0; j < pMatrix.ColumnCount; j++)
                {
                    pMatrix[i, j] -= 1;
                }
            }
            return pMatrix;
        }
        public static Matrix operator *(Matrix pMatrix1, Matrix pMatrix2)
        {
            if (pMatrix1.ColumnCount != pMatrix2.RowCount)
            {
                throw new Exception("Boyut Uyusmazligi");
            }
            Matrix returnMatrix = new Matrix(pMatrix1.RowCount, pMatrix2.ColumnCount);
            for (int i = 0; i < pMatrix1.RowCount; i++)
            {
                float[] rowValues = pMatrix1.GetRow(i);
                for (int j = 0; j < pMatrix2.ColumnCount; j++)
                {
                    float[] columnValues = pMatrix2.GetColumn(j);
                    float value = 0;
                    for (int a = 0; a < rowValues.Length; a++)
                    {
                        value += rowValues[a] * columnValues[a];
                    }
                    returnMatrix[i, j] = value;
                }
            }
            return returnMatrix;
        }

        public Matrix Transpose()
        {
            Matrix mreturnMatrix = new Matrix(ColumnCount, RowCount);
            for (int i = 0; i < mRowCount; i++)
            {
                for (int j = 0; j < mColumnCount; j++)
                {
                    mreturnMatrix[j, i] = mInnerMatrix[i, j];
                }
            }
            return mreturnMatrix;
        }

        public Matrix Inverse()
        {
            if (RowCount != ColumnCount)
            {
                throw new InvalidOperationException();
            }

            Matrix source = new Matrix(this);
            Matrix identity = new Matrix(RowCount, ColumnCount).Identity();

            /*  */
            for (int col = 0; col < RowCount; col++)
            {
                /* normalize frontmost element */
                if (Math.Abs(source[col, col]) == 0)
                {
                    throw new InvalidOperationException();
                }

                float fact = 1 / source[col, col];
                MultiplyRow(source, col, fact);
                MultiplyRow(identity, col, fact);

                /* set all others to zero */
                for (int zerow = col + 1; zerow < RowCount; zerow++)
                {
                    float scale = source[zerow, col];
                    for (int pos = 0; pos < ColumnCount; pos++)
                    {
                        source[zerow, pos] -= source[col, pos] * scale;
                        identity[zerow, pos] -= identity[col, pos] * scale;
                    }
                }
            }

            /* now we have the lower left triangle zeroed and the diagonal zeroed - create identitdy matrix on left side */
            for (int row = 0; row < RowCount; row++)
            {
                for (int col = row + 1; col < ColumnCount; col++)
                {
                    float scale = source[row, col];
                    for (int pos = 0; pos < ColumnCount; pos++)
                    {
                        source[row, pos] -= source[col, pos] * scale;
                        identity[row, pos] -= identity[col, pos] * scale;
                    }
                }
            }

            return identity;
        }

        private static void MultiplyRow(Matrix matrix, int row, float fact)
        {
            for (int col = 0; col < matrix.ColumnCount; col++)
            {
                matrix[row, col] *= fact;
            }
        }

        public override bool Equals(object obj)
        {
            return base.Equals(obj);
        }
        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        public bool IsZeroMatrix()
        {
            for (int i = 0; i < this.RowCount; i++)
            {
                for (int j = 0; j < this.ColumnCount; j++)
                {
                    if (Math.Abs(mInnerMatrix[i, j]) != 0)
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        public bool IsSquareMatrix()
        {
            return (this.RowCount == this.ColumnCount);
        }
        public bool IsLowerTriangle()
        {
            if (!this.IsSquareMatrix())
            {
                return false;
            }
            for (int i = 0; i < this.RowCount; i++)
            {
                for (int j = i + 1; j < this.ColumnCount; j++)
                {
                    if (Math.Abs(mInnerMatrix[i, j]) != 0)
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        public bool IsUpperTriangle()
        {
            if (!this.IsSquareMatrix())
            {
                return false;
            }
            for (int i = 0; i < this.RowCount; i++)
            {
                for (int j = 0; j < i; j++)
                {
                    if (Math.Abs(mInnerMatrix[i, j]) != 0)
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        public bool IsDiagonalMatrix()
        {
            if (!this.IsSquareMatrix())
            {
                return false;
            }
            for (int i = 0; i < this.RowCount; i++)
            {
                for (int j = 0; j < this.ColumnCount; j++)
                {
                    if (i != j && Math.Abs(mInnerMatrix[i, j]) != 0)
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        public bool IsIdentityMatrix()
        {
            if (!this.IsSquareMatrix())
            {
                return false;
            }
            for (int i = 0; i < this.RowCount; i++)
            {
                for (int j = 0; j < this.ColumnCount; j++)
                {
                    float checkValue = 0;
                    if (i == j)
                    {
                        checkValue = 1;
                    }
                    if (mInnerMatrix[i, j] != checkValue)
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        public bool IsSymetricMatrix()
        {
            if (!this.IsSquareMatrix())
            {
                return false;
            }
            Matrix transposeMatrix = this.Transpose();
            if (this == transposeMatrix)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}
