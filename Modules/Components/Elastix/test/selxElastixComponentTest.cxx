/*=========================================================================
 *
 *  Copyright Leiden University Medical Center, Erasmus University Medical
 *  Center and contributors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "selxSuperElastixFilterCustomComponents.h"
#include "elxParameterObject.h"

#include "selxMonolithicElastixComponent.h"
#include "selxMonolithicTransformixComponent.h"
#include "selxItkImageSinkComponent.h"
#include "selxItkImageSourceComponent.h"
#include "selxItkDisplacementFieldSinkComponent.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"


#include "selxDataManager.h"
#include "gtest/gtest.h"

namespace selx
{
class ElastixComponentTest : public ::testing::Test
{
public:

  typedef Blueprint::Pointer BlueprintPointer;
  typedef BlueprintImpl::ParameterMapType ParameterMapType;
  typedef BlueprintImpl::ParameterValueType ParameterValueType;
  typedef DataManager DataManagerType;

  /** Make a list of components to be registered for this test*/
  typedef TypeList< 
    MonolithicElastixComponent< 2, float >,
    MonolithicTransformixComponent< 2, float >,
    ItkImageSinkComponent< 2, float >,
    ItkDisplacementFieldSinkComponent< 2, float >,
    ItkImageSourceComponent< 2, float >,
    ItkImageSourceComponent< 2, unsigned char >, //for masks
    ItkImageSourceComponent< 3, double >> RegisterComponents;

  typedef itk::Image< float, 2 >              Image2DType;
  typedef itk::ImageFileReader< Image2DType > ImageReader2DType;
  typedef itk::ImageFileWriter< Image2DType > ImageWriter2DType;

  typedef itk::Image< unsigned char, 2 >      Mask2DType;
  typedef itk::ImageFileReader< Mask2DType >  MaskReader2DType;

  typedef itk::Image< itk::Vector< float, 2 >, 2 >       DisplacementImage2DType;
  typedef itk::ImageFileWriter< DisplacementImage2DType > DisplacementImageWriter2DType;

  virtual void SetUp()
  {
    Logger::Pointer logger = Logger::New();
    logger->AddStream( "cout", std::cout );
    logger->SetLogLevel( LogLevel::TRC );

    // Instantiate SuperElastixFilter before each test
    // Register the components we want to have available in SuperElastix
    superElastixFilter = SuperElastixFilterCustomComponents< RegisterComponents >::New();
    superElastixFilter->SetLogger(logger);

    dataManager = DataManagerType::New();
  }


  virtual void TearDown()
  {
    // Unregister all components after each test
    itk::ObjectFactoryBase::UnRegisterAllFactories();
    // Delete the SuperElastixFilter after each test
    superElastixFilter = nullptr;
  }


  // BlueprintImpl holds a configuration for SuperElastix
  SuperElastixFilterCustomComponents< RegisterComponents >::Pointer superElastixFilter;
  // Data manager provides the paths to the input and output data for unit tests
  DataManagerType::Pointer dataManager;
};

