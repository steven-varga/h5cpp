// GMTL is (C) Copyright 2001-2010 by Allen Bierbaum
// Distributed under the GNU Lesser General Public License 2.1 with an
// addendum covering inlined code. (See accompanying files LICENSE and
// LICENSE.addendum or http://www.gnu.org/copyleft/lesser.txt)

#ifndef _GMTL_QUAT_H_
#define _GMTL_QUAT_H_

#include <gmtl/Defines.h>
#include <gmtl/Vec.h>
 
namespace gmtl
{

/** Quat: Class to encapsulate quaternion behaviors.
 *
 * this Quaternion is ordered in memory: x,y,z,w.
 * @see Quatf 
 * @see Quatd
 *
 * Note: The code for most of these routines was built using the following
 * references
 * 
 * References:
 * <ul>
 * <li>   Advanced Animation and Rendering Techniques: pp363-365
 * <li>   Animating Rotation with Quaternion Curves,  Ken Shoemake,
 *           SIGGRAPH Proceedings Vol 19, Number 3, 1985
 * <li>   Quaternion Calculus for Animation,  Ken Shoemake SIGGRAPH course
 *           notes 1989
 * <li>   Game Developer Magazine: Feb 98, pg.34-42
 * <li>   Motivation for the use of Quaternions to perform transformations
 *           http://www.rust.net/~kgeoinfo/quat1.htm
 * <li>   On quaternions; or on a new system of imaginaries in algebra,
 *           Sir William Rowan Hamilton, Philosophical Magazine, xxv,
 *           pp. 10-13 (July 1844)
 * <li>   You also can find more on quaternions at
 *    <ul>
 *    <li>  http://www.gamasutra.com/features/19980703/quaternions_01.htm and at
 *    <li>  http://archive.ncsa.uiuc.edu/VEG/VPS/emtc/quaternions/index.html
 *    </ul>
 * <li>   Or search on google....
 * </ul>
 * @ingroup Types
 */
template <typename DATA_TYPE>
class Quat
{
public:
   /** use this to declare single value types of the same type as this matrix.
    */
   typedef DATA_TYPE DataType;

   enum Params { Size = 4 };

   /** default constructor, initializes to quaternion multiplication identity
    *  [x,y,z,w] == [0,0,0,1].
    *  NOTE: the addition identity is [0,0,0,0]
    */
   Quat()
      : mData( (DATA_TYPE)0.0, (DATA_TYPE)0.0, (DATA_TYPE)0.0, (DATA_TYPE)1.0 )
   {
   }
   
   /** data constructor, initializes to quaternion multiplication identity
    *  [x,y,z,w] == [0,0,0,1].
    *  NOTE: the addition identity is [0,0,0,0]
    */
   Quat( const DATA_TYPE& x, const DATA_TYPE& y, const DATA_TYPE& z,
         const DATA_TYPE& w )
      : mData( x, y, z, w )
   {
   }

   /** copy constructor
    */
   Quat( const Quat<DATA_TYPE>& q ) : mData( q.mData )
   {
   }

   /** directly set the quaternion's values
    *  @pre x,y,z,w should be normalized
    *  @post the quaternion is set with the given values
    */
   void set( const DATA_TYPE x, const DATA_TYPE y, const DATA_TYPE z, const DATA_TYPE w )
   {
      mData.set( x, y, z, w );
   }

   /** get the raw data elements of the quaternion.
    *  @post sets the given variables to the quaternion's x, y, z, and w values
    */
   void get( DATA_TYPE& x, DATA_TYPE& y, DATA_TYPE& z, DATA_TYPE& w ) const
   {
      x = mData[Xelt];
      y = mData[Yelt];
      z = mData[Zelt];
      w = mData[Welt];
   }

   /** bracket operator.
    *  raw data accessor.
    *
    * <h3> "Example (access raw data element in a Quat):" </h3>
    * \code
    *    Quatf q;
    *    q[Xelt] = 0.001231176f;
    *    q[Yelt] = 0.1222f;
    *    q[Zelt] = 0.721f;
    *    q[Welt] = 0.982323f;
    * \endcode
    *
    * @see VectorIndex
    */
   DATA_TYPE& operator[]( const int x )
   {
      gmtlASSERT( x >= 0 && x < 4 && "out of bounds error" );
      return mData[x];
   }

   /** bracket operator  (const version).
    * raw data accessor.
    *
    * <h3> "Example (access raw data element in a Quat):" </h3>
    * \code
    *    Quatf q;
    *    float rads = acos( q[Welt] ) / 2.0f;
    * \endcode
    *
    * @see VectorIndex
    */
   const DATA_TYPE& operator[]( const int x ) const
   {
      gmtlASSERT( x >= 0 && x < 4 && "out of bounds error" );
      return mData[x];
   }

   /** Get a DATA_TYPE pointer to the quat internal data.
    * @post Returns a ptr to the head of the quat data
    */
   const DATA_TYPE*  getData() const { return (DATA_TYPE*)mData.getData();}

public:
   // Order x, y, z, w
   Vec<DATA_TYPE, 4> mData;
};

const Quat<float> QUAT_MULT_IDENTITYF( 0.0f, 0.0f, 0.0f, 1.0f );
const Quat<float> QUAT_ADD_IDENTITYF( 0.0f, 0.0f, 0.0f, 0.0f );
const Quat<float> QUAT_IDENTITYF( QUAT_MULT_IDENTITYF );
const Quat<double> QUAT_MULT_IDENTITYD( 0.0, 0.0, 0.0, 1.0 );
const Quat<double> QUAT_ADD_IDENTITYD( 0.0, 0.0, 0.0, 0.0 );
const Quat<double> QUAT_IDENTITYD( QUAT_MULT_IDENTITYD );

typedef Quat<float> Quatf;
typedef Quat<double> Quatd;

} // end of namespace gmtl

#endif
