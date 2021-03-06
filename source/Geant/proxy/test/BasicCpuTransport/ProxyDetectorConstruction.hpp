#pragma once

#include "Geant/geometry/UserDetectorConstruction.hpp"

#include <string>
#include <vector>

namespace geant {
inline namespace GEANT_IMPL_NAMESPACE {
class UserDetectorConstruction;
class RunManager;
} // namespace GEANT_IMPL_NAMESPACE
} // namespace geant

namespace geantx {
inline namespace GEANT_IMPL_NAMESPACE {
class Material;
}
} // namespace geantx

namespace userapplication {

class ProxyDetectorConstruction : public geantx::UserDetectorConstruction {

public:
  // CTR
  ProxyDetectorConstruction(geantx::RunManager *runmgr);
  // DTR
  ~ProxyDetectorConstruction();

  // interface mathod to define a set of custom materials for the application
  virtual void CreateMaterials();
  // interface method to define the geometry for the application
  virtual void CreateGeometry();

  // set/get number of absorbers per layer
  void SetNumberOfAbsorbersPerLayer(int numabs);
  int GetNumberOfAbsorbersPerLayer() const { return fNumberOfAbsorbers; }
  // set/get number of layers
  void SetNumberOfLayers(int numlayers) { fNumberOfLayers = numlayers; }
  int GetNumberOfLayers() const { return fNumberOfLayers; }
  // set/get absorber thickness
  void SetAbsorberThickness(int absindx, double thick);
  double GetAbsorberThickness(int absindx) const;
  // set absorber material name
  void SetAbsorberMaterialName(int absindx, const std::string &matname);
  // get absorber material (must be used after the CreateMaterials interface method is
  // called)
  const geantx::Material *GetAbsorberMaterial(int absindx) const;
  // set/get production cut (value in length for gamma/e-/e+)
  void SetProductionCut(double pcut) { fProductionCut = pcut; }
  double GetProductionCut() const { return fProductionCut; }
  // set YZ size of the calorimeter (same for the layers, absorbers)
  void SetSizeYZ(double sizeyz) { fCaloSizeYZ = sizeyz; }
  // get world and calorimeter size (used to position the primary gun)
  double GetWorldSizeX() const { return fWorldSizeX; }
  double GetCaloSizeX() const { return fCaloSizeX; }
  //
  int GetAbsorberLogicalVolumeID(int absindx) const;
  //
  std::vector<int> GetLayerIDToLayerIndexMap() const { return fLayerIDToLayerIndex; }
  //
  static int GetMaxNumberOfAbsorbers() { return gMaxNumAbsorbers; }

  void DetectorInfo();

  // private methods
private:
  // internal method to compute layer,calorimeter world size
  void ComputeCalorimeter();
  // intenal (must be used after the CreateMaterials interface method is called)
  void SetAbsorberMaterial(int absindx, const std::string &matname);

  // data members
private:
  static const int gMaxNumAbsorbers = 10;

  std::string fWorldMaterialName;
  std::string fAbsorberMaterialNames[gMaxNumAbsorbers];

  int fNumberOfAbsorbers; // number of absorbers per layer (2)
  int fNumberOfLayers;    // number of layers (50)

  int fAbsorberLogicVolIDs[gMaxNumAbsorbers]; // logical volume ID's for each absorbers

  double fAbsorberThicknesses[gMaxNumAbsorbers];

  double fLayerThickness;
  double fCaloSizeX;
  double fCaloSizeYZ;
  double fWorldSizeX;
  double fWorldSizeYZ;

  double fProductionCut; // in length

  std::vector<int>
      fLayerIDToLayerIndex; // map of the layer (as VPlacedVolume) IDs to their index.

  geantx::Material *fAbsorberMaterials[gMaxNumAbsorbers];
  geantx::Material *fWorldMaterial;

}; //
} // namespace userapplication
