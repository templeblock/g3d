/**
  \file G3D-app.lib/include/G3D-app/Surface.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#pragma once
#define GLG3D_Surface_h

#include "G3D-base/platform.h"
#include "G3D-base/Array.h"
#include "G3D-base/Color4.h"
#include "G3D-base/units.h"
#include "G3D-base/CoordinateFrame.h"
#include "G3D-base/CullFace.h"
#include "G3D-base/lazy_ptr.h"
#include "G3D-app/GBuffer.h"
#include "G3D-app/Component.h"
#include "G3D-app/LightingEnvironment.h"
#include "G3D-app/SceneVisualizationSettings.h"

namespace G3D {

class ShadowMap;
class Tri;
class CPUVertexArray;
class Texture;
class Light;
class Projection;
class AmbientOcclusion;
class LightingEnvironment;
class Material;
class SVO;

extern bool ignoreBool;


#ifdef OPAQUE
#   undef OPAQUE
#endif

/**
   \brief The surface of a model, posed and ready for rendering.
   
   Most methods support efficient OpenGL rendering, but this class
   also supports extracting a mesh that approximates the surface for
   ray tracing or collision detection.

   <b>"Homogeneous" Methods</b>:
   Many subclasses of Surface need to bind shader and other state in
   order to render.  To amortize the cost of doing so, renderers use
   categorizeByDerivedType<shared_ptr<Surface> > to distinguish subclasses and
   then invoke the methods with names ending in "Homogeneous" on
   arrays of derived instances.

   <b>"previous" Arguments</b>: To support motion blur and reverse
   reprojection, Surface represents the surface at two times: the
   "current" time, and some "previous" time that is usually the
   previous frame.  The pose of the underlying model at these times is
   specified to the class that created the Surface.  All rendering
   methods, including shading, operate on the current-time version.  A
   GBuffer can represent a forward difference estimate of velocity in
   these with a GBuffer::Field::CS_POSITION_CHANGE field.  Access
   methods on Surface take a boolean argument \a previous that
   specifies whether the "current" or "previous" description of the
   surface is desired.
   
   Note that one could also render at multiple times by posing the
   original models at different times.  However, models do not
   guarantee that they will produce the same number of Surface%s, or
   Surface%s with the same topology each time that they are posed.
   The use of timeOffset allows the caller to assume that the geometry
   deforms but has the same topology across an interval.
 */
class Surface : public ReferenceCountedObject {
public:

    /** Non-physical properties that a Surface might use to affect light transport. */
    class ExpressiveLightScatteringProperties {
    public:

        class Behavior {
        public:

            /** Use LightScatteringBehavior instead of this enum */
            enum Value {
                PERFECT_ABSORPTION,

                /** Effectively perfect transmission, but don't even count as a scattering
                    event (and no Fresnel or other effects!) */
                INVISIBLE,

                PHYSICAL
            };

        private:

            static const char* toString(int i, Value& v) {
                static const char* str[] = { "PERFECT_ABSORPTION", "INVISIBLE", "PHYSICAL", nullptr };
                static const Value val[] = { PERFECT_ABSORPTION, INVISIBLE, PHYSICAL };
                const char* s = str[i];
                if (s) {
                    v = val[i];
                }
                return s;
            }

            Value value;
        public:

            G3D_DECLARE_ENUM_CLASS_METHODS(Behavior);

        };

        /** Does this surface create direct illumination shadows? */
        bool                                    castsShadows;

        /** Does this surface receive direct illumination shadows? */
        bool                                    receivesShadows;

        Behavior                                behaviorForPathsFromSource;

        /** If false, do not appear in paths traced backwards from the eye.  This obviously depends on the algorithm
            employed for rendering, but as a general hint can be taken to mean "is this object visible in coherent images",
            e.g., reflections, refraction, and primary rays. */
        bool                                    visibleForPathsFromEye;

        ExpressiveLightScatteringProperties() : castsShadows(true), receivesShadows(true), behaviorForPathsFromSource(Behavior::PHYSICAL), visibleForPathsFromEye(true) {}
        ExpressiveLightScatteringProperties(bool c, bool r) : castsShadows(c), receivesShadows(r), behaviorForPathsFromSource(Behavior::PHYSICAL), visibleForPathsFromEye(true) {}

        explicit ExpressiveLightScatteringProperties(const Any& any);

        Any toAny() const;

