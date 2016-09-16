#ifndef SPHERICALHARMONICACCELERATIONPARTIAL_H
#define SPHERICALHARMONICACCELERATIONPARTIAL_H


#include "Tudat/Mathematics/BasicMathematics/coordinateConversions.h"

#include "Tudat/Astrodynamics/Gravitation/sphericalHarmonicsGravityModel.h"

#include "Tudat/Astrodynamics/ReferenceFrames/referenceFrameTransformations.h"
#include "Tudat/Astrodynamics/Gravitation/sphericalHarmonicsGravityField.h"
#include "Tudat/Astrodynamics/OrbitDetermination/AccelerationPartials/accelerationPartial.h"
#include "Tudat/Astrodynamics/OrbitDetermination/AccelerationPartials/sphericalHarmonicAccelerationPartial.h"
#include "Tudat/Astrodynamics/OrbitDetermination/ObservationPartials/rotationMatrixPartial.h"



namespace tudat
{

namespace orbit_determination
{

namespace partial_derivatives
{

//! Class for calculating partial derivatives of a spherical harmonic gravitational acceleration.
/*!
 *  Class for calculating partial derivatives of a spherical harmonic gravitational acceleration, as calculated by the
 *  SphericalHarmonicsGravitationalAccelerationModel class.
 */
class SphericalHarmonicsGravityPartial: public AccelerationPartial
{
public:

    //! Contructor.
    /*!
     *  Constructor, requires input on the acceleration model as well as the gravity field from which the spherical harmonic coefficients
     *  are calculated. Additionally, if any partials of paramaters of tidal models are to be calculated, interfaces for these partials
     *  are to be pre-constructed and passed to this constructor, one TidalLoveNumberPartialInterface class per tide model identifier.
     *  Similarly, if any partials of parameters of the rotation model of the body exerting acceleration are to be calculated,
     *  RotationMatrixPartial objects must be pre-constructed and passed here as a map, with one object for each parameter wrt which a
     *  partial is to be taken.
     *  \param acceleratedBody Name of body undergoing acceleration.
     *  \param acceleratingBody Name of body exerting acceleration.
     *  \param accelerationModel Spherical harmonic gravity acceleration model from which acceleration is calculated wrt which the object
     *  being constructed is to calculate partials.
     *  \param gravityField Spherical harmonic gravity field of the body exerting the acceleration.
     *  \param rotationMatrixPartials Map of RotationMatrixPartial, one for each paramater representing a property of the rotation of the
     *  body exerting the acceleration wrt which an acceleration partial will be calculated.
     */
    SphericalHarmonicsGravityPartial(
            const std::string& acceleratedBody,
            const std::string& acceleratingBody,
            const boost::shared_ptr< gravitation::SphericalHarmonicsGravitationalAccelerationModel > accelerationModel,
            const std::map< std::pair< estimatable_parameters::EstimatebleParametersEnum, std::string >, boost::shared_ptr< observation_partials::RotationMatrixPartial > >&
            rotationMatrixPartials = ( std::map< std::pair< estimatable_parameters::EstimatebleParametersEnum, std::string >,
            boost::shared_ptr< observation_partials::RotationMatrixPartial > >( ) ));

    SphericalHarmonicsGravityPartial(
            const boost::shared_ptr< SphericalHarmonicsGravityPartial > originalAccelerationPartial );

    //! Function for calculating the partial of the acceleration w.r.t. the position of body undergoing acceleration..
    /*!
     *  Function for calculating the partial of the acceleration w.r.t. the position of body undergoing acceleration
     *  and adding it to the existing partial block
     *  Update( ) function must have been called during current time step before calling this function.
     *  \param partialMatrix Block of partial derivatives of acceleration w.r.t. Cartesian position of body
     *  undergoing acceleration where current partial is to be added.
     *  \param addContribution Variable denoting whether to return the partial itself (true) or the negative partial (false).
     *  \param startRow First row in partialMatrix block where the computed partial is to be added.
     *  \param startColumn First column in partialMatrix block where the computed partial is to be added.
     */
    void wrtPositionOfAcceleratedBody(
            Eigen::Block< Eigen::MatrixXd > partialMatrix,
            const bool addContribution = 1, const int startRow = 0, const int startColumn = 0 )
    {
        if( addContribution )
        {
            partialMatrix.block( startRow, startColumn, 3, 3 ) += currentPartialWrtPosition_;
        }
        else
        {
            partialMatrix.block( startRow, startColumn, 3, 3 ) -= currentPartialWrtPosition_;
        }
    }

