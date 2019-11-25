//
//===------------------ GeantX --------------------------------------------===//
//
// Geant Exascale Pilot
//
// For the licensing terms see LICENSE file.
// For the list of contributors see CREDITS file.
// Copyright (C) 2019, Geant Exascale Pilot team,  All rights reserved.
//===----------------------------------------------------------------------===//
//
/**
 * @file
 * @brief
 */
//===----------------------------------------------------------------------===//
//

#include "Geant/core/Config.hpp"
#include "Geant/core/Logger.hpp"
#include "Geant/core/Memory.hpp"
#include "Geant/core/MemoryPool.hpp"
#include "Geant/core/SystemOfUnits.hpp"
#include "Geant/geometry/UserDetectorConstruction.hpp"
#include "Geant/track/TrackState.hpp"

#include "management/GeoManager.h"
#include "volumes/Box.h"
#include "volumes/LogicalVolume.h"
#include "volumes/Orb.h"
#include "volumes/PlacedVolume.h"
#include "volumes/Trapezoid.h"

#include "navigation/HybridNavigator2.h"
#include "navigation/NewSimpleNavigator.h"
#include "navigation/SimpleABBoxLevelLocator.h"
#include "navigation/SimpleABBoxNavigator.h"
#include "navigation/VNavigator.h"


#include "BasicCpuTransport/TrackManager.hpp"
#include "BasicCpuTransport/Types.hpp"
#include "BasicCpuTransport/ProxyDetectorConstruction.hpp"

using namespace geantx;
using namespace vecgeom;

//===----------------------------------------------------------------------===//
// I doubt someone will see this but if you do, please replace with whatever
// is needed to get a random number generator from VecMath.
//
// And if this is Philippe looking, consider me warned about static thread_local
// being slow.
//
/*
// moved to Geant/processes/Process.hpp
inline double
get_rand()
{
    static auto _get_generator = []() {
        std::random_device rd;
        std::mt19937_64    gen(rd());
        return gen;
    };
    static thread_local auto _gen = _get_generator();
    return std::generate_canonical<double, 10>(_gen);
}
*/

//===----------------------------------------------------------------------===//

// VPlacedVolume *
void
initialize_geometry()
{
    UnplacedBox*       worldUnplaced = new UnplacedBox(10, 10, 10);
    UnplacedTrapezoid* trapUnplaced =
        new UnplacedTrapezoid(4, 0, 0, 4, 4, 4, 0, 4, 4, 4, 0);
    UnplacedBox* boxUnplaced = new UnplacedBox(2.5, 2.5, 2.5);
    UnplacedOrb* orbUnplaced = new UnplacedOrb(2.8);

    LogicalVolume* world = new LogicalVolume("world", worldUnplaced);
    LogicalVolume* trap  = new LogicalVolume("trap", trapUnplaced);
    LogicalVolume* box   = new LogicalVolume("box", boxUnplaced);
    LogicalVolume* orb   = new LogicalVolume("orb", orbUnplaced);

    Transformation3D* ident = new Transformation3D(0, 0, 0, 0, 0, 0);
    orb->PlaceDaughter("orb1", box, ident);
    trap->PlaceDaughter("box1", orb, ident);

    Transformation3D* placement1 = new Transformation3D(5, 5, 5, 0, 0, 0);
    Transformation3D* placement2 =
        new Transformation3D(-5, 5, 5, 0, 0, 0);  // 45,  0,  0);
    Transformation3D* placement3 = new Transformation3D(5, -5, 5, 0, 0, 0);  // 0, 45, 0);
    Transformation3D* placement4 =
        new Transformation3D(5, 5, -5, 0, 0, 0);  // 0,  0, 45);
    Transformation3D* placement5 =
        new Transformation3D(-5, -5, 5, 0, 0, 0);  // 45, 45,  0);
    Transformation3D* placement6 =
        new Transformation3D(-5, 5, -5, 0, 0, 0);  // 45,  0, 45);
    Transformation3D* placement7 =
        new Transformation3D(5, -5, -5, 0, 0, 0);  // 0, 45, 45);
    Transformation3D* placement8 =
        new Transformation3D(-5, -5, -5, 0, 0, 0);  // 45, 45, 45);

    world->PlaceDaughter("trap1", trap, placement1);
    world->PlaceDaughter("trap2", trap, placement2);
    world->PlaceDaughter("trap3", trap, placement3);
    world->PlaceDaughter("trap4", trap, placement4);
    world->PlaceDaughter("trap5", trap, placement5);
    world->PlaceDaughter("trap6", trap, placement6);
    world->PlaceDaughter("trap7", trap, placement7);
    world->PlaceDaughter("trap8", trap, placement8);

    VPlacedVolume* w = world->Place();
    GeoManager::Instance().SetWorld(w);
    GeoManager::Instance().CloseGeometry();
}

