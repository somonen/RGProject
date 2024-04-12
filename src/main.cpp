#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <cstdlib>

#define N_SKYBOX_VERTICES (108)
#define N_FIREFLIES (10)

float skyboxVertices[N_SKYBOX_VERTICES];
float catTrumpetVertices[N_SKYBOX_VERTICES];

float Gamma = 1.0f;
float exposure = 1.0f;
bool bloom = true;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadRGBCubemap(vector<std::string>& faces);

unsigned int loadRGBACubemap(vector<std::string>& faces);

void loadVertices(const std::string& filename, float* vertices, int n);

unsigned int loadRGBATexture(const std::string& path);

void loadGLMVertices(const std::string& filename, glm::vec3 coords[], int n);

void renderQuad();

float rectangleVertices[] =
        {
                // Coords    // texCoords
                1.0f, -1.0f,  1.0f, 0.0f,
                -1.0f, -1.0f,  0.0f, 0.0f,
                -1.0f,  1.0f,  0.0f, 1.0f,

                1.0f,  1.0f,  1.0f, 1.0f,
                1.0f, -1.0f,  1.0f, 0.0f,
                -1.0f,  1.0f,  0.0f, 1.0f
        };

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 750;
const int MAX_RAND = 200;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

int timer=0;
float xs[N_FIREFLIES];
float ys[N_FIREFLIES];
float yp = 0;
float zs[N_FIREFLIES];

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

PointLight pointLights[N_FIREFLIES];

struct DirLight {
    glm::vec3 direction;

    glm::vec3 specular;
    glm::vec3 diffuse;
    glm::vec3 ambient;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(1);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 forestPosition = glm::vec3(0.0f, -5.0f, 10.0f);
    float forestScale = 1.0f;
    DirLight dirLight;

    void LoadFromFile(std::string filename);
};

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

void loadPointLight(const std::string& filename, PointLight& light);

void loadDirLight(const std::string& filename, DirLight& light);

ProgramState *programState;

void DrawImGui(ProgramState *pSta);

