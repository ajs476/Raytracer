#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

// pixel struct
typedef struct Pixel{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} Pixel;

// object struct
typedef struct {
    int kind;
    union {
        // camera
        struct {
            double width;
            double height;
        } camera;
        // sphere
        struct {
            double position[3];
            double radius;
            double diffuse[3];
            double specular[3];
        } sphere;
        // plane
        struct {
            double position[3];
            double normal[3];
            double diffuse[3];
            double specular[3];
        } plane;
        // light
        struct {
            double position[3];
            double color[3];
            double theta;
            double direction[3];
            double radial[3];
            double angular;
        } light;
    };
} Object;


// array of objects, 128 max length
Object* object_array[128];
int obj = 0;
int line = 1;

// next character from json file
int next_c(FILE* json) {
    int c = fgetc(json);
#ifdef DEBUG
    printf("next_c: '%c'\n", c);
#endif
    if (c == '\n') {
        line += 1;
    }
    if (c == EOF) {
        fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
        exit(1);
    }
    return c;
}


void expect_c(FILE* json, int d) {
    int c = next_c(json);
    if (c == d) return;
    fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
    exit(1);
}


// skip white space
void skip_ws(FILE* json) {
    int c = next_c(json);
    while (isspace(c)) {
        c = next_c(json);
    }
    ungetc(c, json);
}


// get next string
char* next_string(FILE* json) {
    char buffer[129];
    int c = next_c(json);
    if (c != '"') {
        fprintf(stderr, "Error: Expected string on line %d.\n", line);
        exit(1);
    }
    c = next_c(json);
    int i = 0;
    while (c != '"') {
        if (i >= 128) {
            fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
            exit(1);
        }
        if (c == '\\') {
            fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
            exit(1);
        }
        if (c < 32 || c > 126) {
            fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
            exit(1);
        }
        buffer[i] = c;
        i += 1;
        c = next_c(json);
    }
    buffer[i] = 0;
    return strdup(buffer);
}

// get next number
double next_number(FILE* json) {
    double value;
    fscanf(json, "%lf", &value);
    // Error check this..
    return value;
}

// get next vector
double* next_vector(FILE* json) {
    double* v = malloc(3*sizeof(double));
    expect_c(json, '[');
    skip_ws(json);
    v[0] = next_number(json);
    skip_ws(json);
    expect_c(json, ',');
    skip_ws(json);
    v[1] = next_number(json);
    skip_ws(json);
    expect_c(json, ',');
    skip_ws(json);
    v[2] = next_number(json);
    skip_ws(json);
    expect_c(json, ']');
    return v;
}

