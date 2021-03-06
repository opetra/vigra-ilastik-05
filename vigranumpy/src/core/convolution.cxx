/************************************************************************/
/*                                                                      */
/*                 Copyright 2009 by Ullrich Koethe                     */
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

#define PY_ARRAY_UNIQUE_SYMBOL vigranumpyfilters_PyArray_API
#define NO_IMPORT_ARRAY

#include <Python.h>
#include <boost/python.hpp>
#include <vigra/numpy_array.hxx>
#include <vigra/numpy_array_converters.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra/convolution.hxx>
#include <vigra/multi_convolution.hxx>
#include <vigra/recursiveconvolution.hxx>
#include <vigra/nonlineardiffusion.hxx>
#include "vigranumpykernel.hxx"

namespace python = boost::python;

namespace vigra
{

template < class VoxelType, unsigned int ndim >
NumpyAnyArray 
pythonConvolveOneDimensionND(NumpyArray<ndim, Multiband<VoxelType> > volume,
                             unsigned int dim,
                             Kernel const & kernel,
                             NumpyArray<ndim, Multiband<VoxelType> > res=python::object())
{
    vigra_precondition(dim < ndim-1,
           "convolveOneDimension(): dim out of range.");

    res.reshapeIfEmpty(volume.shape(), "convolveOneDimension(): Output array has wrong shape.");
    
    for(int k=0;k<volume.shape(ndim-1);++k)
	{
    	MultiArrayView<ndim-1, VoxelType, StridedArrayTag> bvolume = volume.bindOuter(k);
    	MultiArrayView<ndim-1, VoxelType, StridedArrayTag> bres = res.bindOuter(k);
    	convolveMultiArrayOneDimension(srcMultiArrayRange(bvolume), destMultiArray(bres), dim, kernel);
	}
    return res;
}

template < class VoxelType, unsigned int ndim >
NumpyAnyArray 
pythonSeparableConvolveND_1Kernel(NumpyArray<ndim, Multiband<VoxelType> > volume,
                                  Kernel const & kernel,
                                  NumpyArray<ndim, Multiband<VoxelType> > res=python::object())
{
    res.reshapeIfEmpty(volume.shape(), "convolve(): Output array has wrong shape.");
    
    for(int k=0;k<volume.shape(ndim-1);++k)
    {
    	MultiArrayView<ndim-1, VoxelType, StridedArrayTag> bvolume = volume.bindOuter(k);
    	MultiArrayView<ndim-1, VoxelType, StridedArrayTag> bres = res.bindOuter(k);
    	separableConvolveMultiArray(srcMultiArrayRange(bvolume), destMultiArray(bres), kernel);
    }
    return res;
}

template < class VoxelType, unsigned int ndim >
NumpyAnyArray 
pythonSeparableConvolveND_NKernels(NumpyArray<ndim, Multiband<VoxelType> > volume,
                                   python::tuple pykernels,
                                   NumpyArray<ndim, Multiband<VoxelType> > res=python::object())
{
    if(python::len(pykernels) == 1)
        return pythonSeparableConvolveND_1Kernel(volume, 
                              python::extract<Kernel1D<KernelValueType> const &>(pykernels[0]), res);
        
    vigra_precondition(python::len(pykernels) == ndim-1,
       "convolve(): Number of kernels must be 1 or equal to the number of spatial dimensions.");
       
    res.reshapeIfEmpty(volume.shape(), "convolve(): Output array has wrong shape.");
    
    ArrayVector<Kernel1D<KernelValueType> > kernels;
    for(unsigned int k=0; k < ndim-1; ++k)
        kernels.push_back(python::extract<Kernel1D<KernelValueType> const &>(pykernels[k]));

    for(int k=0; k < volume.shape(ndim-1); ++k)
    {
    	MultiArrayView<ndim-1, VoxelType, StridedArrayTag> bvolume = volume.bindOuter(k);
    	MultiArrayView<ndim-1, VoxelType, StridedArrayTag> bres = res.bindOuter(k);
    	separableConvolveMultiArray(srcMultiArrayRange(bvolume), destMultiArray(bres), kernels.begin());
    }
    return res;
}

template <class PixelType>
NumpyAnyArray 
pythonConvolveImage(NumpyArray<3, Multiband<PixelType> > image,
                    TwoDKernel const & kernel, 
                    NumpyArray<3, Multiband<PixelType> > res = python::object())
{
	res.reshapeIfEmpty(image.shape(), "convolve(): Output array has wrong shape.");

	for(int k=0;k<image.shape(2);++k)
	{
	    MultiArrayView<2, PixelType, StridedArrayTag> bimage = image.bindOuter(k);
	    MultiArrayView<2, PixelType, StridedArrayTag> bres = res.bindOuter(k);
	    convolveImage(srcImageRange(bimage), destImage(bres),
	                  kernel2d(kernel));
	}
    return res;
}

template <class PixelType>
NumpyAnyArray 
pythonNormalizedConvolveImage(NumpyArray<3, Multiband<PixelType> > image,
                              NumpyArray<3, Multiband<PixelType> > mask,
                              TwoDKernel const & kernel, 
                              NumpyArray<3, Multiband<PixelType> > res = python::object())
{
    vigra_precondition(mask.shape(2)==1 || mask.shape(2)==image.shape(2),
               "normalizedConvolveImage(): mask image must either have 1 channel or as many as the input image");
    vigra_precondition(mask.shape(0)==image.shape(0) && mask.shape(1)==image.shape(1),
               "normalizedConvolveImage(): mask dimensions must be same as image dimensions");

    res.reshapeIfEmpty(image.shape(), "normalizedConvolveImage(): Output array has wrong shape.");

	for(int k=0;k<image.shape(2);++k)
	{
	    MultiArrayView<2, PixelType, StridedArrayTag> bimage = image.bindOuter(k);
	    MultiArrayView<2, PixelType, StridedArrayTag> bmask = mask.bindOuter(mask.shape(2)==1?0:k);
	    MultiArrayView<2, PixelType, StridedArrayTag> bres = res.bindOuter(k);
	    normalizedConvolveImage(srcImageRange(bimage), srcImage(bmask), destImage(bres),
	                            kernel2d(kernel));
	}
    return res;
}

template < class VoxelType, unsigned int ndim >
NumpyAnyArray 
pythonGaussianSmoothing(NumpyArray<ndim, Multiband<VoxelType> > volume,
                        python::tuple sigmas,
                        NumpyArray<ndim, Multiband<VoxelType> > res=python::object())
{
    unsigned int sigmaCount = python::len(sigmas);
    vigra_precondition(sigmaCount == 1 || sigmaCount == ndim-1,
       "gaussianSmoothing(): Number of kernels must be 1 or equal to the number of spatial dimensions.");
       
    ArrayVector<Kernel1D<KernelValueType> > kernels;
    for(unsigned int k=0; k < sigmaCount; ++k)
    {
        KernelValueType sigma = python::extract<KernelValueType>(sigmas[k]);        
        kernels.push_back(Kernel1D<KernelValueType>());
        kernels.back().initGaussian(sigma);
    }
    for(unsigned int k=sigmaCount; k < ndim-1; ++k)
    {
        kernels.push_back(Kernel1D<KernelValueType>(kernels.back()));
    }
    
    res.reshapeIfEmpty(volume.shape(), "gaussianSmoothing(): Output array has wrong shape.");

	{
        PyAllowThreads _pythread;
        for(int k=0;k<volume.shape(ndim-1);++k)
        {
            MultiArrayView<ndim-1, VoxelType, StridedArrayTag> bvolume = volume.bindOuter(k);
            MultiArrayView<ndim-1, VoxelType, StridedArrayTag> bres = res.bindOuter(k);
            separableConvolveMultiArray(srcMultiArrayRange(bvolume), destMultiArray(bres), kernels.begin());
        }
	}
    return res;

}

template < class VoxelType, unsigned int ndim >
NumpyAnyArray 
pythonGaussianSmoothingIsotropic(NumpyArray<ndim, Multiband<VoxelType> > volume,
                                 double sigma,
                                 NumpyArray<ndim, Multiband<VoxelType> > res=python::object())
{
    return pythonGaussianSmoothing(volume, python::make_tuple(sigma), res);
}

template < class VoxelType>
NumpyAnyArray 
pythonRecursiveGaussian(NumpyArray<3, Multiband<VoxelType> > image,
                        python::tuple sigmas,
                        NumpyArray<3, Multiband<VoxelType> > res=python::object())
{
    typedef typename NumericTraits<VoxelType>::RealPromote TmpType;

    const unsigned int ndim = 3;
    unsigned int sigmaCount = python::len(sigmas);
    vigra_precondition(sigmaCount == 1 || sigmaCount == ndim-1,
       "recursiveGaussianSmoothing(): Number of kernels must be 1 or equal to the number of spatial dimensions.");
       
    ArrayVector<double> scales;
    for(unsigned int k=0; k < sigmaCount; ++k)
    {
        scales.push_back(python::extract<double>(sigmas[k]));        
    }
    for(unsigned int k=sigmaCount; k < ndim-1; ++k)
    {
        scales.push_back(scales.back());
    }
    
    res.reshapeIfEmpty(image.shape(), "recursiveGaussianSmoothing(): Output array has wrong shape.");
    MultiArray<ndim-1, TmpType> tmp(image.bindOuter(0).shape());

    for(int k=0;k<image.shape(ndim-1);++k)
    {
    	MultiArrayView<ndim-1, VoxelType, StridedArrayTag> bimage = image.bindOuter(k);
    	MultiArrayView<ndim-1, VoxelType, StridedArrayTag> bres = res.bindOuter(k);
    	recursiveGaussianFilterX(srcImageRange(bimage), destImage(tmp), scales[0]);
    	recursiveGaussianFilterY(srcImageRange(tmp), destImage(bres), scales[1]);
    }
    return res;

}

template < class VoxelType >
NumpyAnyArray 
pythonRecursiveGaussianIsotropic(NumpyArray<3, Multiband<VoxelType> > image,
                                 double sigma,
                                 NumpyArray<3, Multiband<VoxelType> > res=python::object())
{
    return pythonRecursiveGaussian(image, python::make_tuple(sigma), res);
}

template <class PixelType>
NumpyAnyArray 
pythonSimpleSharpening2D(NumpyArray<3, Multiband<PixelType> > image, 
                         double sharpeningFactor,
				         NumpyArray<3, Multiband<PixelType> > res=python::object() )
{
	res.reshapeIfEmpty(image.shape(), "simpleSharpening2D(): Output array has wrong shape.");
    
    vigra_precondition(sharpeningFactor >= 0 ,
       "simpleSharpening2D(): sharpeningFactor must be >= 0.");
       
    for(int k=0;k<image.shape(2);++k)
    {
        MultiArrayView<2, PixelType, StridedArrayTag> bimage = image.bindOuter(k);
        MultiArrayView<2, PixelType, StridedArrayTag> bres = res.bindOuter(k);
        simpleSharpening(srcImageRange(bimage), destImage(bres),
                         sharpeningFactor);
    }
    return res;
}

template <class PixelType>
NumpyAnyArray 
pythonGaussianSharpening2D(NumpyArray<3, Multiband<PixelType> > image,
                           double sharpeningFactor, double scale, 
                           NumpyArray<3, Multiband<PixelType> > res=python::object() )
{
	res.reshapeIfEmpty(image.shape(), "gaussianSharpening2D(): Output array has wrong shape.");
    
	vigra_precondition(sharpeningFactor >= 0 ,
       "gaussianSharpening2D(): sharpeningFactor must be >= 0.");
    vigra_precondition(sharpeningFactor >= 0 ,
       "gaussianSharpening2D(): scale must be >= 0.");
       
    for(int k=0;k<image.shape(2);++k)
	{
	    MultiArrayView<2, PixelType, StridedArrayTag> bimage = image.bindOuter(k);
	    MultiArrayView<2, PixelType, StridedArrayTag> bres = res.bindOuter(k);
	    gaussianSharpening(srcImageRange(bimage), destImage(bres),
	                       sharpeningFactor, scale);
	}
    return res;
}

template <class PixelType, unsigned int N>
NumpyAnyArray 
pythonLaplacianOfGaussian(NumpyArray<N, Multiband<PixelType> > image,
                          double scale, 
                          NumpyArray<N, Multiband<PixelType> > res=python::object() )
{
	res.reshapeIfEmpty(image.shape(), "laplacianOfGaussian(): Output array has wrong shape.");
    
	{
        PyAllowThreads _pythread;
        for(int k=0; k<image.shape(N-1); ++k)
        {
            MultiArrayView<N-1, PixelType, StridedArrayTag> bimage = image.bindOuter(k);
            MultiArrayView<N-1, PixelType, StridedArrayTag> bres = res.bindOuter(k);
            laplacianOfGaussianMultiArray(srcMultiArrayRange(bimage), destMultiArray(bres), scale);
        }
	}
    return res;
}

template <class PixelType>
NumpyAnyArray pythonRecursiveFilter1(NumpyArray<3, Multiband<PixelType> > image,
                                     double b, BorderTreatmentMode borderTreatment, 
                                     NumpyArray<3, Multiband<PixelType> > res = python::object())
{
	res.reshapeIfEmpty(image.shape(), "recursiveFilter2D(): Output array has wrong shape.");

	for(int k=0;k<image.shape(2);++k)
	{
	    MultiArrayView<2, PixelType, StridedArrayTag> bimage = image.bindOuter(k);
	    MultiArrayView<2, PixelType, StridedArrayTag> bres = res.bindOuter(k);
	    recursiveFilterX(srcImageRange(bimage), destImage(bres), b, borderTreatment);
	    recursiveFilterY(srcImageRange(bres), destImage(bres), b, borderTreatment);
	}
    return res;
}

template <class PixelType>
NumpyAnyArray pythonRecursiveFilter2(NumpyArray<3, Multiband<PixelType> > image,
                                     double b1, double b2, 
                                     NumpyArray<3, Multiband<PixelType> > res = python::object())
{
	res.reshapeIfEmpty(image.shape(), "recursiveFilter2D(): Output array has wrong shape.");

	for(int k=0;k<image.shape(2);++k)
	{
	    MultiArrayView<2, PixelType, StridedArrayTag> bimage = image.bindOuter(k);
	    MultiArrayView<2, PixelType, StridedArrayTag> bres = res.bindOuter(k);
	    recursiveFilterX(srcImageRange(bimage), destImage(bres), b1, b2);
	    recursiveFilterY(srcImageRange(bres), destImage(bres), b1, b2);
	}
    return res;
}


template <class PixelType>
NumpyAnyArray pythonRecursiveSmooth(NumpyArray<3, Multiband<PixelType> > image,
                                    double scale, BorderTreatmentMode borderTreatment, 
                                    NumpyArray<3, Multiband<PixelType> > res = python::object())
{
    return pythonRecursiveFilter1(image, std::exp(-1.0/scale), borderTreatment, res);
}

template <class PixelType>
NumpyAnyArray pythonRecursiveGradient(NumpyArray<2, Singleband<PixelType> > image,
                                      double scale, 
                                      NumpyArray<2, TinyVector<PixelType, 2> > res = python::object())
{
	res.reshapeIfEmpty(image.shape(), "recursiveGradient2D(): Output array has wrong shape.");

    VectorComponentValueAccessor<TinyVector<PixelType, 2> > band(0);
    recursiveFirstDerivativeX(srcImageRange(image), destImage(res, band), scale);
    recursiveSmoothY(srcImageRange(res, band), destImage(res, band), scale);

    band.setIndex(1);
    recursiveSmoothX(srcImageRange(image), destImage(res, band), scale);
    recursiveFirstDerivativeY(srcImageRange(res, band), destImage(res, band), scale);
    
    return res;
}

template <class PixelType>
NumpyAnyArray pythonRecursiveLaplacian(NumpyArray<3, Multiband<PixelType> > image,
                                     double scale, 
                                     NumpyArray<3, Multiband<PixelType> > res = python::object())
{
	using namespace vigra::functor;
	
	res.reshapeIfEmpty(image.shape(), "recursiveLaplacian2D(): Output array has wrong shape.");

    MultiArrayShape<2>::type tmpShape(image.shape().begin());
    MultiArray<2, PixelType > tmp(tmpShape);
	for(int k=0;k<image.shape(2);++k)
	{
	    MultiArrayView<2, PixelType, StridedArrayTag> bimage = image.bindOuter(k);
	    MultiArrayView<2, PixelType, StridedArrayTag> bres = res.bindOuter(k);

        recursiveSecondDerivativeX(srcImageRange(bimage), destImage(bres), scale);
        recursiveSmoothY(srcImageRange(bres), destImage(bres), scale);

        recursiveSmoothX(srcImageRange(bimage), destImage(tmp), scale);
        recursiveSecondDerivativeY(srcImageRange(tmp), destImage(tmp), scale);
        
        combineTwoImages(srcImageRange(bres), srcImage(tmp), destImage(bres), Arg1()+Arg2());
	}
    return res;
}

void defineConvolutionFunctions()
{
    using namespace python;
    
    docstring_options doc_options(true, true, false);

    def("convolveOneDimension",
    	registerConverters(&pythonConvolveOneDimensionND<float,3>),
    	(arg("image"), arg("dim"), arg("kernel"), arg("out")=python::object()),
        "Convolution along a single dimension of a 2D scalar or multiband image. "
        "'kernel' must be an instance of Kernel1D.\n"
        "\n"
        "For details see convolveMultiArrayOneDimension_ in the vigra C++ documentation.\n");

    def("convolveOneDimension",
    	registerConverters(&pythonConvolveOneDimensionND<float,4>),
    	(arg("volume"), arg("dim"), arg("kernel"), arg("out")=python::object()), 
        "Likewise for a 3D scalar or multiband volume.\n");

    def("convolve", registerConverters(&pythonSeparableConvolveND_1Kernel<float,3>),
        (arg("image"), arg("kernel"), arg("out")=python::object()),
        "Convolve an image with the given 'kernel' (or kernels).\n"
        "If the input has multiple channels, the filter is applied to each channel\n"
        "independently. The function can be used in 3 different ways:\n"
        "\n"
        "* When 'kernel' is a single object of type :class:`Kernel1D`, this kernel\n"
        "  is applied along all spatial dimensions of the data (separable filtering).\n"
        "* When 'kernel' is a tuple of :class:`Kernel1D` objects, one different kernel\n"
        "  is used for each spatial dimension (separable filtering). The number of\n"
        "  kernels must equal the number of dimensions).\n"
        "* When 'kernel' is an instance of :class:`Kernel2D`, a 2-dimensional convolution\n"
        "  is performed (non-separable filtering). This is only applicable to 2D images.\n"
        "\n"
        "For details see separableConvolveMultiArray_ and "
        "|StandardConvolution.convolveImage|_ in the vigra C++ documentation.\n");

    def("convolve", registerConverters(&pythonSeparableConvolveND_1Kernel<float,4>),
        (arg("volume"), arg("kernel"), arg("out")=python::object()),
        "Convolve a volume with the same 1D kernel along all dimensions.\n");

    def("convolve", registerConverters(&pythonSeparableConvolveND_NKernels<float,3>),
        (arg("image"), arg("kernels"), arg("out")=python::object()),
        "Convolve an image with a different 1D kernel along each dimensions.\n");

    def("convolve", registerConverters(&pythonSeparableConvolveND_NKernels<float,4>),
        (arg("volume"), arg("kernels"), arg("out")=python::object()),
        "Convolve a volume with a different 1D kernel along each dimensions.\n");

    def("convolve", registerConverters(&pythonConvolveImage<float>),
        (arg("image"), arg("kernel"), arg("out") = python::object()),
        "Convolve an image with a 2D kernel.\n");

    def("normalizedConvolveImage", registerConverters(&pythonNormalizedConvolveImage<float>),
        (arg("image"), arg("mask"), arg("kernel"), arg("out") = python::object()),
        "Perform normalized convolution of an image. If the image has multiple channels, "
        "every channel is convolved independently. The 'mask' tells the algorithm "
        "whether input pixels are valid (non-zero mask value) or not. Invalid pixels "
        "are ignored in the convolution. The mask must have one channel (which is then "
        "used for all channels input channels) or as many channels as the input image.\n\n"
        "For details, see normalizedConvolveImage_ in the C++ documentation.\n");

    def("gaussianSmoothing",
        registerConverters(&pythonGaussianSmoothingIsotropic<float,3>),               
        (arg("image"), arg("sigma"), arg("out")=python::object()),
        "Perform Gaussian smoothing of a 2D or 3D scalar or multiband image.\n\n"
        "Each channel of the array is smoothed independently. "
        "If 'sigma' is a single value, an isotropic Gaussian filter at this scale is "
        "applied (i.e. each dimension is smoothed in the same way). "
        "If 'sigma' is a tuple of values, the amount of smoothing will be different "
        "for each spatial dimension. The length of the tuple must be equal to the "
        "number of spatial dimensions.\n\n"        
        "For details see gaussianSmoothing_ in the vigra C++ documentation.\n");

    def("gaussianSmoothing",
        registerConverters(&pythonGaussianSmoothing<float,3>),               
        (arg("image"), arg("sigmas"), arg("out")=python::object()),
        "Smooth image with an anisotropic Gaussian.\n");

    def("gaussianSmoothing",
        registerConverters(&pythonGaussianSmoothingIsotropic<float,4>),               
        (arg("volume"), arg("sigma"), arg("out")=python::object()),
        "Smooth volume with an isotropic Gaussian.\n");

    def("gaussianSmoothing",
        registerConverters(&pythonGaussianSmoothing<float,4>),
        (arg("volume"), arg("sigmas"), arg("out")=python::object()),
        "Smooth volume with an anisotropic Gaussian.\n");

    def("recursiveGaussianSmoothing2D",
        registerConverters(&pythonRecursiveGaussian<float>),               
        (arg("image"), arg("sigma"), arg("out")=python::object()),
        "Compute a fast approximate Gaussian smoothing of a 2D scalar or multiband image.\n\n"
        "This function uses the third-order recursive filter approximation to the "
        "Gaussian filter proposed by Young and van Vliet. "
        "Each channel of the array is smoothed independently. "
        "If 'sigma' is a single value, an isotropic Gaussian filter at this scale is "
        "applied (i.e. each dimension is smoothed in the same way). "
        "If 'sigma' is a tuple of values, the amount of smoothing will be different "
        "for each spatial dimension. The length of the tuple must be equal to the "
        "number of spatial dimensions.\n\n"        
        "For details see recursiveGaussianFilterLine_ in the vigra C++ documentation.\n");

    def("recursiveGaussianSmoothing2D",
        registerConverters(&pythonRecursiveGaussianIsotropic<float>),               
        (arg("image"), arg("sigma"), arg("out")=python::object()),
        "Compute isotropic fast approximate Gaussian smoothing.\n");

    def("simpleSharpening2D", 
        registerConverters(&pythonSimpleSharpening2D<float>),
        (arg("image"), arg("sharpeningFactor")=1.0, arg("out") = python::object()),
        "Perform simple sharpening function.\n"
        "\n"
        "For details see simpleSharpening_ in the vigra C++ documentation.\n");

    def("gaussianSharpening2D", 
        registerConverters(&pythonGaussianSharpening2D<float>),
        (arg("image"), arg("sharpeningFactor")=1.0, arg("scale")=1.0, arg("out") = python::object()),
          "Perform sharpening function with gaussian filter."
          "\n\n"
          "For details see gaussianSharpening_ in the vigra C++ documentation.\n");
          
    def("laplacianOfGaussian", 
         registerConverters(&pythonLaplacianOfGaussian<float,3>),
         (arg("image"), arg("scale") = 1.0, arg("out") = python::object()),
          "Filter scalar image with the Laplacian of Gaussian operator at the given scale.\n"
          "\n"
          "For details see laplacianOfGaussianMultiArray_ in the vigra C++ documentation.\n");

    def("laplacianOfGaussian", 
         registerConverters(&pythonLaplacianOfGaussian<float,4>),
         (arg("volume"), arg("scale") = 1.0, arg("out") = python::object()),
         "Likewise for a scalar volume.\n");

    def("recursiveFilter2D", registerConverters(&pythonRecursiveFilter1<float>),
              (arg("image"), arg("b"), arg("borderTreament") = BORDER_TREATMENT_REFLECT, arg("out") = python::object()),
              "Perform 2D convolution with a first-order recursive filter with "
              "parameter 'b' and given 'borderTreatment'. 'b' must be between -1 and 1.\n"
              "\n"
              "For details see recursiveFilterX_ and recursiveFilterY_ (which "
              "this function calls in succession) in the vigra C++ documentation.\n");

    def("recursiveFilter2D", registerConverters(&pythonRecursiveFilter2<float>),
              (arg("image"), arg("b1"), arg("b2"), arg("out") = python::object()),
              "Perform 2D convolution with a second-order recursive filter with "
              "parameters 'b1' and 'b2'. Border treatment is always BORDER_TREATMENT_REFLECT.\n"
              "\n"
              "For details see recursiveFilterX_ and recursiveFilterY_ (which "
              "this function calls in succession) in the vigra C++ documentation.\n");

    def("recursiveSmooth2D", registerConverters(&pythonRecursiveSmooth<float>),
              (arg("image"), arg("scale"), arg("borderTreament") = BORDER_TREATMENT_REFLECT, arg("out") = python::object()),
              "Calls recursiveFilter2D() with b = exp(-1/scale), which "
              "corresponds to smoothing with an exponential filter exp(-abs(x)/scale).\n"
              "\n"
              "For details see recursiveSmoothLine_ in the vigra C++ documentation.\n");

    def("recursiveGradient2D", registerConverters(&pythonRecursiveSmooth<float>),
              (arg("image"), arg("scale"), arg("out") = python::object()),
              "Compute the gradient of a scalar image using a recursive (exponential) filter "
              "at the given 'scale'. The output image (if given) must have two channels.\n"
              "\n"
              "For details see recursiveSmoothLine_ and recursiveFirstDerivativeLine_ (which "
              "this function calls internally) in the vigra C++ documentation.\n");

    def("recursiveLaplacian2D", registerConverters(&pythonRecursiveLaplacian<float>),
              (arg("image"), arg("scale"), arg("out") = python::object()),
              "Compute the gradient of a 2D scalar or multiband image using a recursive (exponential) filter "
              "at the given 'scale'. The output image (if given) must have as many channels as the input.\n"
              "\n"
              "For details see recursiveSmoothLine_ and recursiveSecondDerivativeLine_ (which "
              "this function calls internally) in the vigra C++ documentation.\n");

}

} // namespace vigra