        bool operator==(const ExpressiveLightScatteringProperties& other) const {
            return
                (castsShadows == other.castsShadows) ||
                (receivesShadows == other.receivesShadows) ||
                (behaviorForPathsFromSource == other.behaviorForPathsFromSource) ||
                (visibleForPathsFromEye == other.visibleForPathsFromEye);
        }

        void andWith(const ExpressiveLightScatteringProperties& other) {
            castsShadows    = castsShadows && other.castsShadows;
            receivesShadows = receivesShadows && other.receivesShadows;
        }

        bool operator!=(const ExpressiveLightScatteringProperties& other) const {
            return !(*this == other);
        }
    };

    const ExpressiveLightScatteringProperties expressiveLightScatteringProperties;

protected:

    /** Hint for renderers to use low resolution rendering. */
    bool                                m_preferLowResolutionTransparency;

    uint64                              m_rigidBodyID = 0;
    shared_ptr<class Model>             m_model;
    shared_ptr<class Entity>            m_entity;

    Surface(const ExpressiveLightScatteringProperties& e = ExpressiveLightScatteringProperties(), bool preferLowResTransparency = false) :
        expressiveLightScatteringProperties(e), m_preferLowResolutionTransparency(preferLowResTransparency) {
    }

    static void cull
       (const CoordinateFrame&             cameraFrame,
        const class Projection&            cameraProjection,
        const class Rect2D&                viewport,
        Array<shared_ptr<Surface> >&       allSurfaces,
        Array<shared_ptr<Surface> >&       outSurfaces,
        bool                               previous,
        bool                               inPlace);

public:

    /** The Model that created this surface. May be nullptr */
    const shared_ptr<Model>& model() const {
        return m_model;
    }

    /** The Entity that created this surface. May be nullptr */
    const shared_ptr<Entity>& entity() const {
        return m_entity;
    }

    /** An identifier that, if nonzero, is unique for a combination of geometry and material.
        This allows high-level data structures such as G3D::TriTreeBase to identify corresponding 
        Surface%s between frames that have moved but not otherwise changed and avoid rebuilding
        the data structure.
      */
    uint64 rigidBodyID() const {
        return m_rigidBodyID;
    }

    bool preferLowResolutionTransparency() const {
        return m_preferLowResolutionTransparency;
    }

    virtual ~Surface() {}

    /** \brief Name of the underlying model or part for debugging purposes */
    virtual String name() const;

	RenderMask renderMask() const;

    virtual void getCoordinateFrame(CoordinateFrame& cframe, bool previous = false) const;

    virtual CoordinateFrame frame(bool previous = false) const {
        CoordinateFrame c;
        getCoordinateFrame(c, previous);
        return c;
    }

    /** May be infinite */
    virtual void getObjectSpaceBoundingBox(AABox& box, bool previous = false) const = 0;

    /** May be infinite */
    virtual void getObjectSpaceBoundingSphere(Sphere& sphere, bool previous = false) const = 0;
    
    /** \brief Clears the arrays and appends indexed triangle list
        information.

     Many subclasses will ignore \a previous because they only use
     that for rigid-body transformations and can be represented by the
     current geometry and moving coordinate frame.  However, it is
     possible to include skinning or keyframe information in a Surface
     and respond to timeOffset.

     Not required to be implemented.*/
    virtual void getObjectSpaceGeometry
       (Array<int>&                  index,
        Array<Point3>&               vertex,
        Array<Vector3>&              normal,
        Array<Vector4>&              packedTangent,
        Array<Point2>&               texCoord,
        bool                         previous = false) const {}

    /** If true, this object transmits light, potentially refracting it and
        filtering the background with color or diffusion. This says nothing about whether it
        has partial coverage.

        The default implementation returns false.*/
    virtual bool hasTransmission() const {
        return false;
    }

    virtual bool isSkybox() const { return false; }

    /** What type of transparency (= alpha and transmission) does this surface have? */
    virtual TransparencyType transparencyType() const = 0;
    
    /*{
        if (requiresBlending()) {
            if (hasTransmission()) {
                return TransparencyType::SOME;
            }
            else if (anyUnblended()) {
                return TransparencyType::SOME;
            }
            else {
                return TransparencyType::ALL;
            }
        }
        else {
            return TransparencyType::NONE;
        }
    }*/

    /** Wall-clock time at which the source of this surface changed in some way,
        e.g., that might require recomputing a shadow map or spatial data structure.

        The default implementation returns the Entity's last change time if it is not null
        or System::time() otherwise, indicating that the surface
        is always out of date.

        \sa canChange */
    virtual RealTime lastChangeTime() const;

