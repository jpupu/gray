#include "gray.hpp"
#include "lisc.hpp"
#include "util.hpp"

BBox::BBox ()
    : min(vec3(99999)), max(vec3(-99999))
{ }

BBox::BBox (const vec3& min, const vec3& max)
    : min(min), max(max)
{ }

void BBox::extend (const vec3& v)
{
    if (v.x < min.x) min.x = v.x;
    if (v.y < min.y) min.y = v.y;
    if (v.z < min.z) min.z = v.z;
    if (v.x > max.x) max.x = v.x;
    if (v.y > max.y) max.y = v.y;
    if (v.z > max.z) max.z = v.z;
}

bool BBox::intersect (const Ray& ray) const
{
    // http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-box-intersection/
    
    float tmin = (min.x - ray.o.x) / ray.d.x;
    float tmax = (max.x - ray.o.x) / ray.d.x;
    if (tmin > tmax) std::swap(tmin, tmax);
    float tymin = (min.y - ray.o.y) / ray.d.y;
    float tymax = (max.y - ray.o.y) / ray.d.y;
    if (tymin > tymax) std::swap(tymin, tymax);
    if ((tmin > tymax) || (tymin > tmax))
    return false;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;
    float tzmin = (min.z - ray.o.z) / ray.d.z;
    float tzmax = (max.z - ray.o.z) / ray.d.z;
    if (tzmin > tzmax) std::swap(tzmin, tzmax);
    if ((tmin > tzmax) || (tzmin > tmax)) return false;
    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;
    if ((tmin > ray.tmax) || (tmax < ray.tmin)) return false;
    // if (ray.tmin < tmin) ray.tmin = tmin;
    // if (ray.tmax > tmax) ray.tmax = tmax;
    return true;
}



class Sphere : public Shape
{
public:
    bool intersect (Ray& r, Isect* isect);

    BBox get_bbox () const { return BBox(vec3(-1), vec3(1)); }
};


bool Sphere::intersect (Ray& ray, Isect* isect)
{
    const vec3& o = ray.o;
    const vec3& d = ray.d;

    float A = dot(d, d);
    float B = 2 * dot(d, o);
    float C = dot(o,o) - 1;

    float discrim = B*B - 4*A*C;
    if (discrim < 0) {
        return false;
    }

    // TODO: t0 and t1 can be written differently to improve precision.
    // see http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    float t0 = (-B - sqrtf(discrim)) / (2*A);
    float t1 = (-B + sqrtf(discrim)) / (2*A);
    if (t0 > t1) std::swap(t0, t1);

    float t = (t0 >= ray.tmin) ? t0 : t1;
    if (t < ray.tmin || t > ray.tmax) return false;

    ray.tmax = t;
    isect->p = o + t*d;
    isect->n = normalize(isect->p);

    return true;
}


class Plane : public Shape
{
public:
    bool intersect (Ray& ray, Isect* isect)
    {
        if (ray.d.y == 0) return false;

        float t = -ray.o.y / ray.d.y;
        
        if (t < ray.tmin || t > ray.tmax) return false;

        ray.tmax = t;
        isect->p = ray.o + t*ray.d;
        isect->n = vec3(0,1,0);

        return true;
    }

    BBox get_bbox () const
    {
        const float inf = std::numeric_limits<double>::infinity();
        return BBox(vec3(-inf,0,-inf), vec3(inf,0,inf));
    }
};


class Rectangle : public Shape
{
public:
    bool intersect (Ray& ray, Isect* isect)
    {
        if (ray.d.y == 0) return false;

        float t = -ray.o.y / ray.d.y;

        if (t < ray.tmin || t > ray.tmax) return false;

        vec3 p = ray.o + t*ray.d;
        if (p.x < -1 || p.x > 1 || p.z < -1 || p.z > 1) return false;
        
        ray.tmax = t;
        isect->p = p;
        isect->n = vec3(0,1,0);

        return true;
    }

    BBox get_bbox () const { return BBox(vec3(-1,0,-1), vec3(1,0,1)); }

};