int main() {

    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    {
        if (window == NULL) {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return -1;
        }
        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetKeyCallback(window, key_callback);
        // tell GLFW to capture our mouse
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // glad: load all OpenGL function pointers
        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return -1;
        }
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // Init Imgui
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    // configure global opengl state
    {
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    for(int i=0; i<N_FIREFLIES; i++){
        xs[i] = 0;
        ys[i] = 0;
        zs[i] = 0;
    }

    loadVertices("resources/vertices/skybox_vertices.txt", skyboxVertices, N_SKYBOX_VERTICES);
    loadVertices("resources/vertices/cat_trumpet_vertices.txt", catTrumpetVertices, N_SKYBOX_VERTICES);

/*
    FILE* f = fopen("resources/vertices/cat_trumpet_vertices.txt", "w");
    for(int i=0; i < N_SKYBOX_VERTICES; i++){
        if(skyboxVertices[i] > 0){
            fprintf(f, "%.1f ", skyboxVertices[i] - 0.2f);
        }else{
            fprintf(f, "%.1f ", skyboxVertices[i] + 0.2f);
        }
        if((i+1) % 3 == 0 && i != N_SKYBOX_VERTICES-1){
            fprintf(f, "\n");
        }
        if((i+1) % 18 == 0 && i != N_SKYBOX_VERTICES-1){
            fprintf(f, "\n");
        }
    }
*/

    // build and compile shaders
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader catSkyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader lightShader("resources/shaders/2.model_lighting.vs", "resources/shaders/light_box.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader bloomFinalShader("resources/shaders/bloom_final.vs", "resources/shaders/bloom_final.fs");


    // skybox vertex initialization
    unsigned int skyboxVAO, skyboxVBO;
    {
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) nullptr);
        glEnableVertexAttribArray(0);
    }

    // creating and loading skybox
    unsigned int skyboxTexture;
    {
        vector<std::string> faces
                {
                        "resources/textures/skybox/skybox_px.jpg",
                        "resources/textures/skybox/skybox_nx.jpg",
                        "resources/textures/skybox/skybox_py.jpg",
                        "resources/textures/skybox/skybox_ny.jpg",
                        "resources/textures/skybox/skybox_pz.jpg",
                        "resources/textures/skybox/skybox_nz.jpg"
                };
        skyboxTexture = loadRGBCubemap(faces);
    }

    unsigned int catTrumpetVAO, catTrumpetVBO;
    {
        glGenVertexArrays(1, &catTrumpetVAO);
        glGenBuffers(1, &catTrumpetVBO);
        glBindVertexArray(catTrumpetVAO);
        glBindBuffer(GL_ARRAY_BUFFER, catTrumpetVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(catTrumpetVertices), &catTrumpetVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) nullptr);
        glEnableVertexAttribArray(0);
    }

    unsigned int catTrumpetTexture;
    {
        vector<std::string> faces
                {
                        "resources/textures/cat.png",
                        "resources/textures/cat.png",
                        "resources/textures/cat.png",
                        "resources/textures/cat.png",
                        "resources/textures/cat.png",
                        "resources/textures/cat.png",
                };
        catTrumpetTexture = loadRGBACubemap(faces);
    }

    unsigned int cubeVAO, cubeVBO;
    {
        glGenVertexArrays(1, &cubeVAO);
        glGenVertexArrays(1, &cubeVBO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) nullptr);
        glEnableVertexAttribArray(0);
    }

    unsigned int cubeTexture;
    {
        std::string texture = "resources/textures/blank.png";
        cubeTexture = loadRGBATexture(texture);
    }

    glm::vec3 cubePositions[N_FIREFLIES];
    loadGLMVertices("resources/vertices/firefliesPositions.txt", cubePositions, N_FIREFLIES);

    // load models
    Model forestModel("resources/objects/forest/forest.obj");
    forestModel.SetShaderTextureNamePrefix("material.");

    //creating

    // pointLight
    //PointLight pointLights[N_FIREFLIES];
    loadPointLight("resources/lightSources/pointLight.txt", pointLights[0]);
    pointLights[0].position = cubePositions[0];
    for(int i=1; i<N_FIREFLIES; i++){
        pointLights[i].position = cubePositions[i];
        pointLights[i].ambient = pointLights[0].ambient;
        pointLights[i].diffuse = pointLights[0].diffuse;
        pointLights[i].specular = pointLights[0].specular;
        pointLights[i].constant = pointLights[0].constant;
        pointLights[i].linear = pointLights[0].linear;
        pointLights[i].quadratic = pointLights[0].quadratic;
    }

    // dirLight
    DirLight& dirLight = programState->dirLight;
    loadDirLight("resources/lightSources/dirLight.txt", dirLight);

    ourShader.use();
    ourShader.setFloat("material.shininess", 128.0f);
    ourShader.setInt("nPointLights", N_FIREFLIES);
    
    
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    catSkyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    blurShader.use();
    blurShader.setInt("image", 0);

    bloomFinalShader.use();
    bloomFinalShader.setInt("scene", 0);
    bloomFinalShader.setInt("bloomBlur", 1);

    // Prepare framebuffer rectangle VBO and VAO
    unsigned int rectVAO, rectVBO;
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), &rectangleVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // configure (floating point) framebuffers
    // ---------------------------------------
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }


    // render loop
    while (!glfwWindowShouldClose(window)) {

        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // Bind the custom framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

        // render
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        // don't forget to enable shader before setting uniforms
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // loading dirLight into shader
        {
            ourShader.setVec3("dirLight.direction", dirLight.direction);
            ourShader.setVec3("dirLight.ambient", dirLight.ambient);
            ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
            ourShader.setVec3("dirLight.specular", dirLight.specular);
        }

        // loading pointLights into shader
        //pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        {
            for(int i=0; i<N_FIREFLIES; i++) {

                if(timer + 2 < (int)currentFrame) {
                xs[i] = ((rand() % MAX_RAND) - 100) * 1.0f / 10000.0f;
                yp = ((rand() % MAX_RAND) - 100) * 1.0f / 10000.0f;
                ys[i] = yp*(yp + pointLights[i].position.y > -5.0f && yp + pointLights[i].position.y < 5.0f);
                zs[i] = ((rand() % MAX_RAND) - 100) * 1.0f / 10000.0f;
                }

                pointLights[i].position += glm::vec3(xs[i], ys[i], zs[i]);
                pointLights[i].position.y = min(5.0f, pointLights[i].position.y);
                pointLights[i].position.y = max(-5.0f, pointLights[i].position.y);

                ourShader.setVec3("pointLight[" + to_string(i) + "].position", pointLights[i].position);
                ourShader.setVec3("pointLight[" + to_string(i) + "].ambient", pointLights[i].ambient);
                ourShader.setVec3("pointLight[" + to_string(i) + "].diffuse", pointLights[i].diffuse);
                ourShader.setVec3("pointLight[" + to_string(i) + "].specular", pointLights[i].specular);
                ourShader.setFloat("pointLight[" + to_string(i) + "].constant", pointLights[i].constant);
                ourShader.setFloat("pointLight[" + to_string(i) + "].linear", pointLights[i].linear);
                ourShader.setFloat("pointLight[" + to_string(i) + "].quadratic", pointLights[i].quadratic);
                cubePositions[i] = {
                         pointLights[i].position.x,
                         pointLights[i].position.y,
                         pointLights[i].position.z
                };
            }
            if(timer + 2 < (int)currentFrame) {
                timer += 3;
            }
            ourShader.use();
            ourShader.setVec3("viewPosition", programState->camera.Position);
        }

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model,programState->forestPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->forestScale));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);

        glDisable(GL_CULL_FACE);
        forestModel.Draw(ourShader);
        glEnable(GL_CULL_FACE);

        // drawing fireflies
        glCullFace(GL_FRONT);
        glBindVertexArray(cubeVAO);


        // drawing skybox
        glCullFace(GL_BACK);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        {
            skyboxShader.use();
            skyboxShader.setMat4("projection", projection);
            skyboxShader.setMat4("view", glm::mat4(glm::mat3(view)));
            glBindVertexArray(skyboxVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        //drawing cat skybox
        {
            catSkyboxShader.use();
            catSkyboxShader.setMat4("projection", projection);
            catSkyboxShader.setMat4("view", glm::mat4(glm::mat3(view)));
            glBindVertexArray(catTrumpetVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, catTrumpetTexture);
            glDrawArrays(GL_TRIANGLES, 30, 6);
        }

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // finally show all the light sources as bright cubes
        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);

            for(unsigned int i = 0; i < N_FIREFLIES; i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, cubePositions[i]);
                //float angle = 20.0f * i;
                //model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
                model = glm::scale(model, glm::vec3(0.05f));
                lightShader.setMat4("model", model);
                lightShader.setVec3("lightColor", (pointLights[i].ambient + pointLights[i].diffuse + pointLights[i].specular));
                glBindTexture(GL_TEXTURE_2D, cubeTexture);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

        // Bind the default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // 2. blur bright fragments with two-pass Gaussian Blur
        // --------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        blurShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)

            renderQuad();

            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);