    ///////////////////////////////////////////////////////////////////////
    // Aggregate methods

    /** \brief Render a set of surfaces from the same most-derived class type.

    The default implementation calls render() on each surface.

    \param surfaceArray Pre-sorted from back to front and culled to
     the current camera.  Invoking this method with elements of \a surfaceArray that are not of
     the same most-derived type as \a this will result in an error.
    */
    virtual void renderHomogeneous
    (RenderDevice*                        rd,
        const Array<shared_ptr<Surface> >&   surfaceArray,
        const LightingEnvironment&           lightingEnvironment,
        RenderPassType                       passType) const;

    /**
    \brief Render all instances of \a surfaceArray to the
    currently-bound Framebuffer using the fields and mapping dictated
    by \a specification.  This is also used for depth-only (e.g.,
    z-prepass) rendering.

    Invoking this with elements of \a surfaceArray that are not of the
    same most-derived type as \a this will result in an error.

    If \a depthPeelTexture is not null, then use it and \a minZSeparation
    to perform a depth peel. This will result in the GBuffer representing
    the closest geometry at least minZSeparation behind the geometry in
    depthPeelTexture.

    \param previousCameraFrame Used for rendering
    GBuffer::CS_POSITION_CHANGE frames.

    \sa renderIntoGBuffer
    */
    virtual void renderIntoGBufferHomogeneous
       (RenderDevice*                      rd,
        const Array<shared_ptr<Surface> >& surfaceArray,
        const shared_ptr<GBuffer>&         gbuffer,
        const shared_ptr<Texture>&         depthPeelTexture,
        const float                        minZSeparation,
        const LightingEnvironment&         lighting) const {};

    virtual void renderIntoSVOHomogeneous(RenderDevice* rd, Array<shared_ptr<Surface> >& surfaceArray, const shared_ptr<SVO>& svo, const CFrame& previousCameraFrame) const {}

    /** \brief Rendering a set of surfaces in wireframe, using the
       current blending mode.  This is primarily used for debugging.

       Invoking this with elements of \a surfaceArray that are not of
       the same most-derived type as \a this will result in an error.

        If \a depthPeelTexture is not null, then use it and \a minZSeparation
        to perform a depth peel. This will result in the depth buffer representing
        the closest geometry at least minZSeparation behind the geometry in
        depthPeelTexture.

       \param previous If true, the caller should set the RenderDevice
       camera transformation to the previous one.  This is provided
       for debugging previous frame data.

       \sa renderWireframe
    */
    virtual void renderWireframeHomogeneous
    (RenderDevice*                      rd,
        const Array<shared_ptr<Surface> >& surfaceArray,
        const Color4&                      color,
        bool                               previous) const = 0;

    /** Use the current RenderDevice::cullFace.
        Assume that surfaceArray is sorted back to front, so render in reverse order for optimal
        early-z test behavior.

        \param transparencyTestMode net coverage testing mode applied _after_ the alphaFilter's processing.

        \param transmissionWeight How wavelength-varying transmission elements
         (for shadow map rendering: lightPower/dot(lightPower, vec3(1,1,1)))
        \sa renderDepthOnly
     */
    virtual void renderDepthOnlyHomogeneous
    (RenderDevice*                      rd,
        const Array<shared_ptr<Surface> >& surfaceArray,
        const shared_ptr<Texture>&         depthPeelTexture,
        const float                        depthPeelEpsilon,
        TransparencyTestMode               transparencyTestMode,
        const Color3&                      transmissionWeight) const;

    /**
      Returns true if this surface should be included in static data structures
      because it is from an object that never changes. The default implementation
      tests whether the surface comes from an Entity and that Entity::canChange.

      \sa Entity::canChange
      \sa lastChangeTime
    */
    virtual bool canChange() const;

    /** \brief Can this particular instance of this type of Surface be fully described in
        a G3D::GBuffer using the given \a specification?

        Often set to false for Surface%s with fractional alpha values, transmission,
        special back-to-front rendering needs, that require more dynamic range in
        the emissive channel, or that
        simply lack a renderIntoGBufferHomogeneous implementation.

        Surface%s that return false for canBeFullyRepresentedInGBuffer may still
        implement renderIntoGBufferHomogeneous for the parts of their surfaces that
        are representable, and doing so will improve the quality of AmbientOcclusion
        and post-processing techniques like MotionBlur and DepthOfField.

        \sa anyUnblended, requiresBlending
      */
    virtual bool canBeFullyRepresentedInGBuffer(const GBuffer::Specification& specification) const = 0;