// read json file
void read_scene(char* filename) {
    int c;
    FILE* json = fopen(filename, "r");
    
    if (json == NULL) {
        fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
        exit(1);
    }
    
    skip_ws(json);
    
    // Find the beginning of the list
    expect_c(json, '[');
    
    skip_ws(json);
    
    // Find the objects
    
    while (1) {
        c = fgetc(json);
        if (c == ']') {
            fprintf(stderr, "Error: This is the worst scene file EVER.\n");
            fclose(json);
            return;
        }
        if (c == '{') {
            skip_ws(json);
            
            // Parse the object
            char* key = next_string(json);
            if (strcmp(key, "type") != 0) {
                fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
                exit(1);
            }
            
            skip_ws(json);
            
            expect_c(json, ':');
            
            skip_ws(json);
            
            char* value = next_string(json);
            object_array[obj] = malloc(sizeof(Object));
            Object new;
            if (strcmp(value, "camera") == 0) {
                (*object_array[obj]).kind = 0;
            } else if (strcmp(value, "sphere") == 0) {
                (*object_array[obj]).kind = 1;
            } else if (strcmp(value, "plane") == 0) {
                (*object_array[obj]).kind = 2;
            } else if (strcmp(value, "light") == 0) {
                (*object_array[obj]).kind = 3;
            } else {
                fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
                exit(1);
            }
            
            skip_ws(json);
            
            while (1) {
                c = next_c(json);
                if (c == '}') {
                    // end of current object
                    obj++;
                    break;
                } else if (c == ',') {
                    // there are more objects to be read
                    skip_ws(json);
                    char* key = next_string(json);
                    skip_ws(json);
                    expect_c(json, ':');
                    skip_ws(json);
                    // check double field values
                    if ((strcmp(key, "width") == 0) ||
                        (strcmp(key, "height") == 0) ||
                        (strcmp(key, "radius") == 0) ||
                        (strcmp(key, "theta") == 0) ||
                        (strcmp(key, "radial-a2") == 0) ||
                        (strcmp(key, "radial-a1") == 0) ||
                        (strcmp(key, "radial-a0") == 0) ||
                        (strcmp(key, "angular-a0") == 0)) {
                        double value = next_number(json);
                        if(strcmp(key, "width") == 0){
                            if((*object_array[obj]).kind == 0){
                              (*object_array[obj]).camera.width = value;
                            }
                        }
                        else if(strcmp(key, "height") == 0){
                            if((*object_array[obj]).kind == 0){
                                (*object_array[obj]).camera.height = value;
                            }
                        }
                        else if(strcmp(key, "radius") == 0){
                            (*object_array[obj]).sphere.radius = value;
                        }
                        else if(strcmp(key, "theta") == 0) {
                            (*object_array[obj]).light.theta = value;
                        }
                        else if(strcmp(key, "radial-a2") == 0) {
                            (*object_array[obj]).light.radial[2] = value;
                        }
                        else if(strcmp(key, "radial-a1") == 0) {
                            (*object_array[obj]).light.radial[1] = value;
                        }
                        else if(strcmp(key, "radial-a0") == 0) {
                            (*object_array[obj]).light.radial[0] = value;
                        }
                        else if(strcmp(key, "angular-a0") == 0) {
                            (*object_array[obj]).light.angular = value;
                        }
                        // check object vector values
                    } else if ((strcmp(key, "color") == 0) ||
                               (strcmp(key, "position") == 0) ||
                               (strcmp(key, "normal") == 0) ||
                               (strcmp(key, "diffuse_color") == 0) ||
                               (strcmp(key, "specular_color") == 0) ||
                               (strcmp(key, "direction") == 0)) {
                        double* value = next_vector(json);
                        if(strcmp(key, "color") == 0){
                            (*object_array[obj]).light.color[0] = value[0];
                            (*object_array[obj]).light.color[1] = value[1];
                            (*object_array[obj]).light.color[2] = value[2];
                        }
                        else if(strcmp(key, "position") == 0){
                            if((*object_array[obj]).kind == 1){
                                (*object_array[obj]).sphere.position[0] = value[0];
                                (*object_array[obj]).sphere.position[1] = value[1];
                                (*object_array[obj]).sphere.position[2] = value[2];
                            }
                            else if((*object_array[obj]).kind == 2){
                                (*object_array[obj]).plane.position[0] = value[0];
                                (*object_array[obj]).plane.position[1] = value[1];
                                (*object_array[obj]).plane.position[2] = value[2];
                            }
                            else if((*object_array[obj]).kind == 3){
                                (*object_array[obj]).light.position[0] = value[0];
                                (*object_array[obj]).light.position[1] = value[1];
                                (*object_array[obj]).light.position[2] = value[2];
                            }
                        }
                        else if(strcmp(key, "normal") == 0){
                            (*object_array[obj]).plane.normal[0] = value[0];
                            (*object_array[obj]).plane.normal[1] = value[1];
                            (*object_array[obj]).plane.normal[2] = value[2];
                        }
                        else if(strcmp(key, "diffuse_color") == 0){
                            if((*object_array[obj]).kind == 1){
                                (*object_array[obj]).sphere.diffuse[0] = value[0];
                                (*object_array[obj]).sphere.diffuse[1] = value[1];
                                (*object_array[obj]).sphere.diffuse[2] = value[2];
                            }
                            else if((*object_array[obj]).kind == 2){
                                (*object_array[obj]).plane.diffuse[0] = value[0];
                                (*object_array[obj]).plane.diffuse[1] = value[1];
                                (*object_array[obj]).plane.diffuse[2] = value[2];
                            }
                        }
                        else if(strcmp(key, "specular_color") == 0){
                            if((*object_array[obj]).kind == 1){
                                (*object_array[obj]).sphere.specular[0] = value[0];
                                (*object_array[obj]).sphere.specular[1] = value[1];
                                (*object_array[obj]).sphere.specular[2] = value[2];
                            }
                            else if((*object_array[obj]).kind == 2){
                                (*object_array[obj]).plane.specular[0] = value[0];
                                (*object_array[obj]).plane.specular[1] = value[1];
                                (*object_array[obj]).plane.specular[2] = value[2];
                            }
                        }
                        else if(strcmp(key, "direction") == 0) {
                            (*object_array[obj]).light.direction[0] = value[0];
                            (*object_array[obj]).light.direction[1] = value[1];
                            (*object_array[obj]).light.direction[2] = value[2];
                        }
                    } else {
                        fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
                                key, line);
                    }
                    skip_ws(json);
                } else {
                    fprintf(stderr, "Error: Unexpected value on line %d\n", line);
                    exit(1);
                }
            }
            skip_ws(json);
            c = next_c(json);
            if (c == ',') {
                skip_ws(json);
            } else if (c == ']') {
                fclose(json);
                object_array[obj] = NULL;
                return;
            } else {
                fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
                exit(1);
            }
        }
    }
}