class Box : public Shape
{
public:
    bool intersect (Ray& ray, Isect* isect)
    {
        // float tt[7];
        // for (int k = 0; k < 3; k++) {
        //     tt[k*2+0] = (-1 - ray.o[k]) / ray.d[k];
        //     tt[k*2+1] = (1 - ray.o[k]) / ray.d[k];
        // }
        // tt[6] = ray.tmax;
        // int ii = 6;
        // for (int i = 0; i < 6; i++) {
        //     if (tt[i] < tt[ii] && tt[i] >= ray.tmin) ii = i;
        // }
        // if (ii == 6) return false;

        // ray.tmax = tt[ii];
        // isect->p = ray.o + ray.d * tt[ii];
        // switch (ii) {
        //     case 0: isect->n = vec3(-1, 0, 0); break;
        //     case 1: isect->n = vec3( 1, 0, 0); break;
        //     case 2: isect->n = vec3( 0,-1, 0); break;
        //     case 3: isect->n = vec3( 0, 1, 0); break;
        //     case 4: isect->n = vec3( 0, 0,-1); break;
        //     case 5: isect->n = vec3( 0, 0, 1); break;
        // }
        // return true;


        // http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-7-intersecting-simple-shapes/ray-box-intersection/
        
        float tmin = (-1 - ray.o.x) / ray.d.x;
        float tmax = (1 - ray.o.x) / ray.d.x;
        if (tmin > tmax) std::swap(tmin, tmax);
        float tymin = (-1 - ray.o.y) / ray.d.y;
        float tymax = (1 - ray.o.y) / ray.d.y;
        if (tymin > tymax) std::swap(tymin, tymax);
        if ((tmin > tymax) || (tymin > tmax)) return false;
        if (tymin > tmin) tmin = tymin;
        if (tymax < tmax) tmax = tymax;
        float tzmin = (-1 - ray.o.z) / ray.d.z;
        float tzmax = (1 - ray.o.z) / ray.d.z;
        if (tzmin > tzmax) std::swap(tzmin, tzmax);
        if ((tmin > tzmax) || (tzmin > tmax)) return false;
        if (tzmin > tmin) tmin = tzmin;
        if (tzmax < tmax) tmax = tzmax;
        if ((tmin > ray.tmax) || (tmax < ray.tmin)) return false;

        float t = tmin;
        if (tmin < ray.tmin) t = tmax;

        ray.tmax = t;
        isect->p = ray.o + ray.d * t;
        int k = abs_max_elem(isect->p);
        isect->n = vec3(0.0f);
        isect->n[k] = (isect->p[k] < 0) ? -1.0f : 1.0f;
        // if (ray.tmin < tmin) ray.tmin = tmin;
        // if (ray.tmax > tmax) ray.tmax = tmax;
        return true;


    }

    BBox get_bbox () const { return BBox(vec3(-1), vec3(1)); }
};


class Triangle : public Shape
{
public:
    vec3 v[3];

    bool intersect (Ray& ray, Isect* isect)
    {
        constexpr float EPSILON = 1e-6f;
        // Möller–Trumbore intersection algorithm
        // http://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
        // http://www.scratchapixel.com/lessons/3d-basic-lessons/lesson-9-ray-triangle-intersection/m-ller-trumbore-algorithm/

        vec3 e1 = v[1] - v[0];
        vec3 e2 = v[2] - v[0];
        vec3 pvec = cross(ray.d, e2);
        float det = dot(e1, pvec);

        // if the determinant is negative, the triangle is backfacing
        // if the determinant is close to zero, the ray won't hit
        if (det < EPSILON) return false;

        float inv_det = 1 / det;

        // Calculate u.
        vec3 tvec = ray.o - v[0];
        float u = dot(tvec, pvec) * inv_det;
        if (u < 0 || u > 1) return false;

        // Calculate v.
        vec3 qvec = cross(tvec, e1);
        float v = dot(ray.d, qvec) * inv_det;
        if (v < 0 || v > 1 || u + v > 1) return false;

        // Calculate t.
        float t = dot(e2, qvec) * inv_det;
        if (t < ray.tmin || t > ray.tmax) return false;

        // Hit.
        ray.tmax = t;
        isect->p = ray.o + t * ray.d;
        isect->n = normalize(cross(e1, e2));

        return true;
    }

    BBox get_bbox () const { return BBox(vec3(-1,0,-1), vec3(1,0,1)); }
};


class Mesh : public Shape
{
public:
    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    std::vector<int> vertex_indices;
    bool smooth;
    BBox bbox;

    BBox get_bbox () const { return bbox; }

    vec3& vertex (int face, int v)
    {
        return vertices[vertex_indices[face*3+v]];
    }

    vec3& normal (int face, int v)
    {
        return normals[vertex_indices[face*3+v]];
    }

    virtual bool intersect (Ray& ray, Isect* isect)
    {
        bool hit = false;
        for (unsigned int i = 0; i < vertex_indices.size()/3; ++i) {
            hit |= intersect_triangle(i, ray, isect);
        }
        return hit;
    }

