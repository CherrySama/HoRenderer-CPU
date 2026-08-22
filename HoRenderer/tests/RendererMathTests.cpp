#include "../src/Core/BSDF.hpp"
#include "../src/Core/Hittable.hpp"
#include "../src/Core/Integrator.hpp"
#include "../src/Core/Light.hpp"
#include "../src/Core/Material.hpp"
#include "../src/Core/Medium.hpp"
#include "../src/Core/PhaseFunction.hpp"
#include "../src/Core/Sampler.hpp"
#include "../src/Core/Scene.hpp"

#include <filesystem>
#include <functional>
#include <iomanip>
#include <string_view>

namespace {

class TestDiffuse : public Diffuse {
public:
    using Diffuse::Diffuse;

    void SetNormalTexture(std::shared_ptr<Texture>& texture)
    {
        SetNormal(texture);
    }

    Vector3f SurfaceNormal(const Hit_Payload& hit) const
    {
        return GetSurfaceNormal(hit);
    }
};

class CenterDarkEmissionTexture : public Texture {
public:
    CenterDarkEmissionTexture() : Texture(TextureType::SOLIDCOLOR) {}

    Vector3f GetColor(float u, float v) const override
    {
        if (std::abs(u - 0.5f) <= 1e-6f && std::abs(v - 0.5f) <= 1e-6f) {
            return Vector3f(0.0f);
        }
        return Vector3f(1.0f);
    }
};

class TestRunner {
public:
    void Run(std::string_view name, const std::function<void()>& test)
    {
        const int failures_before = failed_checks;
        test();

        if (failed_checks == failures_before) {
            ++passed_tests;
            std::cout << "[PASS] " << name << '\n';
        } else {
            ++failed_tests;
            std::cout << "[FAIL] " << name << '\n';
        }
    }

    void Check(bool condition, std::string_view message)
    {
        if (condition) {
            return;
        }

        ++failed_checks;
        std::cout << "       " << message << '\n';
    }

    void CheckNear(float actual, float expected, float tolerance, std::string_view message)
    {
        if (std::isfinite(actual) && std::abs(actual - expected) <= tolerance) {
            return;
        }

        ++failed_checks;
        std::cout << "       " << message
                  << " (actual=" << actual
                  << ", expected=" << expected
                  << ", tolerance=" << tolerance << ")\n";
    }

    void CheckVectorNear(const Vector3f& actual,
                         const Vector3f& expected,
                         float tolerance,
                         std::string_view message)
    {
        const bool finite = std::isfinite(actual.x) && std::isfinite(actual.y) && std::isfinite(actual.z);
        if (finite && glm::all(glm::lessThanEqual(glm::abs(actual - expected), Vector3f(tolerance)))) {
            return;
        }

        ++failed_checks;
        std::cout << "       " << message
                  << " (actual=" << actual.x << ", " << actual.y << ", " << actual.z
                  << "; expected=" << expected.x << ", " << expected.y << ", " << expected.z
                  << "; tolerance=" << tolerance << ")\n";
    }

    int Finish() const
    {
        std::cout << "\nSummary: " << passed_tests << " tests passed, "
                  << failed_tests << " tests failed, "
                  << failed_checks << " checks failed\n";
        return failed_tests == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

private:
    int passed_tests = 0;
    int failed_tests = 0;
    int failed_checks = 0;
};

void TestCosineHemisphere(TestRunner& runner)
{
    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(17, 31);

    constexpr int sample_count = 8192;
    const Vector3f normal(0.0f, 0.0f, 1.0f);
    float mean_cosine = 0.0f;
    bool all_finite = true;
    bool all_in_hemisphere = true;

    for (int i = 0; i < sample_count; ++i) {
        sampler.SetCurrentSample(i);
        const Vector3f direction = sampler.SampleCosineHemisphere(normal);
        all_finite = all_finite &&
                     std::isfinite(direction.x) &&
                     std::isfinite(direction.y) &&
                     std::isfinite(direction.z);
        all_in_hemisphere = all_in_hemisphere && glm::dot(direction, normal) >= -1e-6f;
        mean_cosine += glm::dot(direction, normal);
    }

    mean_cosine /= static_cast<float>(sample_count);
    runner.Check(all_finite, "cosine hemisphere sampler produced a non-finite direction");
    runner.Check(all_in_hemisphere, "cosine hemisphere sampler produced a direction below the hemisphere");
    runner.CheckNear(mean_cosine, 2.0f / 3.0f, 0.01f,
                     "cosine-weighted hemisphere mean cosine is incorrect");
}

void TestDiffuseLambertLimit(TestRunner& runner)
{
    const Vector3f albedo(0.8f, 0.6f, 0.4f);
    Diffuse material(albedo, 0.0f);

    Hit_Payload hit;
    hit.normal = Vector3f(0.0f, 0.0f, 1.0f);
    hit.uv = Vector2f(0.5f);
    hit.front_face = true;

    const Ray incoming(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -1.0f));
    const Vector3f outgoing = glm::normalize(Vector3f(0.6f, 0.0f, 0.8f));

    float pdf = 0.0f;
    const Vector3f evaluated = material.Evaluate(incoming, hit, outgoing, pdf);

