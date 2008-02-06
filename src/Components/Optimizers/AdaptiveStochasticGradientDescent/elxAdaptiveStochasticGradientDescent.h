#ifndef __elxAdaptiveStochasticGradientDescent_h
#define __elxAdaptiveStochasticGradientDescent_h

#include "itkAdaptiveStochasticGradientDescentOptimizer.h"
#include "itkImageGridSampler.h"
#include "elxIncludes.h"
#include "elxProgressCommand.h"
#include "itkBSplineCombinationTransform.h"

namespace elastix
{
  using namespace itk;


  /**
  * \class AdaptiveStochasticGradientDescent
  * \brief A gradient descent optimizer with an adaptive gain.
  *
  * This class is a wrap around the AdaptiveStochasticGradientDescentOptimizer class.
  * It takes care of setting parameters and printing progress information.
  * For more information about the optimisation method, please read the documentation
  * of the AdaptiveStochasticGradientDescentOptimizer class.
  *
  * This optimizer is very suitable to be used in combination with the Random image sampler,
  * or with the RandomCoordinate image sampler, with the setting (NewSamplesEveryIteration "true"). 
  * Much effort has been spent on providing reasonable default values for all parameters, to 
  * simplify usage. In most registration problems, good results should be obtained without specifying
  * any of the parameters described below (except the first of course, which defines the optimizer
  * to use).
  *
  * The parameters used in this class are:
  * \parameter Optimizer: Select this optimizer as follows:\n
  *   <tt>(Optimizer "AdaptiveStochasticGradientDescent")</tt>
  * \parameter MaximumNumberOfIterations: The maximum number of iterations in each resolution. \n
  *   example: <tt>(MaximumNumberOfIterations 100 100 50)</tt> \n
  *    Default/recommended value: 500. When you are in a hurry, you may go down to 250 for example.
  *    When you have plenty of time, and want to be absolutely sure of the best results, a setting
  *    of 2000 is reasonable. In general, 500 gives satisfactory results.
  * \parameter AutomaticParameterEstimation: When this parameter is set to "true",
  *   many other parameters are calculated automatically: SP_a, SP_alpha, SigmoidMax,
  *   SigmoidMin, and SigmoidScale. In the elastix.log file the actually chosen values for
  *   these parameters can be found. \n
  *   example: <tt>(AutomaticParameterEstimation "true")</tt>\n
  *   Default/recommended value: "true". The parameter can be specified for each resolution,
  *   or for all resolutions at once.
  * \parameter UseAdaptiveStepSizes: When this parameter is set to "true", the adaptive
  *   step size mechanism described in the documentation of 
  *   itk::AdaptiveStochasticGradientDescentOptimizer is used.
  *   The parameter can be specified for each resolution, or for all resolutions at once.\n
  *   example: <tt>(UseAdaptiveStepSizes "true")</tt>\n
  *   Default/recommend value: "true", because it makes the registration more robust. In case
  *   of using a RandomCoordinate sampler, with (UseRandomSampleRegion "true"), the adaptive
  *   step size mechanism is turned off, no matter the user setting.
  * \parameter MaximumStepLength: Also called \f$\delta\f$. This parameter can be considered as 
  *   the maximum voxel displacement between two iterations. The larger this parameter, the 
  *   more agressive the optimization. 
  *   The parameter can be specified for each resolution, or for all resolutions at once.\n
  *   example: <tt>(MaximumStepLength 1.0)</tt>\n
  *   Default: mean voxel spacing of fixed and moving image. This seems to work well in general.
  *   This parameter only has influence when AutomaticParameterEstimation is used.
  * \parameter SP_a: The gain \f$a(k)\f$ at each iteration \f$k\f$ is defined by \n
  *   \f$a(k) =  SP\_a / (SP\_A + k + 1)^{SP\_alpha}\f$. \n
  *   SP_a can be defined for each resolution. \n
  *   example: <tt>(SP_a 3200.0 3200.0 1600.0)</tt> \n
  *   The default value is 400.0. Tuning this variable for you specific problem is recommended.
  *   Alternatively set the AutomaticParameterEstimation to "true". In that case, you do not
  *   need to specify SP_a. SP_a has no influence when AutomaticParameterEstimation is used.
  * \parameter SP_A: The gain \f$a(k)\f$ at each iteration \f$k\f$ is defined by \n
  *   \f$a(k) =  SP\_a / (SP\_A + k + 1)^{SP\_alpha}\f$. \n
  *   SP_A can be defined for each resolution. \n
  *   example: <tt>(SP_A 50.0 50.0 100.0)</tt> \n
  *   The default/recommended value for this particular optimizer is 20.0.
  * \parameter SP_alpha: The gain \f$a(k)\f$ at each iteration \f$k\f$ is defined by \n
  *   \f$a(k) =  SP\_a / (SP\_A + k + 1)^{SP\_alpha}\f$. \n
  *   SP_alpha can be defined for each resolution. \n
  *   example: <tt>(SP_alpha 0.602 0.602 0.602)</tt> \n
  *   The default/recommended value for this particular optimizer is 1.0.
  *   Alternatively set the AutomaticParameterEstimation to "true". In that case, you do not
  *   need to specify SP_alpha. SP_alpha has no influence when AutomaticParameterEstimation is used.
  * \parameter SigmoidMax: The maximum of the sigmoid function (\f$f_{max}\f$). Must be larger than 0.
  *   The parameter can be specified for each resolution, or for all resolutions at once.\n
  *   example: <tt>(SigmoidMax 1.0)</tt>\n
  *   Default/recommended value: 1.0. This parameter has no influence when AutomaticParameterEstimation
  *   is used. In that case, always a value 1.0 is used.   
  * \parameter SigmoidMin: The minimum of the sigmoid function (\f$f_{min}\f$). Must be smaller than 0.
  *   The parameter can be specified for each resolution, or for all resolutions at once.\n
  *   example: <tt>(SigmoidMin -0.8)</tt>\n
  *   Default value: -0.8. This parameter has no influence when AutomaticParameterEstimation
  *   is used. In that case, the value is automatically determined, depending on the images,
  *   metric etc.
  * \parameter SigmoidScale: The scale/width of the sigmoid function (\f$\omega\f$). 
  *   The parameter can be specified for each resolution, or for all resolutions at once.\n
  *   example: <tt>(SigmoidMin 0.00001)</tt>\n
  *   Default value: 1e-8. This parameter has no influence when AutomaticParameterEstimation
  *   is used. In that case, the value is automatically determined, depending on the images,
  *   metric etc.
  * \parameter SigmoidInitialTime: the initial time input for the sigmoid (\f$t_0\f$). Must be
  *   larger than 0.0.
  *   The parameter can be specified for each resolution, or for all resolutions at once.\n
  *   example: <tt>(SigmoidInitialTime 0.0 5.0 5.0)</tt>\n
  *   Default value: 0.0. When increased, the optimization starts with smaller steps, leaving
  *   the possibility to increase the steps when necessary. If set to 0.0, the method starts with
  *   with the largest step allowed. 
  * \parameter UseMaximumLikelihoodMethod: Experimental parameter. Leave to default setting.
  *   example: <tt>(UseMaximumLikelihood "false")</tt>\n
  *   Default/recommended value: "false".
  * \parameter SaveCovarianceMatrix: Experimental parameter. Leave to default setting. 
  *   example: <tt>(SaveCovarianceMatrix "false")</tt>\n
  *   Default/recommended value: "false".
  * \parameter NumberOfGradientMeasurements: Number of gradients N to estimate the
  *   average square magnitudes of the exact gradient and the approximation error.
  *   The parameter can be specified for each resolution, or for all resolutions at once.\n
  *   example: <tt>(NumberOfGradientMeasurements 10)</tt>\n
  *   Default value: N = max( 2, min(5, 500 / nrofparams) ), with nrofparams the 
  *   number of transform parameters. The maximum value N=5 seems to be sufficient in practice. 
  *   In principle, the more the better, but the slower.
  *   The parameter has only influence when AutomaticParameterEstimation is used.
  * \parameter NumberOfJacobianMeasurements: The number of voxels M where the jacobian is measured,
  *   which is used to estimate the covariance matrix. 
  *   The parameter can be specified for each resolution, or for all resolutions at once.\n
  *   example: <tt>(NumberOfJacobianMeasurements 5000 10000 20000)</tt>\n
  *   Default value: M = max( 1000, nrofparams*3 ), with nrofparams the 
  *   number of transform parameters. This is a rather crude rule of thumb,
  *   which seems to work in practice. In principle, the more the better, but the slower.
  *   The parameter has only influence when AutomaticParameterEstimation is used.
  * \parameter NumberOfSamplesForExactGradient: The number of image samples used to compute
  *   the 'exact' gradient. The samples are chosen on a uniform grid. 
  *   The parameter can be specified for each resolution, or for all resolutions at once.\n
  *   example: <tt>(NumberOfSamplesForExactGradient 100000)</tt>\n
  *   Default/recommended: 100000. This works in general. If the image is smaller, the number
  *   of samples is automatically reduced. In principle, the more the better, but the slower.
  *   The parameter has only influence when AutomaticParameterEstimation is used.
  *
  * \todo: this class contains a lot of functional code, which actually does not belong here.
  *
  * \sa AdaptiveStochasticGradientDescentOptimizer
  * \ingroup Optimizers
  */

