/************************************************************************/
/*                                                                      */
/*               Copyright 1998-2003 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    You may use, modify, and distribute this software according       */
/*    to the terms stated in the LICENSE file included in               */
/*    the VIGRA distribution.                                           */
/*                                                                      */
/*    The VIGRA Website is                                              */
/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */
/*    Please direct questions, bug reports, and contributions to        */
/*        koethe@informatik.uni-hamburg.de                              */
/*                                                                      */
/*  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR          */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */
/*                                                                      */
/************************************************************************/

#ifndef VIGRA_SPLINEIMAGEVIEW_HXX
#define VIGRA_SPLINEIMAGEVIEW_HXX

#include <cmath>
#include "vigra/resizeimage.hxx"
#include "vigra/array_vector.hxx"

namespace vigra {

template <class VALUETYPE>
class SplineImageView
{
    typedef typename NumericTraits<VALUETYPE>::RealPromote InternalValue;
    typedef BasicImage<InternalValue> InternalImage;
    typedef typename InternalImage::traverser InternalTraverser;
    typedef typename InternalTraverser::row_iterator InternalRowIterator;
    typedef typename InternalTraverser::column_iterator InternalColumnIterator;
  
  public:
  
    typedef VALUETYPE value_type;
    typedef Size2D size_type;
    typedef TinyVector<double, 2> difference_type;
    
    template <class SrcIterator, class SrcAccessor>
    SplineImageView(SrcIterator is, SrcIterator iend, SrcAccessor sa)
    : x_(-1), y_(-1),
      w_(iend.x - is.x), h_(iend.y - is.y),
      v_(w_, h_), dx_(w_, h_), dy_(w_, h_), dxy_(w_, h_),
      W1_(4, 4), W2_(4, 4)
    {
        copyImage(srcIterRange(is, iend, sa), destImage(v_));
        
        init();
    }
    
    template <class SrcIterator, class SrcAccessor>
    SplineImageView(triple<SrcIterator, SrcIterator, SrcAccessor> src)
    : x_(-1), y_(-1),
      w_(src.second.x - src.first.x), h_(src.second.y - src.first.y),
      v_(w_, h_), dx_(w_, h_), dy_(w_, h_), dxy_(w_, h_),
      W1_(4, 4), W2_(4, 4)
    {
        copyImage(src, destImage(v_));
        
        init();
    }
    
    value_type operator()(double x, double y) const;
    value_type dx(double x, double y) const;
    value_type dy(double x, double y) const;
    value_type dxx(double x, double y) const;
    value_type dxy(double x, double y) const;
    value_type dyy(double x, double y) const;
    
    value_type operator()(difference_type const & d) const
        { return operator()(d[0], d[1]); }
    value_type dx(difference_type const & d) const
        { return dx(d[0], d[1]); }
    value_type dy(difference_type const & d) const
        { return dy(d[0], d[1]); }
    value_type dxx(difference_type const & d) const
        { return dxx(d[0], d[1]); }
    value_type dxy(difference_type const & d) const
        { return dxy(d[0], d[1]); }
    value_type dyy(difference_type const & d) const
        { return dyy(d[0], d[1]); }
    
    unsigned int width() const
        { return w_; }
    unsigned int height() const
        { return h_; }
    size_type size() const
        { return size_type(w_, h_); }
    bool isInside(double x, double y) const
        { return (x >= 0.0 && y >= 0.0 && x <= w_ - 1 && y <= h_ -1); }
        
  private:
  
    void init();
    
    void calculateCoefficientMatrix(int x, int y) const;
    
    void calculateIndices(double x, double y, int & ix, int & iy, double & u, double & v) const;
  