//--------------------------------------------------------------------------------------//

template <typename ParticleType, typename ProcessTuple>
void
ApplyAtRest(Track* track, size_t idx)
{
    using Apply_t = typename PhysicsProcessAtRest<ParticleType, ProcessTuple>::type;
    using Funct_t = std::function<void()>;
    TIMEMORY_BASIC_MARKER(toolset_t, "");

    geantx::Log(kInfo) << GEANT_HERE << "stepping track AtRest: " << *track;
    ///
    /// Reference: \file Geant/processes/ProcessConcepts.hpp
    ///
    ///     doit_idx:       index of smallest PIL
    ///     doit_value:     value of smallest PIL
    ///     doit_apply:     lambda that stores the "DoIt" for the smallest PIL
    ///
    intmax_t doit_idx   = -1;
    double   doit_value = std::numeric_limits<double>::max();
    Funct_t  doit_apply = [=]() {
        // geantx::Log(kInfo) << GEANT_HERE << "no process selected: " << *track;
    };
    ///
    /// Calculate all of the AtRest PIL lengths for all the processes
    ///
    Apply<void>::unroll_indices<Apply_t>(track, &doit_idx, &doit_value, &doit_apply);
    ///
    /// Invoke the DoIt of smallest PIL
    ///
    doit_apply();
    ///
    /// Invoke all the AtRest processes that don't propose a PIL
    ///
    Apply<void>::apply<Apply_t>(track);
}

//--------------------------------------------------------------------------------------//

template <typename ParticleType, typename ProcessTuple>
void
ApplyAlongStep(Track* track, size_t idx)
{
    using Apply_t = typename PhysicsProcessAlongStep<ParticleType, ProcessTuple>::type;
    using Funct_t = std::function<void()>;
    TIMEMORY_BASIC_MARKER(toolset_t, "");

    geantx::Log(kInfo) << GEANT_HERE << "stepping track AlongStep: " << *track;
    ///
    /// Reference: \file Geant/processes/ProcessConcepts.hpp
    ///
    ///     doit_idx:       index of smallest PIL
    ///     doit_value:     value of smallest PIL
    ///     doit_apply:     lambda that stores the "DoIt" for the smallest PIL
    ///
    intmax_t doit_idx   = -1;
    double   doit_value = std::numeric_limits<double>::max();
    Funct_t  doit_apply = [=]() {
        // geantx::Log(kInfo) << GEANT_HERE << "no process selected: " << *track;
    };
    ///
    /// Calculate all of the AlongStep PIL lengths for all the processes
    ///
    Apply<void>::unroll_indices<Apply_t>(track, &doit_idx, &doit_value, &doit_apply);
    ///
    /// Invoke the DoIt of smallest PIL
    ///
    doit_apply();
    ///
    /// Invoke all the AlongStep processes that don't propose a PIL
    ///
    Apply<void>::apply<Apply_t>(track);
}

//--------------------------------------------------------------------------------------//

