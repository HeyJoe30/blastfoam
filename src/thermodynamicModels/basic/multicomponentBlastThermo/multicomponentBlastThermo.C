/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2020 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
2020-04-02 Jeff Heylmun:    Modified class for a density based BasicThermodynamic
                            class
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

#include "multicomponentBlastThermo.H"
#include "fvc.H"
#include "fvm.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::multicomponentBlastThermo::multicomponentBlastThermo
(
    const fvMesh& mesh,
    const dictionary& dict,
    const word& phaseName,
    const word& masterName
)
:
    basicSpecieMixture
    (
        dict,
        dict.lookup<wordList>("species"),
        mesh,
        phaseName
    ),
    masterName_(masterName),
    massTransferRates_(this->species_.size()),
    implicitSources_(this->species_.size())
{
    forAll(species_, i)
    {
        massTransferRates_.set
        (
            i,
            species_[i],
            new volScalarField
            (
                IOobject
                (
                    "massTransfer:" + IOobject::groupName(species_[i], phaseName),
                    mesh.time().timeName(),
                    mesh
                ),
                mesh,
                dimensionedScalar("0", dimDensity/dimTime, 0.0)
            )
        );
    }
}


Foam::multicomponentBlastThermo::multicomponentBlastThermo
(
    const speciesTable& species,
    const fvMesh& mesh,
    const dictionary& dict,
    const word& phaseName,
    const word& masterName
)
:
    basicSpecieMixture
    (
        dict,
        species,
        mesh,
        phaseName
    ),
    masterName_(masterName),
    massTransferRates_(this->species_.size()),
    implicitSources_(this->species_.size())
{
    forAll(species_, i)
    {
        massTransferRates_.set
        (
            i,
            species_[i],
            new volScalarField
            (
                IOobject
                (
                    "massTransfer:" + IOobject::groupName(species_[i], phaseName),
                    mesh.time().timeName(),
                    mesh
                ),
                mesh,
                dimensionedScalar("0", dimDensity/dimTime, 0.0)
            )
        );
    }
}


Foam::multicomponentBlastThermo::integrator::integrator
(
    const fvMesh& mesh,
    PtrList<volScalarField>& Y,
    PtrListDictionary<volScalarField>& massTransferRates,
    PtrListDictionary<fvScalarMatrix>& implicitSources,
    const List<bool>& active,
    const word& alphaRhoName,
    const word& alphaRhoPhiName
)
:
    timeIntegrationSystem
    (
        IOobject::groupName("multicomponentIntegrator", Y[0].group()),
        mesh
    ),
    mesh_(mesh),
    Y_(Y),
    massTransferRates_(massTransferRates),
    implicitSources_(implicitSources),
    active_(active),
    alphaRho_(mesh_.lookupObject<volScalarField>(alphaRhoName)),
    alphaRhoPhi_(mesh_.lookupObject<surfaceScalarField>(alphaRhoPhiName)),
    models_(fvModels::New(mesh)),
    constraints_(fvConstraints::New(mesh))
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::multicomponentBlastThermo::~multicomponentBlastThermo()
{}

Foam::multicomponentBlastThermo::integrator::~integrator()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::multicomponentBlastThermo::initializeModels()
{
    const fvMesh& mesh = Y_[0].mesh();
    word alphaRhoName(IOobject::groupName("rho", masterName_));
    word alphaRhoPhiName(IOobject::groupName("rhoPhi", masterName_));
    if
    (
        mesh.foundObject<surfaceScalarField>
        (
            IOobject::groupName("alphaRhoPhi", phaseName_)
        )
    )
    {
        alphaRhoPhiName = IOobject::groupName("alphaRhoPhi", phaseName_);
    }
    else if
    (
        mesh.foundObject<surfaceScalarField>
        (
            IOobject::groupName("alphaRhoPhi", masterName_)
        )
    )
    {
        alphaRhoPhiName = IOobject::groupName("alphaRhoPhi", masterName_);
    }

    if
    (
        mesh.foundObject<volScalarField>
        (
            IOobject::groupName("alphaRho", phaseName_)
        )
    )
    {
        alphaRhoName = IOobject::groupName("alphaRho", phaseName_);
    }
    else if
    (
        mesh.foundObject<volScalarField>
        (
            IOobject::groupName("alphaRho", masterName_)
        )
    )
    {
        alphaRhoName = IOobject::groupName("alphaRho", masterName_);
    }

    integratorPtr_.reset
    (
        new integrator
        (
            mesh,
            Y_,
            massTransferRates_,
            implicitSources_,
            active_,
            alphaRhoName,
            alphaRhoPhiName
        )
    );
}


void Foam::multicomponentBlastThermo::update()
{
    integratorPtr_->update();
}


void Foam::multicomponentBlastThermo::solve()
{
    integratorPtr_->solve();
}


void Foam::multicomponentBlastThermo::postUpdate()
{
    integratorPtr_->postUpdate();
}


void Foam::multicomponentBlastThermo::addDelta
(
    const word& name,
    tmp<volScalarField>& delta
)
{
    if (massTransferRates_.found(name))
    {
        massTransferRates_[name] += delta;
        return;
    }
}


void Foam::multicomponentBlastThermo::addSource
(
    const word& name,
    tmp<fvScalarMatrix>& source
)
{
    if (species_.found(name))
    {
        if (implicitSources_.found(name))
        {
            implicitSources_[name] += source;
        }
        else
        {
            implicitSources_.set
            (
                species_[name],
                name,
                source
            );
        }
    }
}


void Foam::multicomponentBlastThermo::integrator::update()
{}


void Foam::multicomponentBlastThermo::integrator::solve()
{
    const dimensionedScalar& dT(mesh_.time().deltaT());
    dimensionedScalar residualAlphaRho(dimDensity, 1e-10);

    forAll(Y_, i)
    {
        if (active_[i])
        {
            volScalarField YOld(Y_[i]);
            this->storeAndBlendOld(YOld);

            volScalarField deltaAlphaRhoY
            (
                fvc::div
                (
                    alphaRhoPhi_,
                    Y_[i],
                    "div(" + alphaRhoPhi_.name() + ",Yi)"
                )
              + massTransferRates_[i]
            );
            this->storeAndBlendDelta(deltaAlphaRhoY);

            Y_[i] =
                (
                    alphaRho_.prevIter()*YOld - dT*deltaAlphaRhoY
                )/max(residualAlphaRho, alphaRho_);
            Y_[i].max(0.0);
            Y_[i].correctBoundaryConditions();



            // Clear mass transfer after adding
            massTransferRates_[i] = Zero;
        }
    }
}


void Foam::multicomponentBlastThermo::integrator::postUpdate()
{
    const dimensionedScalar& dT(mesh_.time().deltaT());
    dimensionedScalar residualAlphaRho(dimDensity, 1e-10);

    forAll(Y_, i)
    {
        if (active_[i] && implicitSources_.PtrList<fvScalarMatrix>::set(i))
        {
            volScalarField& Yi(Y_[i]);
            fvScalarMatrix YEqn
            (
                fvm::ddt(alphaRho_, Yi)
              - fvc::ddt(alphaRho_, Yi)
              + fvc::ddt(residualAlphaRho, Yi)
              - fvm::ddt(residualAlphaRho, Yi)
             ==
                implicitSources_[i]
              + models_.source(alphaRho_, Yi)
            );

            YEqn.relax();

            constraints_.constrain(YEqn);

            YEqn.solve("Yi");
            implicitSources_[i].negate();

            constraints_.constrain(Yi);
        }
    }
}


// ************************************************************************* //