    mutable int x_, y_;
    unsigned int w_, h_;
    InternalImage v_, dx_, dy_, dxy_;
    mutable InternalImage W1_, W2_;
};

template <class VALUETYPE>
void SplineImageView<VALUETYPE>::init()
{
    unsigned int x, y;
    typename InternalImage::Accessor a = v_.accessor();
    ArrayVector<double> R1(w_ > h_ ? w_ : h_);
    ArrayVector<InternalValue> R2(w_ > h_ ? w_ : h_);
    
    // calculate x derivatives
    InternalTraverser iv = v_.upperLeft();
    InternalTraverser idx = dx_.upperLeft();
    for(y=0; y<h_; ++y, ++iv.y, ++idx.y)
    {
        InternalRowIterator rv = iv.rowIterator();
        resizeImageInternalSplineGradient(rv, rv + w_, a,
                                          R1.begin(), R2.begin(), idx.rowIterator());
    }

    iv = v_.upperLeft();
    InternalTraverser idy = dy_.upperLeft();
    // calculate y derivatives
    for(x=0; x<w_; ++x, ++iv.x, ++idy.x)
    {
        InternalColumnIterator cv = iv.columnIterator();
        resizeImageInternalSplineGradient(cv, cv + h_, a,
                                          R1.begin(), R2.begin(), idy.columnIterator());
    }

    idy = dy_.upperLeft();
    InternalTraverser idxy = dxy_.upperLeft();
    // calculate mixed derivatives
    for(y=0; y<h_; ++y, ++idy.y, ++idxy.y)
    {
        InternalRowIterator rdy = idy.rowIterator();
        resizeImageInternalSplineGradient(rdy, rdy + w_, a,
                                          R1.begin(), R2.begin(), idxy.rowIterator());
    }
}

template <class VALUETYPE>
void SplineImageView<VALUETYPE>::calculateCoefficientMatrix(int x, int y) const
{
    if(x == x_ && y == y_)
        return;  // still in cache

    x_ = x;
    y_ = y;
    int x1 = x+1, y1 = y+1;

    static double g[] = { 1.0, 0.0, -3.0,  2.0,
                          0.0, 1.0, -2.0,  1.0,
                          0.0, 0.0,  3.0, -2.0,
                          0.0, 0.0, -1.0,  1.0 };

    W1_[0][0] = v_(x, y);
    W1_[0][1] = dy_(x, y);
    W1_[0][2] = v_(x, y1);
    W1_[0][3] = dy_(x, y1);
    W1_[1][0] = dx_(x, y);
    W1_[1][1] = dxy_(x, y);
    W1_[1][2] = dx_(x, y1);
    W1_[1][3] = dxy_(x, y1);
    W1_[2][0] = v_(x1, y);
    W1_[2][1] = dy_(x1, y);
    W1_[2][2] = v_(x1, y1);
    W1_[2][3] = dy_(x1, y1);
    W1_[3][0] = dx_(x1, y);
    W1_[3][1] = dxy_(x1, y);
    W1_[3][2] = dx_(x1, y1);
    W1_[3][3] = dxy_(x1, y1);

    for(int j=0; j<4; ++j)
    {
        for(int i=0; i<4; ++i)
        {
            W2_[j][i] = g[j] * W1_[0][i];
            for(int k=1; k<4; ++k)
            {
                W2_[j][i] += g[j+4*k] * W1_[k][i];
            }
        }
    }
    for(int j=0; j<4; ++j)
    {
        for(int i=0; i<4; ++i)
        {
            W1_[j][i] = g[i] * W2_[j][0];
            for(int k=1; k<4; ++k)
            {
               W1_[j][i] += g[4*k+i] * W2_[j][k];
            }
        }
    }
}
    
template <class VALUETYPE>
void SplineImageView<VALUETYPE>::calculateIndices(double x, double y, 
                                       int & ix, int & iy, double & u, double & v) const
{
    vigra_precondition(x >= 0.0 && y >= 0.0 && x <= w_ - 1 && y <= h_ - 1,
         "SplineImageView<VALUETYPE>::calculateIndices(): index out of bounds.");

    ix = (int)x;
    iy = (int)y;
    
    if(ix == w_ - 1)
        ix -= 1;
    if(iy == h_ - 1)
        iy -= 1;
    u = x - ix;
    v = y - iy;
}

template <class VALUETYPE>
VALUETYPE SplineImageView<VALUETYPE>::operator()(double x, double y) const
{
    int ix, iy;
    double u, v;
    calculateIndices(x, y, ix, iy, u, v);
    calculateCoefficientMatrix(ix, iy);
    
    InternalValue a1 = W1_[0][0] + v * (W1_[0][1] + v * (W1_[0][2] + v * W1_[0][3]));
    InternalValue a2 = W1_[1][0] + v * (W1_[1][1] + v * (W1_[1][2] + v * W1_[1][3]));
    InternalValue a3 = W1_[2][0] + v * (W1_[2][1] + v * (W1_[2][2] + v * W1_[2][3]));
    InternalValue a4 = W1_[3][0] + v * (W1_[3][1] + v * (W1_[3][2] + v * W1_[3][3]));
    return NumericTraits<VALUETYPE>::fromRealPromote(a1 + u * (a2 + u * (a3 + u * a4)));
}

template <class VALUETYPE>
VALUETYPE SplineImageView<VALUETYPE>::dx(double x, double y) const
{
    int ix, iy;
    double u, v;
    calculateIndices(x, y, ix, iy, u, v);
    calculateCoefficientMatrix(ix, iy);
    
    InternalValue a2 = W1_[1][0] + v * (W1_[1][1] + v * (W1_[1][2] + v * W1_[1][3]));
    InternalValue a3 = W1_[2][0] + v * (W1_[2][1] + v * (W1_[2][2] + v * W1_[2][3]));
    InternalValue a4 = W1_[3][0] + v * (W1_[3][1] + v * (W1_[3][2] + v * W1_[3][3]));
    return NumericTraits<VALUETYPE>::fromRealPromote(a2 + u * (2.0 * a3 + u * 3.0 * a4));
}

template <class VALUETYPE>
VALUETYPE SplineImageView<VALUETYPE>::dy(double x, double y) const
{
    int ix, iy;
    double u, v;
    calculateIndices(x, y, ix, iy, u, v);
    calculateCoefficientMatrix(ix, iy);
    
    InternalValue a1 = W1_[0][1] + v * (2.0 * W1_[0][2] + v * 3.0 * W1_[0][3]);
    InternalValue a2 = W1_[1][1] + v * (2.0 * W1_[1][2] + v * 3.0 * W1_[1][3]);
    InternalValue a3 = W1_[2][1] + v * (2.0 * W1_[2][2] + v * 3.0 * W1_[2][3]);
    InternalValue a4 = W1_[3][1] + v * (2.0 * W1_[3][2] + v * 3.0 * W1_[3][3]);
    return NumericTraits<VALUETYPE>::fromRealPromote(a1 + u * (a2 + u * (a3 + u * a4)));
}

template <class VALUETYPE>
VALUETYPE SplineImageView<VALUETYPE>::dxx(double x, double y) const
{
    int ix, iy;
    double u, v;
    calculateIndices(x, y, ix, iy, u, v);
    calculateCoefficientMatrix(ix, iy);
    
    InternalValue a3 = W1_[2][0] + v * (W1_[2][1] + v * (W1_[2][2] + v * W1_[2][3]));
    InternalValue a4 = W1_[3][0] + v * (W1_[3][1] + v * (W1_[3][2] + v * W1_[3][3]));
    return NumericTraits<VALUETYPE>::fromRealPromote(2.0 * a3 + u * 6.0 * a4);
}

template <class VALUETYPE>
VALUETYPE SplineImageView<VALUETYPE>::dxy(double x, double y) const
{
    int ix, iy;
    double u, v;
    calculateIndices(x, y, ix, iy, u, v);
    calculateCoefficientMatrix(ix, iy);
    
    InternalValue a2 = W1_[1][1] + v * (2.0 * W1_[1][2] + v * 3.0 * W1_[1][3]);
    InternalValue a3 = W1_[2][1] + v * (2.0 * W1_[2][2] + v * 3.0 * W1_[2][3]);
    InternalValue a4 = W1_[3][1] + v * (2.0 * W1_[3][2] + v * 3.0 * W1_[3][3]);
    return NumericTraits<VALUETYPE>::fromRealPromote(a2 + u * (2.0 * a3 + u * 3.0 * a4));
}

template <class VALUETYPE>
VALUETYPE SplineImageView<VALUETYPE>::dyy(double x, double y) const
{
    int ix, iy;
    double u, v;
    calculateIndices(x, y, ix, iy, u, v);
    calculateCoefficientMatrix(ix, iy);
    
    InternalValue a1 = 2.0 * W1_[0][2] + v * 6.0 * W1_[0][3];
    InternalValue a2 = 2.0 * W1_[1][2] + v * 6.0 * W1_[1][3];
    InternalValue a3 = 2.0 * W1_[2][2] + v * 6.0 * W1_[2][3];
    InternalValue a4 = 2.0 * W1_[3][2] + v * 6.0 * W1_[3][3];
    return NumericTraits<VALUETYPE>::fromRealPromote(a1 + u * (a2 + u * (a3 + u * a4)));
}

template <class VALUETYPE>
class CubicSplineImageView
{
    typedef typename NumericTraits<VALUETYPE>::RealPromote InternalValue;
    typedef BasicImage<InternalValue> InternalImage;
    typedef typename InternalImage::traverser InternalTraverser;
    typedef typename InternalTraverser::row_iterator InternalRowIterator;
    typedef typename InternalTraverser::column_iterator InternalColumnIterator;
  