// array of light objects
Object* lights[128];
int light = 0;

// square root
static inline double sqr(double v) {
    return v*v;
}

// normalize a vector
static inline void normalize(double* v) {
    double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
}

// compute dot product of two vectors
double dot(double* a, double* b) {
    double result;
    result = a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    return result;
}

// calculate magnitude of vector
double magnitude(double* v){
    return sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
}

// scale vector by s
void scale(double* v, double s){
    v[0] *= s;
    v[1] *= s;
    v[2] *= s;
}

// subtract two vectors
void subtract(double* v1, double* v2){
    v1[0] -= v2[0];
    v1[1] -= v2[1];
    v1[2] -= v2[2];
}

// calculate reflection vector
void reflect(double* v, double* n, double* r){
    double dotResult = dot(v, n);
    dotResult *= 2;
    double nNew[3] = {
        n[0],
        n[1],
        n[2]
    };
    scale(nNew, dotResult);
    r[0] = v[0];
    r[1] = v[1];
    r[2] = v[2];
    subtract(r, nNew);
}

// clamp our color values
double clamp(double number){
    number *= 255;
    if (number < 0) {
        return 0;
    }
    else if (number > 255){
        return 255;
    }
    else {
        return number;
    }
}

// go through objects and add lights to light array
void collect_lights (){
    int i = 0;
    for (i = 0; object_array[i] != 0; i++) {
        if (object_array[i]->kind == 3){
            lights[light] = object_array[i];
            light++;
        }
    }
}

// angular light equation
double fangular(double* Vo, double* Vl, double a1, double angle) {
    double dotResult = dot(Vo, Vl);
    if (acos(dotResult) > angle / 2) {
        return 0;
    } else {
        return pow(dotResult, a1);
    }
}

// radial light equation
double fradial(double a2, double a1, double a0, double d) {
    double quotient = a2 * sqr(d) + a1 * d + a0;
    if (quotient == 0) {
        return 0;
    }
    if (d == INFINITY) {
        return 1;
    } else {
        return 1.0 / quotient;
    }
}

// diffuse light equation
double diffuse_l(double Kd, double Il, double* N, double* L) {
    double dotResult = dot(N, L);
    if (dotResult > 0) {
        return Kd * Il * dotResult;
    } else {
        return 0;
    }
}

// specular light equation
double specular_l(double Ks, double Il, double* V, double* R, double* N, double* L, double ns) {
    double dotResult = dot(V, R);
    if (dotResult > 0 && dot(N, L) > 0) {
        return Ks * Il * pow(dotResult, ns);
    } else {
        return 0;
    }
}

