/**
  \file G3D-app.lib/include/G3D-app/Surfel.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#pragma once
#define G3D_app_Surfel_h

#include "G3D-base/platform.h"
#include "G3D-base/Vector3.h"
#include "G3D-base/Color3.h"
#include "G3D-base/Array.h"
#include "G3D-base/SmallArray.h"
#include "G3D-base/PathDirection.h"

namespace G3D {

class Random;
class Any;
class CoordinateFrame;
class Material;
class Surface;

/** 
\brief Local surface geometry + BSDF + emission function.

The Surfel models the interface between two homogeneous media at a small patch
on a surface.  It is oriented, and the normal is used to give
orientation to distinguish the two sides.  This combines the
mathematical models of a BSDF, and emission function, and a
surface patch.

Scene geometric sampling functions (e.g., ray intersection) return
Surfel subclasses that have been initialized with the appropriate
coefficients for that location in the scene (i.e., they sample
the texture map).

All vectors and positions are in world space.  In this documentation,

- \f$X\f$ is a point at which scattering (or absorption) occurs
- \f$\hat{n}\f$ is a unit surface normal at \f$X\f$
- \f$\hat{\omega}_\mathrm{i}\f$ is a unit vector pointing backwards along the direction that a real photon arrived a surface before scattering
- \f$\hat{\omega}_\mathrm{o}\f$ is a unit vector in the direction that a real photon leaves a surface after scattering
- \f$ f_X \f$ is the Bidirectional Scattering Distribution Function (BSDF) at point \f$ X \f$, which is proportional to the probability density function describing how a photon scatters
- \f$ g \f$ is proportional to a probability density function that is used for sampling; it is ideally close to \f$ f_X \f$ to give good efficiency, but achieving that is impractical for complex BSDFs

   \htmlonly 
   <center><img src="surfel-variables.png"/></center>
   \endhtmlonly

<hr>

The BSDF (\f$ f_X \f$) must obey the following constraints:

- Normalized: \f$ \int_{\mathbf{S}^2} f_X (\hat{\omega}_\mathrm{i}, \hat{\omega}_\mathrm{o}) |\hat{\omega}_\mathrm{i} \cdot \hat{n}| d\hat{\omega}_\mathrm{i} \leq 1 \f$ for all \f$ \hat{\omega_\mathrm{o}} \f$
- Real and non-negative everywhere
- Obey the reciprocity principle \f$ \frac{ f_X(\hat{\omega}_\mathrm{i}, \hat{\omega}_\mathrm{o}) }{ \eta_\mathrm{o}^2} = \frac{ f_X(\hat{\omega}_\mathrm{o}, \hat{\omega}_\mathrm{i}) }{ \eta_\mathrm{i}^2 } \f$

<hr>

Most methods require a PathDirection because transmissive BSDFs
are different depending on whether the path denotes a photon
moving along its physical direction of propagation or a virtual
path backwards from an eye.  For non-transmissive BSDFs the
result is the same either way.

<hr>

scatter() returns a weight that compensates for
distortion of the sampling distribution within the functions.  When
the scattering is non refractive, the caller must adjust the power or 
radiance scattered by this weight:

\f{eqnarray*}
    \Phi_\mathrm{o} = \Phi_\mathrm{i} \cdot \mathrm{weight}\\
    L_\mathrm{o} = L_\mathrm{i} \cdot \mathrm{weight}
\f}


The refractive case requires more care.  If you're are tracing paths
that will be connected as rays as in ray tracing or path tracing,
then at a refractive interface you must adjust the radiance by:

\f[
    L_\mathrm{o} = \frac{\eta_\mathrm{i} }{ \eta_\mathrm{o}} \cdot L_\mathrm{i} \cdot \mathrm{weight}
\f]

after invoking scatterIn or scatterOut when the path refracts.  You
can detect the refractive case by:

\code
  bool refracted = (sign(wo.dot(surfel->shadingNormal)) != sign(wi.dot(surfel->shadingNormal)));
\endcode

However, if you're tracing paths that represent photons that will be
gathered to estimate radiance by:

\f[
    L_\mathrm{o} = (\Phi_\mathrm{i} / \mathrm{gatherArea}) \cdot f_X(\hat{\omega_\mathrm{i}}, \hat{\omega_\mathrm{o}}) \cdot |\hat{\omega_\mathrm{o}} \cdot \hat{n}|
\f]

then adjust the power after scatter() by:

\f[
    \Phi_\mathrm{o} = \Phi_\mathrm{i} \cdot \mathrm{weight}
\f]
 
In this case, the divergence (or convergence) of the photons at a
refractive interface will naturally be captured by the number of
photons within gatherArea and no explicit correction is required.

<hr>

For subclass implementers:

If it is computationally harder to importance sample than to trace
rays, make the scatter functions use weights and select directions
naively (e.g., uniformly).  In this case, more rays will be needed
for convergence because the amount of energy they contribute to the
final signal will have high variance.

If it is computationally harder to trace rays than to importance
sample, then it is worth spending a longer time on importance
sampling to drive the weights close to 1.0, meaning that each ray
contributes approximately the same energy towards convergence of
the image.

Of course, the Surfel implementor cannot make these decisions 
in the context of a single class--they are end-to-end decisions
for an entire system.    

\cite Thanks to John F. Hughes, Matt Pharr, and Michael Mara for help designing this API
 */
