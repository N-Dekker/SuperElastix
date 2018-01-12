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

#include "selxItkCompositeTransformComponent.h"
#include "selxCheckTemplateProperties.h"

namespace selx
{
template< class InternalComputationValueType, int Dimensionality >
ItkCompositeTransformComponent< InternalComputationValueType,
Dimensionality >::ItkCompositeTransformComponent( const std::string & name, LoggerImpl & logger ) : Superclass( name, logger )
{
  m_CompositeTransform = CompositeTransformType::New();

  //TODO: instantiating the filter in the constructor might be heavy for the use in component selector factory, since all components of the database are created during the selection process.
  // we could choose to keep the component light weighted (for checking criteria such as names and connections) until the settings are passed to the filter, but this requires an additional initialization step.
}


template< class InternalComputationValueType, int Dimensionality >
ItkCompositeTransformComponent< InternalComputationValueType, Dimensionality >::~ItkCompositeTransformComponent()
{
}


template< class InternalComputationValueType, int Dimensionality >
int
ItkCompositeTransformComponent< InternalComputationValueType, Dimensionality >
::Accept( typename MultiStageTransformInterface< InternalComputationValueType, Dimensionality >::Pointer component )
{
  // todo how do we organize the fixedtransforms?
  this->m_registrationStages.push_back( component );
  return 1;
}


template< class InternalComputationValueType, int Dimensionality >
int
ItkCompositeTransformComponent< InternalComputationValueType, Dimensionality >
::Accept( ReconnectTransformInterface::Pointer other )
{
  this->m_ReconnectTransformInterfaces.insert( other );
  return 0;
}


template< class InternalComputationValueType, int Dimensionality >
typename ItkCompositeTransformComponent< InternalComputationValueType, Dimensionality >::TransformType::Pointer
ItkCompositeTransformComponent< InternalComputationValueType, Dimensionality >::GetItkTransform()
{
  return ( typename TransformType::Pointer ) this->m_CompositeTransform;
}


template< class InternalComputationValueType, int Dimensionality >
void
ItkCompositeTransformComponent< InternalComputationValueType, Dimensionality >::RegistrationControllerStart()
{
  // Check if the names connected stages are compatible with the provided execution order
  // TODO: should we handle such component sanity checks as separate NetworkBuilder check instead of as execution stage?
  std::vector< std::string > sortedExecutionNames( this->m_ExecutionOrder ); // copy container
  std::vector< std::string > sortedStageNames;                               // empty container
  sortedStageNames.resize( sortedExecutionNames.size() );                    // allocate space

  std::transform( this->m_registrationStages.begin(), this->m_registrationStages.end(), sortedStageNames.begin(),
    [ ]( typename MultiStageTransformInterface< InternalComputationValueType, Dimensionality >::Pointer stageIterator ) {
      return stageIterator->GetComponentName();
    } );

  sort( sortedExecutionNames.begin(), sortedExecutionNames.end() );
  sort( sortedStageNames.begin(), sortedStageNames.end() );

  std::vector< std::string > mismatchNames;
  //mismatchNames.resize(sortedExecutionNames.size() + sortedStageNames.size()); // allocate space worst case, no overlap
  std::set_symmetric_difference( sortedExecutionNames.begin(), sortedExecutionNames.end(), sortedStageNames.begin(),
    sortedStageNames.end(), mismatchNames.begin() );

  if( mismatchNames.size() > 0 )
  {
    this->Error( "The names of ExecutionOrder and the connected Stages do not match for %s ", this->m_Name );
    this->Error( "Mismatch is [ " );
    for( auto const & name : mismatchNames )
    {
      this->Error( "  %s," );
    }
    this->Error( "]" );
  }

  // Perform execution flow
  for( auto const & stageName : this->m_ExecutionOrder )
  {
    auto && stageIterator = std::find_if( this->m_registrationStages.begin(),
      this->m_registrationStages.end(),
      [ stageName ]( typename MultiStageTransformInterface< InternalComputationValueType, Dimensionality >::Pointer thisStage ) {
        return thisStage->GetComponentName() == stageName;
      } );
    ( *stageIterator )->SetMovingInitialTransform( this->m_CompositeTransform );
    ( *stageIterator )->RunRegistration();

    this->m_CompositeTransform->AppendTransform( ( *stageIterator )->GetItkTransform() );
  }
  for( auto && reconnectTransformInterface : this->m_ReconnectTransformInterfaces )
  {
    reconnectTransformInterface->ReconnectTransform();
  }
  return;
}


template< class InternalComputationValueType, int Dimensionality >
bool
ItkCompositeTransformComponent< InternalComputationValueType, Dimensionality >
::MeetsCriterion( const ComponentBase::CriterionType & criterion )
{
  bool hasUndefinedCriteria( false );

  auto status = CheckTemplateProperties( this->TemplateProperties(), criterion );
  if( status == CriterionStatus::Satisfied )
  {
    return true;
  }
  else if( status == CriterionStatus::Failed )
  {
    return false;
  } // else: CriterionStatus::Unknown

  if( criterion.first == "ExecutionOrder" ) //Supports this?
  {
    // should we check on non-repeating names?
    this->m_ExecutionOrder = criterion.second;
    return true;
  }
  return hasUndefinedCriteria;
}


template< class InternalComputationValueType, int Dimensionality >
bool
ItkCompositeTransformComponent< InternalComputationValueType, Dimensionality >
::ConnectionsSatisfied()
{
  // This function overrides the default behavior, in which all accepting interfaces must be set, by allowing the some interfaces not being set.
  // TODO: see I we can reduce the amount of code with helper (meta-)functions
  if( this->InterfaceAcceptor< MultiStageTransformInterface< InternalComputationValueType, Dimensionality >>::isSet() == false )
  {
    return false;
  }
  // Allow unconnected ReconnectTransformInterface
  return true;
}
} //end namespace selx
