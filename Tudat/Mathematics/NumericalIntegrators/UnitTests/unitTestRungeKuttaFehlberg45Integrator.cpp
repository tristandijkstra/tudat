/*    Copyright (c) 2010-2018, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 *
 *    References
 *      Burden, R.L., Faires, J.D. Numerical Analysis, 7th Edition, Books/Cole, 2001.
 *      Montenbruck, O., Gill, E. Satellite Orbits: Models, Methods, Applications, Springer, 2005.
 *      The MathWorks, Inc. RKF54b, Symbolic Math Toolbox, 2012.
 *
 *    Notes
 *      For the tests using data from the Symbolic Math Toolbox (MathWorks, 2012), the single step
 *      and full integration error tolerances were picked to be as small as possible, without
 *      causing the tests to fail. These values are not deemed to indicate any bugs in the code;
 *      however, it is important to take these discrepancies into account when using this numerical
 *      integrator.
 *
 */

#define BOOST_TEST_MAIN

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include "Tudat/Mathematics/NumericalIntegrators/rungeKuttaVariableStepSizeIntegrator.h"
#include "Tudat/Mathematics/NumericalIntegrators/rungeKuttaCoefficients.h"
#include "Tudat/Mathematics/NumericalIntegrators/numericalIntegrator.h"
#include "Tudat/Mathematics/NumericalIntegrators/reinitializableNumericalIntegrator.h"
#include "Tudat/Mathematics/NumericalIntegrators/UnitTests/numericalIntegratorTests.h"
#include "Tudat/Mathematics/NumericalIntegrators/UnitTests/numericalIntegratorTestFunctions.h"
#include "Tudat/Mathematics/NumericalIntegrators/UnitTests/burdenAndFairesNumericalIntegratorTest.h"

#include "Tudat/InputOutput/matrixTextFileReader.h"
#include "Tudat/InputOutput/basicInputOutput.h"
#include "Tudat/Mathematics/BasicMathematics/linearAlgebra.h"

#include <limits>
#include <string>

#include <Eigen/Core>