  public:
  
    typedef VALUETYPE value_type;
    typedef Size2D size_type;
    typedef TinyVector<double, 2> difference_type;
    
    template <class SrcIterator, class SrcAccessor>
    CubicSplineImageView(SrcIterator is, SrcIterator iend, SrcAccessor sa, double scale)
    : w_(iend.x - is.x), h_(iend.y - is.y), w1_(w_-1), h1_(h_-1),
      image_(w_, h_),
      x_(-1.0), y_(-1.0),
      u_(-1.0), v_(-1.0)
    {
        copyImage(srcIterRange(is, iend, sa), destImage(image_));
        init(scale);
    }
    
    template <class SrcIterator, class SrcAccessor>
    CubicSplineImageView(triple<SrcIterator, SrcIterator, SrcAccessor> s, double scale)
    : w_(s.second.x - s.first.x), h_(s.second.y - s.first.y), w1_(w_-1), h1_(h_-1),
      image_(w_, h_),
      x_(-1.0), y_(-1.0),
      u_(-1.0), v_(-1.0)
    {
        copyImage(srcIterRange(s.first, s.second, s.third), destImage(image_));
        init(scale);
    }
    
    value_type operator()(double x, double y) const;
    value_type dx(double x, double y) const;
    value_type dy(double x, double y) const;
    value_type dxx(double x, double y) const;
    value_type dxy(double x, double y) const;
    value_type dyy(double x, double y) const;
    value_type g2(double x, double y) const;
    value_type g2x(double x, double y) const;
    value_type g2y(double x, double y) const;
    
