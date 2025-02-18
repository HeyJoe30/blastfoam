/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2020 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
20-06-2020 Jeff Heylmun     | Added use of blastFoam thermodynamics
-------------------------------------------------------------------------------
License
    This file is derivative work of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    blastMultiRegionFoam

Description

    EXPERIMENTAL

    Solver for transient, compressible fluid flow and solid heat
    conduction, with conjugate heat transfer between regions, buoyancy
    effects, turbulence, and radiation modeling. Riemann fluxes are
    used to transport the fluid phase.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "timeIntegrator.H"
#include "solidBlastThermo.H"
#include "fixedGradientFvPatchFields.H"
#include "regionProperties.H"
#include "compressibleSystem.H"
#include "solidRegionDiffNo.H"
#include "radiationModel.H"
#include "coordinateSystem.H"
#include "dynamicBlastFvMesh.H"

#include "fvModels.H"
#include "fvConstraints.H"

#include "mappedPatchSelector.H"
#include "mappedPointPatchSelector.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #define NO_CONTROL
    #define CREATE_MESH createMeshesPostProcess.H
    #include "postProcess.H"

    #include "setRootCaseLists.H"
    #include "createTime.H"
    #include "createMeshes.H"
    #include "createFields.H"
    #include "createTimeControls.H"
    #include "readSolidTimeControls.H"
    #include "compressibleMultiRegionCourantNo.H"
    #include "solidRegionDiffusionNo.H"
    #include "setInitialMultiRegionDeltaT.H"

    while (runTime.run())
    {
        #include "refineMeshes.H"

        #include "readTimeControls.H"
        #include "readSolidTimeControls.H"

        #include "compressibleMultiRegionCourantNo.H"
        #include "solidRegionDiffusionNo.H"
        #include "setMultiRegionDeltaT.H"

        runTime++;

        Info<< "Time = " << runTime.timeName() << nl << endl;

        #include "updateMeshes.H"

        #include "clearPatches.H"

        forAll(fluidRegions, i)
        {
            Info<< "\nSolving for fluid region "
                << fluidRegions[i].name() << endl;
            #include "solveFluid.H"
        }

        forAll(solidRegions, i)
        {
            Info<< "\nSolving for solid region "
                << solidRegions[i].name() << endl;
            #include "setRegionSolidFields.H"
            #include "solveSolid.H"
        }

        runTime.write();

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