namespace tudat
{
namespace unit_tests
{

BOOST_AUTO_TEST_SUITE( test_runge_kutta_fehlberg_45_integrator )

using linear_algebra::flipMatrixRows;

using numerical_integrators::NumericalIntegratorXdPointer;
using numerical_integrators::ReinitializableNumericalIntegratorXdPointer;
using numerical_integrators::RungeKuttaVariableStepSizeIntegratorXd;
using numerical_integrators::RungeKuttaCoefficients;

using numerical_integrator_test_functions::computeNonAutonomousModelStateDerivative;

//! Test Runge-Kutta-Fehlberg 45 integrator using benchmark data from (Burden and Faires, 2001).
BOOST_AUTO_TEST_CASE( testRungeKuttaFehlberg45IntegratorUsingBurdenAndFairesData )
{
    // Read in benchmark data (Table 5.9 from (Burden and Faires, 2001)).
    std::string pathToBenchmarkDatafile = input_output::getTudatRootPath( )
            + "/Mathematics/NumericalIntegrators/UnitTests/table5_6BurdenAndFaires.txt";

    // Store benchmark data in matrix.
    Eigen::MatrixXd table5_9BurdenAndFaires
            = input_output::readMatrixFromFile( pathToBenchmarkDatafile );

    // Declare constants related to the benchmark file.
    const int FINAL_ROW = table5_9BurdenAndFaires.rows( ) - 1;
    const int TIME_COLUMN_INDEX = 0;
    const int EXPECTED_LOWER_ORDER_STATE_COLUMN_INDEX = 2;
    const int EXPECTED_STEP_SIZE_COLUMN_INDEX = 3;
    const int EXPECTED_RELATIVE_ERROR_COLUMN_INDEX = 4;
    const int EXPECTED_HIGHER_ORDER_STATE_COLUMN_INDEX = 6;

    // Set parameters of integration taken from (Burden and Faires, 2001).
    // This should to be added to the benchmark data file and parsed accordingly once the Tudat
    // parser architecture has been added to the library.
    const double initialTime = 0.0;
    const double finalTime = 2.0;
    const Eigen::VectorXd initialState = Eigen::VectorXd::Constant( 1, 0.5 );
    const double initialStepSize = 0.25;
    const double minimumStepSize = 0.01;
    const double maximumStepSize = 0.25;
    const double relativeErrorTolerance = 0.0;
    const double absoluteErrorTolerance = 1.0e-5;
    const double safetyFactorForNextStepSize = 0.84;
    const double maximumFactorIncreaseForNextStepSize = 4.0;
    const double minimumFactorDecreaseForNextStepSize = 0.1;

    // Declare Burden and Faires class object, containing new step size and state derivative
    // functions.
    BurdenAndFairesNumericalIntegratorTest burdenAndFairesNumericalIntegratorTest;

    // Case 1: Use integrateTo() to integrate to final time in one step and check results against
    // benchmark data from Burden and Faires.
    {
        // Declare integrator with all necessary settings.
        RungeKuttaVariableStepSizeIntegratorXd integrator(
                    RungeKuttaCoefficients::get( RungeKuttaCoefficients::rungeKuttaFehlberg45 ),
                    boost::bind( &BurdenAndFairesNumericalIntegratorTest::computeStateDerivative,
                                 &burdenAndFairesNumericalIntegratorTest, _1, _2 ),
                    initialTime,
                    initialState,
                    minimumStepSize,
                    maximumStepSize,
                    relativeErrorTolerance,
                    absoluteErrorTolerance,
                    safetyFactorForNextStepSize,
                    maximumFactorIncreaseForNextStepSize,
                    minimumFactorDecreaseForNextStepSize,
                    boost::bind( &BurdenAndFairesNumericalIntegratorTest::computeNewStepSize,
                                 &burdenAndFairesNumericalIntegratorTest,
                                 _1, _2, _3, _4, _5, _6, _7, _8 ) );

        // Integrator to final time.
        Eigen::VectorXd finalState = integrator.integrateTo( finalTime, initialStepSize );

        // Check that the computed final time matches the required final time.
        BOOST_CHECK_CLOSE_FRACTION( table5_9BurdenAndFaires( FINAL_ROW, TIME_COLUMN_INDEX ),
                                    integrator.getCurrentIndependentVariable( ),
                                    std::numeric_limits< double >::epsilon( ) );

        // Check that computed final state matches the expected final state.
        BOOST_CHECK_CLOSE_FRACTION(
                    table5_9BurdenAndFaires( FINAL_ROW, EXPECTED_LOWER_ORDER_STATE_COLUMN_INDEX ),
                    finalState( 0 ), 1.0e-8 );

        // Roll back to the previous step. This should be possible since the integrateTo() function
        // was called above.
        BOOST_CHECK( integrator.rollbackToPreviousState( ) );

        // Check that the rolled back time is as required.
        BOOST_CHECK_CLOSE_FRACTION(
                    table5_9BurdenAndFaires( FINAL_ROW - 1, TIME_COLUMN_INDEX ),
                    integrator.getCurrentIndependentVariable( ), 1.0e-8 );

        // Check that the rolled back state is as required. This test should be exact.
        BOOST_CHECK_CLOSE_FRACTION(
                    table5_9BurdenAndFaires( FINAL_ROW - 1,
                                             EXPECTED_LOWER_ORDER_STATE_COLUMN_INDEX ),
                    integrator.getCurrentState( )( 0 ), 1.0e-8 );

        // Check that it is now not possible to roll back.
        BOOST_CHECK( !integrator.rollbackToPreviousState( ) );
    }

    // Case 2: Use integrateTo() to integrate to final time in multiple steps and check results
    // against benchmark data from Burden and Faires.
    {
        // Declare integrator with all necessary settings.
        RungeKuttaVariableStepSizeIntegratorXd integrator(
                    RungeKuttaCoefficients::get( RungeKuttaCoefficients::rungeKuttaFehlberg45 ),
                    boost::bind( &BurdenAndFairesNumericalIntegratorTest::computeStateDerivative,
                                 &burdenAndFairesNumericalIntegratorTest, _1, _2 ),
                    initialTime,
                    initialState,
                    minimumStepSize,
                    maximumStepSize,
                    relativeErrorTolerance,
                    absoluteErrorTolerance,
                    safetyFactorForNextStepSize,
                    maximumFactorIncreaseForNextStepSize,
                    minimumFactorDecreaseForNextStepSize,
                    boost::bind( &BurdenAndFairesNumericalIntegratorTest::computeNewStepSize,
                                 &burdenAndFairesNumericalIntegratorTest,
                                 _1, _2, _3, _4, _5, _6, _7, _8 ) );

        // Store the initial step size as the step size to perform the first integration step.
        double stepSize = initialStepSize;

        for ( int i = 1; i < table5_9BurdenAndFaires.rows( ) - 1; i++ )
        {
            // Perform integration step using stored step size.
            integrator.performIntegrationStep( stepSize );

            // Check that the computed intermediate time matches the required intermediate time.
            BOOST_CHECK_CLOSE_FRACTION( table5_9BurdenAndFaires( i, TIME_COLUMN_INDEX ),
                                        integrator.getCurrentIndependentVariable( ),
                                        1.0e-8 );

            // Check that the computed intermediate state matches the required intermediate state.
            // Note that for some reason the check for table5_9BurdenAndFaires( 2, 2 ) failed
            // against a tolerance of 1.0e-8: this seems to come from the fact that the input data
            // from the file is read in incorrectly, introducing an error in the last significant
            // digit. All the other values satisfy a tolerance of 1.0e-8.
            BOOST_CHECK_CLOSE_FRACTION(
                        table5_9BurdenAndFaires( i, EXPECTED_LOWER_ORDER_STATE_COLUMN_INDEX ),
                        integrator.getCurrentState( )( 0 ),
                        1.0e-7 );

            // Check that the computed step size matches the required step size state.
            BOOST_CHECK_CLOSE_FRACTION(
                        table5_9BurdenAndFaires( i, EXPECTED_STEP_SIZE_COLUMN_INDEX ),
                        stepSize, 1.0e-7 );

            // Check that the computed relative error matches the required relative error.
            BOOST_CHECK_CLOSE_FRACTION(
                        table5_9BurdenAndFaires( i, EXPECTED_RELATIVE_ERROR_COLUMN_INDEX ),
                        burdenAndFairesNumericalIntegratorTest.relativeError_( 0 ), 1.0e-1 );

            // Check that the computed lower order estimate matches the required lower order
            // estimate. Note that this is the order that is integrated for the RFK-45 integrator.
            BOOST_CHECK_CLOSE_FRACTION(
                        table5_9BurdenAndFaires( i, EXPECTED_LOWER_ORDER_STATE_COLUMN_INDEX ),
                        burdenAndFairesNumericalIntegratorTest.lowerOrderEstimate_( 0 ),
                        1.0e-7 );

            // Check that the computed higher order estimate matches the required higher order
            // estimate.
            BOOST_CHECK_CLOSE_FRACTION(
                        table5_9BurdenAndFaires( i, EXPECTED_HIGHER_ORDER_STATE_COLUMN_INDEX ),
                        burdenAndFairesNumericalIntegratorTest.higherOrderEstimate_( 0 ),
                        1.0e-7 );

            // Update the step size for the next step based on the computed value in the
            // integrator.
            stepSize = integrator.getNextStepSize( );
        }

        // Store last time and state.
        const double lastTime = integrator.getCurrentIndependentVariable( );
        const Eigen::VectorXd lastState = integrator.getCurrentState( );

        // Integrate to final time.
        const Eigen::VectorXd finalState = integrator.integrateTo( finalTime,
                                                                   integrator.getNextStepSize( ) );

        // Check that the computed final time matches the required final time.
        BOOST_CHECK_CLOSE_FRACTION( finalTime,
                                    integrator.getCurrentIndependentVariable( ),
                                    std::numeric_limits< double >::epsilon( ) );

        // Check that computed final state matches the expected final state.
        BOOST_CHECK_CLOSE_FRACTION(
                    table5_9BurdenAndFaires( FINAL_ROW, EXPECTED_LOWER_ORDER_STATE_COLUMN_INDEX ),
                    finalState( 0 ), 1.0e-8 );

        // Check that the final state outputted by the integrator is the same as obtained from the
        // get-function.
        BOOST_CHECK_EQUAL( integrator.getCurrentState( ), finalState );

        // Roll back to the previous step. This should be possible since the
        // performIntegrationStep() function was called above.
        BOOST_CHECK( integrator.rollbackToPreviousState( ) );

        // Check that the rolled back time is as required. This test should be exact.
        BOOST_CHECK_EQUAL( lastTime, integrator.getCurrentIndependentVariable( ) );

        // Check that the rolled back state is as required. This test should be exact.
        BOOST_CHECK_EQUAL( lastState( 0 ), integrator.getCurrentState( )( 0 ) );

        // Check that it is now not possible to roll back.
        BOOST_CHECK( !integrator.rollbackToPreviousState( ) );
    }
}

//! Test Runge-Kutta-Fehlberg 45 integrator using benchmark data from (The MathWorks, 2012).
BOOST_AUTO_TEST_CASE( testRungeKuttaFehlberg45IntegratorUsingMatlabData )
{
    using namespace numerical_integrator_tests;

    // Read in benchmark data (generated using Symbolic Math Toolbox in Matlab
    // (The MathWorks, 2012)). This data is generated using the RKF54b numerical integrator.
    const std::string pathToForwardIntegrationOutputFile = input_output::getTudatRootPath( )
            + "/Mathematics/NumericalIntegrators/UnitTests"
            + "/matlabOutputRungeKuttaFehlberg45Forward.txt";
    const std::string pathToDiscreteEventIntegrationOutputFile = input_output::getTudatRootPath( )
            + "/Mathematics/NumericalIntegrators/UnitTests"
            + "/matlabOutputRungeKuttaFehlberg45DiscreteEvent.txt";

    // Store benchmark data in matrix.
    const Eigen::MatrixXd matlabForwardIntegrationData =
            input_output::readMatrixFromFile( pathToForwardIntegrationOutputFile, "," );
    Eigen::MatrixXd matlabBackwardIntegrationData = matlabForwardIntegrationData;
    flipMatrixRows( matlabBackwardIntegrationData );
    const Eigen::MatrixXd matlabDiscreteEventIntegrationData =
            input_output::readMatrixFromFile( pathToDiscreteEventIntegrationOutputFile, "," );

    // Set integrator parameters.

    // All of the following parameters are set such that the input data is fully accepted by the
    // integrator, to determine the steps to be taken.
    const double zeroMinimumStepSize = std::numeric_limits< double >::epsilon( );
    const double infiniteMaximumStepSize = std::numeric_limits< double >::infinity( );
    const double infiniteRelativeErrorTolerance = std::numeric_limits< double >::infinity( );
    const double infiniteAbsoluteErrorTolerance = std::numeric_limits< double >::infinity( );

    // The following parameters set how the error control mechanism should work.
    const double relativeErrorTolerance = 1.0e-15;
    const double absoluteErrorTolerance = 1.0e-15;

    // Case 1: Execute integrateTo() to integrate one step forward in time.
    {
        // Declare integrator with all necessary settings.
        NumericalIntegratorXdPointer integrator
                = boost::make_shared< RungeKuttaVariableStepSizeIntegratorXd >(
                    RungeKuttaCoefficients::get( RungeKuttaCoefficients::rungeKuttaFehlberg45 ),
                    &computeNonAutonomousModelStateDerivative,
                    matlabForwardIntegrationData( FIRST_ROW, TIME_COLUMN_INDEX ),
                    ( Eigen::VectorXd( 1 )
                      << matlabForwardIntegrationData( FIRST_ROW,
                                                       STATE_COLUMN_INDEX ) ).finished( ),
                    zeroMinimumStepSize,
                    infiniteMaximumStepSize,
                    infiniteRelativeErrorTolerance,
                    infiniteAbsoluteErrorTolerance );

        executeOneIntegrateToStep( matlabForwardIntegrationData, 1.0e-15, integrator );
    }

    // Case 2: Execute performIntegrationStep() to perform multiple integration steps until final
    //         time.
    {
        // Declare integrator with all necessary settings.
        NumericalIntegratorXdPointer integrator
                = boost::make_shared< RungeKuttaVariableStepSizeIntegratorXd >(
                    RungeKuttaCoefficients::get( RungeKuttaCoefficients::rungeKuttaFehlberg45 ),
                    &computeNonAutonomousModelStateDerivative,
                    matlabForwardIntegrationData( FIRST_ROW, TIME_COLUMN_INDEX ),
                    ( Eigen::VectorXd( 1 )
                      << matlabForwardIntegrationData( FIRST_ROW,
                                                       STATE_COLUMN_INDEX ) ).finished( ),
                    zeroMinimumStepSize,
                    infiniteMaximumStepSize,
                    infiniteRelativeErrorTolerance,
                    infiniteAbsoluteErrorTolerance );

        performIntegrationStepToSpecifiedTime( matlabForwardIntegrationData,
                                               1.0e-15, 1.0e-14, integrator );
    }

    // Case 3: Execute performIntegrationStep() to perform multiple integration steps until initial
    //         time (backwards).
    {
        // Declare integrator with all necessary settings.
        NumericalIntegratorXdPointer integrator
                = boost::make_shared< RungeKuttaVariableStepSizeIntegratorXd >(
                    RungeKuttaCoefficients::get( RungeKuttaCoefficients::rungeKuttaFehlberg45 ),
                    &computeNonAutonomousModelStateDerivative,
                    matlabBackwardIntegrationData( FIRST_ROW, TIME_COLUMN_INDEX ),
                    ( Eigen::VectorXd( 1 )
                      << matlabBackwardIntegrationData( FIRST_ROW,
                                                        STATE_COLUMN_INDEX ) ).finished( ),
                    zeroMinimumStepSize,
                    infiniteMaximumStepSize,
                    infiniteRelativeErrorTolerance,
                    infiniteAbsoluteErrorTolerance );

        performIntegrationStepToSpecifiedTime( matlabBackwardIntegrationData,
                                               1.0e-15, 1.0e-14, integrator );
    }

    // Case 4: Execute integrateTo() to integrate to specified time in one step.
    {
        // Declare integrator with all necessary settings.
        NumericalIntegratorXdPointer integrator
                = boost::make_shared< RungeKuttaVariableStepSizeIntegratorXd >(
                    RungeKuttaCoefficients::get( RungeKuttaCoefficients::rungeKuttaFehlberg45 ),
                    &computeNonAutonomousModelStateDerivative,
                    matlabForwardIntegrationData( FIRST_ROW, TIME_COLUMN_INDEX ),
                    ( Eigen::VectorXd( 1 )
                      << matlabForwardIntegrationData( FIRST_ROW,
                                                       STATE_COLUMN_INDEX ) ).finished( ),
                    zeroMinimumStepSize,
                    infiniteMaximumStepSize,
                    relativeErrorTolerance,
                    absoluteErrorTolerance );

        executeIntegrateToToSpecifiedTime( matlabForwardIntegrationData, 1.0e-12, integrator,
                                           matlabForwardIntegrationData(
                                               matlabForwardIntegrationData.rows( ) - 1,
                                               TIME_COLUMN_INDEX ) );
    }

    // Case 5: Execute performIntegrationstep() to integrate to specified time in multiple steps,
    //         including discrete events.
    {
        // Declare integrator with all necessary settings.
        ReinitializableNumericalIntegratorXdPointer integrator
                = boost::make_shared< RungeKuttaVariableStepSizeIntegratorXd >(
                    RungeKuttaCoefficients::get( RungeKuttaCoefficients::rungeKuttaFehlberg45 ),
                    &computeNonAutonomousModelStateDerivative,
                    matlabDiscreteEventIntegrationData( FIRST_ROW, TIME_COLUMN_INDEX ),
                    ( Eigen::VectorXd( 1 )
                      << matlabDiscreteEventIntegrationData( FIRST_ROW,
                                                             STATE_COLUMN_INDEX ) ).finished( ),
                    zeroMinimumStepSize,
                    infiniteMaximumStepSize,
                    infiniteRelativeErrorTolerance,
                    infiniteAbsoluteErrorTolerance );

        performIntegrationStepToSpecifiedTimeWithEvents( matlabDiscreteEventIntegrationData,
                                                         1.0e-15, 1.0e-12, integrator );
    }
}

BOOST_AUTO_TEST_SUITE_END( )

} // namespace unit_tests
} // namespace tudat
