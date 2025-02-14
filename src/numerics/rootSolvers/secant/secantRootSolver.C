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

#include "secantRootSolver.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(secantRootSolver, 0);
    addToRunTimeSelectionTable
    (
        rootSolver,
        secantRootSolver,
        dictionaryZero
    );
    addToRunTimeSelectionTable
    (
        rootSolver,
        secantRootSolver,
        dictionaryOne
    );
    addToRunTimeSelectionTable
    (
        rootSolver,
        secantRootSolver,
        dictionaryTwo
    );
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::secantRootSolver::secantRootSolver
(
    const scalarEquation& eqn,
    const dictionary& dict
)
:
    rootSolver(eqn, dict)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::scalar Foam::secantRootSolver::findRoot
(
    const scalar x0,
    const scalar x1,
    const scalar x2,
    const label li
) const
{
    scalar xNew = x0;
    scalar xLow = x1;
    scalar xHigh = x2;

    if (!eqn_.containsRoot(li))
    {
        return x0;
    }

    for (stepi_ = 0; stepi_ < maxSteps_; stepi_++)
    {
        scalar yHigh = eqn_.f(xHigh, li);
        xNew =
            xHigh - yHigh*(xHigh - xLow)
           /stabilise(yHigh - eqn_.f(xLow, li), small);
        eqn_.limit(xNew);

        xLow = xHigh;
        xHigh = xNew;

        if (converged(xHigh - xLow))
        {
            return xNew;
        }

    }
    printNoConvergence();

    return xNew;
}

// ************************************************************************* //