// 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        // --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloomFinalShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        bloomFinalShader.setInt("bloom", bloom);
        bloomFinalShader.setFloat("exposure", exposure);
        bloomFinalShader.setFloat("gamma", Gamma);

        renderQuad();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // termination
    {
        delete programState;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glDeleteVertexArrays(1, &skyboxVAO);
        glDeleteBuffers(1, &skyboxVBO);
        glDeleteVertexArrays(1, &catTrumpetVAO);
        glDeleteBuffers(1, &catTrumpetVBO);
        glDeleteVertexArrays(1, &cubeVAO);
        glDeleteBuffers(1, &cubeVBO);
        // glfw: terminate, clearing all previously allocated GLFW resources.
        glfwTerminate();
    }
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    deltaTime *= 2;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(3.0f, 0);
    if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(-3.0f, 0);
    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(0, 3.0f);
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(0, -3.0f);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *pState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Hello window");
        //ImGui::Text("Hello text");
        ImGui::ColorEdit3("Background color", (float *) &pState->clearColor);
        ImGui::DragFloat3("Forest position", (float*)&pState->forestPosition);
        ImGui::DragFloat("Forest scale", &pState->forestScale, 0.05, 0.1, 4.0);
        ImGui::DragFloat("Gamma", &Gamma, 0.05, 0.1, 4.0);
        ImGui::DragFloat("Exposure", &exposure, 0.05, 0.1, 4.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = pState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &pState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }
    {
        ImGui::Begin("Dirlight info");
        ImGui::DragFloat3("Direction", (float*)&pState->dirLight.direction, 0.05, -10, 10);
        ImGui::DragFloat3("Ambient", (float*)&pState->dirLight.ambient, 0.05, 0 ,5);
        ImGui::DragFloat3("Diffuse", (float*)&pState->dirLight.diffuse, 0.05, 0 ,5);
        ImGui::DragFloat3("Specular", (float*)&pState->dirLight.specular, 0.05, 0 ,5);
        ImGui::End();
    }
    {
        ImGui::Begin("PointLight info");
        for(int i=0; i<N_FIREFLIES; i++) {
            ImGui::Text(to_string(i).c_str());
            ImGui::DragFloat3("Position", (float *) &pointLights[i].position, 0.05, -10, 10);
            ImGui::DragFloat3("Ambient", (float *) &pointLights[i].ambient, 0.05, 0, 5);
            ImGui::DragFloat3("Diffuse", (float *) &pointLights[i].diffuse, 0.05, 0, 5);
            ImGui::DragFloat3("Specular", (float *) &pointLights[i].specular, 0.05, 0, 5);
            ImGui::DragFloat("Constant", (float*)&pointLights[i].constant, 0.05, 0 ,5);
            ImGui::DragFloat("Linear", (float*)&pointLights[i].linear, 0.05, 0 ,5);
            ImGui::DragFloat("Quadratic", (float*)&pointLights[i].quadratic, 0.05, 0 ,5);
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

unsigned int loadRGBCubemap(vector<std::string>& faces){

    unsigned int skyboxID;
    glGenTextures(1, &skyboxID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);

    int width, height, nrChannels;
    unsigned char *data;
    for(unsigned int i = 0; i < faces.size(); i++) {

        data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if(data) {
            glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }else{
            std::cerr << "Failed to load cubemap face at path: " << faces[i] << '\n';
        }
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return skyboxID;
}

unsigned int loadRGBACubemap(vector<std::string>& faces){

    unsigned int skyboxID;
    glGenTextures(1, &skyboxID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);

    int width, height, nrChannels;
    unsigned char *data;
    for(unsigned int i = 0; i < faces.size(); i++) {

        data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if(data) {
            glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }else{
            std::cerr << "Failed to load cubemap face at path: " << faces[i] << '\n';
        }
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return skyboxID;
}

void loadPointLight(const std::string& filename, PointLight& light){
    std::ifstream in(filename);
    if(in) {
        float x, y, z;
        std::string trash;
        in >> x >> y >> z >> trash;
        light.position = glm::vec3(x, y, z);

        in >> x >> y >> z >> trash;
        light.ambient = glm::vec3(x, y, z);

        in >> x >> y >> z >> trash;
        light.diffuse = glm::vec3(x, y, z);

        in >> x >> y >> z >> trash;
        light.specular = glm::vec3(x, y, z);

        in
                >> light.constant >> trash
                >> light.linear >> trash
                >> light.quadratic >> trash;
    }else{
        std::cout << "Failed to load file at path: " << filename << " \n";
    }
}

void loadDirLight(const std::string& filename, DirLight& light){
    std::ifstream in(filename);
    if (in){
        float x, y, z;
        std::string trash;
        in >> x >> y >> z >> trash;
        light.direction = glm::vec3(x, y, z);

        in >> x >> y >> z >> trash;
        light.ambient = glm::vec3(x, y, z);

        in >> x >> y >> z >> trash;
        light.diffuse = glm::vec3(x, y, z);

        in >> x >> y >> z >> trash;
        light.specular = glm::vec3(x, y, z);
    }
}

void loadVertices(const std::string& filename, float* vertices, int n){
    std::ifstream in(filename);
    for (int i = 0; in && i < n; i++) {
        in >> vertices[i];
    }
}

void loadGLMVertices(const std::string& filename, glm::vec3 coords[], int n){
    std::ifstream in(filename);
    for (int i = 0; in && i < n; i++) {
        float x, y, z;
        in >> x >> y >> z;
        coords[i] = {x, y, z};
    }
}

unsigned int loadRGBATexture(const std::string& path){
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if(data) {
        glTexImage2D(GL_TEXTURE_2D,0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }else{
        std::cerr << "Failed to load cubemap face at path: " << path << '\n';
    }
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