class Surfel : public ReferenceCountedObject {
public:

    /** Non-physical manipulations of the BSDF commonly employed for expressive rendering effects. */
    class ExpressiveParameters {
    public:

        /** Scale the diffuse (i.e., non-impulse) reflectivity of surfaces with saturated 
            diffuse spectra by this amount.

            In <i>Mirror's Edge</i>, values as high as 10 were used for this term.

            Note that many BSDFs will not be energy conserving if this value is greater than 1.0f.
         */
        float               saturatedMaterialBoost;

        /** Scale the diffuse (i.e., non-impulse) reflectivity of surfaces with <b>un</b>saturated 
            diffuse spectra by this amount.
         */
        float               unsaturatedMaterialBoost;

        /** Return the amount to boost reflectivity for a surface with a priori reflectivity diffuseReflectivity. */
        float boost(const Color3& diffuseReflectivity) const;

        ExpressiveParameters() : saturatedMaterialBoost(1.0f), unsaturatedMaterialBoost(1.0f) {}

        ExpressiveParameters(const Any& a);
        Any toAny() const;
    };
    
    /** 
        \brief A BSDF impulse ("delta function").
     */
    class Impulse {
    public:
        /** Unit length facing away from the scattering point. This
            may either be an incoming or outgoing direction depending
            on how it was sampled. */
        Vector3       direction;

        /** Probability of scattering along this impulse; the integral
            of the non-finite portion of the BSDF over a small area
            about direction.  The magnitude must be on the range [0,
            1] for each channel because it is a probability.

            The factor by which incoming radiance is scaled to get
            outgoing radiance.
        */
        Color3        magnitude;

        Impulse(const Vector3& d, const Color3& m) : direction(d), magnitude(m) {}

        // For SmallArray's default constructor
        Impulse() {}
    };

    /** 
        \brief Impulses in the BSDF.

        This contains three inline-allocated elements to support reflection and
        refraction without heap allocation. */
    typedef SmallArray<Impulse, 2> ImpulseArray;
        
    /** Point in world space at the geometric center of
        this surfel. */ 
    Point3                  position;

    /** Point in world space at the geometric center of
        this surfel in the previously rendered frame of animation.
        Useful for computing velocity. */
    Point3                  prevPosition;

    /** The normal to the true underlying geometry of the patch that
        was sampled (e.g., the face normal).  This is often useful for ray bumping. 

        Always a unit vector.
    */
    Vector3                 geometricNormal;

    /** The normal to the patch for shading purposes, i.e., after
        normal mapping.  e.g., the interpolated vertex normal
        or normal-mapped normal.

        Always a unit vector.*/
    Vector3                 shadingNormal;

    /** Primary tangent vector for use in shading anisotropic surfaces.  
       This is the tangent space after normal mapping has been applied.*/
    Vector3                 shadingTangent1;

    /** Secondary shading tangent. \see shadingTangent1 */
    Vector3                 shadingTangent2;
    
    /** Ratio of complex refractive indices for each side of the interface.  

        eta (\f$\eta\f$) is the real part, which determines the angle of refraction.
        kappa (\f$\kappa\f$) is the imaginary part, which determines absorption within
        the medium.

        For a non-transmissive medium, eta should be NaN() and kappa
        should be Color3::inf().

        The "Pos" versions are for the material on the side of
        the surface that the geometricNormal faces towards.  The "Neg"
        versions are for the material on the side opposite the
        geometricNormal.

        We ignore the frequency-variation in eta, since three color
        samples isn't enough to produce meaningful chromatic
        dispersion anyway.
    */
    float                   etaRatio = 1.0f;