    virtual bool canRenderIntoSVO() const {
        return false;
    }

    /**
         \brief Forward-render all illumination terms for each element of
         \a surfaceArray, which must all be of the same most-derived type
         as \a this.

         Implementations must obey the semantics of the current stencil,
         viewport, clipping, and depth tests.

         \param environment World-space, screen-space, and light-space
         data needed for illumination.

         Use the writePixel() function defined by the Framebuffer for generating output.
         when \a passType == RenderPassType::SINGLE_PASS_UNORDERED_BLENDED_SAMPLES.
         */
    virtual void render
    (RenderDevice*                   rd,
     const LightingEnvironment&      environment,
     RenderPassType                  passTypen) const = 0;

   
    ///////////////////////////////////////////////////////////////////////
    // Static methods

    /**

     \brief Renders front-to-back to a GBuffer using current stencil and depth operations.

     \param sortedVisible Surfaces that are visible to the camera (i.e., already culled)
     and sorted from back to front.

     \param previousCameraFrame Used for rendering
     GBuffer::CS_POSITION_CHANGE frames.

     If \a depthPeelTexture is not null, then use it and \a minZSeparation
    to perform a depth peel. This will result in the depth buffer representing
    the closest geometry at least minZSeparation behind the geometry in
    depthPeelTexture.


    \param lightingEnvironment Provided even though this method is primarily used for
    deferred shading to allow objects with partly pre-computed lighting to complete their
    shading and write directly to the emissive buffer.

     \sa cull, sortBackToFront
     */
    static void renderIntoGBuffer
       (RenderDevice*                      rd,
        const Array<shared_ptr<Surface> >& sortedVisible,
        const shared_ptr<GBuffer>&         gbuffer,
        const shared_ptr<Texture>&         depthPeelTexture = shared_ptr<Texture>(),
        const float                        minZSeparation = 0.0f,
        const LightingEnvironment&         lightingEnvironment = LightingEnvironment());


    static void renderIntoSVO
    (RenderDevice*                      rd,
        Array<shared_ptr<Surface> >&       visible,
        const shared_ptr<SVO>&             svo,
        const CoordinateFrame&             previousCameraFrame = CoordinateFrame());
    /**
      Divides the inModels into a front-to-back sorted array of opaque
      models and a back-to-front sorted array of potentially
      transparent models.  Any data originally in the output arrays is
      cleared.

      \param wsLookVector Sort axis; usually the -Z axis of the camera.
     */
    static void sortFrontToBack
    (Array<shared_ptr<Surface> >&       surfaces,
        const Vector3&                     wsLookVector);


    static void sortBackToFront
    (Array<shared_ptr<Surface> >&       surfaces,
        const Vector3&                     wsLookVector) {
        sortFrontToBack(surfaces, -wsLookVector);
    }


    /** Utility function for rendering a set of surfaces in wireframe
     using the current blending mode.

     \param previous If true, the caller should set the RenderDevice
     camera transformation to the previous one.*/
    static void renderWireframe
    (RenderDevice*                      rd,
        const Array<shared_ptr<Surface> >& surfaceArray,
        const Color4&                      color = Color3::black(),
        bool                               previous = false);


    /** Computes the world-space bounding box of an array of Surface%s
        of any type.  Ignores infinite bounding boxes.

      \param anyInfinite Set to true if any bounding box in surfaceArray was infinite.
        Not modified otherwise.

      \param onlyShadowCasters If true, only get the bounds of shadow casting surfaces.
     */
    static void getBoxBounds
    (const Array<shared_ptr<Surface>>&  surfaceArray,
     AABox&                             bounds,
     bool                               previous          = false,
     bool&                              anyInfinite       = ignoreBool,
     bool                               onlyShadowCasters = false);

    /** Computes the world-space bounding sphere of an array of
        Surface%s of any type. Ignores infinite bounding boxes.

      \param anyInfinite Set to true if any bounding box in surfaceArray was infinite.
        Not modified otherwise.

      \param onlyShadowCasters If true, only get the bounds of shadow casting surfaces.
     */
    static void getSphereBounds
    (const Array<shared_ptr<Surface> >& surfaceArray,
     Sphere&                            bounds,
     bool                               previous = false,
     bool&                              anyInfinite = ignoreBool,
     bool                               onlyShadowCasters = false);
    

