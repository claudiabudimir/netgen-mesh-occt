// Includes from OCCT.
#include <iostream>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <RWStl.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <stdexcept>

// OCCT stuff is available through OCCGEOMETRY flag.
#pragma warning(push, 0)

#ifndef OCCGEOMETRY
#define OCCGEOMETRY
#endif

#include <occgeom.hpp>

namespace nglib
{
#include "nglib.h"
}

#pragma warning(pop)

struct NgContext
{
    NgContext()
    {
        nglib::Ng_Init();
    }
    ~NgContext()
    {
        nglib::Ng_Exit();
    }
};

enum class MeshingStatus
{
    Ok = 0,
    NotOk
};

bool computeUniformMesh(const TopoDS_Shape &face, double maxTriangleSideLength)
{
    NgContext securityIssueGenerator;

    netgen::OCCGeometry geometry;
    geometry.shape = face;
    geometry.changed = 1;
    geometry.BuildFMap();
    geometry.CalcBoundingBox();

    netgen::Mesh mesh;
    mesh.SetGeometry(shared_ptr<netgen::NetgenGeometry>(&geometry, [](netgen::NetgenGeometry *) {}));
    mesh.geomtype = netgen::Mesh::GEOM_OCC;
    std::shared_ptr<netgen::Mesh> meshPtr(&mesh, [](netgen::Mesh *) {});

    netgen::MeshingParameters mparam;
    mparam.secondorder = 0;
    mparam.maxh = maxTriangleSideLength;
    mparam.minh = maxTriangleSideLength / 2;
    mparam.grading = 0.8;
    mparam.uselocalh = true;
    mparam.perfstepsstart = netgen::MESHCONST_ANALYSE;
    mparam.perfstepsend = netgen::MESHCONST_MESHSURFACE;

    try
    {
        if (geometry.GenerateMesh(meshPtr, mparam) == static_cast<int>(MeshingStatus::NotOk))
        {
            std::cout << "Netgen mesh generation failed";
            return false;
        }
    }
    catch (const Standard_Failure &failure)
    {
        std::stringstream ss;
        failure.Print(ss);
        std::cout << "Netgen mesh generation failed with OCCT error";
        return false;
    }
    catch (const std::exception &except)
    {
        std::cout << "Netgen mesh generation failed with std exception ";
        return false;
    }
    catch (...)
    {
        std::cout << "Unknown exception in Netgen mesh generation; rethrowing.";
        throw;
    }

    auto pointCount = mesh.GetNP();
    auto surfElemCount = mesh.GetNSE();

    Handle(Poly_Triangulation) result = new Poly_Triangulation(pointCount, surfElemCount, false);
    for (decltype(pointCount) i = 1; i <= pointCount; ++i)
    {
        auto currPoint = mesh.Point(i + 1);
        result->SetNode(i, gp_Pnt(currPoint[0], currPoint[1], currPoint[2]));
    }

    for (decltype(surfElemCount) i = 1; i <= surfElemCount; ++i)
    {
        auto &surfaceElem = mesh.SurfaceElement(netgen::SurfaceElementIndex(i - 1));
        auto elemPointCount = surfaceElem.GetNP();
        Poly_Triangle tri(surfaceElem[0], surfaceElem[1], surfaceElem[2]);
    }

    RWStl::WriteBinary(result, OSD_Path("result.stl"));
    return true;
}

TopoDS_Shape importBREP(std::filesystem::path path)
{
    BRep_Builder builder;
    TopoDS_Shape shape;
    if (!BRepTools::Read(shape, path.c_str(), builder))
    {
        throw std::runtime_error("Failed to import shape.");
    }
    return shape;
}

int main()
{  
    TopoDS_Shape s = importBREP("/usr/src/executables/solid.brep");

    bool success = true;
    for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next())
    {
        auto currShape = ex.Current();
        success = success && computeUniformMesh(currShape, 1.0);
    }

    if(!success)
        std::cout<<"Not all faces were meshed!\n";
    else
        std::cout<<"The meshing was successful\n";

    return 0;
}
