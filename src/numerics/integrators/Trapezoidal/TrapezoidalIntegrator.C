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

#include "TrapezoidalIntegrator.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::TrapezoidalIntegrator<Type>::TrapezoidalIntegrator
(
    const Equation<Type>& eqn,
    const dictionary& dict
)
:
    Integrator<Type>(eqn, dict)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Type Foam::TrapezoidalIntegrator<Type>::integrate
(
    const scalar x0,
    const scalar x1,
    const label li
) const
{
    scalar dx = x1 - x0;
    if (mag(dx) < small)
    {
        return dx*this->eqnPtr_->f(x0, li);
    }
    dx /= scalar(this->nSteps_);

    scalar a = x0;
    scalar b = a + dx;
    Type fa(this->eqnPtr_->f(a, li));
    Type fb(this->eqnPtr_->f(b, li));
    Type res(0.5*(fa + fb));

    for (label i = 1; i < this->nSteps_; i++)
    {
        a += dx;
        b += dx;
        fa = fb;
        fb = this->eqnPtr_->f(b, li);
        res += 0.5*(fa + fb);
    }

    return dx*res;
}

// ************************************************************************* //