template <typename ParticleType, typename ProcessTuple>
void
ApplyPostStep(Track* track, size_t idx)
{
    using Apply_t = typename PhysicsProcessPostStep<ParticleType, ProcessTuple>::type;
    using Funct_t = std::function<void()>;
    TIMEMORY_BASIC_MARKER(toolset_t, "");

    geantx::Log(kInfo) << GEANT_HERE << "stepping track PostStep: " << *track;
    ///
    /// Reference: \file Geant/processes/ProcessConcepts.hpp
    ///
    ///     doit_idx:       index of smallest PIL
    ///     doit_value:     value of smallest PIL
    ///     doit_apply:     lambda that stores the "DoIt" for the smallest PIL
    ///
    intmax_t doit_idx   = -1;
    double   doit_value = std::numeric_limits<double>::max();
    Funct_t  doit_apply = [=]() {
        geantx::Log(kInfo) << GEANT_HERE << "no process selected: " << *track;
    };
    ///
    /// Calculate all of the PostStep PIL lengths for all the processes
    ///
    Apply<void>::unroll_indices<Apply_t>(track, &doit_idx, &doit_value, &doit_apply);
    ///
    /// Invoke the DoIt of smallest PIL
    ///
    doit_apply();
    ///
    /// Invoke all the PostStep processes that don't propose a PIL
    ///
    Apply<void>::apply<Apply_t>(track);
}

//--------------------------------------------------------------------------------------//
// converts all tracks to a vector (not-optimal) and then transport them in a loop
//
template <typename ParticlePhysics, typename... ParticleTypes,
          typename ParticleType          = typename ParticlePhysics::particle,
          typename ParticleTypeProcesses = typename ParticlePhysics::physics>
auto
DoStep(VariadicTrackManager<ParticleTypes...>* primary,
       VariadicTrackManager<ParticleTypes...>* secondary)
{
    TIMEMORY_BASIC_MARKER(toolset_t, "");

    // using AtRestProcesses =
    //     sort::sort<PhysicsProcessAtRestPriority, ParticleTypeProcesses>::type;

    //
    // here would be a memory transfer to GPU, if needed
    //
    while(!primary->Empty())
    {
        auto sz = primary->template Size<ParticleType>();
        for(size_t i = 0; i < sz; ++i)
        {
            auto* _track = primary->template PopTrack<ParticleType>(i);
            if(!_track) break;

            _track->fStatus = TrackStatus::Alive;
            ApplyAtRest<ParticleType, ParticleTypeProcesses>(_track, i);
            if(_track->fStatus != TrackStatus::Killed)
                ApplyAlongStep<ParticleType, ParticleTypeProcesses>(_track, i);
            if(_track->fStatus != TrackStatus::Killed)
                ApplyPostStep<ParticleType, ParticleTypeProcesses>(_track, i);
            ++_track->fPhysicsState.fPstep;

            ///
            /// Push the track back into primary
            ///
            secondary->template PushTrack<ParticleType>(_track, i);
        }
        // ... etc.
        std::swap(primary, secondary);
    }
}

//--------------------------------------------------------------------------------------//
// general implementation that launches kernels until all ready and secondary are
// finished
//
Track*
get_primary_particle(VolumePath_t *startpath, double Ekin)
{
    TIMEMORY_BASIC_MARKER(toolset_t, "");
    Track* _track = new Track;
    _track->fPhysicsState.fEkin  = Ekin;
    _track->fDir  = { get_rand(), get_rand(), get_rand() };
    _track->fPos  = { get_rand(), get_rand(), get_rand() };
    _track->fDir.Normalize();

    _track->fGeometryState.fPath = startpath;
    _track->fGeometryState.fNextpath = startpath;
    auto top = startpath->Top();
    auto *vol = (top) ? top->GetLogicalVolume() : nullptr;
    _track->fGeometryState.fVolume = vol;
    _track->fMaterialState.fMaterial = ((Material_t *)vol->GetMaterialPtr());

    return _track;
}

//===----------------------------------------------------------------------===//