    //! Function for calculating the partial of the acceleration w.r.t. the velocity of body undergoing acceleration..
    /*!
     *  Function for calculating the partial of the acceleration w.r.t. the velocity of body undergoing acceleration and
     *  adding it to the existing partial block.
     *  The update( ) function must have been called during current time step before calling this function.
     *  \param partialMatrix Block of partial derivatives of acceleration w.r.t. Cartesian position of body
     *  exerting acceleration where current partial is to be added.
     *  \param addContribution Variable denoting whether to return the partial itself (true) or the negative partial (false).
     *  \param startRow First row in partialMatrix block where the computed partial is to be added.
     *  \param startColumn First column in partialMatrix block where the computed partial is to be added.
     */
    void wrtPositionOfAcceleratingBody( Eigen::Block< Eigen::MatrixXd > partialMatrix,
                                        const bool addContribution = 1, const int startRow = 0, const int startColumn = 0 )
    {
        if( addContribution )
        {
            partialMatrix.block( startRow, startColumn, 3, 3 ) -= currentPartialWrtPosition_;
        }
        else
        {
            partialMatrix.block( startRow, startColumn, 3, 3 ) += currentPartialWrtPosition_;
        }
    }


    //! Function for determining if the acceleration is dependent on a non-translational integrated state.
    /*!
     *  Function for determining if the acceleration is dependent on a non-translational integrated state.
     *  No dependency is implemented, but a warning is provided if partial w.r.t. mass of body exerting acceleration
     *  (and undergoing acceleration if mutual attraction is used) is requested.
     *  \param stateReferencePoint Reference point id of propagated state
     *  \param integratedStateType Type of propagated state for which dependency is to be determined.
     *  \return True if dependency exists (non-zero partial), false otherwise.
     */
    bool isStateDerivativeDependentOnIntegratedNonTranslationalState(
                const std::pair< std::string, std::string >& stateReferencePoint,
                const propagators::IntegratedStateType integratedStateType )
    {
        if( ( ( stateReferencePoint.first == acceleratingBody_ ||
              ( stateReferencePoint.first == acceleratedBody_  && accelerationUsesMutualAttraction_ ) )
              && integratedStateType == propagators::body_mass_state ) )
        {
            throw std::runtime_error( "Warning, dependency of central gravity on body masses not yet implemented" );
        }
        return 0;
    }

    //! Function for setting up and retrieving a function returning a partial w.r.t. a double parameter.
    /*!
     *  Function for setting up and retrieving a function returning a partial w.r.t. a double parameter.
     *  Function returns empty function and zero size indicator for parameters with no dependency for current acceleration.
     *  \param parameter Parameter w.r.t. which partial is to be taken.
     *  \return Pair of parameter partial function and number of columns in partial (0 for no dependency, 1 otherwise).
     */
    std::pair< boost::function< void( Eigen::MatrixXd& ) >, int >
    getParameterPartialFunction( boost::shared_ptr< estimatable_parameters::EstimatableParameter< double > > parameter );

    //! Function for setting up and retrieving a function returning a partial w.r.t. a vector parameter.
    /*!
     *  Function for setting up and retrieving a function returning a partial w.r.t. a vector parameter.
     *  Function returns empty function and zero size indicator for parameters with no dependency for current acceleration.
     *  \param parameter Parameter w.r.t. which partial is to be taken.
     *  \return Pair of parameter partial function and number of columns in partial (0 for no dependency).
     */
    std::pair< boost::function< void( Eigen::MatrixXd& ) >, int > getParameterPartialFunction(
            boost::shared_ptr< estimatable_parameters::EstimatableParameter< Eigen::VectorXd > > parameter );


    virtual std::pair< boost::function< void( Eigen::MatrixXd& ) >, int > getGravitationalParameterPartialFunction(
            const estimatable_parameters::EstimatebleParameterIdentifier& parameterId );

