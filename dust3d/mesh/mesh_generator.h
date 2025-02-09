/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef DUST3D_MESH_MESH_GENERATOR_H_
#define DUST3D_MESH_MESH_GENERATOR_H_

#include <set>
#include <tuple>
#include <dust3d/base/position_key.h>
#include <dust3d/base/uuid.h>
#include <dust3d/base/object.h>
#include <dust3d/base/snapshot.h>
#include <dust3d/base/combine_mode.h>
#include <dust3d/mesh/mesh_combiner.h>
#include <dust3d/mesh/stroke_mesh_builder.h>

namespace dust3d
{

class MeshGenerator
{
public:

    class GeneratedPart
    {
    public:
        ~GeneratedPart()
        {
            releaseMeshes();
        };
        void releaseMeshes()
        {
            delete mesh;
            mesh = nullptr;
        }
        MeshCombiner::Mesh *mesh = nullptr;
        std::vector<Vector3> vertices;
        std::vector<std::vector<size_t>> faces;
        std::vector<ObjectNode> objectNodes;
        std::vector<std::pair<std::pair<Uuid, Uuid>, std::pair<Uuid, Uuid>>> objectEdges;
        std::vector<std::pair<Vector3, std::pair<Uuid, Uuid>>> objectNodeVertices;
        std::vector<Vector3> previewVertices;
        std::vector<std::vector<size_t>> previewTriangles;
        bool isSuccessful = false;
        bool joined = true;
    };

    class GeneratedComponent
    {
    public:
        ~GeneratedComponent()
        {
            releaseMeshes();
        };
        void releaseMeshes()
        {
            delete mesh;
            mesh = nullptr;
            for (auto &it: incombinableMeshes)
                delete it;
            incombinableMeshes.clear();
        }
        MeshCombiner::Mesh *mesh = nullptr;
        std::vector<MeshCombiner::Mesh *> incombinableMeshes;
        std::set<std::pair<PositionKey, PositionKey>> sharedQuadEdges;
        std::set<PositionKey> noneSeamVertices;
        std::vector<ObjectNode> objectNodes;
        std::vector<std::pair<std::pair<Uuid, Uuid>, std::pair<Uuid, Uuid>>> objectEdges;
        std::vector<std::pair<Vector3, std::pair<Uuid, Uuid>>> objectNodeVertices;
    };

    class GeneratedCacheContext
    {
    public:
        ~GeneratedCacheContext()
        {
            for (auto &it: cachedCombination)
                delete it.second;
            for (auto &it: parts)
                it.second.releaseMeshes();
            for (auto &it: components)
                it.second.releaseMeshes();
        }
        std::map<std::string, GeneratedComponent> components;
        std::map<std::string, GeneratedPart> parts;
        std::map<std::string, std::string> partMirrorIdMap;
        std::map<std::string, MeshCombiner::Mesh *> cachedCombination;
    };
    
    class PartPreview
    {
    public:
        std::vector<Vector2> cutTemplate;
        
        std::vector<Vector3> vertices;
        std::vector<std::vector<size_t>> triangles;
        std::vector<std::vector<Vector3>> vertexNormals;
        Color color;
        float metalness;
        float roughness;
    };