int
main(int argc, char** argv)
{
    tim::settings::precision() = 6;
    tim::settings::width()     = 12;
    tim::timemory_init(argc, argv);

    TIMEMORY_BLANK_MARKER(toolset_t, argv[0]);

    /*
    if(tim::get_env<bool>("TRANSPORT_GEOM", false))
    {
        initialize_geometry();
        // basic geometry checks
        if(GeoManager::Instance().GetWorld())
        {
            const auto* logWorld = GeoManager::Instance().GetWorld()->GetLogicalVolume();
            if(logWorld)
            {
                // print detector information
                logWorld->PrintContent();
                std::cout << "\n # placed volumes: " << logWorld->GetNTotal() << "\n";
            }
        }
    }
    */

    // Create and configure run manager
    geantx::RunManager *runMgr = NULL;

    // Create CMS detector construction
    userapplication::ProxyDetectorConstruction *det = new userapplication::ProxyDetectorConstruction(runMgr);
    geantx::vector_t<geantx::Volume_t const *> volumes;

    int numVolumes = 0;
    if (det) {
      det->CreateMaterials();
      det->CreateGeometry();
      numVolumes = det->SetupGeometry(volumes);
    }
    det->DetectorInfo();
    std::cout << " Number of the maximum volume depth = " << numVolumes << std::endl;

    // initialize negivation
    det->InitNavigators();          

    VariadicTrackManager<CpuGamma, CpuElectron, GpuGamma, GpuElectron> primary;
    VariadicTrackManager<CpuGamma, CpuElectron, GpuGamma, GpuElectron> secondary;

    /*
    //
    // This will eventually provide a re-ordering for the sequence of which the
    // processes are applied. So one can do something like:
    //
    //      template <>
    //      struct PhysicsProcessPostStepPriority<Transportation>
    //      : std::integral_constant<int, -100>
    //      {};
    //
    //      template <>
    //      struct PhysicsProcessPostStepPriority<ProxyStepLimiter>
    //      : std::integral_constant<int, 100>
    //      {};
    //
    //  so that for the PostStep stage, Transportation is prioritized ahead of
    //  other processes and ProxyStepLimiter is, in general, applied after
    //  most other processes
    //

    using PhysList_A = std::tuple<ProxyTrackLimiter>;
    using PhysList_B =
        InsertSorted<PhysList_A, ProxyScattering, SortPhysicsProcessPostStep>;
    using PhysList_C =
        InsertSorted<PhysList_B, ProxyStepLimiter, SortPhysicsProcessPostStep>;
    using PhysList_D =
        InsertSorted<PhysList_C, Transportation, SortPhysicsProcessPostStep>;

    std::cout << "Phys List A : " << tim::demangle(typeid(PhysList_A).name())
              << std::endl;
    std::cout << "Phys List B : " << tim::demangle(typeid(PhysList_B).name())
              << std::endl;
    std::cout << "Phys List C : " << tim::demangle(typeid(PhysList_C).name())
              << std::endl;
    std::cout << "Phys List D : " << tim::demangle(typeid(PhysList_D).name())
              << std::endl;
    */

    // at the beginning of an event/tracking - initialize the navigation path
    int maxDepth = vecgeom::GeoManager::Instance().getMaxDepth();
    vecgeom::Vector3D<double> vertex(0.,0.,0.);
    geantx::VolumePath_t *startpath = geantx::VolumePath_t::MakeInstance(maxDepth);
    startpath->Clear();
    vecgeom::GlobalLocator::LocateGlobalPoint(vecgeom::GeoManager::Instance().GetWorld(), vertex, *startpath, true);

    // prepare primary tracks - TODO: use a particle gun 
    double energy = 10. * geantx::units::GeV;

    printf("\n");
    primary.PushTrack<CpuGamma>(get_primary_particle(startpath, energy));
    primary.PushTrack<CpuGamma>(get_primary_particle(startpath, energy));

    printf("\n");
    primary.PushTrack<GpuGamma>(get_primary_particle(startpath, energy));

    printf("\n");
    primary.PushTrack<CpuElectron>(get_primary_particle(startpath, energy));

    printf("\n");
    primary.PushTrack<GpuElectron>(get_primary_particle(startpath, energy));

    //stepping

    printf("\n");
    DoStep<CpuGammaPhysics>(&primary, &secondary);

    printf("\n");
    DoStep<CpuElectronPhysics>(&primary, &secondary);

    printf("\n");
    DoStep<GpuGammaPhysics>(&primary, &secondary);

    printf("\n");
    DoStep<GpuElectronPhysics>(&primary, &secondary);
}

//===----------------------------------------------------------------------===//