    runner.CheckNear(pdf, outgoing.z * INV_PI, 1e-6f,
                     "diffuse PDF does not match cosine hemisphere sampling");
    runner.CheckVectorNear(evaluated, albedo * INV_PI, 1e-5f,
                           "zero-roughness diffuse does not reduce to Lambertian albedo / pi");
}

void TestLinearRadianceAccumulation(TestRunner& runner)
{
    const Vector3f first_sample(0.0f);
    const Vector3f second_sample(4.0f);

    const Vector3f linear_average = 0.5f * (first_sample + second_sample);
    const Vector3f display_after_average = LinearToSRGB(ACESFilmicToneMapping(linear_average));
    const Vector3f average_after_display =
        0.5f * (LinearToSRGB(ACESFilmicToneMapping(first_sample)) +
                LinearToSRGB(ACESFilmicToneMapping(second_sample)));

    runner.CheckVectorNear(display_after_average, Vector3f(0.961598f), 1e-5f,
                           "linear radiance average is not transformed to the expected display value");
    runner.Check(glm::compMax(glm::abs(display_after_average - average_after_display)) > 0.4f,
                 "test samples do not expose nonlinear display-space accumulation");
}

void TestLightCategoryPdfConsistency(TestRunner& runner)
{
    const std::filesystem::path hdr_path =
        std::filesystem::path(__FILE__).parent_path().parent_path() /
        "assets/scenes/spaichingen_hill_4k.hdr";
    auto hdr_texture = std::make_shared<HDRTexture>(hdr_path.string());
    if (hdr_texture->getWidth() <= 0 || hdr_texture->getHeight() <= 0) {
        runner.Check(false, "failed to load the HDR fixture for light-category PDF validation");
        return;
    }

    auto environment_light = std::make_shared<InfiniteAreaLight>(hdr_texture, 1.0f);
    auto emission = std::make_shared<Emission>(Vector3f(15.0f, 12.0f, 8.0f));
    auto quad = std::make_shared<Quad>(Vector3f(-1.0f, 1.0f, 2.0f),
                                      Vector3f(2.0f, 0.0f, 0.0f),
                                      Vector3f(0.0f, -2.0f, 0.0f),
                                      Transform(),
                                      emission);
    auto finite_light = std::make_shared<QuadAreaLight>(quad);

    Scene combined_scene;
    combined_scene.AddLights(finite_light);
    combined_scene.AddEnvLight(environment_light);
    combined_scene.BuildLightTable();

    const Ray light_ray(Vector3f(0.0f), Vector3f(0.0f, 0.0f, 1.0f));
    Hit_Payload light_hit;
    const bool hit_light = quad->isHit(light_ray, Vector2f(Epsilon, Infinity), light_hit);
    runner.Check(hit_light, "light-category PDF fixture ray did not hit the finite light");
    if (!hit_light) {
        return;
    }

    float finite_base_pdf = 0.0f;
    const Vector3f finite_base_radiance = finite_light->Evaluate(light_ray, light_hit, finite_base_pdf);
    float finite_combined_pdf = 0.0f;
    const Vector3f finite_combined_radiance =
        combined_scene.EvaluateLights(light_ray, light_hit, finite_combined_pdf);

    runner.Check(finite_base_pdf > 0.0f, "finite-light fixture produced a zero base PDF");
    runner.CheckVectorNear(finite_combined_radiance, finite_base_radiance, 1e-6f,
                           "finite-light radiance changed when adding an environment light");
    runner.CheckNear(finite_combined_pdf, 0.5f * finite_base_pdf, 1e-6f,
                     "finite-light MIS PDF does not include the 50 percent category probability");

    const Ray environment_ray(Vector3f(0.0f), glm::normalize(Vector3f(1.0f, 0.25f, 0.5f)));
    Hit_Payload dummy_hit;
    float environment_base_pdf = 0.0f;
    const Vector3f environment_base_radiance =
        environment_light->Evaluate(environment_ray, dummy_hit, environment_base_pdf);
    float environment_combined_pdf = 0.0f;
    const Vector3f environment_combined_radiance =
        combined_scene.EvaluateEnvLight(environment_ray, environment_combined_pdf);

    runner.Check(environment_base_pdf > 0.0f, "environment-light fixture produced a zero base PDF");
    runner.CheckVectorNear(environment_combined_radiance, environment_base_radiance, 1e-6f,
                           "environment radiance changed when adding a finite light");
    runner.CheckNear(environment_combined_pdf, 0.5f * environment_base_pdf, 1e-6f,
                     "environment MIS PDF does not include the 50 percent category probability");

    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(11, 29);
    Hit_Payload shading_hit;
    shading_hit.p = Vector3f(0.0f);
    constexpr int sample_count = 4096;
    int finite_sample_count = 0;
    bool all_sample_pdfs_positive = true;
    const Vector3f finite_emission = emission->Emit(Vector2f(0.5f));
    for (int sample_index = 0; sample_index < sample_count; ++sample_index) {
        sampler.SetCurrentSample(sample_index);
        Vector3f sampled_direction;
        float sampled_pdf = 0.0f;
        const Hittable* sampled_light_shape = nullptr;
        const Vector3f sampled_radiance =
            combined_scene.SampleLights(light_ray, shading_hit, sampled_direction, sampled_pdf, sampler, sampled_light_shape);
        all_sample_pdfs_positive = all_sample_pdfs_positive && sampled_pdf > 0.0f;
        if (glm::all(glm::lessThanEqual(glm::abs(sampled_radiance - finite_emission), Vector3f(1e-6f)))) {
            ++finite_sample_count;
        }
    }

    const float finite_sample_fraction =
        static_cast<float>(finite_sample_count) / static_cast<float>(sample_count);
    runner.Check(all_sample_pdfs_positive,
                 "combined light sampling produced a non-positive component PDF");
    runner.CheckNear(finite_sample_fraction, 0.5f, 0.02f,
                     "combined light sampling does not select both light categories equally");

    Scene environment_only_scene;
    environment_only_scene.AddEnvLight(environment_light);
    float environment_only_pdf = 0.0f;
    environment_only_scene.EvaluateEnvLight(environment_ray, environment_only_pdf);
    runner.CheckNear(environment_only_pdf, environment_base_pdf, 1e-6f,
                     "environment-only scene does not retain the full environment PDF");

    Scene finite_only_scene;
    finite_only_scene.AddLights(finite_light);
    finite_only_scene.BuildLightTable();
    float finite_only_pdf = 0.0f;
    finite_only_scene.EvaluateLights(light_ray, light_hit, finite_only_pdf);
    runner.CheckNear(finite_only_pdf, finite_base_pdf, 1e-6f,
                     "finite-light-only scene does not retain the full finite-light PDF");
}

void TestEnvironmentContinuousSampling(TestRunner& runner)
{
    const std::filesystem::path hdr_path =
        std::filesystem::path(__FILE__).parent_path().parent_path() /
        "assets/scenes/spaichingen_hill_4k.hdr";
    auto hdr_texture = std::make_shared<HDRTexture>(hdr_path.string());
    if (hdr_texture->getWidth() <= 0 || hdr_texture->getHeight() <= 0) {
        runner.Check(false, "failed to load the HDR fixture for continuous sampling validation");
        return;
    }

    InfiniteAreaLight environment_light(hdr_texture, 1.0f);
    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(41, 67);
    const Ray reference_ray(Vector3f(0.0f), Vector3f(0.0f, 0.0f, 1.0f));
    Hit_Payload dummy_hit;
    dummy_hit.p = Vector3f(0.0f);

    bool sampled_inside_texel = false;
    bool all_finite = true;
    bool sample_evaluate_match = true;
    for (int sample_index = 0; sample_index < 256; ++sample_index) {
        sampler.SetCurrentSample(sample_index);
        Vector3f direction;
        float sampled_pdf = 0.0f;
        const Vector3f sampled_radiance =
            environment_light.Sample(reference_ray, dummy_hit, direction, sampled_pdf, sampler);

        const Vector2f uv = CartesianToSpherical(direction);
        const float pixel_x = uv.x * hdr_texture->getWidth();
        const float pixel_y = uv.y * hdr_texture->getHeight();
        const float fraction_x = pixel_x - std::floor(pixel_x);
        const float fraction_y = pixel_y - std::floor(pixel_y);
        sampled_inside_texel = sampled_inside_texel ||
            std::abs(fraction_x - 0.5f) > 0.05f ||
            std::abs(fraction_y - 0.5f) > 0.05f;

        float evaluated_pdf = 0.0f;
        const Vector3f evaluated_radiance =
            environment_light.Evaluate(Ray(Vector3f(0.0f), direction), dummy_hit, evaluated_pdf);
        const float pdf_scale = std::max({sampled_pdf, evaluated_pdf, 1e-6f});

        const bool vectors_finite =
            std::isfinite(direction.x) && std::isfinite(direction.y) && std::isfinite(direction.z) &&
            std::isfinite(sampled_radiance.x) && std::isfinite(sampled_radiance.y) &&
            std::isfinite(sampled_radiance.z) && std::isfinite(evaluated_radiance.x) &&
            std::isfinite(evaluated_radiance.y) && std::isfinite(evaluated_radiance.z);
        all_finite = all_finite && vectors_finite && std::isfinite(sampled_pdf) &&
                     std::isfinite(evaluated_pdf) && sampled_pdf > 0.0f && evaluated_pdf > 0.0f;
        sample_evaluate_match = sample_evaluate_match &&
            glm::all(glm::lessThanEqual(glm::abs(sampled_radiance - evaluated_radiance), Vector3f(1e-6f))) &&
            std::abs(sampled_pdf - evaluated_pdf) <= 1e-5f * pdf_scale;
    }

    runner.Check(sampled_inside_texel,
                 "environment sampling remained restricted to texel centers");
    runner.Check(all_finite,
                 "environment continuous sampling produced an invalid direction or PDF");
    runner.Check(sample_evaluate_match,
                 "environment Sample and Evaluate disagree on radiance or directional PDF");
}

void TestMeshFrontFaceUsesGeometry(TestRunner& runner)
{
    const std::filesystem::path mesh_path =
        std::filesystem::path(__FILE__).parent_path() /
        "assets/opposed_shading_normal.obj";
    auto material = std::make_shared<Diffuse>(Vector3f(0.5f));
    Mesh mesh(mesh_path.string(), Transform(), material);

    const Ray ray(Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, -1.0f));
    Hit_Payload hit;
    const bool did_hit = mesh.isHit(ray, Vector2f(Epsilon, Infinity), hit);

    runner.Check(did_hit, "geometric-normal fixture ray missed its triangle");
    if (!did_hit) {
        return;
    }
    runner.Check(hit.front_face,
                 "mesh front-face classification used the opposed shading normal");
    runner.CheckVectorNear(hit.geometric_normal, Vector3f(0.0f, 0.0f, 1.0f), 1e-6f,
                           "mesh did not preserve the triangle geometric normal");
    runner.Check(glm::dot(hit.normal, hit.geometric_normal) > 0.0f,
                 "mesh shading normal was not aligned to the geometric hemisphere");
    runner.Check(glm::length(hit.normal - hit.geometric_normal) > 0.1f,
                 "mesh shading normal was replaced by the geometric normal");
}

