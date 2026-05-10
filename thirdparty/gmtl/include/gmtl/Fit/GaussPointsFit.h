// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

// Based on code from:
// Magic Software, Inc.
// http://www.magic-software.com
//
#ifndef _GMTL_GAUSSPOINTSFIT_H
#define _GMTL_GAUSSPOINTSFIT_H

// Fit points with a Gaussian distribution.  The center is the mean of the
// points, the axes are the eigenvectors of the covariance matrix, and the
// extents are the eigenvalues of the covariance matrix and are returned in
// increasing order.  The last two functions allow selection of valid
// vertices from a pool.  The return value is 'true' if and only if at least
// one vertex was valid.

#include <gmtl/Vec3.h>
#include <gmtl/Point3.h>
#include <gmtl/Numerics/Eigen.h>

namespace gmtl
{

/*
void MgcGaussPointsFit (int iQuantity, const MgcVector2* akPoint,
    MgcVector2& rkCenter, MgcVector2 akAxis[2], MgcReal afExtent[2]);
*/

void GaussPointsFit (int iQuantity, const Point3* akPoint,
    Point3& rkCenter, Vec3 akAxis[3], float afExtent[3]);

/*
bool MgcGaussPointsFit (int iQuantity, const MgcVector2* akPoint,
    const bool* abValid, MgcVector2& rkCenter, MgcVector2 akAxis[2],
    MgcReal afExtent[2]);
*/

bool GaussPointsFit (int iQuantity, const Vec3* akPoint,
    const bool* abValid, Vec3& rkCenter, Vec3 akAxis[3],
    float afExtent[3]);
	

// --- Implementations ---- //
void GaussPointsFit (int iQuantity, const Point3* akPoint,
    Point3& rkCenter, Vec3 akAxis[3], float afExtent[3])
{
    // compute mean of points
    rkCenter = akPoint[0];
    unsigned i;
    for (i = 1; i < iQuantity; i++)
        rkCenter += akPoint[i];
    float fInvQuantity = 1.0f/iQuantity;
    rkCenter *= fInvQuantity;

    // compute covariances of points
    float fSumXX = 0.0, fSumXY = 0.0, fSumXZ = 0.0;
    float fSumYY = 0.0, fSumYZ = 0.0, fSumZZ = 0.0;
    for (i = 0; i < iQuantity; i++)
    {
        Vec3 kDiff = akPoint[i] - rkCenter;
        fSumXX += kDiff[Xelt]*kDiff[Xelt];
        fSumXY += kDiff[Xelt]*kDiff[Yelt];
        fSumXZ += kDiff[Xelt]*kDiff[Zelt];
        fSumYY += kDiff[Yelt]*kDiff[Yelt];
        fSumYZ += kDiff[Yelt]*kDiff[Zelt];
        fSumZZ += kDiff[Zelt]*kDiff[Zelt];
    }
    fSumXX *= fInvQuantity;
    fSumXY *= fInvQuantity;
    fSumXZ *= fInvQuantity;
    fSumYY *= fInvQuantity;
    fSumYZ *= fInvQuantity;
    fSumZZ *= fInvQuantity;

    // compute eigenvectors for covariance matrix
    gmtl::Eigen kES(3);
    kES.Matrix(0,0) = fSumXX;
    kES.Matrix(0,1) = fSumXY;
    kES.Matrix(0,2) = fSumXZ;
    kES.Matrix(1,0) = fSumXY;
    kES.Matrix(1,1) = fSumYY;
    kES.Matrix(1,2) = fSumYZ;
    kES.Matrix(2,0) = fSumXZ;
    kES.Matrix(2,1) = fSumYZ;
    kES.Matrix(2,2) = fSumZZ;
    kES.IncrSortEigenStuff3();

    akAxis[0][Xelt] = kES.GetEigenvector(0,0);
    akAxis[0][Yelt] = kES.GetEigenvector(1,0);
    akAxis[0][Zelt] = kES.GetEigenvector(2,0);

    akAxis[1][Xelt] = kES.GetEigenvector(0,1);
    akAxis[1][Yelt] = kES.GetEigenvector(1,1);
    akAxis[1][Zelt] = kES.GetEigenvector(2,1);

    akAxis[2][Xelt] = kES.GetEigenvector(0,2);
    akAxis[2][Yelt] = kES.GetEigenvector(1,2);
    akAxis[2][Zelt] = kES.GetEigenvector(2,2);

    afExtent[0] = kES.GetEigenvalue(0);
    afExtent[1] = kES.GetEigenvalue(1);
    afExtent[2] = kES.GetEigenvalue(2);
}


//
bool GaussPointsFit (int iQuantity, const Vec3* akPoint,
    const bool* abValid, Vec3& rkCenter, Vec3 akAxis[3],
    float afExtent[3])
{
    // compute mean of points
    rkCenter = ZeroVec3;
    int i, iValidQuantity = 0;
    for (i = 0; i < iQuantity; i++)
    {
        if ( abValid[i] )
        {
            rkCenter += akPoint[i];
            iValidQuantity++;
        }
    }
    if ( iValidQuantity == 0 )
        return false;

    float fInvQuantity = 1.0/iValidQuantity;
    rkCenter *= fInvQuantity;

    // compute covariances of points
    float fSumXX = 0.0, fSumXY = 0.0, fSumXZ = 0.0;
    float fSumYY = 0.0, fSumYZ = 0.0, fSumZZ = 0.0;
    for (i = 0; i < iQuantity; i++)
    {
        if ( abValid[i] )
        {
            Vec3 kDiff = akPoint[i] - rkCenter;
            fSumXX += kDiff[Xelt]*kDiff[Xelt];
            fSumXY += kDiff[Xelt]*kDiff[Yelt];
            fSumXZ += kDiff[Xelt]*kDiff[Zelt];
            fSumYY += kDiff[Yelt]*kDiff[Yelt];
            fSumYZ += kDiff[Yelt]*kDiff[Zelt];
            fSumZZ += kDiff[Zelt]*kDiff[Zelt];
        }
    }
    fSumXX *= fInvQuantity;
    fSumXY *= fInvQuantity;
    fSumXZ *= fInvQuantity;
    fSumYY *= fInvQuantity;
    fSumYZ *= fInvQuantity;
    fSumZZ *= fInvQuantity;

    // compute eigenvectors for covariance matrix
    Eigen kES(3);
    kES.Matrix(0,0) = fSumXX;
    kES.Matrix(0,1) = fSumXY;
    kES.Matrix(0,2) = fSumXZ;
    kES.Matrix(1,0) = fSumXY;
    kES.Matrix(1,1) = fSumYY;
    kES.Matrix(1,2) = fSumYZ;
    kES.Matrix(2,0) = fSumXZ;
    kES.Matrix(2,1) = fSumYZ;
    kES.Matrix(2,2) = fSumZZ;
    kES.IncrSortEigenStuff3();

    akAxis[0][Xelt] = kES.GetEigenvector(0,0);
    akAxis[0][Yelt] = kES.GetEigenvector(1,0);
    akAxis[0][Zelt] = kES.GetEigenvector(2,0);

    akAxis[1][Xelt] = kES.GetEigenvector(0,1);
    akAxis[1][Yelt] = kES.GetEigenvector(1,1);
    akAxis[1][Zelt] = kES.GetEigenvector(2,1);

    akAxis[2][Xelt] = kES.GetEigenvector(0,2);
    akAxis[2][Yelt] = kES.GetEigenvector(1,2);
    akAxis[2][Zelt] = kES.GetEigenvector(2,2);

    afExtent[0] = kES.GetEigenvalue(0);
    afExtent[1] = kES.GetEigenvalue(1);
    afExtent[2] = kES.GetEigenvalue(2);

    return true;
}

};

