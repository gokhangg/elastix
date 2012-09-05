/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/
#ifndef __elxAffineStackTransform_hxx
#define __elxAffineStackTransform_hxx

#include "elxAffineStackTransform.h"

#include "itkImageRegionExclusionConstIteratorWithIndex.h"
#include "vnl/vnl_math.h"

namespace elastix
{

/**
* ********************* Constructor ****************************
*/
template <class TElastix>
AffineStackTransform<TElastix>
::AffineStackTransform()
{
     elxout << "Constructor" << std::endl;

} // end Constructor


template <class TElastix>
unsigned int AffineStackTransform<TElastix>
::InitializeAffineTransform()
{
    elxout << "InitializeAffineTransform" << std::endl;

    /** Initialize the m_AffineDummySubTransform */    
    this->m_AffineDummySubTransform = ReducedDimensionAffineTransformBaseType::New();
    
    /** Create stack transform. */
    this->m_AffineStackTransform = AffineStackTransformType::New();

    /** Set stack transform as current transform. */
    this->SetCurrentTransform( this->m_AffineStackTransform );

    return 0;
}

/**
 * ******************* BeforeAll ***********************
 */

template <class TElastix>
int AffineStackTransform<TElastix>
::BeforeAll( void )
{
  elxout << "BeforeAll" << std::endl;

  /** Initialize affine transform. */
  return InitializeAffineTransform();
}

/**
 * ******************* BeforeRegistration ***********************
 */

template <class TElastix>
void AffineStackTransform<TElastix>::BeforeRegistration( void )
{
  elxout << "BeforeRegistration" << std::endl;

  /** Task 1 - Set the stack transform parameters. */

  /** Determine stack transform settings. Here they are based on the fixed image. */
  const SizeType imageSize = this->GetElastix()->GetFixedImage()->GetLargestPossibleRegion().GetSize();
  this->m_NumberOfSubTransforms = imageSize[ SpaceDimension - 1 ];
  this->m_StackSpacing = this->GetElastix()->GetFixedImage()->GetSpacing()[ SpaceDimension - 1 ];
  this->m_StackOrigin = this->GetElastix()->GetFixedImage()->GetOrigin()[ SpaceDimension - 1 ];

  /** Set stack transform parameters. */
  this->m_AffineStackTransform->SetNumberOfSubTransforms( this->m_NumberOfSubTransforms );
  this->m_AffineStackTransform->SetStackOrigin( this->m_StackOrigin );
  this->m_AffineStackTransform->SetStackSpacing( this->m_StackSpacing );

  /** Initialize stack sub transforms. */
  this->m_AffineStackTransform->SetAllSubTransforms( this->m_AffineDummySubTransform );

  /** Task 2 - Set the scales. */
  this->SetScales();

  /** Task 3 - Give the registration an initial parameter-array. */
  ParametersType dummyInitialParameters( this->GetNumberOfParameters() );
  dummyInitialParameters.Fill( 0.0 );

  /** Put parameters in the registration. */
  this->m_Registration->GetAsITKBaseType()
      ->SetInitialTransformParameters( dummyInitialParameters );

  /** Task 4 - Initialize the transform */
  this->InitializeTransform();

 } // end BeforeRegistration()


/**
 * ********************* InitializeTransform ****************************
 */

template <class TElastix>
void AffineStackTransform<TElastix>::InitializeTransform()
{
   elxout << "InitializeTransform" << std::endl;

   /** Set all parameters to zero (no rotations, no translation). */
   this->m_AffineDummySubTransform->SetIdentity();

 /** Try to read CenterOfRotationIndex from parameter file,
   * which is the rotationPoint, expressed in index-values.
   */
  ReducedDimensionIndexType centerOfRotationIndex;
  ReducedDimensionInputPointType centerOfRotationPoint;

  bool centerGivenAsIndex = true;
  bool centerGivenAsPoint = true;
  for ( unsigned int i = 0; i < ReducedSpaceDimension; i++ )
  {
    /** Initialize. */
    centerOfRotationIndex[ i ] = 0;
    centerOfRotationPoint[ i ] = 0.0;

    /** Check COR index: Returns zero when parameter was in the parameter file. */
    bool foundI = this->m_Configuration->ReadParameter(
      centerOfRotationIndex[ i ], "CenterOfRotation", i, false );
    if ( !foundI )
    {
      centerGivenAsIndex &= false;
    }

    /** Check COR point: Returns zero when parameter was in the parameter file. */
    bool foundP = this->m_Configuration->ReadParameter(
      centerOfRotationPoint[ i ], "CenterOfRotationPoint", i, false );
    if ( !foundP )
    {
      centerGivenAsPoint &= false;
    }
  } // end loop over SpaceDimension

  
  /** Set the translation to zero */ 

  ReducedDimensionOutputVectorType noTranslation;
  noTranslation.Fill(0.0);
  this->m_AffineDummySubTransform->SetTranslation( noTranslation );

  /** Set the center of rotation if it was entered by the user. */
  this->m_AffineDummySubTransform->SetCenter( centerOfRotationPoint );

  /** Set all subtransforms to a copy of the dummy Translation sub transform. */
  this->m_AffineStackTransform->SetAllSubTransforms( this->m_AffineDummySubTransform );

  /** Set the initial parameters in this->m_Registration. */
  this->m_Registration->GetAsITKBaseType()->
      SetInitialTransformParameters( this->GetParameters() );


} // end InitializeTransform()


/**
 * ************************* ReadFromFile ************************
 */

template <class TElastix>
void AffineStackTransform<TElastix>
::ReadFromFile( void )
{

  elxout << "ReadFromFile" << std::endl;

  /** Read stack-spacing, stack-origin and number of sub-transforms. */
  this->GetConfiguration()->ReadParameter( this->m_NumberOfSubTransforms,
      "NumberOfSubTransforms", this->GetComponentLabel(), 0, 0 );
  this->GetConfiguration()->ReadParameter( this->m_StackOrigin,
      "StackOrigin", this->GetComponentLabel(), 0, 0 );
  this->GetConfiguration()->ReadParameter( this->m_StackSpacing,
      "StackSpacing", this->GetComponentLabel(), 0, 0 );

  ReducedDimensionInputPointType centerOfRotationPoint;
  centerOfRotationPoint.Fill( 0.0 );
  bool pointRead = false;
  bool indexRead = false;

  /** Try first to read the CenterOfRotationPoint from the
   * transform parameter file, this is the new, and preferred
   * way, since elastix 3.402.
   */
  pointRead = this->ReadCenterOfRotationPoint( centerOfRotationPoint );

  /** If this did not succeed, probably a transform parameter file
   * is trying to be read that was generated using an older elastix
   * version. Try to read it as an index, and convert to point.
   */
  if ( !pointRead )
  {
    indexRead = this->ReadCenterOfRotationIndex( centerOfRotationPoint );
  }

  if ( !pointRead && !indexRead )
  {
    xl::xout["error"] << "ERROR: No center of rotation is specified in the "
      << "transform parameter file" << std::endl;
    itkExceptionMacro( << "Transform parameter file is corrupt.")
  }

  this->m_AffineDummySubTransform->SetCenter( centerOfRotationPoint );

  /** Set stack transform parameters. */
  this->m_AffineStackTransform->SetNumberOfSubTransforms( this->m_NumberOfSubTransforms );
  this->m_AffineStackTransform->SetStackOrigin( this->m_StackOrigin );
  this->m_AffineStackTransform->SetStackSpacing( this->m_StackSpacing );


  /** Set stack subtransforms. */
  this->m_AffineStackTransform->SetAllSubTransforms( this->m_AffineDummySubTransform );

  /** Call the ReadFromFile from the TransformBase. */
  this->Superclass2::ReadFromFile();

} // end ReadFromFile()


/**
 * ************************* WriteToFile ************************
 *
 * Saves the TransformParameters as a vector and if wanted
 * also as a deformation field.
 */

template <class TElastix>
void AffineStackTransform<TElastix>
::WriteToFile( const ParametersType & param ) const
{

  elxout << "WriteToFile" << std::endl;

  /** Call the WriteToFile from the TransformBase. */
  this->Superclass2::WriteToFile( param );

  /** Add some AffineTransform specific lines. */
  xout["transpar"] << std::endl << "// AffineStackTransform specific" << std::endl;

  /** Set the precision of cout to 10. */
  xout["transpar"] << std::setprecision( 10 );

  /** Get the center of rotation point and write it to file. */
  ReducedDimensionInputPointType rotationPoint = this->m_AffineDummySubTransform->GetCenter();
  xout["transpar"] << "(CenterOfRotationPoint ";
  for ( unsigned int i = 0; i < ReducedSpaceDimension; i++ )
  {
      xout["transpar"] << rotationPoint[ i ] << " ";
  }
  xout["transpar"] << rotationPoint[ SpaceDimension - 1 ] << ")" << std::endl;

  /** Write the stack spacing, stack origin and number of sub transforms. */
  xout["transpar"] << "(StackSpacing " << this->m_AffineStackTransform->GetStackSpacing() << ")" << std::endl;
  xout["transpar"] << "(StackOrigin " << this->m_AffineStackTransform->GetStackOrigin() << ")" << std::endl;
  xout["transpar"] << "(NumberOfSubTransforms " << this->m_AffineStackTransform->GetNumberOfSubTransforms() << ")" << std::endl;

  /** Set the precision back to default value. */
  xout["transpar"] << std::setprecision(
    this->m_Elastix->GetDefaultOutputPrecision() );

} // end WriteToFile()

/**
 * ************************* SetScales *********************
 */

template <class TElastix>
void AffineStackTransform<TElastix>
::SetScales( void )
{
    elxout << "SetScales" << std::endl;

  /** Create the new scales. */
  const NumberOfParametersType N = this->GetNumberOfParameters();
 // const NumberOfParametersType N = 366;
  ScalesType newscales( N );

  /** Check if automatic scales estimation is desired. */
  bool automaticScalesEstimationStackTransform = false;
  this->m_Configuration->ReadParameter( automaticScalesEstimationStackTransform,
    "AutomaticScalesEstimationStackTransform", 0 );

  if ( automaticScalesEstimationStackTransform )
  {
    elxout << "Scales are estimated automatically." << std::endl;
    this->AutomaticScalesEstimationStackTransform( newscales );
  }
  else
  {
    /** Here is an heuristic rule for estimating good values for
     * the rotation/translation scales.
     *
     * 1) Estimate the bounding box of your points (in physical units).
     * 2) Take the 3D Diagonal of that bounding box
     * 3) Multiply that by 10.0.
     * 4) use 1.0 /[ value from (3) ] as the translation scaling value.
     * 5) use 1.0 as the rotation scaling value.
     *
     * With this operation you bring the translation units
     * to the range of rotations (e.g. around -1 to 1).
     * After that, all your registration parameters are
     * in the relaxed range of -1:1. At that point you
     * can start setting your optimizer with step lengths
     * in the ranges of 0.001 if you are conservative, or
     * in the range of 0.1 if you want to live dangerously.
     * (0.1 radians is about 5.7 degrees).
     *
     * This heuristic rule is based on the naive assumption
     * that your registration may require translations as
     * large as 1/10 of the diagonal of the bounding box.
     */

    /** The first SpaceDimension * SpaceDimension number of parameters
     * represent rotations (4 in 2D and 9 in 3D).
     */

    const unsigned int rotationPart = (ReducedSpaceDimension) * (ReducedSpaceDimension);
    const unsigned int totalPart = (SpaceDimension) * (ReducedSpaceDimension);

    /** this->m_Configuration->ReadParameter() returns 0 if there is a value given
     * in the parameter-file, and returns 1 if there is no value given in the
     * parameter-file.
     * Check which option is used:
     * - Nothing given in the parameter-file: rotations are scaled by the default
     *   value 100000.0
     * - Only one scale given in the parameter-file: rotations are scaled by this
     *   value.
     * - All scales are given in the parameter-file: each parameter is assigned its
     *   own scale.
     */
    const double defaultScalingvalue = 10000.0;

    int sizeLastDimension = this->GetElastix()->GetFixedImage()->GetLargestPossibleRegion().GetSize()[SpaceDimension - 1];

    std::size_t count
      = this->m_Configuration->CountNumberOfParameterEntries( "Scales" );

    /** Check which of the above options is used. */
    if ( count == 0 )
    {
      /** In this case the first option is used. */
      newscales.Fill( defaultScalingvalue );

      /** The non-rotation scales are set to 1.0 */
      for(unsigned int i=rotationPart; i < ( totalPart * sizeLastDimension ); i=i+totalPart)
      {
          newscales[ i ] = 1.0;
          newscales[ i+1 ] = 1.0;
      }
    }

    else if ( count == 1 )
    {
      /** In this case the second option is used. */
      double scale = defaultScalingvalue;
      this->m_Configuration->ReadParameter( scale, "Scales", 0 );
      newscales.Fill( scale );

      /** The non-rotation scales are set to 1.0 */
      for(unsigned int i=rotationPart; i < ( totalPart * sizeLastDimension ); i=i+totalPart)
      {
          newscales[ i ] = 1.0;
          newscales[ i+1 ] = 1.0;
      }

    }
    else if ( count == this->GetNumberOfParameters() )
    {
      newscales.Fill( 1.0 );
      /** In this case the third option is used. */
      for ( unsigned int i = 0; i < this->GetNumberOfParameters(); i++ )
      {
        this->m_Configuration->ReadParameter( newscales[ i ], "Scales", i );
      }
    }
    else
    {
      /** In this case an error is made in the parameter-file.
       * An error is thrown, because using erroneous scales in the optimizer
       * can give unpredictable results.
       */
      itkExceptionMacro( << "ERROR: The Scales-option in the parameter-file"
        << " has not been set properly." );
    }

  } // end else: no automaticScalesEstimationStackTransform

  elxout << "Scales for transform parameters are: " << newscales << std::endl;


  /** And set the scales into the optimizer. */
  this->m_Registration->GetAsITKBaseType()->GetOptimizer()->SetScales( newscales );

} // end SetScales()


/**
 * ******************** ReadCenterOfRotationIndex *********************
 */

template <class TElastix>
bool AffineStackTransform<TElastix>
::ReadCenterOfRotationIndex( ReducedDimensionInputPointType & rotationPoint ) const
{
  /** Try to read CenterOfRotationIndex from the transform parameter
   * file, which is the rotationPoint, expressed in index-values.
   */
  ReducedDimensionIndexType centerOfRotationIndex;
  bool centerGivenAsIndex = true;
  for ( unsigned int i = 0; i < ReducedSpaceDimension; i++ )
  {
    centerOfRotationIndex[ i ] = 0;

    /** Returns zero when parameter was in the parameter file. */
    bool found = this->m_Configuration->ReadParameter(
      centerOfRotationIndex[ i ], "CenterOfRotation", i, false );
    if ( !found )
    {
      centerGivenAsIndex &= false;
    }
  }

  if ( !centerGivenAsIndex )
  {
    return false;
  }

  /** Get spacing, origin and size of the fixed image.
   * We put this in a dummy image, so that we can correctly
   * calculate the center of rotation in world coordinates.
   */
  ReducedDimensionSpacingType   spacing;
  ReducedDimensionIndexType     index;
  ReducedDimensionPointType     origin;
  ReducedDimensionSizeType      size;
  ReducedDimensionDirectionType direction;
  direction.SetIdentity();
  for ( unsigned int i = 0; i < ReducedSpaceDimension; i++ )
  {
    /** Read size from the parameter file. Zero by default, which is illegal. */
    size[ i ] = 0;
    this->m_Configuration->ReadParameter( size[ i ], "Size", i );

    /** Default index. Read index from the parameter file. */
    index[ i ] = 0;
    this->m_Configuration->ReadParameter( index[ i ], "Index", i );

    /** Default spacing. Read spacing from the parameter file. */
    spacing[ i ] = 1.0;
    this->m_Configuration->ReadParameter( spacing[ i ], "Spacing", i );

    /** Default origin. Read origin from the parameter file. */
    origin[ i ] = 0.0;
    this->m_Configuration->ReadParameter( origin[ i ], "Origin", i );

    /** Read direction cosines. Default identity */
    for ( unsigned int j = 0; j < ReducedSpaceDimension; j++ )
    {
      this->m_Configuration->ReadParameter( direction( j, i ),
        "Direction", i * ReducedSpaceDimension + j );
    }
  }

  /** Check for image size. */
  bool illegalSize = false;
  for ( unsigned int i = 0; i < ReducedSpaceDimension; i++ )
  {
    if ( size[ i ] == 0 )
    {
      illegalSize = true;
    }
  }

  if ( illegalSize )
  {
    xl::xout["error"] << "ERROR: One or more image sizes are 0!" << std::endl;
    return false;
  }

  /** Make a temporary image with the right region info,
   * so that the TransformIndexToPhysicalPoint-functions will be right.
   */
  typedef ReducedDimensionImageType DummyImageType;
  typename DummyImageType::Pointer dummyImage = DummyImageType::New();
  ReducedDimensionRegionType region;
  region.SetIndex( index );
  region.SetSize( size );
  dummyImage->SetRegions( region );
  dummyImage->SetOrigin( origin );
  dummyImage->SetSpacing( spacing );
  dummyImage->SetDirection( direction );

  /** Convert center of rotation from index-value to physical-point-value. */
  dummyImage->TransformIndexToPhysicalPoint(
    centerOfRotationIndex, rotationPoint );

  /** Successfully read centerOfRotation as Index. */
  return true;

} // end ReadCenterOfRotationIndex()


/**
 * ******************** ReadCenterOfRotationPoint *********************
 */

template <class TElastix>
bool
AffineStackTransform<TElastix>
::ReadCenterOfRotationPoint( ReducedDimensionInputPointType & rotationPoint ) const
{
  /** Try to read CenterOfRotationPoint from the transform parameter
   * file, which is the rotationPoint, expressed in world coordinates.
   */
  ReducedDimensionInputPointType centerOfRotationPoint;
  bool centerGivenAsPoint = true;
  for ( unsigned int i = 0; i < ReducedSpaceDimension; i++ )
  {
    centerOfRotationPoint[ i ] = 0.0;

    /** Returns zero when parameter was in the parameter file. */
    bool found = this->m_Configuration->ReadParameter(
      centerOfRotationPoint[ i ], "CenterOfRotationPoint", i, false );
    if ( !found )
    {
      centerGivenAsPoint &= false;
    }
  }

  if ( !centerGivenAsPoint )
  {
    return false;
  }

  /** copy the temporary variable into the output of this function,
   * if everything went ok.
   */
  rotationPoint = centerOfRotationPoint;

  /** Successfully read centerOfRotation as Point. */
  return true;

} // end ReadCenterOfRotationPoint()

} // end namespace elastix


#endif // end #ifndef __elxAffineStackTransform_hxx