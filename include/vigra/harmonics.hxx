/************************************************************************/
/*                                                                      */
/*        Copyright 2009-2010 by Ullrich Koethe and Janis Fehr          */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    The VIGRA Website is                                              */
/*        http://hci.iwr.uni-heidelberg.de/vigra/                       */
/*    Please direct questions, bug reports, and contributions to        */
/*        ullrich.koethe@iwr.uni-heidelberg.de    or                    */
/*        vigra@informatik.uni-hamburg.de                               */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/

#ifndef VIGRA_HARMONICS_HXX
#define VIGRA_HARMONICS_HXX

#include <complex>
#include "config.hxx"
#include "error.hxx"
#include "utilities.hxx"
#include "mathutil.hxx"
#include "array_vector.hxx"
#include "matrix.hxx"
#include "tinyvector.hxx"
#include "quaternion.hxx"
#include "wigner-matrix.hxx"
#include "clebsch-gordan.hxx"
#include "multi_fft.hxx"
#include "bessel.hxx"


namespace vigra {

namespace detail  {

// computes the normalization for SH base functions
inline double realSH(double l, double m)
{
    return std::sqrt((2.0*l + 1.0) / (4.0*M_PI*facLM(l,m)));

}

template <typename REAL>
inline REAL fac(REAL in)
{   
    REAL temp = 1;
    for (int i=2;i<=in;i++)
    {   
        temp *= i;
    }     
    return temp;
}           


template <typename REAL, typename T> 
TinyVector<REAL, 3> centerOfBB(const vigra::MultiArray<3,T>  &A) 
{
    return TinyVector<REAL, 3>(A.shape()) /= 2.0;                        
}


template <typename REAL>
void eulerAngles(
                REAL sphereRadius_um,
                vigra::MultiArray<3,REAL>& phi,
                vigra::MultiArray<3,REAL>& theta,
                vigra::MultiArray<3,REAL>& psi,
                REAL gaussWidthAtHalfMaximum_um,
		vigra::TinyVector<REAL,3> voxelSize=vigra::TinyVector<REAL,3>(1.0))
{

        REAL radiusLev = sphereRadius_um /voxelSize[0] + gaussWidthAtHalfMaximum_um
*3;
        REAL radiusRow = sphereRadius_um /voxelSize[1] + gaussWidthAtHalfMaximum_um
*3;
        REAL radiusCol = sphereRadius_um /voxelSize[2] + gaussWidthAtHalfMaximum_um
*3;

        int intRadiusLev = (int)std::ceil( radiusLev);
        int intRadiusRow = (int)std::ceil( radiusRow);
        int intRadiusCol = (int)std::ceil( radiusCol);

        vigra::MultiArrayShape<3>::type NewShape( intRadiusLev*2 + 1, intRadiusRow*2 + 1, intRadiusCol*2 + 1 );

        vigra::TinyVector<int,3> M;
        M[0]= NewShape[0] / 2;
        M[1]= NewShape[1] / 2;
        M[2]= NewShape[2] / 2;

        phi.reshape( NewShape,0 );
        theta.reshape( NewShape,0 );
        psi.reshape( NewShape,0 );


        for (int z=0;z<NewShape[0];++z)
        for (int y=0;y<NewShape[1];++y)
        for (int x=0;x<NewShape[2];++x)
        {
                REAL X = (REAL)x *voxelSize[2];
                REAL Y = (REAL)y *voxelSize[1];
                REAL Z = (REAL)z *voxelSize[0];
                int   iZ = z;

                // calculate psi
                phi(z,y,x ) = atan2( Y, X );

                // calculate theta
                REAL r = sqrt( X*X + Y*Y + Z*Z );
                REAL alpha = asinf( Z / r );

                if( iZ == 0 )
                {
                        theta( z,y,x ) = M_PI * 0.5;
                }
                else
                {
                        theta( z,y,x ) = M_PI * 0.5 - alpha;
                }
        }//for
}

} // namespace detail

template <typename REAL>
vigra::MultiArray<3,REAL>
binarySphereREAL( REAL radius_um, REAL gaussWidthAtHalfMaximum_um, vigra::TinyVector<REAL,3> voxelSize=vigra::TinyVector<REAL,3>(1.0))
{
  REAL kernelRadius_um = radius_um;// + gaussWidthAtHalfMaximum_um*3;
  REAL radiusLev = kernelRadius_um /voxelSize[0] + gaussWidthAtHalfMaximum_um*3 ;
  REAL radiusRow = kernelRadius_um /voxelSize[1] + gaussWidthAtHalfMaximum_um*3;
  REAL radiusCol = kernelRadius_um /voxelSize[2] + gaussWidthAtHalfMaximum_um*3;

  int intRadiusLev = (int)std::ceil( radiusLev);
  int intRadiusRow = (int)std::ceil( radiusRow);
  int intRadiusCol = (int)std::ceil( radiusCol);

  vigra::MultiArrayShape<3>::type outshape(intRadiusLev*2 + 1, intRadiusRow*2 + 1, intRadiusCol*2 + 1);
  vigra::MultiArray<3,REAL> output( outshape);


  for( int m = 0; m < outshape[0]; ++m)
  {
    REAL z_um = (m - intRadiusLev) *voxelSize[0];
    REAL sqr_z_um = z_um * z_um;
    for( int r = 0; r < outshape[1]; ++r)
    {
      REAL y_um = (r - intRadiusRow) *voxelSize[1];
      REAL sqr_y_um = y_um * y_um;
      for( int c = 0; c < outshape[2]; ++c)
      {
        REAL x_um = (c - intRadiusCol) *voxelSize[2];
        REAL sqr_x_um = x_um * x_um;
        REAL dist_um = sqrt( sqr_z_um + sqr_y_um + sqr_x_um);

        if( fabs(dist_um - radius_um) <voxelSize[2]/2)
        {
          output(m,r,c) = 1;
        }
        else
        {
          output(m,r,c) = 0;
        }
      }
    }
  }
  return output;
}

/** ------------------------------------------------------
        sphereSurfHarmonic
----------------------------------------------------------
\brief computes a Spherical harmonic base function

\pqrqm out returns 3D SH base function
\param sphereRadius_um radius of the base function
\param gaussWidthAtHalfMaximum_um gaussian smothing of the spherical surface
\param l expansion band, l =[0,l_max]
\param m expansion sub-band, m=[-l,l]
\param full bool, if true, the volume of the sphere is filled, otherwise only surface function  
\param voxelsize optional parameter used to compute base functions for non-equdistant volume samplings
*/

template <typename REAL>
void sphereSurfHarmonic( vigra::MultiArray<3,FFTWComplex<REAL> >& output, REAL sphereRadius_um, REAL gaussWidthAtHalfMaximum_um, int l, int m, bool full, vigra::TinyVector<REAL,3> voxelSize=vigra::TinyVector<REAL,3>(1.0))
{
  if (gaussWidthAtHalfMaximum_um <=1) gaussWidthAtHalfMaximum_um =1;

  REAL radiusLev = sphereRadius_um /voxelSize[0];
  REAL radiusRow = sphereRadius_um /voxelSize[1];
  REAL radiusCol = sphereRadius_um /voxelSize[2];

  radiusLev += gaussWidthAtHalfMaximum_um*3;
  radiusRow += gaussWidthAtHalfMaximum_um*3;
  radiusCol += gaussWidthAtHalfMaximum_um*3;

  int intRadiusLev = (int)std::ceil( radiusLev);
  int intRadiusRow = (int)std::ceil( radiusRow);
  int intRadiusCol = (int)std::ceil( radiusCol);

  vigra::MultiArrayShape<3>::type outshape(intRadiusLev*2 + 1,intRadiusRow*2 + 1, intRadiusCol*2 + 1);
  output.reshape( outshape, 0);

  REAL sigmaFactor = -2*log(0.5) / (gaussWidthAtHalfMaximum_um * gaussWidthAtHalfMaximum_um);
  for( int s = 0; s < outshape[0]; ++s)
  {
    REAL z_um = (s - intRadiusLev) *voxelSize[0];
    REAL sqr_z_um = z_um * z_um;
    for( int r = 0; r < outshape[1]; ++r)
    {
      REAL y_um = (r - intRadiusRow) *voxelSize[1];
      REAL sqr_y_um = y_um * y_um;
      for( int c = 0; c < outshape[2]; ++c)
      {
        REAL x_um = (c - intRadiusCol) *voxelSize[2];
        REAL sqr_x_um = x_um * x_um;
        REAL dist_um = sqrt( sqr_z_um + sqr_y_um + sqr_x_um);
        REAL gauss_x = 0;
        if (!full||(dist_um > sphereRadius_um))
            gauss_x=(dist_um - sphereRadius_um);
        else
            gauss_x=1;
        if( x_um*x_um+y_um*y_um == 0)
            y_um+=0.00001;//avoid nans
        REAL theta;
        REAL temp = z_um/sqrt( (REAL) (x_um*x_um+y_um*y_um+z_um*z_um));
        if (temp == 1) temp-=0.00000001; //avoid nans
        if (temp == -1) temp += 0.00000001;
        theta = acos(temp);

        REAL phi;
        if (y_um>=0)
        {
            REAL temp = x_um/sqrt( (REAL)(x_um*x_um+y_um*y_um));
            if (temp == 1) temp-=0.00000001; //avoid nans
            if (temp == -1) temp += 0.00000001;
            phi = acos(temp);
        }
        else
        {
            REAL temp = x_um/sqrt( (REAL)(x_um*x_um+y_um*y_um));
            if (temp == 1) temp-=0.00000001;
            if (temp == -1) temp += 0.00000001;
            phi = 2*M_PI - acos(temp);
        }

        FFTWComplex<REAL> SHfactor;
        SHfactor.real()= vigra::detail::realSH(l,m)*vigra::legendre(l,m,cos(theta)) * cos(m * phi);
        SHfactor.imag()= vigra::detail::realSH(l,m)*vigra::legendre(l,m,cos(theta)) * sin(m * phi);
        output(s,r,c) = ((REAL)exp( -0.5 * gauss_x * gauss_x * sigmaFactor)) * SHfactor;

     }
    }
  }

}

template <typename REAL>
void sphereVecHarmonic  (vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >  & res,REAL radius, REAL gauss, int l, int k, int m)
{
    //std::cerr<<"computing VH: "<<l<<" "<<k<<" "<<m<<"\n";
    //1-m
    vigra::MultiArray<3,FFTWComplex<REAL> > tmpSH;

    InvariantViolation err("");
    try
    {
        if (abs(1-m)>l) throw err;
        FFTWComplex<REAL> cg;
        cg.real() = clebschGordan(l+k, m, l, 1-m, 1, 1);
        cg.imag() = 0;
        sphereSurfHarmonic(tmpSH,radius, gauss, l, 1-m, false);
        typename vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator p = res.begin();
        for (typename vigra::MultiArray<3,FFTWComplex<REAL> >::iterator q = tmpSH.begin();q!=tmpSH.end();++p,++q)
            (*p)[0] = cg * *q;
    }
    catch(InvariantViolation &err) //in case clebsh gordan dilivers invalid combination
    {
        //std::cerr<<"no Z\n";
        for (typename vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator p = res.begin(); p!=res.end();++p)
            (*p)[0] = (REAL)0;
    }
    //-m
    try
    {
        if (abs(-m)>l) throw err;
        FFTWComplex<REAL> cg;
        cg.real() = clebschGordan(l+k, m, l, -m, 1, 0);
        sphereSurfHarmonic(tmpSH,radius, gauss, l, 1-m, false);
        typename vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator p = res.begin();
        for (typename vigra::MultiArray<3,FFTWComplex<REAL> >::iterator q = tmpSH.begin();q!=tmpSH.end();++p,++q)
            (*p)[1] = cg * *q;
    }
    catch(InvariantViolation &err) //in case clebsh gordan dilivers invalid combination
    {
        //std::cerr<<"no Y\n";
        for (typename vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator p = res.begin(); p!=res.end();++p)
            (*p)[1] = (REAL)0;

    }
    //-(1+m)
    try
   {
        if (abs(-(1+m))>l) throw err;
        FFTWComplex<REAL> cg;
        cg.real() = clebschGordan(l+k, m, l, -(1+m), 1, -1);
        cg.imag() = 0;
        sphereSurfHarmonic(tmpSH,radius, gauss, l, -(m+1), false);
        typename vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator p = res.begin();
        for (typename vigra::MultiArray<3,FFTWComplex<REAL> >::iterator q = tmpSH.begin();q!=tmpSH.end();++p,++q)
            (*p)[0] = cg * *q;
    }
    catch(InvariantViolation &err) //in case clebsh gordan dilivers invalid combination
    {
        //std::cerr<<"no X\n";
        for (typename vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator p = res.begin(); p!=res.end();++p)
            (*p)[2] = (REAL)0;
    }


}

template <typename REAL>
inline REAL bessel_zero_Jnu(unsigned int l, unsigned int n)
{
    InvariantViolation err("max implemented band is 10");
    if (l>10 or n>10)
	throw err;
    //cache of the first 10 bessel zero points (max l=10)
    REAL cache[110] = {0,0,0,0,0,0,0,0,0,0,2.4048255576957729, 3.8317059702075125, 5.1356223018406828, 6.3801618959239841, 7.5883424345038035, 8.7714838159599537, 9.9361095242176845, 11.086370019245084, 12.225092264004656, 13.354300477435331, 5.5200781102863106, 7.0155866698156188, 8.4172441403998643, 9.7610231299816697, 11.064709488501185, 12.338604197466944, 13.589290170541217, 14.821268727013171, 16.03777419088771, 17.241220382489129, 8.6537279129110125, 10.173468135062722, 11.61984117214906, 13.015200721698434, 14.37253667161759, 15.700174079711671, 17.003819667816014, 18.287582832481728, 19.554536430997054, 20.807047789264107, 11.791534439014281, 13.323691936314223, 14.795951782351262, 16.223466160318768, 17.615966049804832, 18.98013387517992, 20.320789213566506, 21.6415410198484, 22.945173131874618, 24.233885257750551, 14.930917708487787, 16.470630050877634, 17.959819494987826, 19.409415226435012, 20.826932956962388, 22.217799896561267, 23.586084435581391, 24.934927887673023, 26.266814641176644, 27.583748963573008, 18.071063967910924, 19.615858510468243, 21.116997053021844, 22.582729593104443, 24.01901952477111, 25.430341154222702, 26.820151983411403, 28.1911884594832, 29.54565967099855, 30.885378967696674, 21.211636629879258, 22.760084380592772, 24.270112313573105, 25.748166699294977, 27.19908776598125, 28.626618307291139, 30.033722386570467, 31.422794192265581, 32.795800037341465, 34.154377923855094, 24.352471530749302, 25.903672087618382, 27.420573549984557, 28.908350780921758, 30.371007667117247, 31.811716724047763, 33.233041762847122, 34.637089352069324, 36.025615063869573, 37.400099977156586, 27.493479132040257, 29.046828534916855, 30.569204495516395, 32.06485240709771, 33.53713771181922, 34.988781294559296, 36.422019668258457, 37.838717382853609, 39.240447995178137, 40.628553718964525, 30.634606468431976, 32.189679910974405, 33.716519509222699, 35.218670738610115, 36.699001128744648, 38.15986856196713, 39.603239416075404, 41.030773691585537, 42.443887743273557, 43.84380142033735};
    return cache[n*10+l];
	
}

template <typename REAL>
void sphereFullHarmonic(vigra::MultiArray<3,FFTWComplex<REAL> >& output, REAL sphereRadius_um, int n, int l, int m, vigra::TinyVector<REAL,3> voxelSize=vigra::TinyVector<REAL,3>(1.0))
{
    REAL radiusLev = sphereRadius_um / voxelSize[0] +3;
    REAL radiusRow = sphereRadius_um / voxelSize[1] +3;
    REAL radiusCol = sphereRadius_um / voxelSize[2] +3;
    int intRadiusLev = (int)std::ceil( radiusLev);
    int intRadiusRow = (int)std::ceil( radiusRow);
    int intRadiusCol = (int)std::ceil( radiusCol);


    //precompute SH parts for l and m
    vigra::MultiArray<3,FFTWComplex<REAL> > SH;
    sphereSurfHarmonic<REAL>(SH,sphereRadius_um, 1, l, m, true);

    vigra::MultiArrayShape<3>::type outshape(intRadiusLev*2 + 1,intRadiusRow*2 + 1, intRadiusCol*2 + 1);
    output.reshape( outshape, 0);

    REAL xnl=sphereRadius_um;
    if(n>0)
    {
            xnl=bessel_zero_Jnu<REAL>(l,n);
    }
    REAL k=xnl/sphereRadius_um;
    REAL J2=pow(besselJ(l+1,xnl),2.0);
    REAL N=(sphereRadius_um*sphereRadius_um*sphereRadius_um)/2*J2;

    REAL sigmaFactor = -2*log(0.5) / (4);
    FFTWComplex<REAL> I(0,1);
    for( int z = 0; z < outshape[0]; ++z)
    {
        REAL z_um = (z - intRadiusLev) * voxelSize[0];
        REAL sqr_z_um = z_um * z_um;
        for( int y = 0; y < outshape[1]; ++y)
        {
            REAL y_um = (y - intRadiusRow) * voxelSize[1];
            REAL sqr_y_um = y_um * y_um;
            for( int x = 0; x < outshape[2]; ++x)
            {
                REAL x_um = (x - intRadiusCol) * voxelSize[2];
                REAL sqr_x_um = x_um * x_um;
                REAL r = sqrt( sqr_z_um+sqr_y_um + sqr_x_um);
                REAL gauss_x = (r - sphereRadius_um);

                FFTWComplex<REAL> Phi = SH(z,y,x);
                REAL J1=besselJ(l,k*r);
                FFTWComplex<REAL> R = 1/sqrt(N)*J1;
                FFTWComplex<REAL> Psi = R*Phi;

                if(r<=sphereRadius_um)
                    output(z,y,x) = Psi;
                else
                    output(z,y,x) = (REAL)exp( -0.5 * gauss_x * gauss_x * sigmaFactor) * Psi;
            }
        }
    }


}

template <typename REAL>
void sphereFullVecHarmonic(vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >& output, REAL sphereRadius_um, int n, int l, int k, int m, vigra::TinyVector<REAL,3> voxelSize=vigra::TinyVector<REAL,3>(1.0))
{
    REAL radiusLev = sphereRadius_um / voxelSize[0] +3;
    REAL radiusRow = sphereRadius_um / voxelSize[1] +3;
    REAL radiusCol = sphereRadius_um / voxelSize[2] +3;
    int intRadiusLev = (int)std::ceil( radiusLev);
    int intRadiusRow = (int)std::ceil( radiusRow);
    int intRadiusCol = (int)std::ceil( radiusCol);


    //precompute VH parts for l and m
    vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > > VH;
    sphereVecHarmonic(VH, sphereRadius_um, 1, l, k, m);

    vigra::MultiArrayShape<3>::type outshape(intRadiusLev*2 + 1,intRadiusRow*2 + 1, intRadiusCol*2 + 1);
    vigra::TinyVector<FFTWComplex<REAL>,3 > zero(0,0,0);
    output.reshape( outshape, zero);

    REAL xnl=sphereRadius_um;
    if(n>0)
    {
            xnl=0;//gsl_sf_bessel_zero_Jnu(l,n);
            //std::cerr<<n<<" "<<m<<" "<<xnm<<"\n";
    }
    REAL K=xnl/sphereRadius_um;
    REAL J2=pow(besselJ(l+1,xnl),2.0);
    REAL N=(sphereRadius_um*sphereRadius_um*sphereRadius_um)/2*J2;

    REAL sigmaFactor = -2*log(0.5) / (4);
    FFTWComplex<REAL> I(0,1);
    for( int z = 0; z < outshape[0]; ++z)
    {
        REAL z_um = (z - intRadiusLev) * voxelSize[0];
        REAL sqr_z_um = z_um * z_um;
        for( int y = 0; y < outshape[1]; ++y)
        {
            REAL y_um = (y - intRadiusRow) * voxelSize[1];
            REAL sqr_y_um = y_um * y_um;
            for( int x = 0; x < outshape[2]; ++x)
            {
                REAL x_um = (x - intRadiusCol) * voxelSize[2];
                REAL sqr_x_um = x_um * x_um;
                REAL r = sqrt( sqr_z_um+sqr_y_um + sqr_x_um);
                REAL gauss_x = (r - sphereRadius_um);
                vigra::TinyVector<FFTWComplex<REAL>,3> Phi = VH(z,y,x);
                REAL J1=besselJ(l,K*r);
                FFTWComplex<REAL> R = 1/sqrt(N)*J1;
                vigra::TinyVector<FFTWComplex<REAL>,3> Psi;
                Psi[0]= R*Phi[0];
                Psi[1]= R*Phi[1];
                Psi[2]= R*Phi[2];

                if(r<=sphereRadius_um)
                    output(z,y,x) = Psi;
                else
                {
                    Psi[0]*=((FFTWComplex<REAL>)exp( -0.5 * gauss_x * gauss_x * sigmaFactor));
                    Psi[1]*=((FFTWComplex<REAL>)exp( -0.5 * gauss_x * gauss_x * sigmaFactor));
                    Psi[2]*=((FFTWComplex<REAL>)exp( -0.5 * gauss_x * gauss_x * sigmaFactor));
                    output(z,y,x) =  Psi;
                }
            }
        }
    }


}

/** ------------------------------------------------------
        computeSHbaseF
----------------------------------------------------------
\brief pre-computes Spherical harmonic base functions

\param radius radius of the spherical expansion
\param gauss smothing of the spherical surface
\param band maximum expansion band
\param SHbaseF holds precomputed SH base functions  
*/
template <typename REAL>
void computeSHbaseF(REAL radius, REAL gauss, unsigned int band, std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > &SHbaseF)
{
        SHbaseF.resize(band + 1);

        for (int l = 0; l <= band; l++)
        {
                SHbaseF[l].resize(2*l + 1);
                for (int m = -l; m <= l; m++)
                {
                        vigra::MultiArray<3, FFTWComplex<REAL> > Coeff;
                        sphereSurfHarmonic(Coeff,radius, gauss, l, m, false);
			SHbaseF[l][m+l].reshape(Coeff.shape());
                        SHbaseF[l][m+l]=Coeff;
                }
        }
}

template <typename REAL>
void computePHbaseF(REAL radius, unsigned int band, std::vector<std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > > &PHbaseF, bool realdata=false)
{
        PHbaseF.resize(band + 1);
        //n=0 undefined
        int mMin;
        int mOff;
	//for real data one only needs posstive coeefs (symmetry)
        for (int n = 1; n <= band; n++)
        {
            PHbaseF[n].resize(band + 1);
            for (int l = 0; l <= band; l++)
            {
                if (realdata)
		{
		    mMin = 0;
		    mOff = 0;
                    PHbaseF[n][l].resize(l + 1);
		}
                else
		{
		    mMin = -l;
		    mOff = l;
                    PHbaseF[n][l].resize(2*l + 1);
		}
                for (int m = mMin; m <= l; m++)
                {
			//std::cerr<<n<<" "<<l<<" "<<m+mOff<<"\n";
                        vigra::MultiArray< 3,FFTWComplex<REAL> > Coeff;
                        sphereFullHarmonic(Coeff, radius, n,l,m);
                        PHbaseF[n][l][m+mOff].reshape(Coeff.shape());
                        PHbaseF[n][l][m+mOff]=Coeff;
                }
            }
        }

}

template <typename REAL>
void computeVHbaseF(REAL radius, REAL gauss, unsigned int band, std::vector<std::vector<std::vector<vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3> > > > > &VHbaseF)
{
        VHbaseF.resize(band + 1);

        for (int l = 0; l <= band; l++)
        {
            VHbaseF[l].resize(3);
            for (int k=-1;k<=1;k++)
            {
                VHbaseF[l][k+1].resize(2*band + 1);
                for (int m = -l; m <= l; m++)
                {
                        vigra::MultiArray<3, vigra::TinyVector< FFTWComplex<REAL> ,3 > > Coeff;

                        sphereVecHarmonic(Coeff,radius, gauss, l, k, m);
                        VHbaseF[l][k+1][m+l].reshape(Coeff.shape());
                        VHbaseF[l][k+1][m+l]=Coeff;
                }
            }
        }
}

template <typename REAL>
void computeVPHbaseF(REAL radius, unsigned int band, std::vector<std::vector<std::vector<std::vector<vigra::MultiArray<3, vigra::TinyVector< FFTWComplex<REAL>,3 > > > > > >&VHbaseF)
{
        VHbaseF.resize(band + 1);
        for(int n=0; n>= band; n++)
        {
            VHbaseF[n].resize(band + 1);

            for (int l = 0; l <= band; l++)
            {
                VHbaseF[n][l].resize(3);
                for (int k=-1;k<=1;k++)
                {
                    VHbaseF[l][k+1].resize(2*band + 1);
                    for (int m = -l; m <= l; m++)
                    {
                        vigra::MultiArray<3, vigra::TinyVector< FFTWComplex<REAL> ,3 > > Coeff;

                        sphereFullVecHarmonic(Coeff,radius, n, l, k, m);
                        VHbaseF[n][l][k+1][m+l].reshape(Coeff.shape());
                        VHbaseF[n][l][k+1][m+l]=Coeff;
                    }
                }
            }
        }
}


template <typename REAL>
vigra::MultiArray<3,REAL> sphereSurfGauss( REAL sphereRadius_um, REAL gaussWidthAtHalfMaximum_um, vigra::TinyVector<REAL,3> voxelSize=vigra::TinyVector<REAL,3>(1.0))
{
  REAL kernelRadius_um = sphereRadius_um;;
  REAL radiusLev = kernelRadius_um /voxelSize[0] + gaussWidthAtHalfMaximum_um*3;
  REAL radiusRow = kernelRadius_um /voxelSize[1] + gaussWidthAtHalfMaximum_um*3;
  REAL radiusCol = kernelRadius_um /voxelSize[2] + gaussWidthAtHalfMaximum_um*3;

  int intRadiusLev = (int)std::ceil( radiusLev);
  int intRadiusRow = (int)std::ceil( radiusRow);
  int intRadiusCol = (int)std::ceil( radiusCol);

  vigra::MultiArrayShape<3>::type outShape(intRadiusLev*2 + 1, intRadiusRow*2 + 1, intRadiusCol*2 + 1);
  vigra::MultiArray<3,REAL> output( outShape );

  REAL sigmaFactor = -2*log(0.5) / (gaussWidthAtHalfMaximum_um * gaussWidthAtHalfMaximum_um);

  for( int m = 0; m < outShape[0]; ++m)
  {
    REAL z_um = (m - intRadiusLev) *voxelSize[0];
    REAL sqr_z_um = z_um * z_um;
    for( int r = 0; r < outShape[1]; ++r)
    {
      REAL y_um = (r - intRadiusRow) *voxelSize[1];
      REAL sqr_y_um = y_um * y_um;
      for( int c = 0; c < outShape[2]; ++c)
      {
        REAL x_um = (c - intRadiusCol) *voxelSize[2];
        REAL sqr_x_um = x_um * x_um;
        REAL dist_um = sqrt( sqr_z_um + sqr_y_um + sqr_x_um);
        REAL gauss_x = (dist_um - sphereRadius_um);
        output(m,r,c) = exp( -0.5 * gauss_x * gauss_x * sigmaFactor);
      }
    }
  }

  REAL kernelSum = 0;
  for (typename vigra::MultiArray<3,REAL>::iterator p=output.begin();p!=output.end();++p)
    kernelSum+=*p;
  output *= 1.0/kernelSum;

  return output;
}





template <typename REAL>
void reconstSH(REAL radius, REAL gauss, unsigned int band, vigra::MultiArray<3,REAL >& reconstruct, const std::vector<std::vector<FFTWComplex<REAL> >  >& SH_A, const std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > >& SHbaseF)
{
	//FIXME reconstSH currently only works for real data
        reconstruct.reshape(SHbaseF[0][0].shape());

        vigra::MultiArray<3,FFTWComplex<REAL> > T;
        std::string name;
        for (int l=0;l<=band;l++)
        {
                for (int m=-l;m<=l;m++)
                {
                    typename vigra::MultiArray<3,FFTWComplex<REAL> >::iterator p=SHbaseF[l][l+m].begin();
                    for (typename vigra::MultiArray<3,REAL >::iterator q=reconstruct.begin();q!=reconstruct.end();++p,++q)
                        *q += real(*p * conj(SH_A[l][l+m]));
                }
        }
}

template <typename REAL>
void reconstPH(REAL radius, unsigned int band, vigra::MultiArray<3, REAL >& reconstruct, const std::vector<std::vector<std::vector<FFTWComplex<REAL> >  > >& PH_A, std::vector<std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > > &PHbaseF)
{
        //FIXME reconstPH currently only works for real data
        reconstruct.reshape(PHbaseF[1][0][0].shape(), 0);

        for (int n=1;n<PH_A.size();n++)
        {
            for (int l=0;l<PH_A[n].size();l++)
            {
                for (int m=0;m<PH_A[n][l].size();m++)
                {
                        typename vigra::MultiArray<3,FFTWComplex<REAL> >::iterator p = PHbaseF[n][l][m].begin();
                        for (typename vigra::MultiArray<3, REAL >::iterator q=reconstruct.begin();q!=reconstruct.end();++q,++p)
                            *q+=real(*p * conj(PH_A[n][l][m]));

                }
            }
        }

}

template <typename REAL>
void reconstVPH(REAL radius, unsigned int band, vigra::MultiArray<3, vigra::TinyVector<REAL,3> >& reconstruct, const std::vector<std::vector<std::vector<std::vector<FFTWComplex<REAL> >  > > >& VPH_A)
{
        vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > > base_tmp;
        sphereVecHarmonic(base_tmp, radius, 1, 0, 0, 0);
        reconstruct.reshape(base_tmp.shape());
        vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3> > tmp(reconstruct.shape(), 0);
        FFTWComplex<REAL> zero(0,0);
        vigra::TinyVector<FFTWComplex<REAL>,3> Zero(zero,zero,zero);

        for (int n=1;n<=band;n++)
        for (int l=0;l<=band;l++)
            for (int k=-1;k<=1;++k)
                for (int m=-(l+k);m<=(l+k);m++)
                {
                    sphereVecHarmonic(base_tmp, radius, n, l, k, m);
                    typename vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3> >::iterator p=tmp.begin();
                    for (typename vigra::MultiArray< 3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator q=base_tmp.begin();q!=base_tmp.end();++p,++q)
                    {
                        (*p)[0] += conj(VPH_A[n][l][k+1][(l+k)+m]) * (*q)[0];
                        (*p)[1] += conj(VPH_A[n][l][k+1][(l+k)+m]) * (*q)[1];
                        (*p)[2] += conj(VPH_A[n][l][k+1][(l+k)+m]) * (*q)[2];
                    }

                }
        //reconst real vec directions
        typename vigra::MultiArray<3, vigra::TinyVector<REAL,3> >::iterator p=reconstruct.begin();
        for (typename vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3> >::iterator q=tmp.begin();q!=tmp.end();++p,++q)
        {
            (*p)[0] = real((*q)[1]);
            (*p)[1] = -sqrt(2)*real((*q)[0]);
            (*p)[2] = sqrt(2)*imag((*q)[0]);
        }

}

template <typename REAL>
void reconstVH(REAL radius, REAL gauss, unsigned int band, vigra::MultiArray<3, vigra::TinyVector<REAL,3> >& reconstruct,
                const std::vector<std::vector<std::vector<FFTWComplex<REAL> >  > >& VH_A)
{

        vigra::MultiArray< 3, vigra::TinyVector<FFTWComplex<REAL>,3 > > base_tmp;
        sphereVecHarmonic(base_tmp, radius, gauss, 0, 0, 0);
        reconstruct.reshape(base_tmp.shape());
        FFTWComplex<REAL> zero(0,0);
        vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3> > tmp(reconstruct.shape(), 0);
        vigra::TinyVector<FFTWComplex<REAL>,3> Zero(zero,zero,zero);

        for (int l=0;l<=band;l++)
            for (int k=-1;k<=1;++k)
                for (int m=-(l+k);m<=(l+k);m++)
                {
                    sphereVecHarmonic(base_tmp, radius, gauss, l, k, m);
                    typename vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3> >::iterator p=tmp.begin();
                    for (typename vigra::MultiArray< 3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator q=base_tmp.begin();q!=base_tmp.end();++p,++q)
                    {
                        (*p)[0] += conj(VH_A[l][k+1][(l+k)+m]) * (*q)[0];
                        (*p)[1] += conj(VH_A[l][k+1][(l+k)+m]) * (*q)[1];
                        (*p)[2] += conj(VH_A[l][k+1][(l+k)+m]) * (*q)[2];
                    }
                }

        //reconst real vec directions
        typename vigra::MultiArray<3, vigra::TinyVector<REAL,3> >::iterator p=reconstruct.begin();
        for (typename vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3> >::iterator q=tmp.begin();q!=tmp.end();++p,++q)
        {
            (*p)[0] = real((*q)[1]);
            (*p)[1] = -sqrt(2)*real((*q)[0]);
            (*p)[2] = sqrt(2)*imag((*q)[0]);
        }
}

/** ------------------------------------------------------
	SHpos
----------------------------------------------------------
\brief computes singe local Spherical harmonic expansion at given 3D position

\param SH_A holds the returned SH coefficients  
\param radius radius of the spherical expansion
\param band maximum expansion band
\param A input volume data
\param pos 3D position of the SH expansion
\param SHbaseF precomputed SH base functions (-> see computeSHbaseF) 
*/
template <typename REAL>
void SHpos(std::vector<std::vector<FFTWComplex<REAL> >  > &SH_A, REAL radius, unsigned int band,
                const vigra::MultiArray<3,REAL> &A, const vigra::TinyVector<REAL, 3> &pos, std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > const &SHbaseF)
{
        SH_A.resize(SHbaseF.size());

        // SH coeffs
        for (int l = 0; l <= band; l++)
        {
                SH_A[l].resize(SHbaseF[l].size());
                for (int m = 0; m < SHbaseF[l].size(); m++)
                {
                        vigra::MultiArrayShape<3>::type coffShape(SHbaseF[l][m].shape());

                        int xa = (int) floor(pos[2] - coffShape[2] / 2);
                        int xe = xa + coffShape[2] - 1;
                        int ya = (int) floor(pos[1] - coffShape[1] / 2);
                        int ye = ya + coffShape[1] - 1;
                        int za = (int) floor(pos[0] - coffShape[0] / 2);
                        int ze = za + coffShape[0] - 1;

                        SH_A[l][m] = (REAL)0;
                        int sz=0;
                        for (int z=ze;z>=za;z--,sz++)
                        {
                            int sy=0;
                            for (int y=ye;y>=ya;y--,sy++)
                            {
                                int sx=0;
                                for (int x=xe;x>=xa;x--,sx++)
                                    SH_A[l][m]+=A(z,y,x)*(SHbaseF[l][m](sz,sy,sx));
                            }
                        }
                }
        }
}

/** ------------------------------------------------------
        SHcenter
----------------------------------------------------------
\brief computes singe local Spherical harmonic expansion at the center of te given 3D volume

\param SH_A holds the returned SH coefficients  
\param radius radius of the spherical expansion
\param band maximum expansion band
\param A input volume data
\param SHbaseF precomputed SH base functions (-> see computeSHbaseF) 
*/

template <typename REAL>
void SHcenter(std::vector<std::vector<FFTWComplex<REAL> >  > &SH_A, REAL radius, unsigned int band,
                const vigra::MultiArray<3,REAL> &A, std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > const &SHbaseF)
{
    SHpos(SH_A, radius, band, A,  vigra::detail::centerOfBB<REAL>(A), SHbaseF);
}

/** ------------------------------------------------------
        SH_Itterator
---------------------------------------------------------- 
\brief itterator class to access the cascaded vector representations of SH base functions and SH coefficients
*/
template <typename T>
class SH_Itterator
{
    public:
	typedef vigra::MultiArray<3,T >* pointer;
	typedef vigra::MultiArray<3,T >& reference;
	typedef vigra::MultiArray<3,T > value_type;
	typedef int difference_type;
	typedef std::random_access_iterator_tag iterator_category;