    MeshGenerator(Snapshot *snapshot);
    ~MeshGenerator();
    bool isSuccessful();
    const std::set<Uuid> &generatedPreviewPartIds();
    const std::set<Uuid> &generatedPreviewImagePartIds();
    Object *takeObject();
    virtual void generate();
    void setGeneratedCacheContext(GeneratedCacheContext *cacheContext);
    void setSmoothShadingThresholdAngleDegrees(float degrees);
    void setInterpolationEnabled(bool interpolationEnabled);
    void setDefaultPartColor(const Color &color);
    void setId(uint64_t id);
    void setWeldEnabled(bool enabled);
    uint64_t id();
    
protected:
    std::set<Uuid> m_generatedPreviewPartIds;
    std::map<Uuid, PartPreview> m_generatedPartPreviews;
    std::set<Uuid> m_generatedPreviewImagePartIds;
    Object *m_object = nullptr;
        
private:
    Color m_defaultPartColor = Color::createWhite();
    Snapshot *m_snapshot = nullptr;
    GeneratedCacheContext *m_cacheContext = nullptr;
    std::set<std::string> m_dirtyComponentIds;
    std::set<std::string> m_dirtyPartIds;
    float m_mainProfileMiddleX = 0;
    float m_sideProfileMiddleX = 0;
    float m_mainProfileMiddleY = 0;
    std::vector<std::pair<Vector3, std::pair<Uuid, Uuid>>> m_nodeVertices;
    std::map<std::string, std::set<std::string>> m_partNodeIds;
    std::map<std::string, std::set<std::string>> m_partEdgeIds;
    bool m_isSuccessful = false;
    bool m_cacheEnabled = false;
    float m_smoothShadingThresholdAngleDegrees = 60;
    uint64_t m_id = 0;
    std::vector<Vector3> m_clothCollisionVertices;
    std::vector<std::vector<size_t>> m_clothCollisionTriangles;
    bool m_weldEnabled = true;
    bool m_interpolationEnabled = true;
    
    void collectParts();
    void collectIncombinableComponentMeshes(const std::string &componentIdString);
    void collectIncombinableMesh(const MeshCombiner::Mesh *mesh, const GeneratedComponent &componentCache);
    bool checkIsComponentDirty(const std::string &componentIdString);
    bool checkIsPartDirty(const std::string &partIdString);
    bool checkIsPartDependencyDirty(const std::string &partIdString);
    void checkDirtyFlags();
    MeshCombiner::Mesh *combinePartMesh(const std::string &partIdString, bool *hasError, bool *retryable, bool addIntermediateNodes=true);
    MeshCombiner::Mesh *combineComponentMesh(const std::string &componentIdString, CombineMode *combineMode);
    void makeXmirror(const std::vector<Vector3> &sourceVertices, const std::vector<std::vector<size_t>> &sourceFaces,
        std::vector<Vector3> *destVertices, std::vector<std::vector<size_t>> *destFaces);
    void collectSharedQuadEdges(const std::vector<Vector3> &vertices, const std::vector<std::vector<size_t>> &faces,
        std::set<std::pair<PositionKey, PositionKey>> *sharedQuadEdges);
    MeshCombiner::Mesh *combineTwoMeshes(const MeshCombiner::Mesh &first, const MeshCombiner::Mesh &second,
        MeshCombiner::Method method,
        bool recombine=true);
    void generateSmoothTriangleVertexNormals(const std::vector<Vector3> &vertices, const std::vector<std::vector<size_t>> &triangles,
        const std::vector<Vector3> &triangleNormals,
        std::vector<std::vector<Vector3>> *triangleVertexNormals);
    const std::map<std::string, std::string> *findComponent(const std::string &componentIdString);
    CombineMode componentCombineMode(const std::map<std::string, std::string> *component);
    MeshCombiner::Mesh *combineComponentChildGroupMesh(const std::vector<std::string> &componentIdStrings,
        GeneratedComponent &componentCache);
    MeshCombiner::Mesh *combineMultipleMeshes(const std::vector<std::tuple<MeshCombiner::Mesh *, CombineMode, std::string>> &multipleMeshes, bool recombine=true);
    std::string componentColorName(const std::map<std::string, std::string> *component);
    void collectUncombinedComponent(const std::string &componentIdString);
    void cutFaceStringToCutTemplate(const std::string &cutFaceString, std::vector<Vector2> &cutTemplate);
    void postprocessObject(Object *object);
    void collectErroredParts();
    void preprocessMirror();
    std::string reverseUuid(const std::string &uuidString);
    void recoverQuads(const std::vector<Vector3> &vertices, const std::vector<std::vector<size_t>> &triangles, const std::set<std::pair<PositionKey, PositionKey>> &sharedQuadEdges, std::vector<std::vector<size_t>> &triangleAndQuads);
    
    static void chamferFace(std::vector<Vector2> *face);
    static bool isWatertight(const std::vector<std::vector<size_t>> &faces);
};

}

#endif