void TestNormalMapUsesMeshUvTangent(TestRunner& runner)
{
    const std::filesystem::path mesh_path =
        std::filesystem::path(__FILE__).parent_path() /
        "assets/uv_tangent_frame.obj";
    auto material = std::make_shared<TestDiffuse>(Vector3f(0.5f));
    std::shared_ptr<Texture> normal_texture =
        std::make_shared<SolidTexture>(Vector3f(1.0f, 0.5f, 0.5f));
    material->SetNormalTexture(normal_texture);
    Mesh mesh(mesh_path.string(), Transform(), material);

    const Ray ray(Vector3f(0.25f, 0.25f, 1.0f), Vector3f(0.0f, 0.0f, -1.0f));
    Hit_Payload hit;
    const bool did_hit = mesh.isHit(ray, Vector2f(Epsilon, Infinity), hit);

    runner.Check(did_hit, "normal-map tangent fixture ray missed its triangle");
    if (!did_hit) {
        return;
    }

    const Vector3f expected_tangent(0.0f, 1.0f, 0.0f);
    const Vector3f mapped_normal = material->SurfaceNormal(hit);
    runner.Check(glm::dot(mapped_normal, expected_tangent) > 0.99f,
                 "normal map did not use the mesh UV tangent frame");
}

void TestScatteringUsesGeometricHemisphere(TestRunner& runner)
{
    Hit_Payload hit;
    hit.normal = glm::normalize(Vector3f(0.0f, 1.0f, 0.25f));
    hit.geometric_normal = Vector3f(0.0f, 0.0f, 1.0f);

    const Vector3f view(0.0f, 0.0f, 1.0f);
    const Vector3f geometric_reflection = glm::normalize(Vector3f(0.0f, 1.0f, 0.25f));
    const Vector3f geometric_transmission = glm::normalize(Vector3f(0.0f, 1.0f, -0.25f));

    Diffuse diffuse(Vector3f(0.5f));
    Dielectric dielectric(Vector3f(1.0f), 0.2f, 0.2f, 1.5f, 1.0f);

    runner.Check(diffuse.IsScatteringDirectionValid(hit, view, geometric_reflection),
                 "opaque reflection was rejected by the geometric hemisphere test");
    runner.Check(!diffuse.IsScatteringDirectionValid(hit, view, geometric_transmission),
                 "opaque material accepted a direction through the geometric surface");
    runner.Check(dielectric.IsScatteringDirectionValid(hit, view, geometric_transmission),
                 "transmissive material rejected the opposite geometric hemisphere");
}

void TestDielectricSamplePreservesGeometricLobe(TestRunner& runner)
{
    Hit_Payload hit;
    hit.normal = glm::normalize(Vector3f(0.0f, 1.0f, 0.25f));
    hit.geometric_normal = Vector3f(0.0f, 0.0f, 1.0f);
    hit.front_face = true;
    hit.uv = Vector2f(0.5f);

    const Vector3f view(0.0f, 0.0f, 1.0f);
    const Ray incoming(Vector3f(0.0f), -view);
    Dielectric dielectric(Vector3f(1.0f), 0.0f, 0.0f, 1.5f, 1.0f);
    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(11, 7);

    int accepted_samples = 0;
    bool all_lobes_agree = true;
    for (int i = 0; i < 256; ++i) {
        sampler.SetCurrentSample(i);
        Vector3f direction;
        float pdf = 0.0f;
        dielectric.Sample(incoming, hit, direction, pdf, sampler);
        if (pdf <= Epsilon) {
            continue;
        }

        ++accepted_samples;
        const bool shading_reflection = glm::dot(view, hit.normal) *
                                        glm::dot(direction, hit.normal) > 0.0f;
        const bool geometric_reflection = glm::dot(view, hit.geometric_normal) *
                                          glm::dot(direction, hit.geometric_normal) > 0.0f;
        all_lobes_agree = all_lobes_agree && shading_reflection == geometric_reflection;
    }

    runner.Check(accepted_samples > 0,
                 "dielectric geometric-lobe validation rejected every sample");
    runner.Check(all_lobes_agree,
                 "dielectric returned a reflection/transmission sample in the wrong geometric lobe");
}

void TestFiniteLightHitIdentity(TestRunner& runner)
{
    auto first_emission = std::make_shared<Emission>(Vector3f(2.0f, 0.5f, 0.25f));
    auto first_shape = std::make_shared<Quad>(Vector3f(9.0f, 1.0f, 2.0f),
                                              Vector3f(2.0f, 0.0f, 0.0f),
                                              Vector3f(0.0f, -2.0f, 0.0f),
                                              Transform(),
                                              first_emission);
    auto first_light = std::make_shared<QuadAreaLight>(first_shape);

    auto second_emission = std::make_shared<Emission>(Vector3f(0.25f, 1.0f, 3.0f));
    auto second_shape = std::make_shared<Quad>(Vector3f(-1.0f, 1.0f, 3.0f),
                                               Vector3f(2.0f, 0.0f, 0.0f),
                                               Vector3f(0.0f, -2.0f, 0.0f),
                                               Transform(),
                                               second_emission);
    auto second_light = std::make_shared<QuadAreaLight>(second_shape);

    Scene scene;
    scene.AddLights(first_light);
    scene.AddLights(second_light);
    scene.BuildBVH();
    scene.BuildLightTable();

    const Ray light_ray(Vector3f(0.0f), Vector3f(0.0f, 0.0f, 1.0f));
    Hit_Payload light_hit;
    const bool hit_light = scene.isHit(light_ray, Vector2f(Epsilon, Infinity), light_hit);
    runner.Check(hit_light, "multi-light fixture ray did not hit the second light");
    if (!hit_light) {
        return;
    }

    float second_base_pdf = 0.0f;
    const Vector3f expected_radiance =
        second_light->Evaluate(light_ray, light_hit, second_base_pdf);
    const float total_power = first_light->GetPower() + second_light->GetPower();
    const float expected_pdf =
        second_base_pdf * second_light->GetPower() / total_power;

    float evaluated_pdf = 0.0f;
    const Vector3f evaluated_radiance =
        scene.EvaluateLights(light_ray, light_hit, evaluated_pdf);

    runner.CheckVectorNear(evaluated_radiance, expected_radiance, 1e-6f,
                           "finite-light evaluation did not use the light that was actually hit");
    runner.CheckNear(evaluated_pdf, expected_pdf, 1e-6f,
                     "finite-light MIS PDF did not belong to the light that was actually hit");
}

void TestTexturedFiniteLightPower(TestRunner& runner)
{
    auto emission_texture = std::make_shared<CenterDarkEmissionTexture>();
    auto material = std::make_shared<Emission>(emission_texture);
    auto quad_shape = std::make_shared<Quad>(Vector3f(-1.0f, -1.0f, 0.0f),
                                             Vector3f(2.0f, 0.0f, 0.0f),
                                             Vector3f(0.0f, 2.0f, 0.0f),
                                             Transform(),
                                             material);
    QuadAreaLight light(quad_shape);

    runner.Check(light.GetPower() > 0.0f,
                 "textured finite light with non-zero emission was assigned zero selection power");
}

void TestEnvironmentShadowBlockedByEmitter(TestRunner& runner)
{
    auto emission = std::make_shared<Emission>(Vector3f(4.0f));
    auto blocker_shape = std::make_shared<Quad>(Vector3f(-1.0f, 1.0f, 1.0f),
                                                Vector3f(2.0f, 0.0f, 0.0f),
                                                Vector3f(0.0f, -2.0f, 0.0f),
                                                Transform(),
                                                emission);
    auto blocker_light = std::make_shared<QuadAreaLight>(blocker_shape);

    Scene scene;
    scene.AddLights(blocker_light);
    scene.BuildBVH();
    scene.BuildLightTable();

    Integrator integrator(1, 1, 1, 1);
    const Ray shadow_ray(Vector3f(0.0f), Vector3f(0.0f, 0.0f, 1.0f));
    const Vector3f transmittance =
        integrator.CalculateShadowTransmittance(shadow_ray, scene, -1, nullptr);

    runner.CheckVectorNear(transmittance, Vector3f(0.0f), 1e-6f,
                           "an emitter incorrectly remained transparent to an environment shadow ray");
}