    bool intersect_triangle (int triangle, Ray& ray, Isect* isect)
    {
        const vec3& vert0 = vertex(triangle, 0);
        const vec3& vert1 = vertex(triangle, 1);
        const vec3& vert2 = vertex(triangle, 2);

        // constexpr float EPSILON = 1e-8f;
        vec3 e1 = vert1 - vert0;
        vec3 e2 = vert2 - vert0;
        vec3 pvec = cross(ray.d, e2);
        float det = dot(e1, pvec);

        // if the determinant is negative, the triangle is backfacing
        // if the determinant is close to zero, the ray won't hit
        // if (-EPSILON < det && det < EPSILON) return false;
        if (det == 0) return false;
        // if (det < EPSILON) return false;

        float inv_det = 1 / det;

        // Calculate u.
        vec3 tvec = ray.o - vert0;
        float u = dot(tvec, pvec) * inv_det;
        if (u < 0 || u > 1) return false;

        // Calculate v.
        vec3 qvec = cross(tvec, e1);
        float v = dot(ray.d, qvec) * inv_det;
        if (v < 0 || v > 1 || u + v > 1) return false;

        // Calculate t.
        float t = dot(e2, qvec) * inv_det;
        if (t < ray.tmin || t > ray.tmax) return false;

        // Hit.
        ray.tmax = t;
        isect->p = ray.o + t * ray.d;
        if (smooth) {
            const vec3& n0 = normal(triangle, 0);
            const vec3& n1 = normal(triangle, 1);
            const vec3& n2 = normal(triangle, 2);
            vec3 n = n0 * (1-u-v) + n1 * u + n2 * v;
            isect->n = normalize(n);
        }
        else {
            isect->n = normalize(cross(e1, e2));
        }

        return true;
    }

    void calculate_bbox ()
    {
        for (auto& v : vertices) {
            bbox.extend(v);
        }
        std::cout << "BBox " << bbox.min << " -- " << bbox.max << std::endl;
    }

    void adjust_floor (float floor_y = 0.0f)
    {
        float bias = floor_y - bbox.min.y;
        bbox = BBox();
        for (auto& v : vertices) {
            v.y += bias;
            bbox.extend(v);
        }
        std::cout << "new BBox " << bbox.min << " -- " << bbox.max << std::endl;
    }

    void adjust_height (float height = 1.0f)
    {
        float scale = height / (bbox.max.y - bbox.min.y);
        bbox = BBox();
        for (auto& v : vertices) {
            v *= scale;
            bbox.extend(v);
        }
        std::cout << "new BBox " << bbox.min << " -- " << bbox.max << std::endl;
    }

    void calculate_smooth_normals ()
    {
        normals.clear();
        normals.resize(vertices.size(), vec3(0.0f));
        for (size_t face = 0; face < vertex_indices.size() / 3; face++) {
            vec3 n = normalize(cross(vertex(face,1) - vertex(face,0),
                                     vertex(face,2) - vertex(face,0)));
            for (int i = 0; i < 3; i++) {
                vec3 e1 = normalize(vertex(face, (i+1)%3) - vertex(face, i));
                vec3 e2 = normalize(vertex(face, (i+2)%3) - vertex(face, i));
                float w = acos(dot(e1, e2));
                normal(face,i) += n * w;
            }
        }
        for (size_t i = 0; i < normals.size(); i++) {
            normals[i] = normalize(normals[i]);
        }
    }
};


class BVHNode
{
public:
    BBox bbox;
    std::vector<int> faces;
    std::vector<BVHNode*> children;
    Mesh* mesh;

    const vec3& vertex (int face, int i) const
    {
        return mesh->vertices[mesh->vertex_indices[face*3+i]];
    }

    BVHNode () {}
    BVHNode (Mesh* m)
        : mesh(m)
    { }

    bool intersect (Ray& ray, Isect* isect)
    {
        if (!bbox.intersect(ray)) return false;
        bool hit = false;
        for (auto* n : children) {
            hit |= n->intersect(ray, isect);
        }
        for (unsigned int i = 0; i < faces.size(); i++) {
            hit |= mesh->intersect_triangle(faces[i], ray, isect);
        }
        return hit;
    }

    void add (int face)
    {
        faces.push_back(face);
        bbox.extend(vertex(face, 0));
        bbox.extend(vertex(face, 1));
        bbox.extend(vertex(face, 2));
    }

    void split ()
    {
        if (faces.size() <= 32) return;
        int axis = abs_max_elem(bbox.max - bbox.min);
        float th = (bbox.max[axis] + bbox.min[axis]) / 2;

        BVHNode* left = new BVHNode(mesh);
        BVHNode* right = new BVHNode(mesh);
        // for (unsigned int i = 0; i < faces.size(); i++) {
        for (int i : faces) {
            bool go_left = false;
            bool go_right = false;
            for (int k = 0; k < 3; k++) {
                if (vertex(i,k)[axis] < th) go_left = true;
                if (vertex(i,k)[axis] >= th) go_right = true;
            }
            if (go_left) left->add(i);
            if (go_right) right->add(i);
            if (!go_left && !go_right) {
                std::cout << "split FAILED: a face not in left or right child!\n";
            }
        }
        if (left->faces.size() == 0 || right->faces.size() == 0 ||
            left->faces.size() == faces.size() || right->faces.size() == faces.size()) {
            // could not partition
            delete left;
            delete right;
            return;
        }
        children.push_back(left);
        children.push_back(right);
        // std::cout << "split "<<faces.size()<<" to "<<left->faces.size()<<"+"<<right->faces.size()<<"\n";
        faces.clear();
        left->split();
        right->split();
    }