	SH_Itterator(std::vector<std::vector<vigra::MultiArray<3,T > > >& data, int l, int m,std::string name="name")
	    :_data(data),_name(name)
	{
	    _l=l;
	    _m=m;
	}
	void operator++()
	{
	    if (_m<_data[_l].size()-1)
	    {
		//std::cerr<<_name<<"++ "<<_m<<" "<<_data[_l].size()<<"\n"<<std::flush;
		_m++;
	    }
	    else
	    {
		if (_l<_data.size())
		{
		    _m=0;
		    _l++;
		}
	    }
	}
	vigra::MultiArray<3,T >& operator*() const
	{
	    return _data[_l][_m];
	}

	const vigra::MultiArray<3,T >* operator->() const
	{
	    //std::cerr<<_name<<" "<<_l<<" "<<_m<<" "<<_data[_l].size()<<" "<<_data[_l][_m].shape()<<"\n";
	    return &_data[_l][_m];
	}

	int getL()
	{
	    return _l;
	}
	int getM()
	{
	    return _m;
	}
	bool operator==(SH_Itterator& A)
	{
	    return ((this->_l==A.getL())&&(this->_m==A.getM()));
	}
	bool operator!=(SH_Itterator& A)
	{
	    return !((this->_l==A.getL())&&(this->_m==A.getM()));
	}