    //! Function for updating the partial object to current state and time.
    /*!
     *  Function for updating the partial object to current state and time. Calculates the variables that are used for the calculation of multple
     *  partials, to prevent multiple calculations of same function.
     *  \param currentTime Time to which object is to be updated (note that most update functions are time-independent, since the 'current' state
     *  of the bodies is typically updated globally by the NBodyStateDerivative class).
     */
    virtual void update( const double currentTime = TUDAT_NAN );


    //! Function to calculate the partial wrt the gravitational parameter.
    /*!
     *  Function to calculate the partial wrt the gravitational parameter of the central body. Note that in the case of mutual attraction (see
     *  SphericalHarmonicsGravitationalAccelerationModel), the partial wrt the gravitational parameter of the body exereting acceleration is equal
     *  to the partial wrt to the body undergoing the acceleration, which is zero otherwise.
     *  \return Partial wrt the gravitational parameter of the central body.
     */
    void wrtGravitationalParameterOfCentralBody( Eigen::MatrixXd& partialMatrix )
    {
        if( gravitationalParameterFunction_( ) != 0.0 )
        {
            partialMatrix = accelerationFunction_( ) / gravitationalParameterFunction_( );
        }
        else
        {
            throw std::runtime_error( "Error cannot compute partial of spherical harminic gravity w.r.t mu for zero value" );
        }
    }


    boost::function< double( ) > getGravitationalParameterFunction( )
    {
        return gravitationalParameterFunction_;
    }
\
    boost::function< double( ) > getBodyReferenceRadiusFunction( )
    {
        return bodyReferenceRadius_;
    }

    boost::function< Eigen::MatrixXd( ) > gteCosineCoefficientsFunction( )
    {
        return cosineCoefficients_;
    }

    boost::function< Eigen::MatrixXd( ) > getSineCoefficientsFunction( )
    {
        return sineCoefficients_;
    }


    boost::function< Eigen::Vector3d( ) > getPositionFunctionOfAcceleratedBody( )
    {
        return positionFunctionOfAcceleratedBody_;
    }

    boost::function< Eigen::Vector3d( ) > getPositionFunctionOfAcceleratingBody( )
    {
        return positionFunctionOfAcceleratingBody_;
    }

    boost::function< Eigen::Matrix3d( ) > getToBodyFixedFrameRotation( )
    {
        return fromBodyFixedToIntegrationFrameRotation_;
    }

    boost::function< void( const double ) > getUpdateFunction( )
    {
        return updateFunction_;
    }

    boost::function< Eigen::Matrix< double, 3, 1 >( ) > getAccelerationFunction( )
    {
        return accelerationFunction_;
    }

    bool getAccelerationUsesMutualAttraction( )
    {
        return accelerationUsesMutualAttraction_;
    }

    //! Map of RotationMatrixPartial, one for each relevant rotation parameter
    /*!
     *  Map of RotationMatrixPartial, one for each parameter representing a property of the rotation of the
     *  body exerting the acceleration wrt which an acceleration partial will be calculated. Map is pre-created and set through the
     *  constructor.
     */
    std::map< std::pair< estimatable_parameters::EstimatebleParametersEnum, std::string >, boost::shared_ptr< observation_partials::RotationMatrixPartial > >
    getRotationMatrixPartials( )
    {
        return rotationMatrixPartials_;
    }

    Eigen::Matrix3d getCurrentPartialWrtPosition( )
    {
        return currentPartialWrtPosition_;
    }

    Eigen::Matrix3d getCurrentPartialWrtVelocity( )
    {
        return currentPartialWrtVelocity_;
    }

protected:


    void resetTimeOfMemberObjects( )
    {

    }

    virtual void updateParameterPartialsOfMemberObjects( )
    {

    }