  template <class TElastix>
  class AdaptiveStochasticGradientDescent :
    public
    itk::AdaptiveStochasticGradientDescentOptimizer,
    public
    OptimizerBase<TElastix>
  {
  public:

    /** Standard ITK.*/
    typedef AdaptiveStochasticGradientDescent          Self;
    typedef AdaptiveStochasticGradientDescentOptimizer Superclass1;
    typedef OptimizerBase<TElastix>             Superclass2;
    typedef SmartPointer<Self>                  Pointer;
    typedef SmartPointer<const Self>            ConstPointer;

    /** Method for creation through the object factory. */
    itkNewMacro( Self );

    /** Run-time type information (and related methods). */
    itkTypeMacro( AdaptiveStochasticGradientDescent, AdaptiveStochasticGradientDescentOptimizer );

    /** Name of this class.
    * Use this name in the parameter file to select this specific optimizer.
    * example: <tt>(Optimizer "AdaptiveStochasticGradientDescent")</tt>\n
    */
    elxClassNameMacro( "AdaptiveStochasticGradientDescent" );

    /** Typedef's inherited from Superclass1, the AdaptiveStochasticGradientDescentOptimizer.*/
    typedef Superclass1::CostFunctionType     CostFunctionType;
    typedef Superclass1::CostFunctionPointer  CostFunctionPointer;
    typedef Superclass1::StopConditionType    StopConditionType;

    /** Typedef's inherited from Superclass2, the elastix OptimizerBase .*/
    typedef typename Superclass2::ElastixType           ElastixType;
    typedef typename Superclass2::ElastixPointer        ElastixPointer;
    typedef typename Superclass2::ConfigurationType     ConfigurationType;
    typedef typename Superclass2::ConfigurationPointer  ConfigurationPointer;
    typedef typename Superclass2::RegistrationType      RegistrationType;
    typedef typename Superclass2::RegistrationPointer   RegistrationPointer;
    typedef typename Superclass2::ITKBaseType           ITKBaseType;

    /** Typedef for the ParametersType. */
    typedef typename Superclass1::ParametersType        ParametersType;

    /** Methods invoked by elastix, in which parameters can be set and 
    * progress information can be printed. */
    virtual void BeforeRegistration(void);
    virtual void BeforeEachResolution(void);
    virtual void AfterEachResolution(void);
    virtual void AfterEachIteration(void);
    virtual void AfterRegistration(void);   

    /** Check if any scales are set, and set the UseScales flag on or off; 
    * after that call the superclass' implementation */
    virtual void StartOptimization(void);

    /** If automatic gain estimation is desired, then estimate SP_a, SP_alpha
    * SigmoidScale, SigmoidMax, SigmoidMin.
    * After that call Superclass' implementation.  */
    virtual void ResumeOptimization(void);

    /** Set/Get whether automatic parameter estimation is desired. 
    * If true, make sure to set the maximum step length.
    *
    * The following parameters are automatically determined:
    * SP_a, SP_alpha (=1), SigmoidMin, SigmoidMax (=1),
    * SigmoidScale. 
    * A usually suitable value for SP_A is 25. This has
    * to be set manually though.
    * \todo: AutomaticParameterEstimation does not work in combination
    * with the MultiMetricMultiResolutionRegistration component.
    */
    itkSetMacro(AutomaticParameterEstimation, bool);
    itkGetConstMacro(AutomaticParameterEstimation, bool);

    /** Set/Get maximum step length */
    itkSetMacro( MaximumStepLength, double );
    itkGetConstMacro( MaximumStepLength, double );

  protected:

    typedef typename RegistrationType::FixedImageType   FixedImageType;
    typedef typename RegistrationType::MovingImageType  MovingImageType;
    typedef typename FixedImageType::RegionType         FixedImageRegionType;
    typedef typename FixedImageType::IndexType          FixedImageIndexType;
    typedef typename FixedImageType::PointType          FixedImagePointType;
    typedef typename RegistrationType::ITKBaseType      itkRegistrationType;
    typedef typename itkRegistrationType::TransformType TransformType;
    typedef typename TransformType::JacobianType        JacobianType;
    typedef typename JacobianType::ValueType            JacobianValueType;
    struct SettingsType { double a, A, alpha, fmax, fmin, omega; };
    typedef typename std::vector<SettingsType>          SettingsVectorType;

    typedef ImageGridSampler< FixedImageType >          ImageSamplerType;
    typedef typename ImageSamplerType::Pointer          ImageSamplerPointer;
    typedef typename 
      ImageSamplerType::ImageSampleContainerType        ImageSampleContainerType;
    typedef typename ImageSampleContainerType::Pointer  ImageSampleContainerPointer;
    typedef ProgressCommand                             ProgressCommandType;
    typedef typename ProgressCommand::Pointer           ProgressCommandPointer;
    typedef Array2D<double>                             CovarianceMatrixType; 

    /** Typedefs for support of sparse jacobians and BSplineTransforms. */
    typedef JacobianType                                          TransformJacobianType;
    itkStaticConstMacro( FixedImageDimension, unsigned int, FixedImageType::ImageDimension );
    enum { DeformationSplineOrder = 3 };
    typedef typename TransformType::ScalarType                    CoordinateRepresentationType;  
    typedef BSplineDeformableTransform<
      CoordinateRepresentationType,
      itkGetStaticConstMacro(FixedImageDimension),
      DeformationSplineOrder>                                     BSplineTransformType;
    typedef typename 
      BSplineTransformType::WeightsType                           BSplineTransformWeightsType;
    typedef typename 
      BSplineTransformType::ParameterIndexArrayType               BSplineTransformIndexArrayType;
    typedef BSplineCombinationTransform<
      CoordinateRepresentationType,
      itkGetStaticConstMacro(FixedImageDimension),
      DeformationSplineOrder>                                     BSplineCombinationTransformType;
    typedef FixedArray< unsigned long, 
      itkGetStaticConstMacro(FixedImageDimension)>                BSplineParametersOffsetType;
    /** Array type for holding parameter indices */
    typedef Array<unsigned int>                                   ParameterIndexArrayType;

    AdaptiveStochasticGradientDescent();
    virtual ~AdaptiveStochasticGradientDescent() {};

    /** Variable to store the automatically determined settings for each resolution */
    SettingsVectorType m_SettingsVector;

    /** Some options for automatic parameter estimation */
    unsigned int m_NumberOfGradientMeasurements;
    unsigned int m_NumberOfJacobianMeasurements;
    unsigned int m_NumberOfSamplesForExactGradient;
    CovarianceMatrixType m_CovarianceMatrix;
    bool m_UseMaximumLikelihoodMethod;
    bool m_SaveCovarianceMatrix;

    /** Variables used when the transform is a bspline transform. */
    bool m_TransformIsBSpline;
    bool m_TransformIsBSplineCombination;
    typename BSplineTransformType::Pointer            m_BSplineTransform;
    mutable BSplineTransformWeightsType               m_BSplineTransformWeights;
    mutable BSplineTransformIndexArrayType            m_BSplineTransformIndices;
    typename BSplineCombinationTransformType::Pointer m_BSplineCombinationTransform;
    BSplineParametersOffsetType                       m_BSplineParametersOffset;

    /** The number of BSpline parameters per image dimension. */
    long                                              m_NumBSplineParametersPerDim;

    /** The number of BSpline transform weights is the number of
    * of parameter in the support region (per dimension ). */   
    unsigned long                                     m_NumBSplineWeights;

    /** The number of transform parameters. */
    unsigned int m_NumberOfParameters;

    /** the parameter indices that have a nonzero jacobian. */
    mutable ParameterIndexArrayType                    m_NonZeroJacobianIndices;


    /** Check if the transform is a bspline transform. Called by Initialize. */
    virtual void CheckForBSplineTransform( void );

    /** This function returns a reference to a sparse transform jacobian.
    * This is either a reference to the full TransformJacobian or
    * a reference to a sparse jacobian. 
    * The m_NonZeroJacobianIndices contains the indices that are nonzero.
    * The length of NonZeroJacobianIndices is set in the CheckForBSplineTransform
    * function. */ 
    virtual const TransformJacobianType & EvaluateBSplineTransformJacobian(
      const FixedImagePointType & fixedImagePoint ) const;

    /** Print the contents of the settings vector to elxout */
    virtual void PrintSettingsVector( const SettingsVectorType & settings ) const;

    /** Save the covariance matrix in matlab format, if desired */
    virtual void SaveCovarianceMatrix( double sigma1, double sigma3, 
      const CovarianceMatrixType & cov );

    /** Estimates some reasonable values for the parameters
    * SP_a, SP_alpha (=1), SigmoidMin, SigmoidMax (=1), and
    * SigmoidScale. */
    virtual void AutomaticParameterEstimation( void );

    /** Measure some derivatives, exact and approximated. Returns
    * the squared magnitude of the gradient and approximation error.
    * Needed for the automatic parameter estimation.
    * Gradients are measured at position mu_n, which are generated according to:
    * mu_n - mu_0 ~ N(0, perturbationSigma^2 I );
    * The value returned indicates whether a maximum likelihood method was
    * used. In case of true, gg=g^T C^{-1} g. else gg = g^T g
    */
    virtual bool SampleGradients(const ParametersType & mu0,
      double perturbationSigma, double & gg, double & ee);

    /** Returns a comtainer of fixed image samples, sampled using a grid sampler
    * The grid size is determined from the user entered number of jacobian measurements,
    * or a default value of 200 is used.
    * The actual number of samples depends on the presence of masks, and 
    * the restriction that the gridspacing of the gridsampler must be integer.
    * The samples input variable contains the sample container after execution.
    * It does not have to be initialised/allocated before.
    */
    virtual void SampleFixedImageForJacobianTerms(
      ImageSampleContainerPointer & sampleContainer );


    /** Functions to compute the jacobian terms needed for the automatic
    * parameter estimation */
    virtual void ComputeJacobianTerms(double & TrC, double & TrCC, 
      double & maxJJ, double & maxJCJ );

    /** Implementation of the jacobian terms, using a method that is
    * linearly complex regarding the number of jacobian measurements.
    * The memory usage is independent on the number of jacobian measurements
    * and quadratically proportional to the number of parameters. */
    virtual void ComputeJacobianTermsGeneric(double & TrC, double & TrCC, 
      double & maxJJ, double & maxJCJ );

    /** For translation transforms, things become much simpler, since 
     * analytic expressions can be derived */
    virtual void ComputeJacobianTermsTranslation(double & TrC, double & TrCC, 
      double & maxJJ, double & maxJCJ );

    /** For B-splines, a speed up can be realised by using the compact support
     * of the B-splines, resulting in sparse jacobians */
    virtual void ComputeJacobianTermsBSpline(double & TrC, double & TrCC, 
      double & maxJJ, double & maxJCJ );

  private:

    AdaptiveStochasticGradientDescent( const Self& );  // purposely not implemented
    void operator=( const Self& );              // purposely not implemented

    bool m_AutomaticParameterEstimation;
    double m_MaximumStepLength;      

    /** This member should only be directly accessed by the
    * EvaluateBSplineTransformJacobian method. */
    mutable TransformJacobianType m_InternalTransformJacobian;


  }; // end class AdaptiveStochasticGradientDescent


} // end namespace elastix

#ifndef ITK_MANUAL_INSTANTIATION
#include "elxAdaptiveStochasticGradientDescent.hxx"
#endif

#endif // end #ifndef __elxAdaptiveStochasticGradientDescent_h