void TestFiniteLightShadowRequiresTarget(TestRunner& runner)
{
    auto blocker_emission = std::make_shared<Emission>(Vector3f(2.0f));
    auto blocker_shape = std::make_shared<Quad>(Vector3f(-1.0f, 1.0f, 1.0f),
                                                Vector3f(2.0f, 0.0f, 0.0f),
                                                Vector3f(0.0f, -2.0f, 0.0f),
                                                Transform(),
                                                blocker_emission);
    auto blocker_light = std::make_shared<QuadAreaLight>(blocker_shape);

    auto target_emission = std::make_shared<Emission>(Vector3f(5.0f));
    auto target_shape = std::make_shared<Quad>(Vector3f(-1.0f, 1.0f, 2.0f),
                                               Vector3f(2.0f, 0.0f, 0.0f),
                                               Vector3f(0.0f, -2.0f, 0.0f),
                                               Transform(),
                                               target_emission);
    auto target_light = std::make_shared<QuadAreaLight>(target_shape);

    Scene scene;
    scene.AddLights(blocker_light);
    scene.AddLights(target_light);
    scene.BuildBVH();
    scene.BuildLightTable();

    Integrator integrator(1, 1, 1, 1);
    const Ray shadow_ray(Vector3f(0.0f), Vector3f(0.0f, 0.0f, 1.0f));
    const Vector3f transmittance =
        integrator.CalculateShadowTransmittance(shadow_ray, scene, -1, target_shape.get());

    runner.CheckVectorNear(transmittance, Vector3f(0.0f), 1e-6f,
                           "a different emitter was incorrectly accepted as the sampled finite light");
}

void TestHenyeyGreensteinConsistency(TestRunner& runner)
{
    constexpr float g = 0.3f;
    constexpr int sample_count = 100000;
    HenyeyGreensteinPhase phase(g);
    const Vector3f incoming(0.0f, 0.0f, -1.0f);
    const Vector3f forward_axis = -incoming;

    double sampled_mean_cosine = 0.0;
    for (int i = 0; i < sample_count; ++i) {
        const float u = (static_cast<float>(i) + 0.5f) / static_cast<float>(sample_count);
        const float v = std::fmod(static_cast<float>(i) * 0.61803398875f, 1.0f);
        Vector3f outgoing;
        float pdf = 0.0f;
        phase.Sample(incoming, Vector2f(u, v), outgoing, pdf);
        sampled_mean_cosine += glm::dot(forward_axis, outgoing);
    }
    sampled_mean_cosine /= sample_count;

    double evaluated_integral = 0.0;
    double evaluated_mean_cosine = 0.0;
    const double dc = 2.0 / sample_count;
    for (int i = 0; i < sample_count; ++i) {
        const float cosine = -1.0f + (static_cast<float>(i) + 0.5f) * (2.0f / sample_count);
        const float sine = std::sqrt(std::max(0.0f, 1.0f - cosine * cosine));
        const Vector3f outgoing(sine, 0.0f, cosine);
        const double density = phase.Evaluate(incoming, outgoing);
        evaluated_integral += density * 2.0 * PI * dc;
        evaluated_mean_cosine += cosine * density * 2.0 * PI * dc;
    }

    runner.CheckNear(static_cast<float>(evaluated_integral), 1.0f, 2e-4f,
                     "Henyey-Greenstein phase function is not normalized");
    runner.CheckNear(static_cast<float>(sampled_mean_cosine), g, 2e-4f,
                     "Henyey-Greenstein sampler does not produce the requested mean cosine");
    runner.CheckNear(static_cast<float>(sampled_mean_cosine),
                     static_cast<float>(evaluated_mean_cosine),
                     2e-3f,
                     "Henyey-Greenstein sampler and Evaluate/Pdf describe opposite distributions");
}

bool IsFinite(const Vector3f& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

Hit_Payload MakeDefaultHit()
{
    Hit_Payload hit;
    hit.normal = Vector3f(0.0f, 0.0f, 1.0f);
    hit.uv = Vector2f(0.5f);
    hit.front_face = true;
    return hit;
}

void TestMicrofacetFiniteValues(TestRunner& runner)
{
    const Hit_Payload hit = MakeDefaultHit();
    const Ray incoming(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -1.0f));
    const Vector3f outgoing(0.0f, 0.0f, 1.0f);

    Conductor conductor(Vector3f(1.0f),
                        0.0f,
                        0.0f,
                        Vector3f(0.2f),
                        Vector3f(3.0f));
    float conductor_pdf = 0.0f;
    const Vector3f conductor_value = conductor.Evaluate(incoming, hit, outgoing, conductor_pdf);
    runner.Check(IsFinite(conductor_value) && std::isfinite(conductor_pdf),
                 "zero-roughness conductor produced NaN or infinity instead of a finite/delta-safe result");

    Plastic black_plastic(Vector3f(0.0f),
                          Vector3f(0.0f),
                          0.2f,
                          0.2f,
                          1.5f,
                          1.0f);
    float plastic_pdf = 0.0f;
    const Vector3f plastic_value = black_plastic.Evaluate(incoming, hit, outgoing, plastic_pdf);
    runner.Check(IsFinite(plastic_value) && std::isfinite(plastic_pdf),
                 "black plastic produced NaN or infinity when diffuse and specular weights are both zero");
    runner.CheckVectorNear(plastic_value, Vector3f(0.0f), 0.0f,
                           "black plastic Evaluate returned non-zero reflectance");
    runner.CheckNear(plastic_pdf, 0.0f, 0.0f,
                     "black plastic Evaluate returned a non-zero PDF");

    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(5, 13);
    sampler.SetCurrentSample(0);
    Vector3f sampled_direction;
    float sampled_plastic_pdf = 0.0f;
    const Vector3f sampled_plastic_value =
        black_plastic.Sample(incoming, hit, sampled_direction, sampled_plastic_pdf, sampler);
    runner.Check(IsFinite(sampled_plastic_value) && std::isfinite(sampled_plastic_pdf),
                 "black plastic Sample produced NaN or infinity");
    runner.CheckVectorNear(sampled_plastic_value, Vector3f(0.0f), 0.0f,
                           "black plastic Sample returned non-zero reflectance");
    runner.CheckNear(sampled_plastic_pdf, 0.0f, 0.0f,
                     "black plastic Sample returned a non-zero PDF");
}

void TestConductorDeltaTransport(TestRunner& runner)
{
    const Vector3f albedo(0.8f, 0.6f, 0.4f);
    const Vector3f eta(0.2f, 0.3f, 0.4f);
    const Vector3f k(3.0f, 2.5f, 2.0f);
    Conductor conductor(albedo, 0.0f, 0.0f, eta, k);
    Hit_Payload hit = MakeDefaultHit();
    hit.normal = glm::normalize(Vector3f(0.6f, 0.0f, 0.8f));
    hit.geometric_normal = Vector3f(0.0f, 0.0f, 1.0f);
    const Ray incoming(Vector3f(0.0f), glm::normalize(Vector3f(0.3f, 0.1f, -0.948683f)));
    const Vector3f view = -incoming.direction();
    const Vector3f expected_direction = glm::reflect(-view, hit.geometric_normal);

    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(7, 17);
    sampler.SetCurrentSample(0);

    Vector3f sampled_direction;
    float sampled_pdf = 0.0f;
    const Vector3f sampled_value =
        conductor.Sample(incoming, hit, sampled_direction, sampled_pdf, sampler);
    const float sampled_cosine = glm::dot(hit.geometric_normal, sampled_direction);
    const Vector3f sampled_throughput = sampled_value * sampled_cosine / sampled_pdf;
    const Vector3f expected_throughput =
        albedo * BSDF::FresnelConductor(view, hit.geometric_normal, eta, k);

    runner.Check(conductor.IsDelta(hit), "zero-roughness conductor is not marked as a delta material");
    runner.CheckVectorNear(sampled_direction, expected_direction, 1e-6f,
                           "delta conductor did not sample the unique mirror direction");
    runner.CheckNear(sampled_pdf, 1.0f, 0.0f,
                     "delta conductor did not return unit discrete probability mass");
    runner.CheckVectorNear(sampled_throughput, expected_throughput, 1e-5f,
                           "delta conductor throughput does not equal albedo times conductor Fresnel");

    float evaluated_pdf = 1.0f;
    const Vector3f evaluated_value =
        conductor.Evaluate(incoming, hit, sampled_direction, evaluated_pdf);
    runner.CheckVectorNear(evaluated_value, Vector3f(0.0f), 0.0f,
                           "delta conductor Evaluate returned a finite-density BRDF value");
    runner.CheckNear(evaluated_pdf, 0.0f, 0.0f,
                     "delta conductor Evaluate returned a finite-density PDF");
}

