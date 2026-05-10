// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

//
// XXX: Based on source code from: Magic Software, Inc.
//
#ifndef _EIGEN_H
#define _EIGEN_H

#include <gmtl/gmtlConfig.h>

namespace gmtl
{

class Eigen
{
public:
    Eigen (int iSize);
    ~Eigen ();

    // set the matrix for eigensolving
    float& Matrix (int iRow, int iCol);
    void SetMatrix (float** aafMat);

    // get the results of eigensolving (eigenvectors are columns of matrix)
    float GetEigenvalue (int i) const;
    float GetEigenvector (int iRow, int iCol) const;
    float* GetEigenvalue ();
    float** GetEigenvector ();

    // solve eigensystem
    void EigenStuff2 ();
    void EigenStuff3 ();
    void EigenStuff4 ();
    void EigenStuffN ();
    void EigenStuff  ();

    // solve eigensystem, use decreasing sort on eigenvalues
    void DecrSortEigenStuff2 ();
    void DecrSortEigenStuff3 ();
    void DecrSortEigenStuff4 ();
    void DecrSortEigenStuffN ();
    void DecrSortEigenStuff  ();

    // solve eigensystem, use increasing sort on eigenvalues
    void IncrSortEigenStuff2 ();
    void IncrSortEigenStuff3 ();
    void IncrSortEigenStuff4 ();
    void IncrSortEigenStuffN ();
    void IncrSortEigenStuff  ();

protected:
    int m_iSize;
    float** m_aafMat;
    float* m_afDiag;
    float* m_afSubd;

    // Householder reduction to tridiagonal form
    static void Tridiagonal2 (float** aafMat, float* afDiag,
        float* afSubd);
    static void Tridiagonal3 (float** aafMat, float* afDiag,
        float* afSubd);
    static void Tridiagonal4 (float** aafMat, float* afDiag,
        float* afSubd);
    static void TridiagonalN (int iSize, float** aafMat, float* afDiag,
        float* afSubd);

    // QL algorithm with implicit shifting, applies to tridiagonal matrices
    static bool QLAlgorithm (int iSize, float* afDiag, float* afSubd,
        float** aafMat);

    // sort eigenvalues from largest to smallest
    static void DecreasingSort (int iSize, float* afEigval,
        float** aafEigvec);