    /** Computes the array of surfaces that can be seen by \a camera.  Preserves order. */
    static void cull
    (const CoordinateFrame&             cameraFrame,
     const class Projection&            cameraProjection,
     const class Rect2D&                viewport,
     const Array<shared_ptr<Surface> >& allSurfaces,
     Array<shared_ptr<Surface> >&       outSurfaces,
     bool                               previous = false) {
        // Const cast so that both this and the in-place version of cull can call the same method
        // However, as that one method needs to be able to modify allSurfaces in the in-place method
        // It must be sent as a non-const reference. The array is not modified, however.
        cull(cameraFrame, cameraProjection, viewport, const_cast<Array<shared_ptr<Surface> >&>(allSurfaces), outSurfaces, previous, false);
    }

    /** Culls surfaces in place */
    static void cull
       (const CoordinateFrame&             cameraFrame,
        const class Projection&            cameraProjection,
        const class Rect2D&                viewport,
        Array<shared_ptr<Surface> >&       allSurfaces,
        bool                               previous = false) {
        // Initializing an array without a constructor allocates no memory, so this is safe
        Array<shared_ptr<Surface> > ignore;
        cull(cameraFrame, cameraProjection, viewport, allSurfaces, ignore, previous, true);
    }


    /** Render geometry only (no shading), and ignore color (but do
        perform alpha testing).  Render only back or front faces
        (two-sided surfaces render no matter what).

        Does not sort or cull based on the view frustum of the camera
        like other batch rendering routines--sort before invoking
        if you want that.

        Used for z prepass and shadow mapping.

     */
    static void renderDepthOnly
    (RenderDevice*                          rd,
     const Array<shared_ptr<Surface> >&     surfaceArray,
     CullFace                               cull,
     const shared_ptr<Texture>&             depthPeelTexture      = nullptr,
     const float                            minZSeparation        = 0.0f,
     TransparencyTestMode                   transparencyTestMode  = TransparencyTestMode::REJECT_TRANSPARENCY,
     const Color3&                          transmissionWeight    = Color3::white() / 3.0f);

    /**
      Returns a CPUVertexArray and an Array<Tri> generated from the surfaces in surfaceArray, with everything transformed to world space
      First separates surfaceArray by derived type and then calls getTrisHomogenous
     */
    static void getTris(const Array<shared_ptr<Surface> >& surfaceArray, CPUVertexArray& cpuVertexArray, Array<Tri>& triArray, bool computePrevPosition = false);

    /** \brief Creates and appends Tris and CPUVertexArray::Vertices onto
        the parameter arrays using the cpuGeom's of the surfaces in surfaceArray

       Invoking this with elements of \a surfaceArray that are not of
       the same most-derived type as \a this will result in an error.

       This function maintains a table of already used cpuGeoms, so that we don't need to
       duplicate things unneccessarily.
    */
    virtual void getTrisHomogeneous
    (const Array<shared_ptr<Surface> >& surfaceArray,
        CPUVertexArray&                    cpuVertexArray,
        Array<Tri>&                        triArray,
        bool                               computePrevPosition = false) const {}

    /** Set the storage on all Materials in the array */
    static void setStorage(const Array<shared_ptr<Surface>>& surfaceArray, ImageStorage newStorage);

    virtual void setStorage(ImageStorage newStorage) = 0;

};

/////////////////////////////////////////////////////////////////

/** Primarily for use in GUI rendering. */
class Surface2D : public ReferenceCountedObject {
public:

    /** Assumes that the RenderDevice is configured in in RenderDevice::push2D mode. */
    virtual void render(RenderDevice* rd) const = 0;

    /** Conservative 2D rendering bounds.
     */
    virtual Rect2D bounds() const = 0;

    /**
     2D objects are drawn from back to front, creating the perception of overlap.
     0 = closest to the front, 1 = closest to the back. 
     */
    virtual float depth() const = 0;

    /** Sorts from farthest to nearest. */
    static void sort(Array<shared_ptr<Surface2D>>& array);

    /** Calls sort, RenderDevice::push2D, and then render on all elements */
    static void sortAndRender(RenderDevice* rd, Array<shared_ptr<Surface2D>>& array);
};

} // namespace G3D

G3D_DECLARE_ENUM_CLASS_HASHCODE(G3D::Surface::ExpressiveLightScatteringProperties::Behavior);
