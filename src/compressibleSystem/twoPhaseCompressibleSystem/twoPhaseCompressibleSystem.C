/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2019-2021
     \\/     M anipulation  | Synthetik Applied Technologies
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

\*---------------------------------------------------------------------------*/

#include "twoPhaseCompressibleSystem.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(twoPhaseCompressibleSystem, 0);
    addToRunTimeSelectionTable
    (
        compressibleSystem,
        twoPhaseCompressibleSystem,
        dictionary
    );
}


// * * * * * * * * * * * * Private Members Functions * * * * * * * * * * * * //

void Foam::twoPhaseCompressibleSystem::setModels()
{
    if (this->isDict("sigma"))
    {
        interfacePropertiesPtr_.set
        (
            new interfaceProperties
            (
                alpha1_,
                alpha2_,
                U_,
                *this
            )
        );
    }
    compressibleBlastSystem::setModels();
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::twoPhaseCompressibleSystem::twoPhaseCompressibleSystem
(
    const fvMesh& mesh
)
:
    compressibleBlastSystem(2, mesh),
    thermo_
    (
        refCast<twoPhaseFluidBlastThermo>(thermoPtr_())
    ),
    alpha1_(thermo_.alpha1()),
    rho1_(thermo_.thermo(0).rho()),
    alpha2_(thermo_.alpha2()),
    rho2_(thermo_.thermo(1).rho()),
    alphaRho1_
    (
        IOobject
        (
            IOobject::groupName("alphaRho", rho1_.group()),
            mesh.time().timeName(),
            mesh
        ),
        alpha1_*rho1_
    ),
    alphaRho2_
    (
        IOobject
        (
            IOobject::groupName("alphaRho", rho2_.group()),
            mesh.time().timeName(),
            mesh
        ),
        alpha2_*rho2_
    ),
    alphaPhi_
    (
        IOobject
        (
            IOobject::groupName("alphaPhi", alphaRho1_.group()),
            mesh.time().timeName(),
            mesh
        ),
        mesh,
        dimensionedScalar("0", dimensionSet(0, 3, -1, 0, 0), 0.0)
    ),
    alphaRhoPhi1_
    (
        IOobject
        (
            IOobject::groupName("alphaRhoPhi", alphaRho1_.group()),
            mesh.time().timeName(),
            mesh
        ),
        mesh,
        dimensionedScalar("0", dimensionSet(1, 0, -1, 0, 0), 0.0)
    ),
    alphaRhoPhi2_
    (
        IOobject
        (
            IOobject::groupName("alphaRhoPhi", alphaRho2_.group()),
            mesh.time().timeName(),
            mesh
        ),
        mesh,
        dimensionedScalar("0", dimensionSet(1, 0, -1, 0, 0), 0.0)
    )
{
    rho_ = alphaRho1_ + alphaRho2_;

    thermo_.initializeModels();
    this->setModels();

    encode();
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::twoPhaseCompressibleSystem::~twoPhaseCompressibleSystem()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::twoPhaseCompressibleSystem::update()
{
    decode();
    fluxScheme_->update
    (
        alpha1_,
        rho1_,
        rho2_,
        U_,
        e_,
        p_,
        speedOfSound()(),
        phi_,
        alphaPhi_,
        alphaRhoPhi1_,
        alphaRhoPhi2_,
        rhoPhi_,
        rhoUPhi_,
        rhoEPhi_
    );
    thermo_.update();
}


void Foam::twoPhaseCompressibleSystem::solve()
{
    //- Update changes in volume fraction and phase mass
    volScalarField deltaAlpha
    (
        fvc::div(alphaPhi_) - alpha1_*fvc::div(phi_)
    );
    volScalarField deltaAlphaRho1(fvc::div(alphaRhoPhi1_));
    volScalarField deltaAlphaRho2(fvc::div(alphaRhoPhi2_));

    this->storeAndBlendDelta(deltaAlpha);
    this->storeAndBlendDelta(deltaAlphaRho1);
    this->storeAndBlendDelta(deltaAlphaRho2);


    dimensionedScalar dT = rho_.time().deltaT();

    // Volume fraction is not scaled by change in volume because it is not
    // conserved
    this->storeAndBlendOld(alpha1_, false);
    this->storeAndBlendOld(alphaRho1_);
    this->storeAndBlendOld(alphaRho2_);
    rho_ = alphaRho1_ + alphaRho2_;
    rho_.storePrevIter();

    alpha1_ -= dT*deltaAlpha;
    alpha1_.correctBoundaryConditions();
    alpha2_ = 1.0 - alpha1_;

    alphaRho1_.storePrevIter();
    alphaRho1_ -= dT*deltaAlphaRho1;
    alphaRho1_.correctBoundaryConditions();

    alphaRho2_.storePrevIter();
    alphaRho2_ -= dT*deltaAlphaRho2;
    alphaRho2_.correctBoundaryConditions();

    rho_ = alphaRho1_ + alphaRho2_;

    thermo_.solve();

    //- Calculate deltas for momentum and energy
    volVectorField deltaRhoU
    (
        "deltaRhoU",
        fvc::div(rhoUPhi_) - g_*rho_
    );

    volScalarField deltaRhoE
    (
        "deltaRhoE",
        fvc::div(rhoEPhi_)
      - ESource()
      - (rhoU_ & g_)
    );

    // Add surface tension force
    if (interfacePropertiesPtr_.valid())
    {
        volVectorField sTF
        (
            fvc::reconstruct
            (
                interfacePropertiesPtr_->surfaceTensionForce()
              * rho_.mesh().magSf()
            )
        );
        deltaRhoU += sTF;
        deltaRhoE += U_ & sTF;
    }

    //- Store old values
    this->storeAndBlendOld(rhoU_);
    this->storeAndBlendOld(rhoE_);

    //- Store changed in momentum and energy
    this->storeAndBlendDelta(deltaRhoU);
    this->storeAndBlendDelta(deltaRhoE);

    //- Solve for momentum and energy
    rhoU_ -= cmptMultiply(dT*deltaRhoU, solutionDs_);
    rhoE_ -= dT*deltaRhoE;
}


void Foam::twoPhaseCompressibleSystem::postUpdate()
{
    if (!needPostUpdate_)
    {
        compressibleBlastSystem::postUpdate();
        return;
    }

    this->decode();

    // Solve volume fraction
    if (solveFields_.found(alpha1_.name()))
    {
        fvScalarMatrix alphaEqn
        (
            fvm::ddt(alpha1_) - fvc::ddt(alpha1_)
        ==
            modelsPtr_->source(alpha1_)
        );
        constraintsPtr_->constrain(alphaEqn);
        alphaEqn.solve();
        constraintsPtr_->constrain(alpha1_);

        alpha1_.maxMin(0.0, 1.0);
        alpha2_ = 1.0 - alpha1_;
    }
    // Solve phase 1 mass
    if (solveFields_.found(rho1_.name()))
    {
        fvScalarMatrix alphaRho1Eqn
        (
            fvm::ddt(alpha1_, rho1_) - fvc::ddt(alpha1_, rho1_)
          + fvm::ddt(thermoPtr_->residualAlpha(), rho1_)
          - fvc::ddt(thermoPtr_->residualAlpha(), rho1_)
        ==
            modelsPtr_->source(alpha1_, rho1_)
        );
        constraintsPtr_->constrain(alphaRho1Eqn);
        alphaRho1Eqn.solve();
        constraintsPtr_->constrain(rho1_);
    }
    // Solve phase 2 mass
    if (solveFields_.found(rho2_.name()))
    {
        fvScalarMatrix alphaRho2Eqn
        (
            fvm::ddt(alpha2_, rho2_) - fvc::ddt(alpha2_, rho2_)
          + fvm::ddt(thermoPtr_->residualAlpha(), rho2_)
          - fvc::ddt(thermoPtr_->residualAlpha(), rho2_)
        ==
            modelsPtr_->source(alpha2_, rho2_)
        );
        constraintsPtr_->constrain(alphaRho2Eqn);
        alphaRho2Eqn.solve();
        constraintsPtr_->constrain(rho2_);
    }

    // Update phase masses
    alphaRho1_ = alpha1_*rho1_;
    alphaRho2_ = alpha2_*rho2_;
    rho_ = alphaRho1_ + alphaRho2_;

    compressibleBlastSystem::postUpdate();
}


void Foam::twoPhaseCompressibleSystem::decode()
{
    // Calculate densities
    alpha1_.maxMin(0.0, 1.0);
    alpha1_.correctBoundaryConditions();
    alpha2_ = 1.0 - alpha1_;

    volScalarField alpha1
    (
        max(alpha1_, thermo_.thermo(0).residualAlpha())
    );
    volScalarField alpha2
    (
        max(alpha2_, thermo_.thermo(1).residualAlpha())
    );

    alphaRho1_.max(0);
    rho1_.ref() = alphaRho1_()/alpha1();
    rho1_.max(small);
    rho1_.correctBoundaryConditions();
    alphaRho1_.boundaryFieldRef() = alpha1_.boundaryField()*rho1_.boundaryField();

    alphaRho2_.max(0);
    rho2_.ref() = alphaRho2_()/alpha2();
    rho2_.max(small);
    rho2_.correctBoundaryConditions();
    alphaRho2_.boundaryFieldRef() = alpha2_.boundaryField()*rho2_.boundaryField();

    rho_ = alphaRho1_ + alphaRho2_;

    compressibleBlastSystem::decode();
}


void Foam::twoPhaseCompressibleSystem::encode()
{
    alphaRho1_ = alpha1_*rho1_;
    alphaRho2_ = alpha2_*rho2_;
    rho_ = alphaRho1_ + alphaRho2_;
    compressibleBlastSystem::encode();
}

// ************************************************************************* //