    // sort eigenvalues from smallest to largest
    static void IncreasingSort (int iSize, float* afEigval,
        float** aafEigvec);
};

//---------------------------------------------------------------------------
inline float& Eigen::Matrix (int iRow, int iCol)
{
    return m_aafMat[iRow][iCol];
}
//---------------------------------------------------------------------------
inline float Eigen::GetEigenvalue (int i) const
{
    return m_afDiag[i];
}
//---------------------------------------------------------------------------
inline float Eigen::GetEigenvector (int iRow, int iCol) const
{
    return m_aafMat[iRow][iCol];
}
//---------------------------------------------------------------------------
inline float* Eigen::GetEigenvalue ()
{
    return m_afDiag;
}
//---------------------------------------------------------------------------
inline float** Eigen::GetEigenvector ()
{
    return m_aafMat;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
Eigen::Eigen (int iSize)
{
    assert( iSize >= 2 );
    m_iSize = iSize;

    m_aafMat = new float*[m_iSize];
    for (int i = 0; i < m_iSize; i++)
        m_aafMat[i] = new float[m_iSize];

    m_afDiag = new float[m_iSize];
    m_afSubd = new float[m_iSize];
}
//---------------------------------------------------------------------------
Eigen::~Eigen ()
{
    delete[] m_afSubd;
    delete[] m_afDiag;
    for (int i = 0; i < m_iSize; i++)
        delete[] m_aafMat[i];
    delete[] m_aafMat;
}
//---------------------------------------------------------------------------
void Eigen::Tridiagonal2 (float** m_aafMat, float* m_afDiag,
    float* m_afSubd)
{
    // matrix is already tridiagonal
    m_afDiag[0] = m_aafMat[0][0];
    m_afDiag[1] = m_aafMat[1][1];
    m_afSubd[0] = m_aafMat[0][1];
    m_afSubd[1] = 0.0;
    m_aafMat[0][0] = 1.0;
    m_aafMat[0][1] = 0.0;
    m_aafMat[1][0] = 0.0;
    m_aafMat[1][1] = 1.0;
}
//---------------------------------------------------------------------------
void Eigen::Tridiagonal3 (float** m_aafMat, float* m_afDiag,
    float* m_afSubd)
{
    float fM00 = m_aafMat[0][0];
    float fM01 = m_aafMat[0][1];
    float fM02 = m_aafMat[0][2];
    float fM11 = m_aafMat[1][1];
    float fM12 = m_aafMat[1][2];
    float fM22 = m_aafMat[2][2];

    m_afDiag[0] = fM00;
    m_afSubd[2] = 0.0;
    if ( fM02 != 0.0 )
    {
        float fLength = Math::sqrt(fM01*fM01+fM02*fM02);
        float fInvLength = 1.0/fLength;
        fM01 *= fInvLength;
        fM02 *= fInvLength;
        float fQ = 2.0*fM01*fM12+fM02*(fM22-fM11);
        m_afDiag[1] = fM11+fM02*fQ;
        m_afDiag[2] = fM22-fM02*fQ;
        m_afSubd[0] = fLength;
        m_afSubd[1] = fM12-fM01*fQ;
        m_aafMat[0][0] = 1.0; m_aafMat[0][1] = 0.0;  m_aafMat[0][2] = 0.0;
        m_aafMat[1][0] = 0.0; m_aafMat[1][1] = fM01; m_aafMat[1][2] = fM02;
        m_aafMat[2][0] = 0.0; m_aafMat[2][1] = fM02; m_aafMat[2][2] = -fM01;
    }
    else
    {
        m_afDiag[1] = fM11;
        m_afDiag[2] = fM22;
        m_afSubd[0] = fM01;
        m_afSubd[1] = fM12;
        m_aafMat[0][0] = 1.0; m_aafMat[0][1] = 0.0; m_aafMat[0][2] = 0.0;
        m_aafMat[1][0] = 0.0; m_aafMat[1][1] = 1.0; m_aafMat[1][2] = 0.0;
        m_aafMat[2][0] = 0.0; m_aafMat[2][1] = 0.0; m_aafMat[2][2] = 1.0;
    }
}
//---------------------------------------------------------------------------
void Eigen::Tridiagonal4 (float** m_aafMat, float* m_afDiag,
    float* m_afSubd)
{
    // save matrix M
    float fM00 = m_aafMat[0][0];
    float fM01 = m_aafMat[0][1];
    float fM02 = m_aafMat[0][2];
    float fM03 = m_aafMat[0][3];
    float fM11 = m_aafMat[1][1];
    float fM12 = m_aafMat[1][2];
    float fM13 = m_aafMat[1][3];
    float fM22 = m_aafMat[2][2];
    float fM23 = m_aafMat[2][3];
    float fM33 = m_aafMat[3][3];

    m_afDiag[0] = fM00;
    m_afSubd[3] = 0.0;

    m_aafMat[0][0] = 1.0;
    m_aafMat[0][1] = 0.0;
    m_aafMat[0][2] = 0.0;
    m_aafMat[0][3] = 0.0;
    m_aafMat[1][0] = 0.0;
    m_aafMat[2][0] = 0.0;
    m_aafMat[3][0] = 0.0;

    float fLength, fInvLength;

    if ( fM02 != 0.0 || fM03 != 0.0 )
    {
        float fQ11, fQ12, fQ13;
        float fQ21, fQ22, fQ23;
        float fQ31, fQ32, fQ33;

        // build column Q1
        fLength = Math::sqrt(fM01*fM01 + fM02*fM02 + fM03*fM03);
        fInvLength = 1.0/fLength;
        fQ11 = fM01*fInvLength;
        fQ21 = fM02*fInvLength;
        fQ31 = fM03*fInvLength;

        m_afSubd[0] = fLength;

        // compute S*Q1
        float fV0 = fM11*fQ11+fM12*fQ21+fM13*fQ31;
        float fV1 = fM12*fQ11+fM22*fQ21+fM23*fQ31;
        float fV2 = fM13*fQ11+fM23*fQ21+fM33*fQ31;

        m_afDiag[1] = fQ11*fV0+fQ21*fV1+fQ31*fV2;

        // build column Q3 = Q1x(S*Q1)
        fQ13 = fQ21*fV2-fQ31*fV1;
        fQ23 = fQ31*fV0-fQ11*fV2;
        fQ33 = fQ11*fV1-fQ21*fV0;
        fLength = Math::sqrt(fQ13*fQ13+fQ23*fQ23+fQ33*fQ33);
        if ( fLength > 0.0 )
        {
            fInvLength = 1.0/fLength;
            fQ13 *= fInvLength;
            fQ23 *= fInvLength;
            fQ33 *= fInvLength;

            // build column Q2 = Q3xQ1
            fQ12 = fQ23*fQ31-fQ33*fQ21;
            fQ22 = fQ33*fQ11-fQ13*fQ31;
            fQ32 = fQ13*fQ21-fQ23*fQ11;

            fV0 = fQ12*fM11+fQ22*fM12+fQ32*fM13;
            fV1 = fQ12*fM12+fQ22*fM22+fQ32*fM23;
            fV2 = fQ12*fM13+fQ22*fM23+fQ32*fM33;
            m_afSubd[1] = fQ11*fV0+fQ21*fV1+fQ31*fV2;
            m_afDiag[2] = fQ12*fV0+fQ22*fV1+fQ32*fV2;
            m_afSubd[2] = fQ13*fV0+fQ23*fV1+fQ33*fV2;

            fV0 = fQ13*fM11+fQ23*fM12+fQ33*fM13;
            fV1 = fQ13*fM12+fQ23*fM22+fQ33*fM23;
            fV2 = fQ13*fM13+fQ23*fM23+fQ33*fM33;
            m_afDiag[3] = fQ13*fV0+fQ23*fV1+fQ33*fV2;
        }
        else
        {
            // S*Q1 parallel to Q1, choose any valid Q2 and Q3
            m_afSubd[1] = 0;

            fLength = fQ21*fQ21+fQ31*fQ31;
            if ( fLength > 0.0 )
            {
                fInvLength = 1.0/fLength;
                float fTmp = fQ11-1.0;
                fQ12 = -fQ21;
                fQ22 = 1.0+fTmp*fQ21*fQ21*fInvLength;
                fQ32 = fTmp*fQ21*fQ31*fInvLength;

                fQ13 = -fQ31;
                fQ23 = fQ32;
                fQ33 = 1.0+fTmp*fQ31*fQ31*fInvLength;

                fV0 = fQ12*fM11+fQ22*fM12+fQ32*fM13;
                fV1 = fQ12*fM12+fQ22*fM22+fQ32*fM23;
                fV2 = fQ12*fM13+fQ22*fM23+fQ32*fM33;
                m_afDiag[2] = fQ12*fV0+fQ22*fV1+fQ32*fV2;
                m_afSubd[2] = fQ13*fV0+fQ23*fV1+fQ33*fV2;

                fV0 = fQ13*fM11+fQ23*fM12+fQ33*fM13;
                fV1 = fQ13*fM12+fQ23*fM22+fQ33*fM23;
                fV2 = fQ13*fM13+fQ23*fM23+fQ33*fM33;
                m_afDiag[3] = fQ13*fV0+fQ23*fV1+fQ33*fV2;
            }
            else
            {
                // Q1 = (+-1,0,0)
                fQ12 = 0.0; fQ22 = 1.0; fQ32 = 0.0;
                fQ13 = 0.0; fQ23 = 0.0; fQ33 = 1.0;

                m_afDiag[2] = fM22;
                m_afDiag[3] = fM33;
                m_afSubd[2] = fM23;
            }
        }

        m_aafMat[1][1] = fQ11; m_aafMat[1][2] = fQ12; m_aafMat[1][3] = fQ13;
        m_aafMat[2][1] = fQ21; m_aafMat[2][2] = fQ22; m_aafMat[2][3] = fQ23;
        m_aafMat[3][1] = fQ31; m_aafMat[3][2] = fQ32; m_aafMat[3][3] = fQ33;
    }
    else
    {
        m_afDiag[1] = fM11;
        m_afSubd[0] = fM01;
        m_aafMat[1][1] = 1.0;
        m_aafMat[2][1] = 0.0;
        m_aafMat[3][1] = 0.0;

        if ( fM13 != 0.0 )
        {
            fLength = Math::sqrt(fM12*fM12+fM13*fM13);
            fInvLength = 1.0/fLength;
            fM12 *= fInvLength;
            fM13 *= fInvLength;
            float fQ = 2.0*fM12*fM23+fM13*(fM33-fM22);

            m_afDiag[2] = fM22+fM13*fQ;
            m_afDiag[3] = fM33-fM13*fQ;
            m_afSubd[1] = fLength;
            m_afSubd[2] = fM23-fM12*fQ;
            m_aafMat[1][2] = 0.0;
            m_aafMat[1][3] = 0.0;
            m_aafMat[2][2] = fM12;
            m_aafMat[2][3] = fM13;
            m_aafMat[3][2] = fM13;
            m_aafMat[3][3] = -fM12;
        }
        else
        {
            m_afDiag[2] = fM22;
            m_afDiag[3] = fM33;
            m_afSubd[1] = fM12;
            m_afSubd[2] = fM23;
            m_aafMat[1][2] = 0.0;
            m_aafMat[1][3] = 0.0;
            m_aafMat[2][2] = 1.0;
            m_aafMat[2][3] = 0.0;
            m_aafMat[3][2] = 0.0;
            m_aafMat[3][3] = 1.0;
        }
    }
}
//---------------------------------------------------------------------------
void Eigen::TridiagonalN (int iSize, float** m_aafMat,
    float* m_afDiag, float* m_afSubd)
{
    int i0, i1, i2, i3;

    for (i0 = iSize-1, i3 = iSize-2; i0 >= 1; i0--, i3--)
    {
        float fH = 0.0, fScale = 0.0;

        if ( i3 > 0 )
        {
            for (i2 = 0; i2 <= i3; i2++)
                fScale += Math::abs(m_aafMat[i0][i2]);
            if ( fScale == 0 )
            {
                m_afSubd[i0] = m_aafMat[i0][i3];
            }
            else
            {
                float fInvScale = 1.0/fScale;
                for (i2 = 0; i2 <= i3; i2++)
                {
                    m_aafMat[i0][i2] *= fInvScale;
                    fH += m_aafMat[i0][i2]*m_aafMat[i0][i2];
                }
                float fF = m_aafMat[i0][i3];
                float fG = Math::sqrt(fH);
                if ( fF > 0.0 )
                    fG = -fG;
                m_afSubd[i0] = fScale*fG;
                fH -= fF*fG;
                m_aafMat[i0][i3] = fF-fG;
                fF = 0.0;
                float fInvH = 1.0/fH;
                for (i1 = 0; i1 <= i3; i1++)
                {
                    m_aafMat[i1][i0] = m_aafMat[i0][i1]*fInvH;
                    fG = 0.0;
                    for (i2 = 0; i2 <= i1; i2++)
                        fG += m_aafMat[i1][i2]*m_aafMat[i0][i2];
                    for (i2 = i1+1; i2 <= i3; i2++)
                        fG += m_aafMat[i2][i1]*m_aafMat[i0][i2];
                    m_afSubd[i1] = fG*fInvH;
                    fF += m_afSubd[i1]*m_aafMat[i0][i1];
                }
                float fHalfFdivH = 0.5*fF*fInvH;
                for (i1 = 0; i1 <= i3; i1++)
                {
                    fF = m_aafMat[i0][i1];
                    fG = m_afSubd[i1] - fHalfFdivH*fF;
                    m_afSubd[i1] = fG;
                    for (i2 = 0; i2 <= i1; i2++)
                    {
                        m_aafMat[i1][i2] -= fF*m_afSubd[i2] +
                            fG*m_aafMat[i0][i2];
                    }
                }
            }
        }
        else
        {
            m_afSubd[i0] = m_aafMat[i0][i3];
        }

        m_afDiag[i0] = fH;
    }

    m_afDiag[0] = m_afSubd[0] = 0;
    for (i0 = 0, i3 = -1; i0 <= iSize-1; i0++, i3++)
    {
        if ( m_afDiag[i0] )
        {
            for (i1 = 0; i1 <= i3; i1++)
            {
                float fSum = 0;
                for (i2 = 0; i2 <= i3; i2++)
                    fSum += m_aafMat[i0][i2]*m_aafMat[i2][i1];
                for (i2 = 0; i2 <= i3; i2++)
                    m_aafMat[i2][i1] -= fSum*m_aafMat[i2][i0];
            }
        }
        m_afDiag[i0] = m_aafMat[i0][i0];
        m_aafMat[i0][i0] = 1;
        for (i1 = 0; i1 <= i3; i1++)
            m_aafMat[i1][i0] = m_aafMat[i0][i1] = 0;
    }

    // re-ordering if Eigen::QLAlgorithm is used subsequently
    for (i0 = 1, i3 = 0; i0 < iSize; i0++, i3++)
        m_afSubd[i3] = m_afSubd[i0];
    m_afSubd[iSize-1] = 0;
}
//---------------------------------------------------------------------------
bool Eigen::QLAlgorithm (int iSize, float* m_afDiag, float* m_afSubd,
    float** m_aafMat)
{
    const int iMaxIter = 32;

    for (int i0 = 0; i0 < iSize; i0++)
    {
        int i1;
        for (i1 = 0; i1 < iMaxIter; i1++)
        {
            int i2;
            for (i2 = i0; i2 <= iSize-2; i2++)
            {
                float fTmp =
                    Math::abs(m_afDiag[i2])+ Math::abs(m_afDiag[i2+1]);
                if ( Math::abs(m_afSubd[i2]) + fTmp == fTmp )
                    break;
            }
            if ( i2 == i0 )
                break;

            float fG = (m_afDiag[i0+1]-m_afDiag[i0])/(2.0*m_afSubd[i0]);
            float fR = Math::sqrt(fG*fG+1.0);
            if ( fG < 0.0 )
                fG = m_afDiag[i2]-m_afDiag[i0]+m_afSubd[i0]/(fG-fR);
            else
                fG = m_afDiag[i2]-m_afDiag[i0]+m_afSubd[i0]/(fG+fR);
            float fSin = 1.0, fCos = 1.0, fP = 0.0;
            for (int i3 = i2-1; i3 >= i0; i3--)
            {
                float fF = fSin*m_afSubd[i3];
                float fB = fCos*m_afSubd[i3];
                if ( Math::abs(fF) >= Math::abs(fG) )
                {
                    fCos = fG/fF;
                    fR = sqrt(fCos*fCos+1.0);
                    m_afSubd[i3+1] = fF*fR;
                    fSin = 1.0/fR;
                    fCos *= fSin;
                }
                else
                {
                    fSin = fF/fG;
                    fR = Math::sqrt(fSin*fSin+1.0);
                    m_afSubd[i3+1] = fG*fR;
                    fCos = 1.0/fR;
                    fSin *= fCos;
                }
                fG = m_afDiag[i3+1]-fP;
                fR = (m_afDiag[i3]-fG)*fSin+2.0*fB*fCos;
                fP = fSin*fR;
                m_afDiag[i3+1] = fG+fP;
                fG = fCos*fR-fB;

                for (int i4 = 0; i4 < iSize; i4++)
                {
                    fF = m_aafMat[i4][i3+1];
                    m_aafMat[i4][i3+1] = fSin*m_aafMat[i4][i3]+fCos*fF;
                    m_aafMat[i4][i3] = fCos*m_aafMat[i4][i3]-fSin*fF;
                }
            }
            m_afDiag[i0] -= fP;
            m_afSubd[i0] = fG;
            m_afSubd[i2] = 0.0;
        }
        if ( i1 == iMaxIter )
            return false;
    }

    return true;
}
//---------------------------------------------------------------------------
void Eigen::DecreasingSort (int iSize, float* afEigval,
    float** aafEigvec)
{
    // sort eigenvalues in decreasing order, e[0] >= ... >= e[iSize-1]
    for (int i0 = 0, i1; i0 <= iSize-2; i0++)
    {
        // locate maximum eigenvalue
        i1 = i0;
        float fMax = afEigval[i1];
        int i2;
        for (i2 = i0+1; i2 < iSize; i2++)
        {
            if ( afEigval[i2] > fMax )
            {
                i1 = i2;
                fMax = afEigval[i1];
            }
        }

        if ( i1 != i0 )
        {
            // swap eigenvalues
            afEigval[i1] = afEigval[i0];
            afEigval[i0] = fMax;

            // swap eigenvectors
            for (i2 = 0; i2 < iSize; i2++)
            {
                float fTmp = aafEigvec[i2][i0];
                aafEigvec[i2][i0] = aafEigvec[i2][i1];
                aafEigvec[i2][i1] = fTmp;
            }
        }
    }
}
//---------------------------------------------------------------------------
void Eigen::IncreasingSort (int iSize, float* afEigval,
    float** aafEigvec)
{
    // sort eigenvalues in increasing order, e[0] <= ... <= e[iSize-1]
    for (int i0 = 0, i1; i0 <= iSize-2; i0++)
    {
        // locate minimum eigenvalue
        i1 = i0;
        float fMin = afEigval[i1];
        int i2;
        for (i2 = i0+1; i2 < iSize; i2++)
        {
            if ( afEigval[i2] < fMin )
            {
                i1 = i2;
                fMin = afEigval[i1];
            }
        }

        if ( i1 != i0 )
        {
            // swap eigenvalues
            afEigval[i1] = afEigval[i0];
            afEigval[i0] = fMin;

            // swap eigenvectors
            for (i2 = 0; i2 < iSize; i2++)
            {
                float fTmp = aafEigvec[i2][i0];
                aafEigvec[i2][i0] = aafEigvec[i2][i1];
                aafEigvec[i2][i1] = fTmp;
            }
        }
    }
}
//---------------------------------------------------------------------------
void Eigen::SetMatrix (float** aafMat)
{
    for (int iRow = 0; iRow < m_iSize; iRow++)
    {
        for (int iCol = 0; iCol < m_iSize; iCol++)
            m_aafMat[iRow][iCol] = aafMat[iRow][iCol];
    }
}
//---------------------------------------------------------------------------
void Eigen::EigenStuff2 ()
{
    Tridiagonal2(m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::EigenStuff3 ()
{
    Tridiagonal3(m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::EigenStuff4 ()
{
    Tridiagonal4(m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::EigenStuffN ()
{
    TridiagonalN(m_iSize,m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::EigenStuff ()
{
    switch ( m_iSize )
    {
        case 2:
            Tridiagonal2(m_aafMat,m_afDiag,m_afSubd);
            break;
        case 3:
            Tridiagonal3(m_aafMat,m_afDiag,m_afSubd);
            break;
        case 4:
            Tridiagonal4(m_aafMat,m_afDiag,m_afSubd);
            break;
        default:
            TridiagonalN(m_iSize,m_aafMat,m_afDiag,m_afSubd);
            break;
    }
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::DecrSortEigenStuff2 ()
{
    Tridiagonal2(m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
    DecreasingSort(m_iSize,m_afDiag,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::DecrSortEigenStuff3 ()
{
    Tridiagonal3(m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
    DecreasingSort(m_iSize,m_afDiag,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::DecrSortEigenStuff4 ()
{
    Tridiagonal4(m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
    DecreasingSort(m_iSize,m_afDiag,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::DecrSortEigenStuffN ()
{
    TridiagonalN(m_iSize,m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
    DecreasingSort(m_iSize,m_afDiag,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::DecrSortEigenStuff ()
{
    switch ( m_iSize )
    {
        case 2:
            Tridiagonal2(m_aafMat,m_afDiag,m_afSubd);
            break;
        case 3:
            Tridiagonal3(m_aafMat,m_afDiag,m_afSubd);
            break;
        case 4:
            Tridiagonal4(m_aafMat,m_afDiag,m_afSubd);
            break;
        default:
            TridiagonalN(m_iSize,m_aafMat,m_afDiag,m_afSubd);
            break;
    }
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
    DecreasingSort(m_iSize,m_afDiag,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::IncrSortEigenStuff2 ()
{
    Tridiagonal2(m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
    IncreasingSort(m_iSize,m_afDiag,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::IncrSortEigenStuff3 ()
{
    Tridiagonal3(m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
    IncreasingSort(m_iSize,m_afDiag,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::IncrSortEigenStuff4 ()
{
    Tridiagonal4(m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
    IncreasingSort(m_iSize,m_afDiag,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::IncrSortEigenStuffN ()
{
    TridiagonalN(m_iSize,m_aafMat,m_afDiag,m_afSubd);
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
    IncreasingSort(m_iSize,m_afDiag,m_aafMat);
}
//---------------------------------------------------------------------------
void Eigen::IncrSortEigenStuff ()
{
    switch ( m_iSize )
    {
        case 2:
            Tridiagonal2(m_aafMat,m_afDiag,m_afSubd);
            break;
        case 3:
            Tridiagonal3(m_aafMat,m_afDiag,m_afSubd);
            break;
        case 4:
            Tridiagonal4(m_aafMat,m_afDiag,m_afSubd);
            break;
        default:
            TridiagonalN(m_iSize,m_aafMat,m_afDiag,m_afSubd);
            break;
    }
    QLAlgorithm(m_iSize,m_afDiag,m_afSubd,m_aafMat);
    IncreasingSort(m_iSize,m_afDiag,m_aafMat);
}
//---------------------------------------------------------------------------

};


/*
#ifdef EIGEN_TEST

int main ()
{
    Eigen kES(3);

    kES.Matrix(0,0) = 2.0;  kES.Matrix(0,1) = 1.0;  kES.Matrix(0,2) = 1.0;
    kES.Matrix(1,0) = 1.0;  kES.Matrix(1,1) = 2.0;  kES.Matrix(1,2) = 1.0;
    kES.Matrix(2,0) = 1.0;  kES.Matrix(2,1) = 1.0;  kES.Matrix(2,2) = 2.0;

    kES.IncrSortEigenStuff3();

    cout.setf(ios::fixed);

    cout << "eigenvalues = " << endl;
    int iRow;
    for (iRow = 0; iRow < 3; iRow++)
        cout << kES.GetEigenvalue(iRow) << ' ';
    cout << endl;

    cout << "eigenvectors = " << endl;
    for (iRow = 0; iRow < 3; iRow++)
    {
        for (int iCol = 0; iCol < 3; iCol++)
            cout << kES.GetEigenvector(iRow,iCol) << ' ';
        cout << endl;
    }

    // eigenvalues =
    //    1.000000 1.000000 4.000000
    // eigenvectors =
    //    0.411953  0.704955 0.577350
    //    0.404533 -0.709239 0.577350
    //   -0.816485  0.004284 0.577350

    return 0;
}

#endif
*/

#endif