// intersection of ray and sphere object
double sphere_intersection(double* Ro, double* Rd,
                           double* C, double r) {
    double a = (sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]));
    double b = (2*(Ro[0]*Rd[0] - Rd[0]*C[0] + Ro[1]*Rd[1] - Rd[1]*C[1] + Ro[2]*Rd[2] - Rd[2]*C[2]));
    double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + Ro[1] - 2*Ro[1]*C[1] + sqr(C[1]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);
    
    double det = sqr(b) - 4 * a * c;
    if (det < 0) return -1;
    
    det = sqrt(det);
    
    double t0 = (-b - det) / (2*a);
    if (t0 > 0) return t0;
    
    double t1 = (-b + det) / (2*a);
    if (t1 > 0) return t1;
    
    return -1;
    
}

// intersection of ray and plane object
double plane_intersection(double* Ro, double* Rd,
                          double* C, double* N) {
    double subtract[3];
    subtract[0] = C[0]-Ro[0];
    subtract[1] = C[1]-Ro[1];
    subtract[2] = C[2]-Ro[2];
    double dot1 = N[0]*subtract[0] + N[1]*subtract[1] + N[2]*subtract[2];
    double dot2 = N[0]*Rd[0] + N[1]*Rd[1] + N[2]*Rd[2];
    
    return dot1/dot2;
}

