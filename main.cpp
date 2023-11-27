#include <SDL.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include "FrameBuffer.h"
#include "GraphicsHeaders.h"
#include "ObjLoader.h"
#include "Shaders.h"

#include <string>

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
Color currentColor;

const std::string modelPath = "../planeta.obj";
Color clearColor(0, 0, 0);  // Color del fondo
float rotationAngle = glm::radians(0.0f);

std::vector<glm::vec3> vertices;
std::vector<glm::vec3> normals;
std::vector<Face> faces;
std::vector<Vertex> verticesArray;
std::vector<Planet> models;
std::vector<std::vector<Vertex>> modelsVertices;

Uniforms uniforms;

glm::mat4 model = glm::mat4(1);
glm::mat4 view = glm::mat4(1);
glm::mat4 projection = glm::mat4(1);

enum class Primitive {
    TRIANGLES,
};

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Error: No se puedo inicializar SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Lab 4: Shaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Error: No se pudo crear una ventana SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error: No se pudo crear SDL_Renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

std::vector<std::vector<Vertex>> primitiveAssembly(
        Primitive polygon,
        const std::vector<Vertex>& transformedVertices
) {
    std::vector<std::vector<Vertex>> assembledVertices;

    switch (polygon) {
        case Primitive::TRIANGLES: {
            assert(transformedVertices.size() % 3 == 0 && "El número de vértices debe ser un múltiplo de 3 para triángulos.");

            for (size_t i = 0; i < transformedVertices.size(); i += 3) {
                std::vector<Vertex> triangle = {
                        transformedVertices[i],
                        transformedVertices[i + 1],
                        transformedVertices[i + 2]
                };
                assembledVertices.push_back(triangle);
            }
            break;
        }
        default:
            std::cerr << "Error: No se reconoce el tipo primitivo." << std::endl;
            break;
    }

    return assembledVertices;
}

std::vector<Fragment> rasterize(Primitive primitive, const std::vector<std::vector<Vertex>>& assembledVertices) {
    std::vector<Fragment> fragments;

    switch (primitive) {
        case Primitive::TRIANGLES: {
            for (const std::vector<Vertex>& triangleVertices : assembledVertices) {
                assert(triangleVertices.size() == 3 && "Triangle vertices must contain exactly 3 vertices.");
                std::vector<Fragment> triangleFragments = triangle(triangleVertices[0], triangleVertices[1], triangleVertices[2]);
                fragments.insert(fragments.end(), triangleFragments.begin(), triangleFragments.end());
            }
            break;
        }
        default:
            std::cerr << "Error: No se reconoce el tipo primitivo para rasterización." << std::endl;
            break;
    }

    return fragments;
}

glm::mat4 createProjectionMatrix() {
    float fovInDegrees = 55.0f;
    float aspectRatio = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
    float nearClip = 0.1f;
    float farClip = 100.0f;

    return glm::perspective(glm::radians(fovInDegrees), aspectRatio, nearClip, farClip);
}

glm::mat4 createViewportMatrix(size_t screenWidth, size_t screenHeight) {
    glm::mat4 viewport = glm::mat4(1.0f);

    // Scale
    viewport = glm::scale(viewport, glm::vec3(screenWidth / 2.0f, screenHeight / 2.0f, 0.5f));

    // Translate
    viewport = glm::translate(viewport, glm::vec3(1.0f, 1.0f, 0.5f));

    return viewport;
}

void render(Primitive polygon, std::string name){

    // 1. Vertex Shader
    std::vector<Vertex> transformedVertices;

    for (const Vertex& vertex : verticesArray) {
        transformedVertices.push_back(vertexShader(vertex, uniforms));
    }

    // 2. Primitive Assembly
    std::vector<std::vector<Vertex>> assembledVertices = primitiveAssembly(polygon, transformedVertices);

    // 3. Rasterization
    std::vector<Fragment> fragments = rasterize(polygon, assembledVertices);

    // 4. Fragment Shader
    for (Fragment& fragment : fragments) {
        // Apply the fragment shader to compute the final color
        fragment = fragmentShader(fragment, name);
        point(fragment);
    }

}

