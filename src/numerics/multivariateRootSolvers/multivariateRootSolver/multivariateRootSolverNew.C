/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2021
     \\/     M anipulation  | Synthetik Applied Technologies
-------------------------------------------------------------------------------
License
    This file is a derivative work of OpenFOAM.

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

#include "multivariateRootSolver.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

Foam::autoPtr<Foam::multivariateRootSolver> Foam::multivariateRootSolver::New
(
    const scalarMultivariateEquation& eqn,
    const dictionary& dict
)
{
    word multivariateRootSolverTypeName(dict.lookup("solver"));
    Info<< "Selecting multivariateRoot solver " << multivariateRootSolverTypeName << endl;

    if (eqn.nDerivatives() <= 0)
    {
        dictionaryZeroConstructorTable::iterator cstrIter =
            dictionaryZeroConstructorTablePtr_->find(multivariateRootSolverTypeName);

        if (cstrIter == dictionaryZeroConstructorTablePtr_->end())
        {
            FatalErrorInFunction
                << "Unknown multivariateRootSolver type "
                << multivariateRootSolverTypeName << nl << nl
                << "Valid multivariateRootSolvers for no derivates are : " << endl
                << dictionaryZeroConstructorTablePtr_->sortedToc()
                << exit(FatalError);
        }
        return autoPtr<multivariateRootSolver>(cstrIter()(eqn, dict));
    }
    dictionaryOneConstructorTable::iterator cstrIter =
        dictionaryOneConstructorTablePtr_->find(multivariateRootSolverTypeName);

    if (cstrIter == dictionaryOneConstructorTablePtr_->end())
    {
        FatalErrorInFunction
            << "Unknown multivariateRootSolver type "
            << multivariateRootSolverTypeName << nl << nl
            << "Valid multivariateRootSolvers are : " << endl
            << dictionaryOneConstructorTablePtr_->sortedToc()
            << exit(FatalError);
    }

    return autoPtr<multivariateRootSolver>(cstrIter()(eqn, dict));
}


// ************************************************************************* //