    private:
	std::vector<std::vector<vigra::MultiArray<3,T > > > &_data;
	int _l;
	int _m;
	std::string _name;
};

template <typename REAL>
class PH_Itterator
{
    public:
        typedef vigra::MultiArray<3,FFTWComplex<REAL> >* pointer;
        typedef vigra::MultiArray<3,FFTWComplex<REAL> >& reference;
        typedef vigra::MultiArray<3,FFTWComplex<REAL> > value_type;
        typedef int difference_type;
        typedef std::random_access_iterator_tag iterator_category;

        PH_Itterator(std::vector<std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > >& data, int k, int l, int m)
            : _data(data)
        {
            _k=k;
            _l=l;
            _m=m;
        }
        void operator++()
        {
            if (_m<_data[_k][_l].size()-1)
            {
                _m++;
            }
            else
            {
                if (_l<_data[_k].size()-1)
                {
                    _m=0;
                    _l++;
                }
                else
                {
                    if (_k<_data.size())
                    {
                        _l=0;
                        _k++;
                    }
                }

            }
        }
        vigra::MultiArray<3,FFTWComplex<REAL> >& operator*() const
        {
            return _data[_k][_l][_m];
        }

        const vigra::MultiArray<3,FFTWComplex<REAL> >* operator->() const
        {
            return &_data[_k][_l][_m];
        }
	int getK()
	{
	    return _k;
	}
        int getL()
        {
            return _l;
        }
        int getM()
        {
            return _m;
        }
        bool operator==(PH_Itterator& A)
        {
            return ((this->_k==A.getL())&&(this->_l==A.getL())&&(this->_m==A.getM()));
        }
        bool operator!=(PH_Itterator& A)
        {
            return !((this->_k==A.getK())&&(this->_l==A.getL())&&(this->_m==A.getM()));
        }
    