    value_type operator()(difference_type const & d) const
        { return operator()(d[0], d[1]); }
    value_type dx(difference_type const & d) const
        { return dx(d[0], d[1]); }
    value_type dy(difference_type const & d) const
        { return dy(d[0], d[1]); }
    value_type dxx(difference_type const & d) const
        { return dxx(d[0], d[1]); }
    value_type dxy(difference_type const & d) const
        { return dxy(d[0], d[1]); }
    value_type dyy(difference_type const & d) const
        { return dyy(d[0], d[1]); }
    value_type g2(difference_type const & d) const
        { return g2(d[0], d[1]); }
    value_type g2x(difference_type const & d) const
        { return g2x(d[0], d[1]); }
    value_type g2y(difference_type const & d) const
        { return g2y(d[0], d[1]); }
    
    unsigned int width() const
        { return w_; }
    unsigned int height() const
        { return h_; }
    size_type size() const
        { return size_type(w_, h_); }
        
    InternalImage const & image() const
    {
        return image_;
    }
        
  protected:
  
    void init(double scale);
    void calculateIndices(double x, double y) const;
    void coefficients0(double t, double * const & c) const;
    void coefficients1(double t, double * const & c) const;
    void coefficients2(double t, double * const & c) const;
    value_type convolve() const;
  
