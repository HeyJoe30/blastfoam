/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2021
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
    Foam::meshSizeObject

Description
    Mesh object to calculate the mesh size

SourceFiles
    meshSizeObject.C

\*---------------------------------------------------------------------------*/

#ifndef meshSizeObject_H
#define meshSizeObject_H

#include "MeshObject.H"
#include "volFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class meshSizeObject;
typedef MeshObject
<
    fvMesh,
    Foam::MoveableMeshObject,
    meshSizeObject
> MeshSizeObject;

/*---------------------------------------------------------------------------*\
        Class meshSizeObject Declaration
\*---------------------------------------------------------------------------*/

class meshSizeObject
:
    public MeshSizeObject
{
public:

    // Private Data

        //- Const reference to the mesh
        const fvMesh& mesh_;

        //- Mesh size
        mutable autoPtr<volScalarField> dxPtr_;

        //- Update the mesh size
        void updateMeshSize() const;


public:

    //- Runtime type information
    TypeName("meshSizeObject");


    // Constructors

        //- Construct from fvMesh and dictionary
        meshSizeObject(const fvMesh&);

    //- Destructor
    virtual ~meshSizeObject();


    // Member Functions

        //- Callback for geometry motion
        virtual bool movePoints()
        {
            dxPtr_.clear();
            return true;
        }

        //- Return the mesh size
        inline const volScalarField& dx() const
        {
            if (!dxPtr_.valid())
            {
                updateMeshSize();
            }
            return dxPtr_();
        }

        virtual bool writeData(Ostream&) const
        {
            return false;
        }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
