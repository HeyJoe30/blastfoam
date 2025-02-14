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

#include "reactingCompressibleSystem.H"
#include "fvm.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * Static member functions * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(reactingCompressibleSystem, 0);
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::reactingCompressibleSystem::reactingCompressibleSystem
(
    const fvMesh& mesh
)
:
    timeIntegrationSystem("phaseCompressibleSystem", mesh),
    thermo_(fluidReactionThermo::New(mesh)),
    rho_
    (
        IOobject
        (
            "rho",
            mesh.time().timeName(),
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("rho", dimDensity, 0.0)
    ),
    U_
    (
        IOobject
        (
            "U",
            mesh.time().timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    ),
    p_(thermo_->p()),
    T_(thermo_->T()),
    e_(thermo_->he()),
    rhoU_
    (
        IOobject
        (
            "rhoU",
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        rho_*U_
    ),
    rhoE_
    (
        IOobject
        (
            "rhoE",
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("0", dimDensity*sqr(dimVelocity), 0.0)
    ),
    phi_
    (
        IOobject
        (
            "phi",
            mesh.time().timeName(),
            mesh
        ),
        mesh,
        dimensionedScalar("0", dimVelocity*dimArea, 0.0)
    ),
    rhoPhi_
    (
        IOobject
        (
            "rhoPhi",
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("0", dimDensity*dimVelocity*dimArea, 0.0)
    ),
    rhoUPhi_
    (
        IOobject
        (
            "rhoUPhi",
            mesh.time().timeName(),
            mesh
        ),
        mesh,
        dimensionedVector("0", dimDensity*sqr(dimVelocity)*dimArea, Zero)
    ),
    rhoEPhi_
    (
        IOobject
        (
            "rhoEPhi",
            mesh.time().timeName(),
            mesh
        ),
        mesh,
        dimensionedScalar("0", dimDensity*pow3(dimVelocity)*dimArea, 0.0)
    ),
    fluxScheme_(fluxScheme::New(mesh)),
    g_(mesh.lookupObject<uniformDimensionedVectorField>("g"))
{
    thermo_->validate("compressibleSystem", "e");
    rho_ = thermo_->rho();

    Switch useChemistry
    (
        thermo_->composition().Y().size() > 1
    );

    turbulence_.set
    (
        compressible::momentumTransportModel::New
        (
            rho_,
            U_,
            rhoPhi_,
            thermo_()
        ).ptr()
    );
    thermophysicalTransport_.set
    (
        fluidReactionThermophysicalTransportModel::New
        (
            turbulence_(),
            thermo_()
        ).ptr()
    );

    if (useChemistry)
    {
        reaction_.set
        (
            combustionModel::New
            (
                thermo_(),
                turbulence_()
            ).ptr()
        );
    }

    IOobject radIO
    (
        "radiationProperties",
        mesh.time().constant(),
        mesh
    );
    if (radIO.typeHeaderOk<IOdictionary>(true))
    {
        radiation_ = radiationModel::New(T_);
    }
    else
    {
        dictionary radDict;
        radDict.add("radiationModel", "none");
        radiation_ = radiationModel::New(radDict, T_);
    }

    modelsPtr_.set(&fvModels::New(mesh));
    constraintsPtr_.set(&fvConstraints::New(mesh));

    encode();
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::reactingCompressibleSystem::~reactingCompressibleSystem()
{}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::reactingCompressibleSystem::solve()
{
    volScalarField rhoOld(rho_);
    volVectorField rhoUOld(rhoU_);
    volScalarField rhoEOld(rhoE_);

    //- Store old values
    this->storeAndBlendOld(rhoOld);
    this->storeAndBlendOld(rhoUOld);
    this->storeAndBlendOld(rhoEOld);

    volScalarField deltaRho(fvc::div(rhoPhi_));
    volVectorField deltaRhoU(fvc::div(rhoUPhi_) - g_*rho_);
    volScalarField deltaRhoE
    (
        fvc::div(rhoEPhi_)
      - (rhoU_ & g_)
    );

    //- Store changed in mass, momentum and energy
    this->storeAndBlendDelta(deltaRho);
    this->storeAndBlendDelta(deltaRhoU);
    this->storeAndBlendDelta(deltaRhoE);


    dimensionedScalar dT = rho_.time().deltaT();
    rho_ = rhoOld - dT*deltaRho;
    rho_.correctBoundaryConditions();

    rhoU_ = rhoUOld - dT*deltaRhoU;
    rhoE_ = rhoEOld - dT*deltaRhoE;

    if (reaction_.valid())
    {
        basicSpecieMixture& composition = thermo_->composition();
        PtrList<volScalarField>& Ys = composition.Y();
        volScalarField Yt(0.0*Ys[0]);
        forAll(Ys, i)
        {
            if (this->step() == 1)
            {
                Ys[i].storeOldTime();
            }

            if (composition.solve(i))
            {
                volScalarField YOld(Ys[i]);
                this->storeAndBlendOld(YOld);

                volScalarField deltaRhoY
                (
                    fvc::div(fluxScheme_->interpolate(Ys[i], "Yi")*rhoPhi_)
                );
                this->storeAndBlendDelta(deltaRhoY);

                Ys[i] = (YOld*rhoOld - dT*deltaRhoY)/rho_;
                Ys[i].correctBoundaryConditions();

                Ys[i].max(0.0);
                Yt += Ys[i];
            }
        }
        composition.normalise();
    }
}


void Foam::reactingCompressibleSystem::postUpdate()
{
    this->decode();

    // Solve mass
    rho_.storePrevIter();
    fvScalarMatrix rhoEqn
    (
        fvm::ddt(rho_) - fvc::ddt(rho_)
     ==
        modelsPtr_->source(rho_)
    );

    constraintsPtr_->constrain(rhoEqn);

    rhoEqn.solve();

    constraintsPtr_->constrain(rho_);

    // Update internal energy
    e_ = rhoE_/rho_ - 0.5*magSqr(U_);

    // Solve momentum
    fvVectorMatrix UEqn
    (
        fvm::ddt(rho_, U_) - fvc::ddt(rhoU_)
     ==
        turbulence_->divDevTau(U_)
      + modelsPtr_->source(rho_, U_)
    );

    // Solve thermal energy
    fvScalarMatrix eEqn
    (
        fvm::ddt(rho_, e_) - fvc::ddt(rho_.prevIter(), e_)
     ==
        thermophysicalTransport_->divq(e_)
      + modelsPtr_->source(rho_, e_)
    );

    if (reaction_.valid())
    {
        Info<< "Solving reactions" << endl;
        reaction_->correct();

        basicSpecieMixture& composition = thermo_->composition();

        eEqn -= reaction_->Qdot();

        PtrList<volScalarField>& Y = composition.Y();
        volScalarField Yt(0.0*Y[0]);
        forAll(Y, i)
        {
            if (composition.solve(i))
            {
                volScalarField& Yi = Y[i];
                fvScalarMatrix YiEqn
                (
                    fvm::ddt(rho_, Yi)
                  - fvc::ddt(rho_.prevIter(), Yi)
                  + thermophysicalTransport_->divj(Yi)
                 ==
                    reaction_->R(Yi)
                  + modelsPtr_->source(rho_, Yi)
                );

                constraintsPtr_->constrain(YiEqn);
                YiEqn.solve("Yi");
                constraintsPtr_->constrain(Yi);

                Yi.max(0.0);
                Yt += Yi;
            }
        }
        composition.normalise();
    }

    // Solve momentum equation
    constraintsPtr_->constrain(UEqn);
    UEqn.solve();
    constraintsPtr_->constrain(U_);

    // Solve energy equation
    constraintsPtr_->constrain(eEqn);
    eEqn.solve();
    constraintsPtr_->constrain(e_);

    // Update conserved quantities
    rhoU_ = rho_*U_;
    rhoE_ = rho_*(e_ + 0.5*magSqr(U_));

    // Update thermo
    thermo_->correct();
    p_.ref() = rho_()/thermo_->psi()();
    if (constraintsPtr_->constrainsField(p_.name()))
    {
        constraintsPtr_->constrain(p_);
    }
    p_.correctBoundaryConditions();

    // Update density boundary conditions
    rho_.boundaryFieldRef() ==
        thermo_->psi().boundaryField()*p_.boundaryField();

    // correct turbulence
    turbulence_->correct();
}


void Foam::reactingCompressibleSystem::update()
{
    decode();
    fluxScheme_->update
    (
        rho_,
        U_,
        e_,
        p_,
        speedOfSound()(),
        phi_,
        rhoPhi_,
        rhoUPhi_,
        rhoEPhi_
    );
}


void Foam::reactingCompressibleSystem::decode()
{
    thermo_->rho() = rho_;

    U_.ref() = rhoU_()/rho_();
    U_.correctBoundaryConditions();

    rhoU_.boundaryFieldRef() = rho_.boundaryField()*U_.boundaryField();

    volScalarField E(rhoE_/rho_);
    e_.ref() = E() - 0.5*magSqr(U_());
    e_.correctBoundaryConditions();

    rhoE_.boundaryFieldRef() =
        rho_.boundaryField()
       *(
            e_.boundaryField()
          + 0.5*magSqr(U_.boundaryField())
        );

    thermo_->correct();
    p_.ref() = rho_/thermo_->psi();
    p_.correctBoundaryConditions();
    rho_.boundaryFieldRef() ==
        thermo_->psi().boundaryField()*p_.boundaryField();
}


void Foam::reactingCompressibleSystem::encode()
{
    rho_ = thermo_->rho();
    rhoU_ = rho_*U_;
    rhoE_ = rho_*(e_ + 0.5*magSqr(U_));
}


Foam::tmp<Foam::volScalarField>
Foam::reactingCompressibleSystem::speedOfSound() const
{
    return sqrt(thermo_->Cp()/(thermo_->Cv()*thermo_->psi()));
}


Foam::tmp<Foam::volScalarField> Foam::reactingCompressibleSystem::Cv() const
{
    return thermo_->Cv();
}


Foam::tmp<Foam::volScalarField> Foam::reactingCompressibleSystem::mu() const
{
    return thermo_->mu();
}


Foam::tmp<Foam::scalarField>
Foam::reactingCompressibleSystem::mu(const label patchi) const
{
    return thermo_->mu(patchi);
}

Foam::tmp<Foam::volScalarField> Foam::reactingCompressibleSystem::nu() const
{
    return thermo_->nu();
}

Foam::tmp<Foam::scalarField>
Foam::reactingCompressibleSystem::nu(const label patchi) const
{
    return thermo_->nu(patchi);
}

Foam::tmp<Foam::volScalarField>
Foam::reactingCompressibleSystem::alpha() const
{
    return thermo_->alpha();
}

Foam::tmp<Foam::scalarField>
Foam::reactingCompressibleSystem::alpha(const label patchi) const
{
    return thermo_->alpha(patchi);
}

Foam::tmp<Foam::volScalarField> Foam::reactingCompressibleSystem::alphaEff
(
    const volScalarField& alphat
) const
{
    return thermo_->alphaEff(alphat);
}

Foam::tmp<Foam::scalarField> Foam::reactingCompressibleSystem::alphaEff
(
    const scalarField& alphat,
    const label patchi
) const
{
    return thermo_->alphaEff(alphat, patchi);
}

Foam::tmp<Foam::volScalarField>
Foam::reactingCompressibleSystem::alphahe() const
{
    return thermo_->alphahe();
}

Foam::tmp<Foam::scalarField>
Foam::reactingCompressibleSystem::alphahe(const label patchi) const
{
    return thermo_->alphahe(patchi);
}

Foam::tmp<Foam::volScalarField> Foam::reactingCompressibleSystem::kappa() const
{
    return thermo_->kappa();
}

Foam::tmp<Foam::scalarField>
Foam::reactingCompressibleSystem::kappa(const label patchi) const
{
    return thermo_->kappa(patchi);
}

Foam::tmp<Foam::volScalarField> Foam::reactingCompressibleSystem::kappaEff
(
    const volScalarField& alphat
) const
{
    return thermo_->kappaEff(alphat);
}

Foam::tmp<Foam::scalarField> Foam::reactingCompressibleSystem::kappaEff
(
    const scalarField& alphat,
    const label patchi
) const
{
    return thermo_->kappaEff(alphat, patchi);
}
// ************************************************************************* //
