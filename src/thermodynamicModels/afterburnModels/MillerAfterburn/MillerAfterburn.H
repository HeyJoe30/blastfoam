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

Class
    Foam::MillerAfterburn

Description
    After burn model of Miller

    References:
    \verbatim
        Schwer, L.E. (2016).
        Jones-Wilkens-Lee (JWL) Equation of State with Afterburning
        14th Internation LS-DYNA Users Conference. Constitutive Modeling
    \endverbatim

SourceFiles
    MillerAfterburn.C

\*---------------------------------------------------------------------------*/

#ifndef MillerAfterburn_H
#define MillerAfterburn_H

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "afterburnModel.H"

namespace Foam
{
namespace afterburnModels
{

/*---------------------------------------------------------------------------*\
                           Class MillerAfterburn Declaration
\*---------------------------------------------------------------------------*/

class MillerAfterburn
:
    public afterburnModel
{
    // Private data

        //- Activation ratio
        volScalarField c_;

        //- Scale for pressure (Mpa = 1e-6)
        scalar pScale_;

        //- Name of pressure
        word pName_;

        //- Reference to total pressure
        const volScalarField& p_;

        //- Density name
        UautoPtr<const volScalarField> alphaRhoPtr_;

        //- Name of flux
        UautoPtr<const surfaceScalarField> alphaRhoPhiPtr_;

        //- Model coefficients
        dimensionedScalar Q0_;
        scalar m_;
        scalar n_;
        dimensionedScalar a_;
        dimensionedScalar pMin_;

        //- Stored rate of change in c (used for energy source)
        tmp<volScalarField> ddtC_;


public:

    //- Runtime type information
    TypeName("Miller");

    // Constructor
    MillerAfterburn
    (
        const fvMesh& mesh,
        const dictionary& dict,
        const word& phaseName
    );


    //- Destructor
    virtual ~MillerAfterburn();


    // Member Functions

        //- Set pointers to mass and mass flux
        virtual void initializeModels();

        //- Solve sub-step stepi
        virtual void solve();

        //- Return energy
        virtual tmp<volScalarField> ESource() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace afterburnModels
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