void TestPlasticZeroRoughnessFiniteValues(TestRunner& runner)
{
    const Hit_Payload hit = MakeDefaultHit();
    const Ray incoming(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -1.0f));
    const Vector3f outgoing(0.0f, 0.0f, 1.0f);
    Plastic plastic(Vector3f(0.0f),
                   Vector3f(1.0f),
                   0.0f,
                   0.0f,
                   1.5f,
                   1.0f);

    float evaluated_pdf = 0.0f;
    const Vector3f evaluated_value =
        plastic.Evaluate(incoming, hit, outgoing, evaluated_pdf);
    runner.Check(IsFinite(evaluated_value) && std::isfinite(evaluated_pdf),
                 "zero-roughness plastic Evaluate produced NaN or infinity");

    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(11, 19);
    sampler.SetCurrentSample(0);
    Vector3f sampled_direction;
    float sampled_pdf = 0.0f;
    const Vector3f sampled_value =
        plastic.Sample(incoming, hit, sampled_direction, sampled_pdf, sampler);
    runner.Check(IsFinite(sampled_value) && std::isfinite(sampled_pdf),
                 "zero-roughness plastic Sample produced NaN or infinity");
}

void TestDeltaEmitterMis(TestRunner& runner)
{
    const Vector3f albedo(0.8f, 0.6f, 0.4f);
    const Vector3f eta(0.2f, 0.3f, 0.4f);
    const Vector3f k(3.0f, 2.5f, 2.0f);
    const Vector3f emission_value(2.0f, 3.0f, 4.0f);

    auto mirror_material = std::make_shared<Conductor>(albedo, 0.0f, 0.0f, eta, k);
    auto mirror = std::make_shared<Quad>(Vector3f(-1.0f, 1.0f, 1.0f),
                                         Vector3f(2.0f, 0.0f, 0.0f),
                                         Vector3f(0.0f, -2.0f, 0.0f),
                                         Transform(),
                                         mirror_material);

    auto emitter_material = std::make_shared<Emission>(emission_value);
    auto emitter_shape = std::make_shared<Quad>(Vector3f(-1.0f, -1.0f, -1.0f),
                                                Vector3f(2.0f, 0.0f, 0.0f),
                                                Vector3f(0.0f, 2.0f, 0.0f),
                                                Transform(),
                                                emitter_material);
    auto emitter = std::make_shared<QuadAreaLight>(emitter_shape);

    Scene scene;
    scene.Add(mirror);
    scene.AddLights(emitter);
    scene.BuildBVH();
    scene.BuildLightTable();

    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(0, 0);
    sampler.SetCurrentSample(0);
    Integrator integrator(1, 1, 1, 4);
    const Ray camera_ray(Vector3f(0.0f), Vector3f(0.0f, 0.0f, 1.0f));
    const Vector3f radiance = integrator.VolumeIntegrator(camera_ray, 4, scene, sampler, -1);
    const Vector3f expected_radiance =
        albedo * BSDF::FresnelConductor(Vector3f(0.0f, 0.0f, -1.0f),
                                       Vector3f(0.0f, 0.0f, -1.0f),
                                       eta,
                                       k) *
        emission_value;

    runner.CheckVectorNear(radiance, expected_radiance, 1e-5f,
                           "delta path incorrectly applied finite-density MIS when it hit an emitter");
}

void TestIncomingMediumSegmentMisInvariance(TestRunner& runner)
{
    auto surface_material = std::make_shared<Diffuse>(Vector3f(0.7f));
    auto surface = std::make_shared<Quad>(Vector3f(-2.0f, -2.0f, 0.0f),
                                          Vector3f(4.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 4.0f, 0.0f),
                                          Transform(),
                                          surface_material);

    auto emitter_material = std::make_shared<Emission>(Vector3f(4.0f));
    auto emitter_shape = std::make_shared<Quad>(Vector3f(-2.0f, 2.0f, 2.0f),
                                                Vector3f(4.0f, 0.0f, 0.0f),
                                                Vector3f(0.0f, -4.0f, 0.0f),
                                                Transform(),
                                                emitter_material);
    auto emitter = std::make_shared<QuadAreaLight>(emitter_shape);

    auto absorbing_medium = std::make_shared<HomogeneousMedium>(
        Vector3f(0.0f),
        Vector3f(1.0f),
        std::make_shared<IsotropicPhase>());

    Scene scene;
    scene.Add(surface);
    scene.AddLights(emitter);
    scene.AddMedium(absorbing_medium);
    scene.BuildBVH();
    scene.BuildLightTable();

    Integrator integrator(1, 1, 1, 1);
    const Ray near_ray(Vector3f(0.0f, 0.0f, 0.25f), Vector3f(0.0f, 0.0f, -1.0f));
    const Ray far_ray(Vector3f(0.0f, 0.0f, 1.5f), Vector3f(0.0f, 0.0f, -1.0f));

    bool estimates_match = true;
    int compared_samples = 0;
    float maximum_relative_error = 0.0f;
    for (int sample_index = 0; sample_index < 4096 && compared_samples < 64; ++sample_index) {
        Sampler near_sampler(FilterType::UNIFORM);
        near_sampler.SetPixel(83, 89);
        near_sampler.SetCurrentSample(sample_index);
        const Vector3f near_radiance =
            integrator.VolumeIntegrator(near_ray, 1, scene, near_sampler, 0);

        Sampler far_sampler(FilterType::UNIFORM);
        far_sampler.SetPixel(83, 89);
        far_sampler.SetCurrentSample(sample_index);
        const Vector3f far_radiance =
            integrator.VolumeIntegrator(far_ray, 1, scene, far_sampler, 0);

        if (glm::compMax(near_radiance) <= Epsilon || glm::compMax(far_radiance) <= Epsilon) {
            continue;
        }

        const float error = glm::compMax(glm::abs(near_radiance - far_radiance));
        const float scale = glm::max(glm::compMax(glm::abs(near_radiance)), 1e-6f);
        const float relative_error = error / scale;
        maximum_relative_error = glm::max(maximum_relative_error, relative_error);
        estimates_match = estimates_match && relative_error <= 1e-5f;
        ++compared_samples;
    }

    if (!estimates_match) {
        std::cout << "       maximum incoming-segment MIS relative error="
                  << maximum_relative_error << '\n';
    }
    runner.Check(compared_samples == 64,
                 "medium MIS fixture did not find enough no-scatter path samples");
    runner.Check(estimates_match,
                 "incoming medium segment probability changed a surface's local MIS estimate");
}

void TestPlasticSampleEvaluateConsistency(TestRunner& runner)
{
    Plastic plastic(Vector3f(0.2f, 0.4f, 0.6f),
                    Vector3f(0.8f, 0.7f, 0.6f),
                    0.2f,
                    0.35f,
                    1.5f,
                    1.0f);
    const Hit_Payload hit = MakeDefaultHit();
    const Ray incoming(Vector3f(0.0f), glm::normalize(Vector3f(-0.45f, 0.1f, -0.8874f)));
    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(19, 37);

    bool values_match = true;
    bool pdfs_match = true;
    bool all_finite = true;
    for (int sample_index = 0; sample_index < 512; ++sample_index) {
        sampler.SetCurrentSample(sample_index);

        Vector3f outgoing;
        float sampled_pdf = 0.0f;
        const Vector3f sampled_value = plastic.Sample(incoming, hit, outgoing, sampled_pdf, sampler);

        float evaluated_pdf = 0.0f;
        const Vector3f evaluated_value = plastic.Evaluate(incoming, hit, outgoing, evaluated_pdf);

        all_finite = all_finite && IsFinite(sampled_value) && IsFinite(evaluated_value) &&
                     std::isfinite(sampled_pdf) && std::isfinite(evaluated_pdf);
        values_match = values_match &&
                       glm::all(glm::lessThanEqual(glm::abs(sampled_value - evaluated_value),
                                                   Vector3f(1e-5f)));
        pdfs_match = pdfs_match && std::abs(sampled_pdf - evaluated_pdf) <= 1e-6f;
    }

    runner.Check(all_finite, "plastic Sample/Evaluate consistency test encountered NaN or infinity");
    runner.Check(values_match, "plastic Sample and Evaluate return different BRDF values");
    runner.Check(pdfs_match, "plastic Sample and Evaluate return different PDFs");
}