    //! Function to calculate the partial of the acceleration wrt a set of cosine coefficients.
    /*!
     *  Function to calculate the partial of the acceleration wrt a set of cosine coefficients. The set of coefficients wrt which a partial is to
     *  be taken is provided as input.
     *  \param blockIndices List of cosine coefficient indices wrt which the partials are to be taken. The key of the map is the degree, the
     *  the value is the start order and the number of order in the current degree wrt which the partials are to be taken. For instance,
     *  an entry (4, (2, 3 ) ) means that partials a degree 4 and order 2,3 and 4 are to be calculated.
     *  \param numberOfParameters Total number of parameter wrt which a partial is to be calculated, must be consistent with blockIndices content
     *  (i.e. the sum of the second part of all values in the map). Included as input to prevent recomputation.
     *  \return Matrix of acceleration partials, with each column containg the partial wrt a single coefficient, the order of the partials is first in
     *  order of increasing degree, followed by increasing order, for instance (2,1),(2,2),(3,1),(3,2),(4,1)....
     */
    void wrtCosineCoefficientBlock(
            const std::map< int, std::pair< int, int > >& blockIndices,
            Eigen::MatrixXd& partialDerivatives );

    //! Function to calculate the partial of the acceleration wrt a set of sine coefficients.
    /*!
     *  Function to calculate the partial of the acceleration wrt a set of sine coefficients. The set of coefficients wrt which a partial is to
     *  be taken is provided as input.
     *  \param blockIndices List of sine coefficient indices wrt which the partials are to be taken. The key of the map is the degree, the
     *  the value is the start order and the number of order in the current degree wrt which the partials are to be taken. For instance,
     *  an entry (4, (2, 3 ) ) means that partials a degree 4 and order 2,3 and 4 are to be calculated.
     *  \param numberOfParameters Total number of parameter wrt which a partial is to be calculated, must be consistent with blockIndices content
     *  (i.e. the sum of the second part of all values in the map). Included as input to prevent recomputation.
     *  \return Matrix of acceleration partials, with each column containg the partial wrt a single coefficient, the order of the partials is first in
     *  order of increasing degree, followed by increasing order, for instance (2,1),(2,2),(3,1),(3,2),(4,1)....
     */
    void wrtSineCoefficientBlock(
            const std::map< int, std::pair< int, int > >& blockIndices,
            Eigen::MatrixXd& partialDerivatives );

    //! Function to calculate an acceleration partial wrt a rotational parameter.
    /*!
     *  Function to calculate an acceleration partial wrt a rotational parameter of the rotation model of the body exerting the acceleration.
     *  \param parameterType Type of parameter wrt which a partial is to be calculated. An entry of the requested type must be present in the
     *  rotationMatrixPartials_ map.
     *  \return Partial of spherical harmonic acceleration wrt a rotational parameter.
     */
    void wrtRotationModelParameter(
            Eigen::MatrixXd& accelerationPartial,
            const estimatable_parameters::EstimatebleParametersEnum parameterType,
            const std::string& secondaryIdentifier );


    //! Function to return the gravitational parameter used for calculating the acceleration.
    /*!
     *  Function to return the gravitational parameter used for calculating the acceleration.
     */
    boost::function< double( ) > gravitationalParameterFunction_;

    //! Function to return the reference radius used for calculating the acceleration.
    /*!
     *  Function to return the reference radius used for calculating the acceleration.
     */
    boost::function< double( ) > bodyReferenceRadius_;

    //! Function to return the current cosine coefficients of the spherical harmonic gravity field.
    /*!
     *  Function to return the current cosine coefficients of the spherical harmonic gravity field.
     */
    boost::function< Eigen::MatrixXd( ) > cosineCoefficients_;

    //! Function to return the current sine coefficients of the spherical harmonic gravity field.
    /*!
     *  Function to return the current sine coefficients of the spherical harmonic gravity field.
     */
    boost::function< Eigen::MatrixXd( ) > sineCoefficients_;

    //! Cache object used for storing calculated values at current time and state for spherical harmonic gravity calculations.
    /*!
     *  Cache object used for storing calculated values at current time and state for spherical harmonic gravity calculations.
     */
    boost::shared_ptr< basic_mathematics::SphericalHarmonicsCache > sphericalHarmonicCache_;

    //! Function returning position of body undergoing acceleration.
    /*!
     *  Function returning position of body undergoing acceleration.
     */
    boost::function< Eigen::Vector3d( ) > positionFunctionOfAcceleratedBody_;

    //! Function returning position of body exerting acceleration.
    /*!
     *  Function returning position of body exerting acceleration.
     */
    boost::function< Eigen::Vector3d( ) > positionFunctionOfAcceleratingBody_;