    int calc ()
    {
        int cnt = 0;
        for (auto* n : children) {
            cnt += n->calc();
        }
        cnt += faces.size();
        return cnt;
    }

    void plot (std::vector<int>& cnts)
    {
        for (auto* n : children) {
            n->plot(cnts);
        }
        for (unsigned int i = 0; i < faces.size(); i++) {
            cnts[faces[i]]++;
        }

    }
};

class BVHMesh : public Mesh
{
public:
    BVHNode root;
    bool intersect (Ray& ray, Isect* isect)
    {
        return root.intersect(ray, isect);
    }

};


#include <fstream>
BVHMesh* load_ply (std::ifstream& ifs, double floor=NAN, double height=NAN)
{
    auto* M = new BVHMesh();
    // auto* M = new Mesh();

    char buf[256];
    ifs.getline(buf, 256);
    if (buf != std::string("ply")) throw std::runtime_error("ply: bad magic");

    int vcount = 0;
    int fcount = 0;

    std::string tok;
    ifs >> tok;
    while (tok != "end_header") {
        if (tok == "element") {
            ifs >> tok;
            if (tok == "vertex") {
                ifs >> vcount;
            }
            else {
                ifs >> fcount;
            }
        }
        else {
            ifs.getline(buf, 256);
        }
        ifs >> tok;
    }
    // ifs.getline(buf, 256);
    // if (buf != std::string("format ascii 1.0")) throw std::runtime_error("ply: bad format");
    // ifs.getline(buf, 256);
    // if (buf != std::string("format ascii 1.0")) throw std::runtime_error("ply: bad format");

    // std::cout << "vertex count " << vcount << std::endl;
    // std::cout << "face count " << fcount << std::endl;

    for (int i = 0; i < vcount; i++) {
        float x, y, z;
        ifs >> x >> y >> z;
        ifs.getline(buf, 256);
        M->vertices.push_back(vec3(x,y,z));
    }
    for (int i = 0; i < fcount; i++) {
        int cnt, a, b, c;
        ifs >> cnt >> a >> b >> c;
        M->vertex_indices.push_back(a);
        M->vertex_indices.push_back(b);
        M->vertex_indices.push_back(c);
    }

    M->calculate_bbox();
    if (!std::isnan(height)) M->adjust_height(height);
    if (!std::isnan(floor)) M->adjust_floor(floor);
    M->smooth = true;
    M->calculate_smooth_normals();

    M->root = BVHNode(M);
    for (int i = 0; i < fcount; i++) M->root.add(i);
    M->root.split();
    // std::cout << "FFF real count "<<fcount<<" counted "<<M->root.calc()<<"\n";
    // std::vector<int> counts(fcount, 0);
    // M->root.plot(counts);
    // for(int x : counts) {
    //     std::cout << "CCC " << x << "\n";
    // }
    return M;
}




void evaluate_shape (Value& val, List& args)
{
    Shape* S;
    auto name = *pop<std::string>(args);
    if (name == "sphere") {
        S = new Sphere();
    }
    else if (name == "plane") {
        S = new Plane();
    }
    else if (name == "rectangle") {
        S = new Rectangle();
    }
    else if (name == "box") {
        S = new Box();
    }
    else if (name == "triangle") {
        auto* t = new Triangle();
        t->v[0] = *pop<vec3>(args);
        t->v[1] = *pop<vec3>(args);
        t->v[2] = *pop<vec3>(args);
        S = t;
    }
    else if (name == "direct_mesh") {
        auto* m = new Mesh();
        int i = 0;
        while (args.size() > 0) {
            m->vertices.push_back( *pop<vec3>(args) );
            m->vertices.push_back( *pop<vec3>(args) );
            m->vertices.push_back( *pop<vec3>(args) );
            m->vertex_indices.push_back( i++ );
            m->vertex_indices.push_back( i++ );
            m->vertex_indices.push_back( i++ );
        }
        S = m;
    }
    else if (name == "ply_mesh") {
        double height = *pop_attr<double>("height", make_shared<double>(NAN), args);
        double floor = *pop_attr<double>("floor", make_shared<double>(NAN), args);
        std::ifstream ifs(*pop<std::string>(args));
        auto* m = load_ply(ifs, floor, height);
        S = m;
    }
    else {
        throw std::runtime_error("invalid shape name "+name);
    }



    std::shared_ptr<Shape> sh(S);
    // val.reset({"_shape", sh});
    val.reset(sh);
}
