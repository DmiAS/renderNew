﻿#include "raythread.h"
#include "scene_manager.h"

const float eps_float = 1e-5;
bool checkIntersection(const float& t, const float& t_min, const float& t_max, const float& closest_t){
    return t > t_min && t < t_max && t < closest_t;
}

InterSectionData RayThread::closestIntersection(const Vec3f& o, const Vec3f& d, float t_min, float t_max){
    auto closest_t = std::numeric_limits<float>::max();
    Model closest_model;
    Vec3f normal;

    for (auto& model: models){
        auto p = model->interSect(o, d);
        if (checkIntersection(p.first.first, t_min, t_max, closest_t)){
            closest_t = p.first.first;
            closest_model = *model;
            normal = p.first.second;
        }
        if (checkIntersection(p.second.first, t_min, t_max, closest_t)) {
            closest_t = p.second.first;
            closest_model = *model;
            normal = p.second.second;
        }
    }

    return {closest_model, closest_t, normal.normalize()};
}

Vec3f RayThread::computeLightning(const Vec3f &p, const Vec3f &n, const Vec3f &direction, float specular){
    Vec3f i;
    Vec3f L;


    for (auto &model: models){
        if (model->isObject()) continue;
        Light* light = dynamic_cast<Light*>(model);
        if (light->t == Light::light_type::ambient)
            i += light->color_intensity;
        else{
            float t_max;
            if (light->t == Light::light_type::point){
                L = {light->position.x - p.x,
                     light->position.y - p.y,
                     light->position.z - p.z,
                    };
                t_max = 1;
            } else{
                L = light->direction;
                t_max = std::numeric_limits<float>::max();
            }

            float shadow_t = std::numeric_limits<float>::max();
            auto res = closestIntersection(p, L, 1e-3, t_max);
            if (fabs(res.t) < eps_float)
                continue;

            float n_dot_l = Vec3f::dot(n, L);
            if (n_dot_l > 0){
                auto denum = (n.len() * L.len());
                auto factor = n_dot_l / denum;
                i.x = light->color_intensity.x * factor;
                i.y = light->color_intensity.y * factor;
                i.z = light->color_intensity.z * factor;
            }

            //mirroing
//            if (!(fabs(specular + 1)< eps_float)){
//                auto r = (n * 2 * Vec3f::dot(n, L)) - L;
//                float r_dot_v = Vec3f::dot(r, direction);
//                if (r_dot_v > 0){
//                    auto len = r.len() * direction.len();
//                    auto factor = pow(r_dot_v / len, specular);
//                    i.x = light->color_intensity.x * factor;
//                    i.y = light->color_intensity.y * factor;
//                    i.z = light->color_intensity.z * factor;
//                }
//            }
        }

    }

    return i;
}

Vec3f reflectRay(const Vec3f& r, const Vec3f& n){
    return (n * 2 * Vec3f::dot(n, r)) - r;
}

Vec3f RayThread::traceRay(const Vec3f& o, const Vec3f& d, float t_min, float t_max, int depth){
    auto res = closestIntersection(o, d, t_min, t_max); // Model - closest_t

    if (fabs(res.t -  std::numeric_limits<float>::max()) < eps_float)
        return Vec3f{0, 0, 0};
//    return res.model.color * 255.f;

    auto p = o + d * res.t;

    auto computed_light = computeLightning(p, res.normal, -d, res.model.specular);

    auto local_color = res.model.color.hadamard(computed_light);

//    float r = res.model.reflective;
//    if (depth <= 0 || r <= 0){
//        return local_color;
//    }

//    if (r >= 0){
//        auto R = reflectRay(-d, res.normal);
//        auto reflected_color = traceRay(p, R, 1e-3, std::numeric_limits<float>::max(),
//                                        depth - 1);
//        local_color = local_color * (1 - r) + reflected_color * r;
//    }

    return local_color;

//    Vec3f p = o + d * res.second;

}

bool RayThread::sceneIntersect(const Ray &ray, InterSectionData &data, float t_min, float t_max){
    float closeset_t = std::numeric_limits<float>::infinity();
    bool intersected = false;
    for (auto& model: models){
        InterSectionData d;
        if (model->intersect(ray, d) && d.t > t_min && d.t < t_max && d.t < closeset_t){
            closeset_t = d.t;
            intersected = true;
            data = d;
            data.point = ray.origin + ray.direction * d.t;
            data.model = *model;
        }
    }

    return intersected;
}

Vec3f reflect(const Vec3f &I, const Vec3f &N) {
    return I - Vec3f::dot(N*2.f, Vec3f::dot(I, N));
}