    private:
        std::vector<std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > > &_data;
        int _k;
        int _l;
        int _m;
};

/** ------------------------------------------------------
        Array2SH
----------------------------------------------------------
\brief computes  local Spherical harmonic expansion at all positions of the given 3D volume

\param SH_A holds the returned SH coefficients  
\param radius radius of the spherical expansion
\param band maximum expansion band
\param A input volume data
\param SHbaseF precomputed SH base functions (-> see computeSHbaseF) 
*/
template <typename REAL>
void Array2SH(std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> >  > > &SH_A, unsigned int band, REAL radius, std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > &SHbaseF,const vigra::MultiArray<3,REAL> &A)
//FIXME add const iterator and make SHbaseF const
{

    SH_A.resize(SHbaseF.size());
    for (int l=0;l<SHbaseF.size();l++)
    {
                SH_A[l].resize(SHbaseF[l].size());
                for (int m=0;m<SHbaseF[l].size();m++)
                {
			SH_A[l][m].reshape(A.shape(),0);
                }
    }

    SH_Itterator< FFTWComplex<REAL> > SHbaseF_Iter(SHbaseF,0,0,"BaseF");
    SH_Itterator< FFTWComplex<REAL> > SHbaseF_Iter_end(SHbaseF,SHbaseF.size(),0);
    SH_Itterator< FFTWComplex<REAL> > SH_A_Iter(SH_A,0,0,"SH_A");
    convolveFFTComplexMany(A, SHbaseF_Iter, SHbaseF_Iter_end, SH_A_Iter,false);
}

template <typename REAL>
void PHpos(std::vector<std::vector<std::vector<FFTWComplex<REAL> >  > > &PH_A, REAL radius, unsigned int band,
                const vigra::MultiArray<3,FFTWComplex<REAL> > &A, std::vector<std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > > &PHbaseF,const vigra::TinyVector<REAL, 3> &pos)
{
    PH_A.resize(PHbaseF.size());

    for (int n=1;n<PHbaseF.size();n++)
    {
        PH_A[n].resize(PHbaseF[n].size());
        for (int l = 0; l < PHbaseF[n].size(); l++)
        {
                PH_A[n][l].resize(PHbaseF[n][l].size());
                for (int m = 0; m < PHbaseF[n][l].size(); m++)
                {
                        vigra::MultiArrayShape<3>::type coffShape(PHbaseF[n][l][m].shape());
                        int xa = (int) floor(pos[2] - coffShape[2] / 2);
                        int xe = xa + coffShape[2] - 1;
                        int ya = (int) floor(pos[1] - coffShape[1] / 2);
                        int ye = ya + coffShape[1] - 1;
                        int za = (int) floor(pos[0] - coffShape[0] / 2);
                        int ze = za + coffShape[0] - 1;
                        PH_A[n][l][m] = (REAL)0;
                        int sz=0;
                        int sy=0;
                        int sx=0;
                        for (int z=za;z<ze;z++,sz++)
                        {
                            sy=0;
                            for (int y=ya;y<ye;y++,sy++)
                            {
                                sx=0;
                                for (int x=xa;x<xe;x++,sx++)
                                    PH_A[n][l][m]+=A(z,y,x)*PHbaseF[n][l][m](sz,sy,sx);
                            }
                        }
                }
        }
    }
}

template <typename REAL>
void PHcenter(std::vector<std::vector<std::vector<FFTWComplex<REAL> >  > > &PH_A, REAL radius, unsigned int band,
                const vigra::MultiArray<3,FFTWComplex<REAL> > &A, std::vector<std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > > &PHbaseF)
{
    PHpos(PH_A, radius, band, A, PHbaseF, vigra::detail::centerOfBB<REAL>(A));
}

template <typename REAL>
void Array2PH(std::vector<std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> >  > > > &PH_A, unsigned int band, REAL radius, bool realData, const vigra::MultiArray<3,FFTWComplex<REAL> > &A, fftwf_plan forward_plan, fftwf_plan backward_plan)
{
    std::vector<std::vector<std::vector<vigra::MultiArray<3,FFTWComplex<REAL> > > > > PHbaseF;
    computePHbaseF(radius, band, PHbaseF, realData);

    PH_A.resize(band+1);

    for (int n=1;n<=band;n++)
    {
        PH_A[n].resize(band+1);
        for (int l=0;l<=band;l++)
            PH_A[n][l].resize(2*band+1);
    }

    PH_Itterator<REAL> PHbaseF_Iter(PHbaseF,0,0,0);
    PH_Itterator<REAL> PHbaseF_Iter_end(PHbaseF,3,PHbaseF.size(),0);
    PH_Itterator<REAL> PH_A_Iter(PH_A,0,0,0);
    convolveFFTComplexMany(A, PHbaseF_Iter, PHbaseF_Iter_end, PH_A_Iter,false);
} 

/*
template <typename REAL>
void Array2VH(std::vector<std::vector<std::vector<vigra::MultiArray<3, FFTWComplex<REAL> > > > > &VH_A, unsigned int band, REAL gauss, REAL radius, const vigra::MultiArray<3, vigra::TinyVector<REAL,3> > &A)
{
    //transform input to C^(2j+1)
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputZ(A.shape());
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputY(A.shape());
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputX(A.shape());
    vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator z=inputZ.begin();
    vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator y=inputY.begin();
    vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator x=inputX.begin();

    for (vigra::MultiArray<3,vigra::TinyVector<REAL,3> >::const_iterator p=A.begin();p!=A.end();++p,++z,++y,++x)
    {
        z->real() = -(*p)[1];
        z->imag() = -(*p)[2];
        *z *= 1/sqrt(2);
        y->real() =  (*p)[0];
        y->imag() =  0;
        x->real() = (*p)[1];
        x->imag() = -(*p)[2];
        *x *= 1/sqrt(2);
    }
   VH_A.resize(band + 1);
   REAL norm = M_PI/2*pow((REAL)radius,2.0);


    for(int k=-1;k<=1;++k)
    {
        VH_A[0][1+k].resize(3);
        for (int m=-1;m<=1;++m)
        {
            vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > > vh;
            sphereVecHarmonic(vh, radius, gauss, 0, k, m);
            vigra::MultiArray<3, FFTWComplex<REAL> > vhz(vh.shape());
            vigra::MultiArray<3, FFTWComplex<REAL> > vhy(vh.shape());
            vigra::MultiArray<3, FFTWComplex<REAL> > vhx(vh.shape());
            vigra::MultiArray<3, FFTWComplex<REAL> >::iterator z = vhz.begin();
            vigra::MultiArray<3, FFTWComplex<REAL> >::iterator y = vhy.begin();
            vigra::MultiArray<3, FFTWComplex<REAL> >::iterator x = vhx.begin();
            for(vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator p= vh.begin();p!=vh.end();p++,z++,y++,x++)
            {
                *z = (*p)[0];
                *y = (*p)[1];
                *x = (*p)[2];
            }
            VH_A[0][1+k][1+m].reshape(A.shape());
            VH_A[0][1+k][1+m] = convolveComplex(inputZ, vhz);
            VH_A[0][1+k][1+m] += convolveComplex(inputY, vhy);
            VH_A[0][1+k][1+m] += convolveComplex(inputX, vhx);

            for (vigra::MultiArray<3, FFTWComplex<REAL> >::iterator p=VH_A[0][1+k][1+m].begin();p!=VH_A[0][1+k][1+m].end();++p)
                *p*= (complex<REAL>)pow(-1.0,(REAL) 0)/(3*norm);
        }
    }

    for (int l=1;l<=band;l++)
    {
        VH_A[l].resize(3);
        for(int k=-1;k<=1;++k)
        {
            VH_A[l][1+k].resize(2*(l+k)+1);
            for (int m=-(l+k);m<=(l+k);++m)
            {
                vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > > vh;
                sphereVecHarmonic(vh, radius, gauss, 0, k, m);
                vigra::MultiArray<3, FFTWComplex<REAL> > vhz(vh.shape());
                vigra::MultiArray<3, FFTWComplex<REAL> > vhy(vh.shape());
                vigra::MultiArray<3, FFTWComplex<REAL> > vhx(vh.shape());
                vigra::MultiArray<3, FFTWComplex<REAL> >::iterator z = vhz.begin();
                vigra::MultiArray<3, FFTWComplex<REAL> >::iterator y = vhy.begin();
                vigra::MultiArray<3, FFTWComplex<REAL> >::iterator x = vhx.begin();
                for(vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator p= vh.begin();p!=vh.end();p++,z++,y++,x++)
                {
                    *z = (*p)[0];
                    *y = (*p)[1];
                    *x = (*p)[2];
                }

                VH_A[l][1+k][1+m].reshape(A.shape());


                VH_A[l][1+k][1+m] = convolveComplex(inputZ, vhz);
                VH_A[l][1+k][1+m] += convolveComplex(inputY, vhy);
                VH_A[l][1+k][1+m] += convolveComplex(inputX, vhx);

                for (vigra::MultiArray<3, FFTWComplex<REAL> >::iterator p=VH_A[l][1+k][1+m].begin();p!=VH_A[l][1+k][1+m].end();++p)
                    *p*= (complex<REAL>)pow(-1.0,(REAL) 0)/(3*norm);

            }

        }
    }
}

template <typename REAL>
void Array2VPH(std::vector<std::vector<std::vector<std::vector<vigra::MultiArray<3, FFTWComplex<REAL> > > > > >&VH_A, unsigned int band, REAL radius, const vigra::MultiArray<3, vigra::TinyVector<REAL,3> > &A)
{
    //transform input to C^(2j+1)
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputZ(A.shape());
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputY(A.shape());
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputX(A.shape());
    vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator z=inputZ.begin();
    vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator y=inputY.begin();
    vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator x=inputX.begin();

    for (vigra::MultiArray<3,vigra::TinyVector<REAL,3> >::const_iterator p=A.begin();p!=A.end();++p,++z,++y,++x)
    {
        real(*z) = -(*p)[1];
        imag(*z) = -(*p)[2];
        *z *= 1/sqrt(2);
        real(*y) =  (*p)[0];
        imag(*y) =  0;
        real(*x) = (*p)[1];
        imag(*x) = -(*p)[2];
        *x *= 1/sqrt(2);
    }

    for(int n=1;n<=band;n++)
    {
        VH_A.resize(band+1);
        //0th coeff
        VH_A[n][0].resize(3);

        REAL norm = M_PI/2*pow((REAL)radius,2.0);

        for(int k=-1;k<=1;++k)
        {
            VH_A[n][0][1+k].resize(3);
            for (int m=-1;m<=1;++m)
            {
                vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > > vh;
                sphereFullVecHarmonic(vh, radius,  n, 0, k, m);
                vigra::MultiArray<3, FFTWComplex<REAL> > vhz(vh.shape());
                vigra::MultiArray<3, FFTWComplex<REAL> > vhy(vh.shape());
                vigra::MultiArray<3, FFTWComplex<REAL> > vhx(vh.shape());
                vigra::MultiArray<3, FFTWComplex<REAL> >::iterator z = vhz.begin();
                vigra::MultiArray<3, FFTWComplex<REAL> >::iterator y = vhy.begin();
                vigra::MultiArray<3, FFTWComplex<REAL> >::iterator x = vhx.begin();
                for(vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator p= vh.begin();p!=vh.end();p++,z++,y++,x++)
                {
                    *z = (*p)[0];
                    *y = (*p)[1];
                    *x = (*p)[2];
                }
                VH_A[n][0][1+k][1+m].reshape(A.shape());
                VH_A[n][0][1+k][1+m] = convolveComplex(inputZ, vhz);
                VH_A[n][0][1+k][1+m] += convolveComplex(inputY, vhy);
                VH_A[n][0][1+k][1+m] += convolveComplex(inputX, vhx);

                for (vigra::MultiArray<3, FFTWComplex<REAL> >::iterator p=VH_A[n][0][1+k][1+m].begin();p!=VH_A[n][0][1+k][1+m].end();++p)
                    *p*= (complex<REAL>)pow(-1.0,(REAL) 0)/(3*norm);

            }
        }

        for (int l=1;l<=band;l++)
        {
            VH_A[n][l].resize(3);
            for(int k=-1;k<=1;++k)
            {
                VH_A[n][l][1+k].resize(2*(l+k)+1);
                for (int m=-(l+k);m<=(l+k);++m)
                {
                    vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > > vh;
                    sphereFullVecHarmonic(vh, radius,  n, 0, k, m);
                    vigra::MultiArray<3, FFTWComplex<REAL> > vhz(vh.shape());
                    vigra::MultiArray<3, FFTWComplex<REAL> > vhy(vh.shape());
                    vigra::MultiArray<3, FFTWComplex<REAL> > vhx(vh.shape());
                    vigra::MultiArray<3, FFTWComplex<REAL> >::iterator z = vhz.begin();
                    vigra::MultiArray<3, FFTWComplex<REAL> >::iterator y = vhy.begin();
                    vigra::MultiArray<3, FFTWComplex<REAL> >::iterator x = vhx.begin();
                    for(vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > >::iterator p= vh.begin();p!=vh.end();p++,z++,y++,x++)
                    {
                        *z = (*p)[0];
                        *y = (*p)[1];
                        *x = (*p)[2];
                    }

                    VH_A[n][l][1+k][1+m].reshape(A.shape());


                    VH_A[n][l][1+k][1+m] = convolveComplex(inputZ, vhz);
                    VH_A[n][l][1+k][1+m] += convolveComplex(inputY, vhy);
                    VH_A[n][l][1+k][1+m] += convolveComplex(inputX, vhx);

                    for (vigra::MultiArray<3, FFTWComplex<REAL> >::iterator p=VH_A[n][l][1+k][1+m].begin();p!=VH_A[n][l][1+k][1+m].end();++p)
                        *p*= (complex<REAL>)pow(-1.0,(REAL) 0)/(3*norm);


                }

            }
        }
    }
}
*/

template <typename REAL>
void VHpos(std::vector<std::vector<std::vector<FFTWComplex<REAL> > > > &VH_A, unsigned int band, REAL gauss, REAL radius, const vigra::MultiArray<3,vigra::TinyVector<REAL,3> > &A, const vigra::TinyVector<REAL, 3> &pos)
{
    //transform input to C^(2j+1)
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputZ(A.shape());
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputY(A.shape());
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputX(A.shape());
    typename vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator z=inputZ.begin();
    typename vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator y=inputY.begin();
    typename vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator x=inputX.begin();

    for (typename vigra::MultiArray<3,vigra::TinyVector<REAL,3> >::const_iterator p=A.begin();p!=A.end();++p,++z,++y,++x)
    {
        z->real() = -(*p)[1];
        z->imag() = -(*p)[2];
        *z *= 1/sqrt(2);
        y->real() =  (*p)[0];
        y->imag() =  0;
        x->real() = (*p)[1];
        x->imag() = -(*p)[2];
        *x *= 1/sqrt(2);
    }
   VH_A.resize(band + 1);
   REAL norm = M_PI/2*pow((REAL)radius,2.0);

   // SH coeffs
   int xa=0,xe=0,ya=0,ye=0,za=0,ze=0;

   VH_A[0].resize(3);
   for(int k=-1;k<=1;++k)
    {
        VH_A[0][1+k].resize(3);
        for (int m=-1;m<=1;++m)
        {
            vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > > vh;
            sphereVecHarmonic(vh, radius, gauss, 0, k, m);

            xa = (int) floor(pos[2] - vh.shape()[2] / 2);
            xe = xa + vh.shape()[2] - 1;
            ya = (int) floor(pos[1] - vh.shape()[1] / 2);
            ye = ya + vh.shape()[1] - 1;
            za = (int) floor(pos[0] - vh.shape()[0] / 2);
            ze = za + vh.shape()[0] - 1;

            int zs=0;
            int ys=0;
            int xs=0;
            for (int z=za;z!=ze;z++,zs++)
                for (int y=ya;y<=ye;y++,ys++)
                    for (int x=xa;x<=xe;x++,xs++)
                    {
                        VH_A[0][1+k][1+m] += inputZ(z,y,x) * vh(zs,ys,xs)[0]/norm + inputZ(z,y,x) * vh(zs,ys,xs)[1]/norm + inputZ(z,y,x) * vh(zs,ys,xs)[2]/norm;
                    }
        }
    }
    for (int l = 1; l <= band; l++)
    {
        VH_A[l].resize(3);
        for (int k=-1;k<=1;++k)
        {
            VH_A[l][k+1].resize(2 * (l+k) + 1);
            for (int m = -(l+k); m <= (l+k); m++)
            {
                vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > > vh;
                sphereVecHarmonic(vh, radius, gauss, 0, k, m);

                xa = (int) floor(pos[2] - vh.shape()[2] / 2);
                xe = xa + vh.shape()[2] - 1;
                ya = (int) floor(pos[1] - vh.shape()[1] / 2);
                ye = ya + vh.shape()[1] - 1;
                za = (int) floor(pos[0] - vh.shape()[0] / 2);
                ze = za + vh.shape()[0] - 1;

                int zs=0;
                for (int z=za;z!=ze;z++,zs++)
                {
                    int ys=0;
                    for (int y=ya;y<=ye;y++,ys++)
                    {
                        int xs=0;
                        for (int x=xa;x<=xe;x++,xs++)
                        {
                            VH_A[l][1+k][1+m] += inputZ(z,y,x) * vh(zs,ys,xs)[0]/norm + inputZ(z,y,x) * vh(zs,ys,xs)[1]/norm + inputZ(z,y,x) * vh(zs,ys,xs)[2]/norm;
                        }
                    }
                }

            }
        }
    }
}

template <typename REAL>
void VHcenter(std::vector<std::vector<std::vector< FFTWComplex<REAL> > > >&VH_A, unsigned int band, REAL gauss, REAL radius, const vigra::MultiArray<3, vigra::TinyVector<REAL,3> > &A)
{
    VHpos(VH_A, band, gauss, radius, A, vigra::detail::centerOfBB(A));
}

template <typename REAL>
void VPHpos(std::vector<std::vector<std::vector<std::vector<FFTWComplex<REAL> > > > >&VH_A, unsigned int band, REAL radius, const vigra::MultiArray<3, vigra::TinyVector<REAL,3> > &A, const vigra::TinyVector<REAL, 3> pos)
{
    REAL gauss = 1;
    //transform input to C^(2j+1)
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputZ(A.shape());
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputY(A.shape());
    vigra::MultiArray< 3,FFTWComplex<REAL> >  inputX(A.shape());
    typename vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator z=inputZ.begin();
    typename vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator y=inputY.begin();
    typename vigra::MultiArray< 3,FFTWComplex<REAL> >::iterator x=inputX.begin();

    for (typename vigra::MultiArray<3,vigra::TinyVector<REAL,3> >::const_iterator p=A.begin();p!=A.end();++p,++z,++y,++x)
    {
        z->real() = -(*p)[1];
        z->imag() = -(*p)[2];
        *z *= 1/sqrt(2);
        y->real() =  (*p)[0];
        y->imag() =  0;
        x->real() = (*p)[1];
        x->imag() = -(*p)[2];
        *x *= 1/sqrt(2);
    }

    VH_A.resize(band + 1);
    for(int n=1;n<=band;n++)
    {
        VH_A[n].resize(band + 1);
        REAL norm = M_PI/2*pow((REAL)radius,2.0);

        // SH coeffs
        int xa=0,xe=0,ya=0,ye=0,za=0,ze=0;

        VH_A[n][0].resize(3);
        for(int k=-1;k<=1;++k)
        {
            VH_A[n][0][1+k].resize(3);
            for (int m=-1;m<=1;++m)
            {
            vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > > vh;
            sphereVecHarmonic(vh, radius, gauss, 0, k, m);

            xa = (int) floor(pos[2] - vh.shape()[2] / 2);
            xe = xa + vh.shape()[2] - 1;
            ya = (int) floor(pos[1] - vh.shape()[1] / 2);
            ye = ya + vh.shape()[1] - 1;
            za = (int) floor(pos[0] - vh.shape()[0] / 2);
            ze = za + vh.shape()[0] - 1;

            int zs=0;
            for (int z=za;z!=ze;z++,zs++)
            {
                int ys=0;
                for (int y=ya;y<=ye;y++,ys++)
                {
                    int xs=0;
                    for (int x=xa;x<=xe;x++,xs++)
                    {
                        VH_A[n][0][1+k][1+m] += inputZ(z,y,x) * vh(zs,ys,xs)[0]/norm + inputZ(z,y,x) * vh(zs,ys,xs)[1]/norm + inputZ(z,y,x) * vh(zs,ys,xs)[2]/norm;
                    }
                }
            }

            }
        }
        for (int l = 1; l <= band; l++)
        {
            VH_A[n][l].resize(3);
            for (int k=-1;k<=1;++k)
            {
                VH_A[n][l][k+1].resize(2 * (l+k) + 1);
                for (int m = -(l+k); m <= (l+k); m++)
                {
                    vigra::MultiArray<3, vigra::TinyVector<FFTWComplex<REAL>,3 > > vh;
                    sphereVecHarmonic(vh, radius, gauss, 0, k, m);

                    xa = (int) floor(pos[2] - vh.shape()[2] / 2);
                    xe = xa + vh.shape()[2] - 1;
                    ya = (int) floor(pos[1] - vh.shape()[1] / 2);
                    ye = ya + vh.shape()[1] - 1;
                    za = (int) floor(pos[0] - vh.shape()[0] / 2);
                    ze = za + vh.shape()[0] - 1;

                    int zs=0;
                    int ys=0;
                    int xs=0;
                    for (int z=za;z!=ze;z++,zs++)
                        for (int y=ya;y<=ye;y++,ys++)
                            for (int x=xa;x<=xe;x++,xs++)
                            {
                                VH_A[n][l][1+k][1+m] += inputZ(z,y,x) * vh(zs,ys,xs)[0]/norm + inputZ(z,y,x) * vh(zs,ys,xs)[1]/norm + inputZ(z,y,x) * vh(zs,ys,xs)[2]/norm;
                            }

                }
            }
        }
    }
}

template <typename REAL>
void VPHcenter(std::vector<std::vector<std::vector<std::vector< FFTWComplex<REAL> > > > >&VH_A, unsigned int band, REAL radius, const vigra::MultiArray<3, vigra::TinyVector<REAL,3> > &A)
{
    VPHpos(VH_A, band, radius, A, vigra::detail::centerOfBB(A));
}


} // namespace vigra 

#endif // VIGRA_INVARIANT_FEATURES3D_HXX