int main(int argc, char **argv) {
    
    // create file pointer for output image
    FILE* output;
    output = fopen(argv[4], "wb+");
    // we will create a p3 image
    fprintf(output, "P3\n");
    fprintf(output, "%d %d\n%d\n", atoi(argv[1]), atoi(argv[2]), 255);
    read_scene(argv[3]);
    collect_lights();
    int i = 0;
    double w;
    double h;

    // set camera width and height
    while(1){
        if (object_array[i]->kind == 0){
            w = object_array[i]->camera.width;
            h = object_array[i]->camera.height;
            break;
        }
        i++;
    }
    
    double cx = 0;
    double cy = 0;
    
    int M = atoi(argv[2]);
    int N = atoi(argv[1]);
    
    Pixel* image = malloc(sizeof(Pixel)*M*N);
    int pixelNum = 0;
    
    double pixheight = h / M;
    double pixwidth = w / N;
    double color[3] = {0,0,0};
    int best;

    for (int y = M; y > 0; y--) {
        for (int x = 0; x < N; x += 1) {
            double Ro[3] = {0, 0, 0};
            double Rd[3] = {
                cx - (w/2) + pixwidth * (x + 0.5),
                cy - (h/2) + pixheight * (y + 0.5),
                1
            };
            normalize(Rd);
            double best_t = INFINITY;
            for (int i=0; object_array[i] != 0; i++) {
                int type;
                double t = 0;
                switch(object_array[i]->kind) {
                    case 0:
                        // pass, it's a camera
                        break;
                    case 1:
                        // sphere intersection check
                        type = 1;
                        t = sphere_intersection(Ro, Rd,
                                                object_array[i]->sphere.position,
                                                object_array[i]->sphere.radius);
                        break;
                    case 2:
                        // plane intersection check
                        type = 2;
                        t = plane_intersection(Ro, Rd,
                                               object_array[i]->plane.position,
                                               object_array[i]->plane.normal);
                        break;
                    case 3:
                        // pass, it's a light
                        break;
                    default:
                        // object not found... whoops...
                        exit(1);
                }
                if (t > 0 && t < best_t) {
                    best_t = t;
                    best = i;
                }
            }
            int closest_shadow_object;
            if (best_t > 0 && best_t != INFINITY) {
                for (int i = 0; lights[i] != NULL; i++) {
                    double ron[3] = {best_t*Rd[0]+Ro[0],
                        best_t*Rd[1]+Ro[1],
                        best_t*Rd[2]+Ro[2]};
                    double rdn[3] = {lights[i]->light.position[0] - ron[0],
                        lights[i]->light.position[1] - ron[1],
                        lights[i]->light.position[2] - ron[2]};
                    normalize(rdn);
                    closest_shadow_object = 0;
                    for (int j = 0; object_array[j] != 0; j++){
                        //printf("wtf..\n");
                        double t = 0;
                        if (object_array[j] == object_array[best]){
                            //printf("wtf..\n");
                            continue;
                        }
                        switch(object_array[j]->kind) {
                                //printf("switch..\n");
                            case 0:
                                // pass, it's a camera
                                //printf("switch camera i:%d..\n", i);

                                break;
                            case 1:
                                // intersectin test for sphere
                                printf("switch. sphere.\n");
                                t = sphere_intersection(ron, rdn,
                                                        object_array[j]->sphere.position,
                                                        object_array[j]->sphere.radius);
                                break;
                            case 2:
                                printf("switch plane..\n");
                                // intersection test for plane
                                t = plane_intersection(ron, rdn,
                                                       object_array[j]->plane.position,
                                                       object_array[j]->plane.normal);
                                break;
                            case 3:
                                printf("switch light..\n");
                                // pass, it's a light
                                break;
                            default:
                                printf("switch rip..\n");
                                // rip
                                exit(1);
                        }
                        
                        
                        if (t>0 && t<magnitude(rdn)){
                            closest_shadow_object = 1;
                            break;
                        }
                    }
                    if (closest_shadow_object == 0){
                        double N[3];
                        if (object_array[best]->kind == 1){
                            N[0] = ron[0] - object_array[best]->sphere.position[0];
                            N[1] = ron[1] - object_array[best]->sphere.position[1];
                            N[2] = ron[2] - object_array[best]->sphere.position[2];
                        }
                        else if (object_array[best]->kind == 2){
                            N[0] = object_array[best]->plane.normal[0];
                            N[1] = object_array[best]->plane.normal[1];
                            N[2] = object_array[best]->plane.normal[2];
                        }
                        normalize(N);
                        double L[3] = {rdn[0], rdn[1], rdn[2]};
                        normalize(L);
                        double nL[3] = {-L[0], -L[1], -L[2]};
                        double R[3];
                        reflect(L, N, R);
                        double V[3] = {Rd[0], Rd[1], Rd[2]};
                        double pos[3] = {lights[i]->light.position[0],
                            lights[i]->light.position[1],
                            lights[i]->light.position[2]};
                        subtract(pos, ron);
                        double d = magnitude(pos);
                        double col;
                        for (int c = 0; c < 3; c++) {
                            col = 1;
                            if (lights[i]->light.angular != INFINITY && lights[i]->light.theta != 0) {
                                col *= fangular(nL, lights[i]->light.direction, lights[i]->light.angular, (lights[i]->light.theta)*0.0174533);
                            }
                            if (lights[i]->light.radial[0] != INFINITY) {
                                col *= fradial(lights[i]->light.radial[2], lights[i]->light.radial[1], lights[i]->light.radial[0], d);
                            }
                            if (object_array[best]->kind == 1){
                                col *= (diffuse_l(object_array[best]->sphere.diffuse[c], lights[i]->light.color[c], N, L) + (specular_l(object_array[best]->sphere.specular[c], lights[i]->light.color[c], V, R, N, L, 20)));
                                color[c] += col;
                            }
                            else if (object_array[best]->kind == 2){
                                col *= (diffuse_l(object_array[best]->plane.diffuse[c], lights[i]->light.color[c], N, L) + (specular_l(object_array[best]->plane.specular[c], lights[i]->light.color[c], V, R, N, L, 20)));
                                color[c] += col;
                            }
                            color[c] = clamp(color[c]);
                        }
                    }
                }
            }
            
            Pixel new;
            if (best_t > 0 && best_t != INFINITY) {
                new.red = color[0];
                new.green = color[1];
                new.blue = color[2];
                image[pixelNum] = new;
                fprintf(output, "%i %i %i ", new.red, new.green, new.blue);
                // reset color to be black
                color[0] = 0;
                color[1] = 0;
                color[2] = 0;
            } else {
                // no object here, draw black pixel instead
                image[pixelNum] = new;
                fprintf(output, "0 0 0 ");
            }
            pixelNum++;
        }
    }
    
    fclose(output);
    return 0;
}