Vec3f refract(const Vec3f &I, const Vec3f &N, const float eta_t, const float eta_i=1.f) { // Snell's law
    float cosi = - std::max(-1.f, std::min(1.f, Vec3f::dot(I, N)));
    if (cosi<0) return refract(I, -N, eta_i, eta_t); // if the ray comes from the inside the object, swap the air and the media
    float eta = eta_i / eta_t;
    float k = 1 - eta*eta*(1 - cosi*cosi);
    return k<0 ? Vec3f(1,0,0) : I*eta + N*(eta*cosi - sqrtf(k)); // k<0 = total reflection, no ray to refract. I refract it anyways, this has no physical meaning
}
Vec3f RayThread::cast_ray(const Ray &ray, int depth){
    float t_min = 1.f, t_max = std::numeric_limits<float>::max();
    InterSectionData data;
    if (depth > 4 || !sceneIntersect(ray, data, t_min, t_max))
        return Vec3f{0, 0, 0};
//    Vec3f reflect_dir = reflect(ray.direction, N).normalize();
//    Vec3f refract_dir = refract(ray.direction, N, data.model.refractive).normalize();
//    Vec3f reflect_orig = Vec3f::dot(reflect_dir, N) < 0 ? point - N*1e-3 : point + N*1e-3; // offset the original point to avoid occlusion by the object itself
//    Vec3f refract_orig = Vec3f::dot(refract_dir, N) < 0 ? point - N*1e-3 : point + N*1e-3;
//    Vec3f reflect_color = cast_ray(Ray(reflect_orig, reflect_dir), depth + 1);
//    Vec3f refract_color = cast_ray(Ray(refract_orig, refract_dir), depth + 1);

    float diffuse_light_intensity = 0, specular_light_intensity = 0;
    for (auto& model: models) {
        if (model->isObject()) continue;
        Light* light = dynamic_cast<Light*>(model);
        if (!light->t == Light::light_type::point) continue;
        Vec3f light_dir      = (light->position - data.point).normalize();
//        float light_distance = (light->position - point).len();

//        Vec3f shadow_orig = Vec3f::dot(light_dir, N) < 0 ? point - N*1e-3 : point + N*1e-3; // checking if the point lies in the shadow of the lights[i]
//        InterSectionData tmpData;
//        if (sceneIntersect(Ray(shadow_orig, light_dir), tmpData, t_min, t_max) && (tmpData.point - shadow_orig).len() < light_distance)
//            continue;

        diffuse_light_intensity  += light->lightning_power * std::max(0.f, Vec3f::dot(light_dir, data.normal));
//        specular_light_intensity += powf(std::max(0.f, Vec3f::dot(-reflect(-light_dir, N), ray.direction)), data.model.specular)*light->lightning_power;
    }
    return data.model.color * diffuse_light_intensity;

//    return data.model.color * diffuse_light_intensity + Vec3f(1., 1., 1.)*specular_light_intensity + reflect_color + refract_color;
}

Vec4f toWorld(int x, int y, const Mat4x4f& inverse, int width, int height){
    float ndc_x = (float)(x << 1) / (float)width - 1.f;
    float ndc_y = 1.f - (float)(y << 1) / (float)height;
    Vec4f res(ndc_x, ndc_y, 1);
    return res * inverse;
}

std::vector<RayBound> subBlock(const RayBound& bound, int depth){
    if (depth == 0){
        return {RayBound{.xs = bound.xs, .xe = bound.xe, .ys = bound.ys, .ye = bound.ye}};
    }
    std::vector<RayBound> output;
    int x_q = (bound.xe - bound.xs) % 2 == 0;
    int y_q = (bound.ye - bound.ys) % 2 == 0;
    int x_step = bound.xe / 2;
    int y_step = bound.ye / 2;
    for (int curr = x_step - x_q, prev = bound.xs; curr <= bound.xe;){
        for (int curr_y = y_step - y_q, prev_y = bound.ys; curr_y <= bound.ye;){
            auto v = subBlock(RayBound{.xs = prev, .xe = curr, .ys = prev_y, .ye = curr_y}, depth - 1);
            output.insert(output.end(), v.begin(), v.end());
            prev_y = curr_y + 1;
            curr_y += y_step + !y_q;
        }
        prev = curr + 1;
        curr += x_step + !x_q;
    }
    return output;
}

std::vector<RayBound> split(int width, int height){
    int cnt = 4;
    int step = height / 4;
    int start = 0;
    std::vector<RayBound> output;
    for (int i = 0; i < cnt; ++i) {
        output.push_back(RayBound{.xs = 0, .xe = width - 1, .ys = start, .ye = (start + step - 1) % width});
        start += step;
    }
//    return subBlock(RayBound{.xs = 0, .xe = width, .ys = 0, .ye = height}, 0);
    return output;
}

void SceneManager::trace(){
    img.fill(Qt::black);
    auto v = split(width, height);
    std::vector<RayThread*> threads;
    auto cam = camers[curr_camera];
    auto origin = cam.position;
    auto mat = cam.viewMatrix() * cam.projectionMatrix;
    auto inverse = Mat4x4f::Inverse(mat);
    for (auto& bound: v){
        auto th = new RayThread(img, models, inverse, bound, width, height, origin);
        th->start();
        qDebug() << "started";
        threads.push_back(th);
    }

    for (auto& th: threads){
        th->wait();
        delete th;
    }

//    for (int x = 0; x < width; x++){
//        for (int y = 0; y < height; y++){
////            float px = (2 * ((float)x / (float)width) - 1) * tan(cam.fov / 2 * M_PI / 180) * cam.aspect_ratio;
////            float py = (1 - 2 * ((float)y / (float)height)) * tan(cam.fov / 2 * M_PI / 180);
////            auto d = Vec3f{px, py, 1}.normalize();
////            auto d = canvasToViewPort(x, y);
////            qDebug() << height / 2 << d.x << d.y << d.z;
////            return;
//            auto d = toWorld(x, y, inverse, width, height).normalize();
//            auto color = traceRay(origin, d, 1, std::numeric_limits<float>::max(), 1) * 255.f;
//            img.setPixelColor(x, y, QColor(color.x, color.y, color.z));
//        }
//    }
    show();
}