void TestDielectricSampleEvaluateConsistency(TestRunner& runner)
{
    Dielectric dielectric(Vector3f(0.95f, 0.97f, 1.0f),
                          0.2f,
                          0.35f,
                          1.5f,
                          1.0f);
    const Hit_Payload hit = MakeDefaultHit();
    const Ray incoming(Vector3f(0.0f), glm::normalize(Vector3f(-0.5f, 0.1f, -0.8602f)));
    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(31, 43);

    bool values_match = true;
    bool pdfs_match = true;
    bool all_finite = true;
    int valid_reflection_count = 0;
    int valid_transmission_count = 0;
    float maximum_reflection_value_error = 0.0f;
    float maximum_transmission_value_error = 0.0f;
    float maximum_reflection_value_relative_error = 0.0f;
    float maximum_transmission_value_relative_error = 0.0f;
    float maximum_reflection_pdf_error = 0.0f;
    float maximum_transmission_pdf_error = 0.0f;
    float maximum_reflection_pdf_relative_error = 0.0f;
    float maximum_transmission_pdf_relative_error = 0.0f;
    constexpr float value_absolute_tolerance = 1e-6f;
    constexpr float pdf_absolute_tolerance = 1e-7f;
    constexpr float relative_tolerance = 1e-5f;

    for (int sample_index = 0; sample_index < 2048; ++sample_index) {
        sampler.SetCurrentSample(sample_index);

        Vector3f outgoing;
        float sampled_pdf = 0.0f;
        const Vector3f sampled_value = dielectric.Sample(incoming, hit, outgoing, sampled_pdf, sampler);

        float evaluated_pdf = 0.0f;
        const Vector3f evaluated_value = dielectric.Evaluate(incoming, hit, outgoing, evaluated_pdf);

        all_finite = all_finite && IsFinite(sampled_value) && IsFinite(evaluated_value) &&
                     std::isfinite(sampled_pdf) && std::isfinite(evaluated_pdf);

        const float value_error = glm::compMax(glm::abs(sampled_value - evaluated_value));
        const float value_scale = glm::max(glm::compMax(glm::abs(sampled_value)), 1e-8f);
        const float value_relative_error = value_error / value_scale;
        const float pdf_error = std::abs(sampled_pdf - evaluated_pdf);
        const float pdf_scale = glm::max(std::abs(sampled_pdf), 1e-8f);
        const float pdf_relative_error = pdf_error / pdf_scale;

        values_match = values_match &&
                       value_error <= value_absolute_tolerance + relative_tolerance * value_scale;
        pdfs_match = pdfs_match &&
                     pdf_error <= pdf_absolute_tolerance + relative_tolerance * pdf_scale;

        if (sampled_pdf > Epsilon) {
            const bool is_reflection =
                glm::dot(outgoing, hit.normal) * glm::dot(-incoming.direction(), hit.normal) > 0.0f;
            if (is_reflection) {
                ++valid_reflection_count;
                maximum_reflection_value_error = glm::max(maximum_reflection_value_error, value_error);
                maximum_reflection_value_relative_error =
                    glm::max(maximum_reflection_value_relative_error, value_relative_error);
                maximum_reflection_pdf_error = glm::max(maximum_reflection_pdf_error, pdf_error);
                maximum_reflection_pdf_relative_error =
                    glm::max(maximum_reflection_pdf_relative_error, pdf_relative_error);
            } else {
                ++valid_transmission_count;
                maximum_transmission_value_error = glm::max(maximum_transmission_value_error, value_error);
                maximum_transmission_value_relative_error =
                    glm::max(maximum_transmission_value_relative_error, value_relative_error);
                maximum_transmission_pdf_error = glm::max(maximum_transmission_pdf_error, pdf_error);
                maximum_transmission_pdf_relative_error =
                    glm::max(maximum_transmission_pdf_relative_error, pdf_relative_error);
            }
        }
    }

    if (!values_match || !pdfs_match) {
        std::cout << "       reflection samples=" << valid_reflection_count
                  << ", max value error=" << maximum_reflection_value_error
                  << ", max value relative error=" << maximum_reflection_value_relative_error
                  << ", max PDF error=" << maximum_reflection_pdf_error
                  << ", max PDF relative error=" << maximum_reflection_pdf_relative_error << '\n';
        std::cout << "       transmission samples=" << valid_transmission_count
                  << ", max value error=" << maximum_transmission_value_error
                  << ", max value relative error=" << maximum_transmission_value_relative_error
                  << ", max PDF error=" << maximum_transmission_pdf_error
                  << ", max PDF relative error=" << maximum_transmission_pdf_relative_error << '\n';
    }

    runner.Check(all_finite, "dielectric Sample/Evaluate consistency test encountered NaN or infinity");
    runner.Check(values_match, "dielectric Sample and Evaluate return different BSDF values");
    runner.Check(pdfs_match, "dielectric Sample and Evaluate return different PDFs");
    runner.Check(valid_reflection_count > 0,
                 "dielectric consistency test did not exercise a valid reflection sample");
    runner.Check(valid_transmission_count > 0,
                 "dielectric consistency test did not exercise a valid transmission sample");
}

void TestDielectricRadianceEtaScaling(TestRunner& runner)
{
    constexpr float interior_ior = 1.5f;
    constexpr float exterior_ior = 1.0f;
    constexpr float roughness = 0.5f;
    Dielectric dielectric(Vector3f(1.0f),
                          roughness,
                          roughness,
                          interior_ior,
                          exterior_ior);

    auto check_transmission = [&](const Hit_Payload& hit,
                                  const Ray& incoming,
                                  const Vector3f& outgoing,
                                  float eta_ratio,
                                  std::string_view message) {
        const Vector3f normal = hit.normal;
        const Vector3f view = -glm::normalize(incoming.direction());
        Vector3f half_vector = -glm::normalize(eta_ratio * view + outgoing);
        if (glm::dot(normal, half_vector) < 0.0f) {
            half_vector = -half_vector;
        }

        const float alpha = roughness * roughness;
        const float NdotV = std::abs(glm::dot(normal, view));
        const float NdotL = std::abs(glm::dot(normal, outgoing));
        const float HdotV = glm::dot(half_vector, view);
        const float HdotL = glm::dot(half_vector, outgoing);
        const float fresnel = BSDF::FresnelDielectric(view, half_vector, eta_ratio);
        const float distribution = BSDF::DistributionGGX(half_vector, normal, alpha, alpha);
        const float geometry =
            BSDF::GeometrySmithG1(view, half_vector, normal, alpha, alpha) *
            BSDF::GeometrySmithG1(outgoing, half_vector, normal, alpha, alpha);
        const float denominator = eta_ratio * HdotV + HdotL;
        const float projected_factor = std::abs(HdotL * HdotV / (NdotL * NdotV));
        const float expected = (1.0f - fresnel) * distribution * geometry * projected_factor /
                               (denominator * denominator) * eta_ratio * eta_ratio;

        float pdf = 0.0f;
        const Vector3f evaluated = dielectric.Evaluate(incoming, hit, outgoing, pdf);
        const float error = glm::compMax(glm::abs(evaluated - Vector3f(expected)));
        const float tolerance = 1e-5f * glm::max(expected, 1.0f);
        runner.Check(error <= tolerance, message);
        runner.Check(pdf > 0.0f && std::isfinite(pdf),
                     "dielectric eta-scaling fixture produced an invalid transmission PDF");
    };

    Hit_Payload entering_hit = MakeDefaultHit();
    const Ray entering_ray(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -1.0f));
    check_transmission(entering_hit,
                       entering_ray,
                       Vector3f(0.0f, 0.0f, -1.0f),
                       exterior_ior / interior_ior,
                       "dielectric entering transmission uses the wrong radiance eta scaling");

    Hit_Payload exiting_hit = MakeDefaultHit();
    exiting_hit.normal = Vector3f(0.0f, 0.0f, -1.0f);
    exiting_hit.front_face = false;
    const Ray exiting_ray(Vector3f(0.0f), Vector3f(0.0f, 0.0f, 1.0f));
    check_transmission(exiting_hit,
                       exiting_ray,
                       Vector3f(0.0f, 0.0f, 1.0f),
                       interior_ior / exterior_ior,
                       "dielectric exiting transmission uses the wrong radiance eta scaling");
}