    /** \copydoc etaRatio */
    Color3                  kappaPos;

    /** \copydoc etaRatio */
    Color3                  kappaNeg;

    /** The material that generated this Surfel. May be nullptr. This is a raw pointer
        because incrementing a shared_ptr is expensive during material sampling. */
    const Material*         material;

    /** The surface that generated this Surfel. May be nullptr. This is a raw pointer
        because incrementing a shared_ptr is expensive during material sampling. */
    const Surface*          surface;

    /** 0x1 = light flag
        
        The other 7 bits are reserved for future use.
    */
    uint8                   flags = 0;

    /** Mostly for debugging */
    struct Source {
        /** Index of this primitive in the source object (interpretation depends on the object type). 
         For TriTree, use this with TriTree:operator[] as a primitive index.*/
        int                 index;

        /** Barycentric coordinate corresponding to vertex 1 (NOT vertex 0) */
        float               u;

        /** Barycentric coordinate corresponding to vertex 2 */
        float               v;

        Source() : index(-1), u(0), v(0) {}
        Source(int i, float u, float v) : index(i), u(u), v(v) {}
    } source;

protected:

    Surfel
    (const Point3&     position,
     const Point3&     prevPosition,
     const Vector3&    geometricNormal,
     const Vector3&    shadingNormal,
     const Vector3&    shadingTangent1,
     const Vector3&    shadingTangent2,
     const float       etaPos,
     const Color3&     kappaPos,
     const float       etaNeg,
     const Color3&     kappaNeg,
     const uint8       flags,
     const Source&     source,
     const Material*   material,
     const Surface*    surface);

    /** Called by the default implementation of scatter. Samples a directional PDF

        Samples \f$ \hat{\omega}_i \f$ from some distribution \f$ p(\hat{\omega}_i) \f$ on the sphere. 
        This is optimal if proportional to \f$ f^*(\hat{\omega}_i, \hat{\omega}_o) \cdot | \hat{\omega}_u \cdot \hat{n}| \f$,
        where \f$ f^* \f$ is the finite portion of the BSDF (no impulses) but can be anything that is nonzero where \$f\$ is nonzero.
        
        Returns \a w_i = \f$ \hat{\omega}_i \f$ and \a pdfValue = \f$ p(\hat{\omega}_i) \f$
        */
    virtual void sampleFiniteDirectionPDF
    (PathDirection      pathDirection,
     const Vector3&     w_o,
     Random&            rng,
     const ExpressiveParameters& expressiveParameters,
     Vector3&           w_i,
     float&             pdfValue) const;

public:

    Surfel() : etaRatio(1.0f) {}

    /**
    True if this surfel was produced by a surface that came from a Light, and thus may have 
        emissive terms that are already accounted for by direct illumination. */
    bool isLight() const {
        return (flags & 1) != 0;
    }

    virtual ~Surfel() {}

    /** 
        \brief Returns the radiance emitted by this surface in
        direction \a wo.

        This models an emission function.

        Defaults to zero. N.B. A "Lambertian emitter", which is often
        considered the ideal area light source, would return a
        constant value on the side

        Note that this interface is intentionally insufficient for
        representing a point light, which requires a biradiance
        function.

        If this function returns a non-zero value, then emissive()
        must return true.
     */
    virtual Radiance3 emittedRadiance(const Vector3& wo) const {
        return Radiance3::zero();
    }

    /** Transform this to world space using the provided \a xform. */
    virtual void transformToWorldSpace(const CoordinateFrame& xform);

    /** Must return true if a ray is ever scattered to the opposite side
        of the surface with respect to the Surfel::shadingNormal.  
        
        This may conservatively always return true (which is the 
        default implementation). Setting this correctly doubles the 
        efficiency of the default implementation of other methods.

        There is no equivalent reflective() method because all physical
        transmissive surfaces are also reflective, so it would rarely be
        false in practice.
       */
    virtual bool transmissive() const {
        return true;
    }

    /** 
        True if this surfel's finiteScatteringDensity function ever
        returns a non-zero value.  (May also be true even if it does
        only ever return zero, however).
     */
    virtual bool nonZeroFiniteScattering() const {
        return true;
    }