/*
void MgcGaussPointsFit (int iQuantity, const MgcVector2* akPoint,
    MgcVector2& rkCenter, MgcVector2 akAxis[2], float afExtent[2])
{
    // compute mean of points
    rkCenter = akPoint[0];
    int i;
    for (i = 1; i < iQuantity; i++)
        rkCenter += akPoint[i];
    float fInvQuantity = 1.0/iQuantity;
    rkCenter *= fInvQuantity;

    // compute covariances of points
    float fSumXX = 0.0, fSumXY = 0.0, fSumYY = 0.0;
    for (i = 0; i < iQuantity; i++)
    {
        MgcVector2 kDiff = akPoint[i] - rkCenter;
        fSumXX += kDiff.x*kDiff.x;
        fSumXY += kDiff.x*kDiff.y;
        fSumYY += kDiff.y*kDiff.y;
    }
    fSumXX *= fInvQuantity;
    fSumXY *= fInvQuantity;
    fSumYY *= fInvQuantity;

    // solve eigensystem of covariance matrix
    MgcEigen kES(2);
    kES.Matrix(0,0) = fSumXX;
    kES.Matrix(0,1) = fSumXY;
    kES.Matrix(1,0) = fSumXY;
    kES.Matrix(1,1) = fSumYY;
    kES.IncrSortEigenStuff2();

    akAxis[0].x = kES.GetEigenvector(0,0);
    akAxis[0].y = kES.GetEigenvector(1,0);
    akAxis[1].x = kES.GetEigenvector(0,1);
    akAxis[1].y = kES.GetEigenvector(1,1);

    afExtent[0] = kES.GetEigenvalue(0);
    afExtent[1] = kES.GetEigenvalue(1);
}
*/


/*
bool MgcGaussPointsFit (int iQuantity, const MgcVector2* akPoint,
    const bool* abValid, MgcVector2& rkCenter, MgcVector2 akAxis[2],
    float afExtent[2])
{
    // compute mean of points
    rkCenter = MgcVector2::ZERO;
    int i, iValidQuantity = 0;
    for (i = 0; i < iQuantity; i++)
    {
        if ( abValid[i] )
        {
            rkCenter += akPoint[i];
            iValidQuantity++;
        }
    }
    if ( iValidQuantity == 0 )
        return false;

    float fInvQuantity = 1.0/iValidQuantity;
    rkCenter *= fInvQuantity;

    // compute covariances of points
    float fSumXX = 0.0, fSumXY = 0.0, fSumYY = 0.0;
    for (i = 0; i < iQuantity; i++)
    {
        if ( abValid[i] )
        {
            MgcVector2 kDiff = akPoint[i] - rkCenter;
            fSumXX += kDiff.x*kDiff.x;
            fSumXY += kDiff.x*kDiff.y;
            fSumYY += kDiff.y*kDiff.y;
        }
    }
    fSumXX *= fInvQuantity;
    fSumXY *= fInvQuantity;
    fSumYY *= fInvQuantity;

    // solve eigensystem of covariance matrix
    MgcEigen kES(2);
    kES.Matrix(0,0) = fSumXX;
    kES.Matrix(0,1) = fSumXY;
    kES.Matrix(1,0) = fSumXY;
    kES.Matrix(1,1) = fSumYY;
    kES.IncrSortEigenStuff2();

    akAxis[0].x = kES.GetEigenvector(0,0);
    akAxis[0].y = kES.GetEigenvector(1,0);
    akAxis[1].x = kES.GetEigenvector(0,1);
    akAxis[1].y = kES.GetEigenvector(1,1);

    afExtent[0] = kES.GetEigenvalue(0);
    afExtent[1] = kES.GetEigenvalue(1);

    return true;
}
*/

#endif