void TestDielectricDeltaTransport(TestRunner& runner)
{
    const Vector3f albedo(0.8f, 0.9f, 1.0f);
    constexpr float interior_ior = 1.5f;
    constexpr float exterior_ior = 1.0f;
    Dielectric dielectric(albedo, 0.0f, 0.0f, interior_ior, exterior_ior);

    runner.Check(dielectric.IsDelta(MakeDefaultHit()),
                 "zero-roughness dielectric is not marked as a delta material");

    auto check_side = [&](const Hit_Payload& hit,
                          const Ray& incoming,
                          float eta_ratio,
                          int pixel_x,
                          std::string_view side_name) {
        Sampler sampler(FilterType::UNIFORM);
        sampler.SetPixel(pixel_x, 61);
        bool all_finite = true;
        bool directions_match = true;
        bool probabilities_match = true;
        bool throughputs_match = true;
        bool sampled_reflection = false;
        bool sampled_transmission = false;

        const Vector3f view = -glm::normalize(incoming.direction());
        const float fresnel = BSDF::FresnelDielectric(view, hit.normal, eta_ratio);
        const Vector3f reflection_direction = glm::reflect(-view, hit.normal);
        const Vector3f transmission_direction = glm::refract(-view, hit.normal, eta_ratio);

        for (int sample_index = 0; sample_index < 2048; ++sample_index) {
            sampler.SetCurrentSample(sample_index);
            Vector3f outgoing;
            float pdf = 0.0f;
            const Vector3f value = dielectric.Sample(incoming, hit, outgoing, pdf, sampler);

            all_finite = all_finite && IsFinite(value) && IsFinite(outgoing) && std::isfinite(pdf);
            if (!IsFinite(value) || !IsFinite(outgoing) || !std::isfinite(pdf) || pdf <= 0.0f) {
                continue;
            }

            const bool is_reflection = glm::dot(outgoing, hit.normal) > 0.0f;
            const float cosine = std::abs(glm::dot(hit.normal, outgoing));
            const Vector3f throughput = value * cosine / pdf;
            if (is_reflection) {
                sampled_reflection = true;
                directions_match = directions_match &&
                                   glm::length(outgoing - reflection_direction) <= 1e-6f;
                probabilities_match = probabilities_match && std::abs(pdf - fresnel) <= 1e-6f;
                throughputs_match = throughputs_match &&
                                    glm::all(glm::lessThanEqual(glm::abs(throughput - albedo),
                                                                Vector3f(1e-5f)));
            } else {
                sampled_transmission = true;
                directions_match = directions_match &&
                                   glm::length(outgoing - transmission_direction) <= 1e-6f;
                probabilities_match = probabilities_match && std::abs(pdf - (1.0f - fresnel)) <= 1e-6f;
                const Vector3f expected_throughput = albedo * eta_ratio * eta_ratio;
                throughputs_match = throughputs_match &&
                                    glm::all(glm::lessThanEqual(glm::abs(throughput - expected_throughput),
                                                                Vector3f(1e-5f)));
            }
        }

        runner.Check(all_finite, std::string(side_name) + " delta dielectric produced NaN or infinity");
        runner.Check(directions_match, std::string(side_name) + " delta dielectric sampled a non-delta direction");
        runner.Check(probabilities_match, std::string(side_name) + " delta dielectric returned the wrong discrete probability");
        runner.Check(throughputs_match, std::string(side_name) + " delta dielectric returned the wrong path throughput");
        runner.Check(sampled_reflection && sampled_transmission,
                     std::string(side_name) + " delta dielectric did not exercise both Fresnel branches");

        float evaluated_pdf = 1.0f;
        const Vector3f evaluated =
            dielectric.Evaluate(incoming, hit, reflection_direction, evaluated_pdf);
        runner.CheckVectorNear(evaluated, Vector3f(0.0f), 0.0f,
                               std::string(side_name) + " delta dielectric Evaluate returned a finite-density value");
        runner.CheckNear(evaluated_pdf, 0.0f, 0.0f,
                         std::string(side_name) + " delta dielectric Evaluate returned a finite-density PDF");
    };

    Hit_Payload entering_hit = MakeDefaultHit();
    check_side(entering_hit,
               Ray(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -1.0f)),
               exterior_ior / interior_ior,
               67,
               "entering");

    Hit_Payload exiting_hit = MakeDefaultHit();
    exiting_hit.normal = Vector3f(0.0f, 0.0f, -1.0f);
    exiting_hit.front_face = false;
    check_side(exiting_hit,
               Ray(Vector3f(0.0f), Vector3f(0.0f, 0.0f, 1.0f)),
               interior_ior / exterior_ior,
               71,
               "exiting");
}

void TestDielectricSingleZeroRoughnessAxis(TestRunner& runner)
{
    const Hit_Payload hit = MakeDefaultHit();
    const Ray incoming(Vector3f(0.0f), glm::normalize(Vector3f(-0.35f, 0.15f, -0.9247f)));
    bool all_finite = true;
    bool every_axis_has_valid_samples = true;
    int axis_index = 0;

    for (const Vector2f roughness : {Vector2f(0.0f, 0.35f), Vector2f(0.35f, 0.0f)}) {
        Dielectric dielectric(Vector3f(1.0f), roughness.x, roughness.y, 1.5f, 1.0f);
        Sampler sampler(FilterType::UNIFORM);
        sampler.SetPixel(73 + axis_index, 79);
        int valid_samples = 0;

        for (int sample_index = 0; sample_index < 512; ++sample_index) {
            sampler.SetCurrentSample(sample_index);
            Vector3f outgoing;
            float sampled_pdf = 0.0f;
            const Vector3f sampled = dielectric.Sample(incoming, hit, outgoing, sampled_pdf, sampler);

            float evaluated_pdf = 0.0f;
            const Vector3f evaluated = dielectric.Evaluate(incoming, hit, outgoing, evaluated_pdf);
            all_finite = all_finite && IsFinite(sampled) && IsFinite(outgoing) &&
                         IsFinite(evaluated) && std::isfinite(sampled_pdf) &&
                         std::isfinite(evaluated_pdf);
            if (sampled_pdf > Epsilon) {
                ++valid_samples;
            }
        }

        every_axis_has_valid_samples = every_axis_has_valid_samples && valid_samples > 0;
        ++axis_index;
    }

    runner.Check(all_finite,
                 "dielectric with one zero roughness axis produced NaN or infinity");
    runner.Check(every_axis_has_valid_samples,
                 "one dielectric zero-roughness axis produced no valid samples");
}

void TestFabricReciprocity(TestRunner& runner)
{
    Fabric fabric(Vector3f(0.9f, 0.7f, 0.2f), 0.12f, 0.2f, 0.9f);
    const Hit_Payload hit = MakeDefaultHit();

    const Vector3f view = glm::normalize(Vector3f(std::sqrt(1.0f - 0.2f * 0.2f), 0.0f, 0.2f));
    const Vector3f light = glm::normalize(Vector3f(std::sqrt(1.0f - 0.8f * 0.8f), 0.0f, 0.8f));

    float forward_pdf = 0.0f;
    float reverse_pdf = 0.0f;
    const Vector3f forward = fabric.Evaluate(Ray(Vector3f(0.0f), -view), hit, light, forward_pdf);
    const Vector3f reverse = fabric.Evaluate(Ray(Vector3f(0.0f), -light), hit, view, reverse_pdf);

    runner.CheckVectorNear(forward, reverse, 1e-4f,
                           "fabric BRDF violates Helmholtz reciprocity when view and light are exchanged");
    runner.CheckNear(forward_pdf, reverse_pdf, 1e-7f,
                     "fabric PDF changes when view and light are exchanged");
}

