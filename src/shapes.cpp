#include "gray.hpp"
#include "lisc.hpp"
#include "util.hpp"

class Sphere : public Shape
{
public:
    bool intersect (Ray& r, Isect* isect);
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
};


class Mesh : public Shape
{
public:
    std::vector<vec3> vertices;
    std::vector<int> vertex_indices;

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
        const vec3& vert0 = vertices[vertex_indices[triangle*3+0]];
        const vec3& vert1 = vertices[vertex_indices[triangle*3+1]];
        const vec3& vert2 = vertices[vertex_indices[triangle*3+2]];

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
        isect->n = normalize(cross(e1, e2));

        return true;
    }
};


class BBox
{
public:
    vec3 min, max;

    BBox ()
        : min(vec3(99999)), max(vec3(-99999))
    { }

    void extend (const vec3& v)
    {
        if (v.x < min.x) min.x = v.x;
        if (v.y < min.y) min.y = v.y;
        if (v.z < min.z) min.z = v.z;
        if (v.x > max.x) max.x = v.x;
        if (v.y > max.y) max.y = v.y;
        if (v.z > max.z) max.z = v.z;
    }

    bool intersect (const Ray& ray) const
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
        if (faces.size() <= 3) return;
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
Mesh* load_ply (std::ifstream& ifs)
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

    std::cout << "vertex count " << vcount << std::endl;
    std::cout << "face count " << fcount << std::endl;

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

    M->root = BVHNode(M);
    for (int i = 0; i < fcount; i++) M->root.add(i);
    M->root.split();
    std::cout << "FFF real count "<<fcount<<" counted "<<M->root.calc()<<"\n";
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
        std::ifstream ifs(*pop<std::string>(args));
        auto* m = load_ply(ifs);
        S = m;
    }
    else {
        throw std::runtime_error("invalid shape name "+name);
    }

    std::shared_ptr<Shape> sh(S);
    val.reset({"_shape", sh});
}
