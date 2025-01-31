/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2020-2021
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

#include "mappedPointPatchSelector.H"

#include "mappedPointPatch.H"

#include "mappedMovingPointPatch.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(mappedPointPatchSelector, 0);
}


// * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * * * * //

Foam::mappedPointPatchSelector::mappedPointPatchSelector
(
    const pointPatch& pp
)
:
    mappedMovingPatchPtr_(nullptr)
{
    if (isA<mappedMovingPointPatch>(pp))
    {
        mappedMovingPatchPtr_ =
            &refCast<const mappedMovingPointPatchBase>(pp);
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::mappedPointPatchSelector::~mappedPointPatchSelector()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

const Foam::polyMesh& Foam::mappedPointPatchSelector::sampleMesh() const
{
    if (mappedMovingPatchPtr_)
    {
        return mappedMovingPatchPtr_->sampleMesh();
    }

    NotImplemented;
    return mappedMovingPatchPtr_->sampleMesh();
}


const Foam::polyPatch& Foam::mappedPointPatchSelector::samplePolyPatch() const
{
    if (mappedMovingPatchPtr_)
    {
        return mappedMovingPatchPtr_->samplePolyPatch();
    }

    NotImplemented;
    return mappedMovingPatchPtr_->samplePolyPatch();
}


Foam::tmp<Foam::pointField> Foam::mappedPointPatchSelector::samplePoints() const
{
    if (mappedMovingPatchPtr_)
    {
        return mappedMovingPatchPtr_->samplePoints();
    }

    NotImplemented;
    return mappedMovingPatchPtr_->samplePoints();
}


// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //
// Function to clear patches if an update has occurred
void Foam::mappedPointPatchSelector::clearMappedPatches(fvMesh& mesh)
{
    if (!mesh.foundObject<pointMesh>("pointMesh"))
    {
        return;
    }

    pointMesh& pMesh = mesh.lookupObjectRef<pointMesh>("pointMesh");
    forAll(pMesh.boundary(), patchi)
    {
        if (isA<mappedMovingPointPatch>(pMesh.boundary()[patchi]))
        {
            pointBoundaryMesh& pbMesh =
                const_cast<pointBoundaryMesh&>(pMesh.boundary());
            refCast<mappedMovingPointPatchBase>
            (
                pbMesh[patchi]
            ).clearOut();
        }
    }
}


bool Foam::mappedPointPatchSelector::isAMappedType
(
    const pointPatch& patch
)
{
    if
    (
        isA<mappedMovingPointPatch>(patch)
     || isA<mappedPointPatch>(patch)
    )
    {
        return true;
    }
    return false;
}


// ************************************************************************* //