    /** 
        \brief Evaluates the finite portion of \f$f_X(\hat{\omega_\mathrm{i}}, \hat{\omega_\mathrm{o}})\f$.
        
        This is the straightforward abstraction of the BSDF, i.e.,
        "forward BSDF evaluation".  It is useful for direct illumination 
        and in the gather kernel of a ray tracer.  Note that this does <i>not</i>
        scale the result by \f$|\hat{n} \cdot \hat{\omega}_\mathrm{i}|\f$.

        It omits the impulses because they would force the return
        value to infinity.

        \param wi Unit vector pointing backwards along the direction from which
        light came ("to the light source")
        
        \param wi Unit vector pointing out in the direction that light will go
        ("to the eye")
     */
    virtual Color3 finiteScatteringDensity
    (const Vector3&    wi, 
     const Vector3&    wo,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const = 0;


    /**
       Provided as a convenience helper method for implementers of scatter().
       Allows programmatically swapping the directions (which only matters for
       refraction if the BSDF is reciprocal).

       - <code>finiteScatteringDensity(PathDirection::SOURCE_TO_EYE, wi, wo)</code> = \f$f_X(\hat{\omega_\mathrm{i}}, \hat{\omega_\mathrm{o}})\f$
       - <code>finiteScatteringDensity(PathDirection::EYE_TO_SOURCE, wo, wi)</code>= \f$f_X(\hat{\omega_\mathrm{o}}, \hat{\omega_\mathrm{i}})\f$
     */
    virtual Color3 finiteScatteringDensity
    (PathDirection pathDirection,
     const Vector3&    wFrom, 
     const Vector3&    wTo,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const;


    /**
     \brief Given \a w, returns all wo directions that yield
      impulses in \f$f_X(\hat{\omega}, \hat{\omega_\mathrm{o}})\f$ for PathDirection::SOURCE_TO_EYE
      paths, and vice versa for PathDirection::EYE_TO_SOURCE paths.

      Overwrites the impulseArray.

      For example, <code>getImpulses(PathDirection::SOURCE_TO_EYE, wi,
      impulseArray)</code> returns the impulses for scattering a
      photon emitted from a light.

      <code>getImpulses(PathDirection::EYE_TO_SOURCE, wo,
      impulseArray)</code> returns the impulses for scattering a
      backwards-traced ray from the ey.
     */
    virtual void getImpulses
    (PathDirection     direction,
     const Vector3&    w,
     ImpulseArray&     impulseArray,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const = 0;


    static float ignore;
    static bool ignoreBool;

    /** 
     \brief Computes the direction of a scattered photon  
     and a \a weight that compensates for the way that the the sampling process is
     performed. 

     For use in Monte Carlo integration of the rendering equation, this computes
     the cosine-weighted BSDF \f$ f(\hat{\omega}_i, \hat{\omega}_o) | \hat{\omega}_i \cdot \hat{n}| \f$ term.

     The \a weight is needed to allow efficient computation of scattering
     from BSDFs that have no computationally efficient method for exact
     analytic sampling.  The caller should scale the radiance or power
     transported by the \a weight. 

     - For forward rays (e.g., photon tracing), call: <code>scatter(PathDirection::SOURCE_TO_EYE, wi, ..., wo)</code></li>
     - For backwards rays (e.g., path tracing), call: <code>scatter(PathDirection::EYE_TO_SOURCE, wo, ..., wi)</code></li>

     Let <code>h(w)</code> be the PDF that the implementation actually samples with respect to.
     Let <code>g(w) = f(w_before, w_after) |w_after.dot(n)|</code>, the function that scatter
     samples.  [Note: <code>g(w)</code> is *not* a PDF; just the terms from the integrand of the rendering equation]

     The function produces two outputs:

     \code
     w = sample with chosen with respect to pdf h(w)
     weight = g(w) / h(w)
     \endcode

     Note that the weight might be zero even if the photon scatters, for
     example, a perfectly "red" photon striking a perfectly "green" surface
     would have a weight of zero.

     The \a probabilityHint is for use by photon mappers to decrease
     convergence time.  If the a priori probability density of scattering
     in this direction was infinity, it returns 1.0.  If the density was
     0.0, it returns 0.  For finite densities it scales between them.

     If \a russianRoulette is true, then with probability proportional
     to the initial weight paths will be terminated. Nonterminated paths
     have weights increased proportionally to maintain the mean. (This is
     useful for photon mapping and pure path tracing.) The weight will be
     zero if the path terminates.

     Note that because they are driven by the finite portion of the BSDF,
     weights returned must be non-negative and finite, but are not bounded.
     However, an efficient importance-sampling implementation will return
     weights close to 1.0. 
        
     The default implementation relies on getIncoming() and inefficient
     cosine (vs. importance) sampling against
     evaluateFiniteScatteringDensity().  Thus although it will work for
     any evaluateFiniteScatteringDensity() implementation, it is desirable to optimize the
     implementation of scatter() in subclasses where possible.

     Consider this in the context of Monte Carlo integration of 
     \f$ \int_{\mathbf{S}^2} L(X, \hat{\omega}) f(\hat{\omega}, \hat{\omega}') |\hat{\omega} \cdot \hat{n}| d\hat{\omega} \f$.
     For that integral, choose \f$ k \f$ samples and weight as described below, and then 
     multiply each by the corresponding \f$ L(X, \hat{\omega}) / k \f$ and sum.

     This is single-importance sampling based on scattering. it gives good convergence
     when \f$ L(X, \hat{\omega}) \f$ is uniform, and is often the best we can do and is at least correct
     otherwise.

     Here are several different ways of *implementing* this method for a common example of a
     perfectly lambertian (`f() = 1 / pif()`) BRDF.

     A noise-minimizing implementation:

\code
      // Direction:
      w      = Vector3::cosHemiRandom(n);

      //                     g(w)                          /  h(w)   
      // weight = ( abs(w.dot(n)) * f(w) )                  / (abs(w.dot(n)) * 2 / (2*pif()))
      // weight = ( abs(w.dot(n)) / pif() )                 / (abs(w.dot(n)) * 2 / (2*pif()))
      weight = Color3::one();
\endcode


   Uniformly random hemisphere implementation:

   \code
      // Direction:
      w      = Vector3::hemiRandom(n);

      //                     g(w)                          /  h(w)   
      // weight = abs(w.dot(n)) * f(w))                     / (1.0 / (2.0f * pif()));
      // weight = abs(w.dot(n)) * (Color3::one() / pif())   / (1.0 / (2.0f * pif()));
      weight = abs(w.dot(n)) * 2.0f;
   \endcode


   Uniformly random sphere functions:

   \code
      // Direction:
      w      = Vector3::random();

      //                   g(w)                          /  h(w)   
      // weight = abs(w.dot(n)) * f(w)                      / (1.0 / (4.0f * pif()));
      // weight = abs(w.dot(n)) * (Color3::one() / pif()) * max(0, sign(w.dot(n))) / (1.0 / (4.0f * pif()));
      weight = abs(w.dot(n)) * max(0, sign(w.dot(n))) * 4.0f;
   \endcode

   There are of course a limitless number of ways of implementing the scattering.

   Returns true if the weight is nonzero.
   */
    virtual bool scatter
    (PathDirection    pathDirection,
     const Vector3&   w_before,
     bool             russianRoulette,
     Random&          rng,
     Color3&          weight,
     Vector3&         w_after,
     bool&            impulseScattered = ignoreBool,
     float&           probabilityHint = ignore,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const;


    /**
\brief Given direction \a w, returns the a priori probability of scattering in any direction (vs. absorption), 
which is the directional-hemispherical reflectance and transmittance when tracing PathDirection::EYE_TO_SOURCE and
hemispherical-directional reflectance and transmittance when tracing PathDirection::SOURCE_TO_EYE:
\f[
\int_{\mathbf{S}^2} f_X(\hat{\omega_\mathrm{i}}, \hat{\omega_\mathrm{o}}) |\hat{\omega_\mathrm{o}} \cdot \hat{n}| d\hat{\omega_\mathrm{o}}.\f]
        
This method is invoked by scatter().
        
By default, the probability computed by sampling
evaluateFiniteScatteringDensity() and getImpulses().
BSDFs that have analytic integrals can override these methods
to execute more efficiently.

The return value is necessarily on [0, 1] for each frequency unless non-default expressive parameters are used.
    */
    virtual Color3 probabilityOfScattering(PathDirection pathDirection, const Vector3& w, Random& rng,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const;

    /** Approximate reflectivity of this surface, primarily used for ambient terms. 
        The default implementation invokes probabilityOfScattering many times 
        and is not very efficient.
    */
    virtual Color3 reflectivity(Random& rng,
     const ExpressiveParameters& expressiveParameters = ExpressiveParameters()) const;
};

} // namespace G3D