    //! Function return current rotation from inertial frame to frame fixed to body exerting acceleration.
    /*!
     *  Function return current rotation from inertial frame to frame fixed to body exerting acceleration.
     */
    boost::function< Eigen::Matrix3d( ) > fromBodyFixedToIntegrationFrameRotation_;

    //! Function to update the acceleration to the current state and time.
    /*!
     *  Function to update the acceleration to the current state and time. Called when updating an object of this class with the update( time ) function,
     *  in case the partial is called before the acceleration model in the current iteration of the numerical integration.
     */
    boost::function< void( const double ) > updateFunction_;

    //! Function to retrieve the current spherical harmonic acceleration.
    /*!
     *  Function to retrieve the current spherical harmonic acceleration.
     */
    boost::function< Eigen::Matrix< double, 3, 1 >( ) > accelerationFunction_;

    //! Current cosine coefficients of the spherical harmonic gravity field.
    /*!
     *  Current cosine coefficients of the spherical harmonic gravity field, set by update( time ) function.
     */
    Eigen::MatrixXd currentCosineCoefficients_;

    //! Current sine coefficients of the spherical harmonic gravity field.
    /*!
     *  Current sine coefficients of the spherical harmonic gravity field, set by update( time ) function.
     */
    Eigen::MatrixXd currentSineCoefficients_;

    Eigen::Vector3d bodyFixedPosition_;


    //! Current spherical coordinate of body undergoing acceleration
    /*!
     *  Current spherical coordinate of body undergoing acceleration, in reference frame fixed to body exerting acceleration.
     *  Order of components is radial distance (from center of body), latitude, longitude. Note that the the second entry differs
     *  from the direct output of the cartesian -> spherical coordinates, which produces a colatitude.
     */
    Eigen::Vector3d bodyFixedSphericalPosition_;

    //! The current partial of the acceleration wrt the position of the body undergoing the acceleration.
    /*!
     *  The current partial of the acceleration wrt the position of the body undergoing the acceleration. The partial wrt the body exerting the
     *  acceleration is minust this value. Value is set by the update( time ) function.
     */
    Eigen::Matrix3d currentPartialWrtPosition_;

    Eigen::Matrix3d currentBodyFixedPartialWrtPosition_;

    Eigen::Matrix3d currentPartialWrtVelocity_;


    //! Maximum degree of spherical harmonic expansion.
    /*!
     *  Maximum degree of spherical harmonic expansion of body exerting acceleration used in the calculation of the acceleration.
     */
    int maximumDegree_;

    //! Maximum order of spherical harmonic expansion.
    /*!
     *  Maximum order of spherical harmonic expansion of body exerting acceleration used in the calculation of the acceleration.
     */
    int maximumOrder_;

    std::vector< std::vector< std::map< int, double > > > partialConverterTermsToNormalized_;


    //! Map of RotationMatrixPartial, one for each relevant rotation parameter
    /*!
     *  Map of RotationMatrixPartial, one for each parameter representing a property of the rotation of the
     *  body exerting the acceleration wrt which an acceleration partial will be calculated. Map is pre-created and set through the
     *  constructor.
     */
    std::map< std::pair< estimatable_parameters::EstimatebleParametersEnum, std::string >, boost::shared_ptr< observation_partials::RotationMatrixPartial > > rotationMatrixPartials_;

    //! Boolean denoting whether the mutual attraction between the bodies is taken into account
    /*!
     *  Boolean denoting whether the mutual attraction between the bodies is taken into account, as is the case when integrting in a
     *  (non-rotating) frame centered on the body exerting the acceleration, in which case the gravitatiinal acceleration of the
     *  body undergoing the acceleration on that exerting the acceleration must be taken into account as an inertial effect.
     */
    bool accelerationUsesMutualAttraction_;

    bool acceleratingBodyStateDependsOnRotation_;

    boost::function< std::vector< Eigen::Matrix3d >( const double ) > partialOfRotationToBaseFrameWrtInertialRelativePositionFunction_;

};

}

}

}

#endif // SPHERICALHARMONICACCELERATIONPARTIAL_H