void TestFabricSampleEvaluateConsistency(TestRunner& runner)
{
    Fabric fabric(Vector3f(0.9f, 0.7f, 0.2f), 0.12f, 0.2f, 0.9f);
    const Hit_Payload hit = MakeDefaultHit();
    const Ray incoming(Vector3f(0.0f), glm::normalize(Vector3f(-0.4f, 0.0f, -0.916515f)));
    Sampler sampler(FilterType::UNIFORM);
    sampler.SetPixel(23, 47);

    bool values_match = true;
    bool pdfs_match = true;
    bool all_finite = true;
    for (int sample_index = 0; sample_index < 256; ++sample_index) {
        sampler.SetCurrentSample(sample_index);

        Vector3f outgoing;
        float sampled_pdf = 0.0f;
        const Vector3f sampled_value = fabric.Sample(incoming, hit, outgoing, sampled_pdf, sampler);

        float evaluated_pdf = 0.0f;
        const Vector3f evaluated_value = fabric.Evaluate(incoming, hit, outgoing, evaluated_pdf);

        all_finite = all_finite && IsFinite(sampled_value) && IsFinite(evaluated_value) &&
                     std::isfinite(sampled_pdf) && std::isfinite(evaluated_pdf);
        values_match = values_match &&
                       glm::all(glm::lessThanEqual(glm::abs(sampled_value - evaluated_value), Vector3f(1e-6f)));
        pdfs_match = pdfs_match && std::abs(sampled_pdf - evaluated_pdf) <= 1e-7f;
    }

    runner.Check(all_finite, "fabric Sample/Evaluate consistency test encountered NaN or infinity");
    runner.Check(values_match, "fabric Sample and Evaluate return different BRDF values");
    runner.Check(pdfs_match, "fabric Sample and Evaluate return different PDFs");
}

void TestFabricWhiteFurnace(TestRunner& runner)
{
    const Hit_Payload hit = MakeDefaultHit();
    constexpr int cosine_steps = 128;
    constexpr int phi_steps = 256;
    const double differential_solid_angle = 2.0 * PI / (cosine_steps * phi_steps);
    bool all_finite = true;
    Vector3d maximum_reflectance(0.0);
    Vector3f maximum_albedo(0.0f);
    float maximum_roughness = 0.0f;
    float maximum_weight = 0.0f;
    float maximum_tint = 0.0f;
    float maximum_view_cosine = 0.0f;

    for (const Vector3f& albedo : {Vector3f(0.9f, 0.7f, 0.2f), Vector3f(1.0f)}) {
        for (float roughness : {0.01f, 0.12f, 0.5f, 1.0f}) {
            for (float weight : {0.0f, 0.2f, 0.5f, 1.0f, 4.0f}) {
                for (float tint : {0.0f, 0.9f, 1.0f}) {
                    Fabric fabric(albedo, roughness, weight, tint);

                    for (float view_cosine : {0.01f, 0.1f, 0.5f, 0.9f}) {
                        const Vector3f view(std::sqrt(1.0f - view_cosine * view_cosine), 0.0f, view_cosine);
                        const Ray incoming(Vector3f(0.0f), -view);
                        Vector3d directional_reflectance(0.0);

                        for (int c = 0; c < cosine_steps; ++c) {
                            const float cosine = (static_cast<float>(c) + 0.5f) / cosine_steps;
                            const float sine = std::sqrt(std::max(0.0f, 1.0f - cosine * cosine));

                            for (int p = 0; p < phi_steps; ++p) {
                                const float phi = 2.0f * PI * (static_cast<float>(p) + 0.5f) / phi_steps;
                                const Vector3f outgoing(sine * std::cos(phi), sine * std::sin(phi), cosine);
                                float pdf = 0.0f;
                                const Vector3f value = fabric.Evaluate(incoming, hit, outgoing, pdf);
                                all_finite = all_finite && IsFinite(value) && std::isfinite(pdf);
                                directional_reflectance += Vector3d(value) * static_cast<double>(cosine) *
                                                           differential_solid_angle;
                            }
                        }

                        if (glm::compMax(directional_reflectance) > glm::compMax(maximum_reflectance)) {
                            maximum_reflectance = directional_reflectance;
                            maximum_albedo = albedo;
                            maximum_roughness = roughness;
                            maximum_weight = weight;
                            maximum_tint = tint;
                            maximum_view_cosine = view_cosine;
                        }
                    }
                }
            }
        }
    }

    runner.Check(all_finite, "fabric white-furnace integration encountered NaN or infinity");
    runner.Check(maximum_reflectance.x <= 1.01 &&
                     maximum_reflectance.y <= 1.01 &&
                     maximum_reflectance.z <= 1.01,
                 std::string("fabric creates energy in the white-furnace test; reflectance = (") +
                     std::to_string(maximum_reflectance.x) + ", " +
                     std::to_string(maximum_reflectance.y) + ", " +
                     std::to_string(maximum_reflectance.z) + "); albedo = (" +
                     std::to_string(maximum_albedo.x) + ", " +
                     std::to_string(maximum_albedo.y) + ", " +
                     std::to_string(maximum_albedo.z) + "), roughness = " +
                     std::to_string(maximum_roughness) + ", weight = " +
                     std::to_string(maximum_weight) + ", tint = " +
                     std::to_string(maximum_tint) + ", NdotV = " +
                     std::to_string(maximum_view_cosine));
}

} // namespace

int main()
{
    std::cout << std::fixed << std::setprecision(6);

    TestRunner runner;
    runner.Run("cosine hemisphere sampling", [&runner]() { TestCosineHemisphere(runner); });
    runner.Run("diffuse Lambertian limit", [&runner]() { TestDiffuseLambertLimit(runner); });
    runner.Run("linear radiance accumulation", [&runner]() { TestLinearRadianceAccumulation(runner); });
    runner.Run("light category PDF consistency", [&runner]() { TestLightCategoryPdfConsistency(runner); });
    runner.Run("environment continuous sampling", [&runner]() { TestEnvironmentContinuousSampling(runner); });
    runner.Run("mesh front face uses geometry", [&runner]() { TestMeshFrontFaceUsesGeometry(runner); });
    runner.Run("normal map uses mesh UV tangent", [&runner]() { TestNormalMapUsesMeshUvTangent(runner); });
    runner.Run("scattering uses geometric hemisphere", [&runner]() { TestScatteringUsesGeometricHemisphere(runner); });
    runner.Run("dielectric sample preserves geometric lobe", [&runner]() { TestDielectricSamplePreservesGeometricLobe(runner); });
    runner.Run("finite light hit identity", [&runner]() { TestFiniteLightHitIdentity(runner); });
    runner.Run("textured finite light power", [&runner]() { TestTexturedFiniteLightPower(runner); });
    runner.Run("environment shadow blocked by emitter", [&runner]() { TestEnvironmentShadowBlockedByEmitter(runner); });
    runner.Run("finite light shadow requires target", [&runner]() { TestFiniteLightShadowRequiresTarget(runner); });
    runner.Run("Henyey-Greenstein consistency", [&runner]() { TestHenyeyGreensteinConsistency(runner); });
    runner.Run("microfacet finite values", [&runner]() { TestMicrofacetFiniteValues(runner); });
    runner.Run("conductor delta transport", [&runner]() { TestConductorDeltaTransport(runner); });
    runner.Run("plastic zero roughness finite values", [&runner]() { TestPlasticZeroRoughnessFiniteValues(runner); });
    runner.Run("delta emitter MIS", [&runner]() { TestDeltaEmitterMis(runner); });
    runner.Run("incoming medium segment MIS invariance", [&runner]() { TestIncomingMediumSegmentMisInvariance(runner); });
    runner.Run("plastic Sample/Evaluate consistency", [&runner]() { TestPlasticSampleEvaluateConsistency(runner); });
    runner.Run("dielectric Sample/Evaluate consistency", [&runner]() { TestDielectricSampleEvaluateConsistency(runner); });
    runner.Run("dielectric radiance eta scaling", [&runner]() { TestDielectricRadianceEtaScaling(runner); });
    runner.Run("dielectric delta transport", [&runner]() { TestDielectricDeltaTransport(runner); });
    runner.Run("dielectric single zero roughness axis", [&runner]() { TestDielectricSingleZeroRoughnessAxis(runner); });
    runner.Run("fabric reciprocity", [&runner]() { TestFabricReciprocity(runner); });
    runner.Run("fabric Sample/Evaluate consistency", [&runner]() { TestFabricSampleEvaluateConsistency(runner); });
    runner.Run("fabric white furnace", [&runner]() { TestFabricWhiteFurnace(runner); });

    return runner.Finish();
}