    unsigned int w_, h_;
    int w1_, h1_;
    InternalImage image_;
    CubicBSplineKernel k_;
    mutable double x_, y_, u_, v_, kx_[4], ky_[4];
    mutable int ix_[4], iy_[4];
};

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
void recursiveFilterLine(SrcIterator is, SrcIterator isend, SrcAccessor as,
                         DestIterator id, DestAccessor ad, double b1, double b2)
{
    int w = isend - is;
    SrcIterator istart = is;
    
    int x;
    
    typedef typename
        NumericTraits<typename SrcAccessor::value_type>::RealPromote TempType;
    typedef NumericTraits<typename DestAccessor::value_type> DestTraits;
    
    // speichert den Ergebnis der linkseitigen Filterung.
    std::vector<TempType> vline(w+1);
    typename std::vector<TempType>::iterator line = vline.begin();
    
    double norm  = 1.0 - b1 - b2;
    double norm1 = (1.0 - b1 - b2) / (1.0 + b1 + b2);
    double norm2 = norm * norm;
    

    // init left side of filter
    int kernelw = std::min(w-1, std::max(8, (int)(1.0 / norm + 0.5)));  
    is += (kernelw - 2);
    line[kernelw] = as(is);
    line[kernelw-1] = as(is);
    for(x = kernelw - 2; x > 0; --x, --is)
    {
        line[x] = as(is) + b1 * line[x+1] + b2 * line[x+2];
    }
    line[0] = as(is) + b1 * line[1] + b2 * line[2];
    ++is;
    line[1] = as(is) + b1 * line[0] + b2 * line[1];
    ++is;
    for(x=2; x < w; ++x, ++is)
    {
        line[x] = as(is) + b1 * line[x-1] + b2 * line[x-2];
    }
    line[w] = line[w-1];

    line[w-1] = norm1 * (line[w-1] + b1 * line[w-2] + b2 * line[w-3]);
    line[w-2] = norm1 * (line[w-2] + b1 * line[w] + b2 * line[w-2]);
    id += w-1;
    ad.set(line[w-1], id);
    --id;
    ad.set(line[w-2], id);
    --id;
    for(x=w-3; x>=0; --x, --id, --is)
    {    
        line[x] = norm2 * line[x] + b1 * line[x+1] + b2 * line[x+2];
        ad.set(line[x], id);
    }
}
            
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor>
void recursiveFilterX(SrcImageIterator supperleft, 
                       SrcImageIterator slowerright, SrcAccessor as,
                       DestImageIterator dupperleft, DestAccessor ad, 
                       double b1, double b2)
{
    int w = slowerright.x - supperleft.x;
    int h = slowerright.y - supperleft.y;
    
    int y;
    
    for(y=0; y<h; ++y, ++supperleft.y, ++dupperleft.y)
    {
        typename SrcImageIterator::row_iterator rs = supperleft.rowIterator();
        typename DestImageIterator::row_iterator rd = dupperleft.rowIterator();

        recursiveFilterLine(rs, rs+w, as, 
                             rd, ad, 
                             b1, b2);
    }
}

template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor>
inline void recursiveFilterX(
            triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
            pair<DestImageIterator, DestAccessor> dest, 
                       double b1, double b2)
{
    recursiveFilterX(src.first, src.second, src.third,
                      dest.first, dest.second, b1, b2);
}
            
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor>
void recursiveFilterY(SrcImageIterator supperleft, 
                       SrcImageIterator slowerright, SrcAccessor as,
                       DestImageIterator dupperleft, DestAccessor ad, 
                       double b1, double b2)
{
    int w = slowerright.x - supperleft.x;
    int h = slowerright.y - supperleft.y;
    
    int x;
    
    for(x=0; x<w; ++x, ++supperleft.x, ++dupperleft.x)
    {
        typename SrcImageIterator::column_iterator cs = supperleft.columnIterator();
        typename DestImageIterator::column_iterator cd = dupperleft.columnIterator();

        recursiveFilterLine(cs, cs+h, as, 
                            cd, ad, 
                            b1, b2);
    }
}

template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor>
inline void recursiveFilterY(
            triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
            pair<DestImageIterator, DestAccessor> dest, 
                       double b1, double b2)
{
    recursiveFilterY(src.first, src.second, src.third,
                      dest.first, dest.second, b1, b2);
}

            


template <class VALUETYPE>
void CubicSplineImageView<VALUETYPE>::init(double scale)
{
    if(scale < 0.0)
        return;   // skip prefiltering
    if(scale == 0.0)
    {
        double z = VIGRA_CSTD::sqrt(3.0) - 2.0;
        recursiveFilterX(srcImageRange(image_), destImage(image_), z, BORDER_TREATMENT_REFLECT);
        recursiveFilterY(srcImageRange(image_), destImage(image_), z, BORDER_TREATMENT_REFLECT);
    }
    else
    {
        double l = scale * scale * scale * scale / 2.0;
        double l1 = VIGRA_CSTD::sqrt(3.0 + 144.0 * l);
        double c = (-3.0 - l1 + VIGRA_CSTD::sqrt(12.0 + 6.0 * l1)) / 12.0;
        double z2 = -c * c / l;
        double z1 = 1.0 - z2 + c / l;
        recursiveFilterX(srcImageRange(image_), destImage(image_), z1, z2);
        recursiveFilterY(srcImageRange(image_), destImage(image_), z1, z2);
    }
}

template <class VALUETYPE>
void 
CubicSplineImageView<VALUETYPE>::calculateIndices(double x, double y) const
{
    vigra_precondition(x >= 0.0 && y >= 0.0 && x <= w_ - 1 && y <= h_ - 1,
         "CubicSplineImageView<VALUETYPE>::calculateIndices(): index out of bounds.");

    if(x == x_ && y == y_)
        return;   // still in cache
    x_ = x;
    y_ = y;
    ix_[1] = (int)x_;
    iy_[1] = (int)y_;
    u_ = x_ - ix_[1];
    v_ = y_ - iy_[1];
    
    ix_[0] = VIGRA_CSTD::abs(ix_[1] - 1);
    ix_[2] = w1_ - VIGRA_CSTD::abs(w1_ - ix_[1] - 1);
    ix_[3] = w1_ - VIGRA_CSTD::abs(w1_ - ix_[1] - 2);
    iy_[0] = VIGRA_CSTD::abs(iy_[1] - 1);
    iy_[2] = h1_ - VIGRA_CSTD::abs(h1_ - iy_[1] - 1);
    iy_[3] = h1_ - VIGRA_CSTD::abs(h1_ - iy_[1] - 2);
}

template <class VALUETYPE>
void CubicSplineImageView<VALUETYPE>::coefficients0(double t, double * const & c) const
{
    t += 1.0;
    for(int i = 0; i<4; ++i)
        c[i] = k_[t-i];
}

template <class VALUETYPE>
void CubicSplineImageView<VALUETYPE>::coefficients1(double t, double * const & c) const
{
    t += 1.0;
    for(int i = 0; i<4; ++i)
        c[i] = k_.dx(t-i);
}

template <class VALUETYPE>
void CubicSplineImageView<VALUETYPE>::coefficients2(double t, double * const & c) const

{
    t += 1.0;
    for(int i = 0; i<4; ++i)
        c[i] = k_.dxx(t-i);
}

template <class VALUETYPE>
VALUETYPE 
CubicSplineImageView<VALUETYPE>::convolve() const
{
    InternalValue sum, a;
    sum = NumericTraits<InternalValue>::zero();
    for(int j=0; j<4; ++j)
    {
        a = kx_[0] * image_(ix_[0], iy_[j]);
        for(int i=1; i<4; ++i)
        {
            a += kx_[i] * image_(ix_[i], iy_[j]);
        }
        sum += ky_[j]*a;
    }
    return NumericTraits<VALUETYPE>::fromRealPromote(sum);
}

template <class VALUETYPE>
VALUETYPE CubicSplineImageView<VALUETYPE>::operator()(double x, double y) const
{
    calculateIndices(x, y);
    coefficients0(u_, kx_);
    coefficients0(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE CubicSplineImageView<VALUETYPE>::dx(double x, double y) const
{
    calculateIndices(x, y);
    coefficients1(u_, kx_);
    coefficients0(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE CubicSplineImageView<VALUETYPE>::dy(double x, double y) const
{
    calculateIndices(x, y);
    coefficients0(u_, kx_);
    coefficients1(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE CubicSplineImageView<VALUETYPE>::dxx(double x, double y) const
{
    calculateIndices(x, y);
    coefficients2(u_, kx_);
    coefficients0(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE CubicSplineImageView<VALUETYPE>::dxy(double x, double y) const
{
    calculateIndices(x, y);
    coefficients1(u_, kx_);
    coefficients1(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE CubicSplineImageView<VALUETYPE>::dyy(double x, double y) const
{
    calculateIndices(x, y);
    coefficients0(u_, kx_);
    coefficients2(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE CubicSplineImageView<VALUETYPE>::g2(double x, double y) const
{
    return sq(dx(x,y)) + sq(dy(x,y));
}

template <class VALUETYPE>
VALUETYPE CubicSplineImageView<VALUETYPE>::g2x(double x, double y) const
{
    return 2.0*(dx(x,y) * dxx(x,y) + dy(x,y) * dxy(x,y));
}

template <class VALUETYPE>
VALUETYPE CubicSplineImageView<VALUETYPE>::g2y(double x, double y) const
{
    return 2.0*(dx(x,y) * dxy(x,y) + dy(x,y) * dyy(x,y));
}

template <class VALUETYPE>
class QuinticSplineImageView
{
    typedef typename NumericTraits<VALUETYPE>::RealPromote InternalValue;
    typedef BasicImage<InternalValue> InternalImage;
    typedef typename InternalImage::traverser InternalTraverser;
    typedef typename InternalTraverser::row_iterator InternalRowIterator;
    typedef typename InternalTraverser::column_iterator InternalColumnIterator;
  
  public:
  
    typedef VALUETYPE value_type;
    typedef Size2D size_type;
    typedef TinyVector<double, 2> difference_type;
    
    template <class SrcIterator, class SrcAccessor>
    QuinticSplineImageView(SrcIterator is, SrcIterator iend, SrcAccessor sa, bool skipPrefiltering = false)
    : w_(iend.x - is.x), h_(iend.y - is.y), w1_(w_-1), h1_(h_-1),
      image_(w_, h_),
      x_(-1.0), y_(-1.0),
      u_(-1.0), v_(-1.0)
    {
        copyImage(srcIterRange(is, iend, sa), destImage(image_));
        if(!skipPrefiltering)
            init();
    }
    
    template <class SrcIterator, class SrcAccessor>
    QuinticSplineImageView(triple<SrcIterator, SrcIterator, SrcAccessor> s, bool skipPrefiltering = false)
    : w_(s.second.x - s.first.x), h_(s.second.y - s.first.y), w1_(w_-1), h1_(h_-1),
      image_(w_, h_),
      x_(-1.0), y_(-1.0),
      u_(-1.0), v_(-1.0)
    {
        copyImage(srcIterRange(s.first, s.second, s.third), destImage(image_));
        if(!skipPrefiltering)
            init();
    }
    
    value_type operator()(double x, double y) const;
    value_type dx(double x, double y) const;
    value_type dy(double x, double y) const;
    value_type dxx(double x, double y) const;
    value_type dxy(double x, double y) const;
    value_type dyy(double x, double y) const;
    value_type dx3(double x, double y) const;
    value_type dy3(double x, double y) const;
    value_type dxxy(double x, double y) const;
    value_type dxyy(double x, double y) const;
    value_type g2(double x, double y) const;
    value_type g2x(double x, double y) const;
    value_type g2y(double x, double y) const;
    value_type g2xx(double x, double y) const;
    value_type g2xy(double x, double y) const;
    value_type g2yy(double x, double y) const;
    
    value_type operator()(difference_type const & d) const
        { return operator()(d[0], d[1]); }
    value_type dx(difference_type const & d) const
        { return dx(d[0], d[1]); }
    value_type dy(difference_type const & d) const
        { return dy(d[0], d[1]); }
    value_type dxx(difference_type const & d) const
        { return dxx(d[0], d[1]); }
    value_type dxy(difference_type const & d) const
        { return dxy(d[0], d[1]); }
    value_type dyy(difference_type const & d) const
        { return dyy(d[0], d[1]); }
    value_type dx3(difference_type const & d) const
        { return dx3(d[0], d[1]); }
    value_type dy3(difference_type const & d) const
        { return dy3(d[0], d[1]); }
    value_type dxxy(difference_type const & d) const
        { return dxxy(d[0], d[1]); }
    value_type dxyy(difference_type const & d) const
        { return dxyy(d[0], d[1]); }
    value_type g2(difference_type const & d) const
        { return g2(d[0], d[1]); }
    value_type g2x(difference_type const & d) const
        { return g2x(d[0], d[1]); }
    value_type g2y(difference_type const & d) const
        { return g2y(d[0], d[1]); }
    value_type g2xx(difference_type const & d) const
        { return g2xx(d[0], d[1]); }
    value_type g2xy(difference_type const & d) const
        { return g2xy(d[0], d[1]); }
    value_type g2yy(difference_type const & d) const
        { return g2yy(d[0], d[1]); }
    
    unsigned int width() const
        { return w_; }
    unsigned int height() const
        { return h_; }
    size_type size() const
        { return size_type(w_, h_); }
        
    InternalImage const & image() const
    {
        return image_;
    }
        
  protected:
  
    void init();
    void calculateIndices(double x, double y) const;
    void coefficients0(double t, double * const & c) const;
    void coefficients1(double t, double * const & c) const;
    void coefficients2(double t, double * const & c) const;
    void coefficients3(double t, double * const & c) const;
    void coefficients4(double t, double * const & c) const;
    value_type convolve() const;
  
    unsigned int w_, h_;
    int w1_, h1_;
    InternalImage image_;
    QuinticBSplineKernel k_;
    mutable double x_, y_, u_, v_, kx_[6], ky_[6];
    mutable int ix_[6], iy_[6];
};

template <class VALUETYPE>
void QuinticSplineImageView<VALUETYPE>::init()
{
    double b1 = -0.430575;
    double b2 = -0.0430963;
    recursiveFilterX(srcImageRange(image_), destImage(image_), b1, BORDER_TREATMENT_REFLECT);
    recursiveFilterY(srcImageRange(image_), destImage(image_), b1, BORDER_TREATMENT_REFLECT);
    recursiveFilterX(srcImageRange(image_), destImage(image_), b2, BORDER_TREATMENT_REFLECT);
    recursiveFilterY(srcImageRange(image_), destImage(image_), b2, BORDER_TREATMENT_REFLECT);
}

template <class VALUETYPE>
void 
QuinticSplineImageView<VALUETYPE>::calculateIndices(double x, double y) const
{
    vigra_precondition(x >= 0.0 && y >= 0.0 && x <= w_ - 1 && y <= h_ - 1,
         "QuinticSplineImageView<VALUETYPE>::calculateIndices(): index out of bounds.");

    if(x == x_ && y == y_)
        return;   // still in cache
    x_ = x;
    y_ = y;
    ix_[2] = (int)x_;
    iy_[2] = (int)y_;
    u_ = x_ - ix_[2];
    v_ = y_ - iy_[2];
    
    ix_[0] = VIGRA_CSTD::abs(ix_[2] - 2);
    ix_[1] = VIGRA_CSTD::abs(ix_[2] - 1);
    ix_[3] = w1_ - VIGRA_CSTD::abs(w1_ - ix_[2] - 1);
    ix_[4] = w1_ - VIGRA_CSTD::abs(w1_ - ix_[2] - 2);
    ix_[5] = w1_ - VIGRA_CSTD::abs(w1_ - ix_[2] - 3);
    iy_[0] = VIGRA_CSTD::abs(iy_[2] - 2);
    iy_[1] = VIGRA_CSTD::abs(iy_[2] - 1);
    iy_[3] = h1_ - VIGRA_CSTD::abs(h1_ - iy_[2] - 1);
    iy_[4] = h1_ - VIGRA_CSTD::abs(h1_ - iy_[2] - 2);
    iy_[5] = h1_ - VIGRA_CSTD::abs(h1_ - iy_[2] - 3);
}

template <class VALUETYPE>
void QuinticSplineImageView<VALUETYPE>::coefficients0(double t, double * const & c) const
{
    t += 2.0;
    for(int i = 0; i<6; ++i)
        c[i] = k_[t-i];
}

template <class VALUETYPE>
void QuinticSplineImageView<VALUETYPE>::coefficients1(double t, double * const & c) const
{
    t += 2.0;
    for(int i = 0; i<6; ++i)
        c[i] = k_.dx(t-i);
}

template <class VALUETYPE>
void QuinticSplineImageView<VALUETYPE>::coefficients2(double t, double * const & c) const

{
    t += 2.0;
    for(int i = 0; i<6; ++i)
        c[i] = k_.dxx(t-i);
}

template <class VALUETYPE>
void QuinticSplineImageView<VALUETYPE>::coefficients3(double t, double * const & c) const
{
    t += 2.0;
    for(int i = 0; i<6; ++i)
        c[i] = k_.dx3(t-i);
}

template <class VALUETYPE>
void QuinticSplineImageView<VALUETYPE>::coefficients4(double t, double * const & c) const
{
    t += 2.0;
    for(int i = 0; i<6; ++i)
        c[i] = k_.dx4(t-i);
}

template <class VALUETYPE>
VALUETYPE 
QuinticSplineImageView<VALUETYPE>::convolve() const
{
    InternalValue sum, a;
    sum = NumericTraits<InternalValue>::zero();
    for(int j=0; j<6; ++j)
    {
        a = kx_[0] * image_(ix_[0], iy_[j]);
        for(int i=1; i<6; ++i)
        {
            a += kx_[i] * image_(ix_[i], iy_[j]);
        }
        sum += ky_[j]*a;
    }
    return NumericTraits<VALUETYPE>::fromRealPromote(sum);
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::operator()(double x, double y) const
{
    calculateIndices(x, y);
    coefficients0(u_, kx_);
    coefficients0(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::dx(double x, double y) const
{
    calculateIndices(x, y);
    coefficients1(u_, kx_);
    coefficients0(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::dy(double x, double y) const
{
    calculateIndices(x, y);
    coefficients0(u_, kx_);
    coefficients1(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::dxx(double x, double y) const
{
    calculateIndices(x, y);
    coefficients2(u_, kx_);
    coefficients0(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::dxy(double x, double y) const
{
    calculateIndices(x, y);
    coefficients1(u_, kx_);
    coefficients1(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::dyy(double x, double y) const
{
    calculateIndices(x, y);
    coefficients0(u_, kx_);
    coefficients2(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::dx3(double x, double y) const
{
    calculateIndices(x, y);
    coefficients3(u_, kx_);
    coefficients0(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::dy3(double x, double y) const
{
    calculateIndices(x, y);
    coefficients0(u_, kx_);
    coefficients3(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::dxxy(double x, double y) const
{
    calculateIndices(x, y);
    coefficients2(u_, kx_);
    coefficients1(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::dxyy(double x, double y) const
{
    calculateIndices(x, y);
    coefficients1(u_, kx_);
    coefficients2(v_, ky_);
    return convolve();
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::g2(double x, double y) const
{
    return sq(dx(x,y)) + sq(dy(x,y));
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::g2x(double x, double y) const
{
    return 2.0*(dx(x,y) * dxx(x,y) + dy(x,y) * dxy(x,y));
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::g2y(double x, double y) const
{
    return 2.0*(dx(x,y) * dxy(x,y) + dy(x,y) * dyy(x,y));
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::g2xx(double x, double y) const
{
    return 2.0*(sq(dxx(x,y)) + dx(x,y) * dx3(x,y) + sq(dxy(x,y)) + dy(x,y) * dxxy(x,y));
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::g2yy(double x, double y) const
{
    return 2.0*(sq(dxy(x,y)) + dx(x,y) * dxyy(x,y) + sq(dyy(x,y)) + dy(x,y) * dy3(x,y));
}

template <class VALUETYPE>
VALUETYPE QuinticSplineImageView<VALUETYPE>::g2xy(double x, double y) const
{
    return 2.0*(dx(x,y) * dxxy(x,y) + dy(x,y) * dxyy(x,y) + dxy(x,y) * (dxx(x,y) + dyy(x,y)));
}

} // namespace vigra


#endif /* VIGRA_SPLINEIMAGEVIEW_HXX */