std::string getCurrentPlanet(int& planetIndex) {
    if (planetIndex == 0) {
        return "sol";
    }

    else if (planetIndex == 1) {
        return "tierra";
    }

    else if (planetIndex == 2) {
        return "rocoso";
    }

    else if (planetIndex == 3) {
        return "gaseoso";
    }

    else if (planetIndex == 4) {
        return "portal";
    }

    else if (planetIndex == 5) {
        return "shine";
    }

    else if (planetIndex == 6) {
        return "urano";
    }

    return "sol";

}

int main([[maybe_unused]]int argv, [[maybe_unused]]char** args)
{
    if (!init()) {
        return 1;
    }

    clear(100, 100);

    int planetIndex = 0;

    // Inicializar cámara
    Camera camera;
    camera.cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);
    camera.targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    // Matriz de proyección
    uniforms.projection = createProjectionMatrix();

    // Matriz de viewport
    uniforms.viewport = createViewportMatrix(SCREEN_WIDTH, SCREEN_HEIGHT);

    /*PREPARAR UNIFORMS*/

    glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f); // Rotar alrededor del eje Y

    // Preparar uniforms del planeta
    Planet planet;
    planet.name = "sol";
    planet.translationVector = {0.0f, 0.0f, 0.0f};
    planet.rotationAngle = 1.0;
    planet.scaleFactor = {1.0f, 1.0f, 1.0f};
    models.push_back(planet);

    // Preparar uniforms de la luna
    Planet moon;
    moon.name = "luna";
    moon.translationVector = {1.0f, 0.3f, 0.0f};
    moon.rotationAngle = 1.0;
    moon.scaleFactor = {0.2f, 0.2f, 0.2f};

    for (Planet planet : models) {
        loadOBJ(modelPath, vertices, normals, faces);
        verticesArray = setupVertexArray(vertices, normals, faces);
        modelsVertices.push_back(verticesArray);
    };

    //Matriz de vista

    bool isRunning = true;
    bool mouseClicked = false;

    while (isRunning) {
        frame += 1;
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                isRunning = false;
            }

            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouseClicked = true;
                }
            }

            else if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouseClicked = false;
                }
            }
        }

        if (mouseClicked) {
            // Handle planet change based on mouse click
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);

            // Check if the mouse click is on the right side of the window
            if (mouseX > SCREEN_WIDTH / 2) {
                if (planetIndex < 6) {
                    planetIndex++;
                    if (planetIndex == 2) models.pop_back();
                    models.pop_back();
                    planet.name = getCurrentPlanet(planetIndex);
                    models.push_back(planet);
                    if (planetIndex == 1) models.push_back(moon);
                }
            } else {
                if (planetIndex > 0) {
                    planetIndex--;
                    if (planetIndex == 0) models.pop_back();
                    models.pop_back();
                    planet.name = getCurrentPlanet(planetIndex);
                    models.push_back(planet);
                    if (planetIndex == 1) models.push_back(moon);
                }
            }
        }

        clear(100, 100);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Delay(1);

        // Crear la matriz de vista usando el objeto cámara
        uniforms.view = glm::lookAt(
                camera.cameraPosition, // The position of the camera
                camera.targetPosition, // The point the camera is looking at
                camera.upVector        // The up vector defining the camera's orientation
        );


        for (Planet model : models) {

            if (model.name == "luna") {
                model.translationVector.x = 1.5f * cos(glm::radians(rotationAngle));
                model.translationVector.z = 1.5f * sin(glm::radians(rotationAngle));
            }

            rotationAngle += model.rotationAngle;
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), rotationAxis);

            glm::mat4 translation = glm::translate(glm::mat4(1.0f), model.translationVector);
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), model.scaleFactor);

            // Calcular la matriz de modelo
            uniforms.model = translation * rotation * scale;

            render(Primitive::TRIANGLES, model.name);

            renderBuffer(renderer);

        }

        SDL_RenderPresent(renderer);

    }

    return 0;
}