TEST_F( ElastixComponentTest, MonolithicElastixTransformix )
{

  /** make example blueprint configuration */
  BlueprintPointer blueprint = Blueprint::New();

  blueprint->SetComponent( "RegistrationMethod", { { "NameOfClass", { "MonolithicElastixComponent" } },
                                                   { "RegistrationSettings", { "rigid" } }, { "MaximumNumberOfIterations", { "2" } },
                                                   { "Dimensionality", { "2" } },
                                                   { "PixelType", { "float" } },
                                                   { "ResultImagePixelType", { "float" } } } );
  blueprint->SetComponent( "TransformDisplacementField", { { "NameOfClass", { "MonolithicTransformixComponent" } } } );

  blueprint->SetComponent( "FixedImageSource", { { "NameOfClass", { "ItkImageSourceComponent" } }, { "Dimensionality", { "2" } } } );

  blueprint->SetComponent( "MovingImageSource", { { "NameOfClass", { "ItkImageSourceComponent" } }, { "Dimensionality", { "2" } } } );

  blueprint->SetComponent( "FixedMaskImageSource", { { "NameOfClass", { "ItkImageSourceComponent" } }, { "Dimensionality", { "2" } } } );

  blueprint->SetComponent( "MovingMaskImageSource", { { "NameOfClass", { "ItkImageSourceComponent" } }, { "Dimensionality", { "2" } } } );

  blueprint->SetComponent( "ResultImageSink", { { "NameOfClass", { "ItkImageSinkComponent" } }, { "Dimensionality", { "2" } } } );

  blueprint->SetComponent( "ResultDisplacementFieldSink", { { "NameOfClass", { "ItkDisplacementFieldSinkComponent" } }, { "Dimensionality", { "2" } } });

  blueprint->SetConnection( "FixedImageSource", "RegistrationMethod", { { "NameOfInterface", { "itkImageFixedInterface" } } } ); // ;

  blueprint->SetConnection( "MovingImageSource", "RegistrationMethod", { { "NameOfInterface", { "itkImageMovingInterface" } } } ); // ;

  blueprint->SetConnection( "RegistrationMethod", "TransformDisplacementField", { { "NameOfInterface", { "elastixTransformParameterObjectInterface" } } } ); // ;

  blueprint->SetConnection( "FixedImageSource", "TransformDisplacementField", { { "NameOfInterface", { "itkImageDomainFixedInterface" } } } ); // ;

  blueprint->SetConnection( "MovingImageSource", "TransformDisplacementField", { { "NameOfInterface", { "itkImageMovingInterface" } } } ); //;

  blueprint->SetConnection( "FixedMaskImageSource", "RegistrationMethod", { { "NameOfInterface", { "itkImageFixedMaskInterface" } } } ); // ;

  blueprint->SetConnection( "MovingMaskImageSource", "RegistrationMethod", { { "NameOfInterface", { "itkImageMovingMaskInterface" } } } ); //;

  blueprint->SetConnection( "TransformDisplacementField", "ResultImageSink", { { "NameOfInterface", { "itkImageInterface" } } } ); // ;

  blueprint->SetConnection( "TransformDisplacementField", "ResultDisplacementFieldSink", { { "NameOfInterface", { "itkDisplacementFieldInterface" } } }); // ;


  // Set up the readers and writers
  auto fixedImageReader = ImageReader2DType::New();
  fixedImageReader->SetFileName( dataManager->GetInputFile( "BrainProtonDensitySliceBorder20.png" ) );

  auto movingImageReader = ImageReader2DType::New();
  movingImageReader->SetFileName( dataManager->GetInputFile( "BrainProtonDensitySliceR10X13Y17.png" ) );

  auto fixedMaskReader = MaskReader2DType::New();
  fixedMaskReader->SetFileName( dataManager->GetInputFile( "BrainProtonDensitySliceBorder20Mask.png" ) );
  
  auto movingMaskReader = MaskReader2DType::New();
  movingMaskReader->SetFileName(dataManager->GetInputFile("BrainProtonDensitySliceBorder20Mask.png")); // same as fixedmask: good enough for unit test, but probably bad practice for registration.  

  auto resultImageWriter = ImageWriter2DType::New();
  resultImageWriter->SetFileName( dataManager->GetOutputFile( "ElastixComponentTest_BrainProtonDensity.mhd" ) );

  DisplacementImageWriter2DType::Pointer resultDisplacementWriter = DisplacementImageWriter2DType::New();
  resultDisplacementWriter->SetFileName(dataManager->GetOutputFile("MonolithicElastixTransformix_displacement.mhd"));

  // Connect SuperElastix in an itk pipeline
  superElastixFilter->SetInput( "FixedImageSource", fixedImageReader->GetOutput() );
  superElastixFilter->SetInput( "MovingImageSource", movingImageReader->GetOutput() );
  superElastixFilter->SetInput( "FixedMaskImageSource", fixedMaskReader->GetOutput() );
  superElastixFilter->SetInput( "MovingMaskImageSource", movingMaskReader->GetOutput() );

  resultImageWriter->SetInput( superElastixFilter->GetOutput< Image2DType >( "ResultImageSink" ) );
  resultDisplacementWriter->SetInput(superElastixFilter->GetOutput< DisplacementImage2DType >("ResultDisplacementFieldSink"));

  EXPECT_NO_THROW( superElastixFilter->SetBlueprint( blueprint ) );

  // Update call on the writers triggers SuperElastix to configure and execute
  EXPECT_NO_THROW( resultImageWriter->Update() );
  EXPECT_NO_THROW( resultDisplacementWriter->Update() );

}
} // namespace selx
