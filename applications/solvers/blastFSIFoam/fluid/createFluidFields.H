// Initialise fluid field pointer lists
PtrList<compressibleSystem> fluids(fluidRegions.size());
PtrList<timeIntegrator> timeIntegrators(fluidRegions.size());
PtrList<uniformDimensionedVectorField> gFluid(fluidRegions.size());
PtrList<volSymmTensorField> devRhoReffs(fluidRegions.size());

// Populate fluid field pointer lists
forAll(fluidRegions, i)
{
    Info<< "*** Reading fluid mesh thermophysical properties for region "
        << fluidRegions[i].name() << nl << endl;

    // Read gravitational acceleration
    Info<< "\nReading g" << endl;
    gFluid.set
    (
        i,
        new uniformDimensionedVectorField
        (
            IOobject
            (
                "g",
                runTime.constant(),
                fluidRegions[i],
                IOobject::READ_IF_PRESENT,
                IOobject::NO_WRITE
            ),
            dimensionedVector(dimAcceleration, Zero)
        )
    );

    timeIntegrators.set(i, timeIntegrator::New(fluidRegions[i]));
    fluids.set
    (
        i,
        compressibleSystem::New(fluidRegions[i])
    );
    timeIntegrators[i].addSystem(fluids[i]);
    fluids[i].update();

    //- Store viscous stresses to pass to other regions
    devRhoReffs.set
    (
        i,
        new volSymmTensorField
        (
            IOobject
            (
                "devRhoReff",
                runTime.timeName(),
                fluidRegions[i],
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            fluidRegions[i],
            dimensionedSymmTensor
            (
                "Zero",
                dimensionSet(1,1,-2,0,0,0,0),
                Zero
            )
        )
    );

}
