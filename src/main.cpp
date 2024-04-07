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

#define N_SKYBOX_VERTICES (108)

float skyboxVertices[N_SKYBOX_VERTICES];
float catTrumpetVertices[N_SKYBOX_VERTICES];

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadRGBCubemap(vector<std::string>& faces);

unsigned int loadRGBACubemap(vector<std::string>& faces);

void loadSkyboxVertices(std::string filename, float* vertices, int n);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 750;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

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
    PointLight pointLight;
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

void loadPointLight(std::string filename, PointLight& light){
    std::ifstream in(filename);
    if (in){
        float x, y, z;
        std::string trash;
        in >> x >> y >> z >> trash;
        light.position = glm::vec3(x, y, z);

        in >> x >> y >> z >> trash;
        light.ambient = glm::vec3(x, y, z);

        in >> x >> y >> z >>trash;
        light.diffuse = glm::vec3(x, y, z);

        in >> x >> y >> z >> trash;
        light.specular = glm::vec3(x, y, z);

        in
                >> light.constant >> trash
                >> light.linear >> trash
                >> light.quadratic >> trash;
    }
}

void loadDirLight(std::string filename, DirLight& light){
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
        glCullFace(GL_BACK);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    loadSkyboxVertices("resources/vertices/skybox_vertices.txt", skyboxVertices, N_SKYBOX_VERTICES);
    loadSkyboxVertices("resources/vertices/cat_trumpet_vertices.txt", catTrumpetVertices, N_SKYBOX_VERTICES);

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

    // load models
    Model forestModel("resources/objects/forest/forest.obj");
    forestModel.SetShaderTextureNamePrefix("material.");

    // pointLight
    PointLight& pointLight = programState->pointLight;
    loadPointLight("resources/lightSources/pointLight.txt", pointLight);

    // dirLight
    DirLight& dirLight = programState->dirLight;
    loadDirLight("resources/lightSources/dirLight.txt", dirLight);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    catSkyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    ourShader.use();
    ourShader.setFloat("material.shininess", 128.0f);

    // render loop
    while (!glfwWindowShouldClose(window)) {

        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // render
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

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

        // loading pointLight into shader
        //pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        pointLight.position = glm::vec3(0.0f);
        {
            ourShader.setVec3("pointLight.position", pointLight.position);
            ourShader.setVec3("pointLight.ambient", pointLight.ambient);
            ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
            ourShader.setVec3("pointLight.specular", pointLight.specular);
            ourShader.setFloat("pointLight.constant", pointLight.constant);
            ourShader.setFloat("pointLight.linear", pointLight.linear);
            ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
            ourShader.setVec3("viewPosition", programState->camera.Position);
        }

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,programState->forestPosition); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(programState->forestScale));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);

        glDisable(GL_CULL_FACE);
        forestModel.Draw(ourShader);
        glEnable(GL_CULL_FACE);

        // drawing skybox
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
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        //ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &pState->clearColor);
        ImGui::DragFloat3("Forest position", (float*)&pState->forestPosition);
        ImGui::DragFloat("Forest scale", &pState->forestScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &pState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &pState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &pState->pointLight.quadratic, 0.05, 0.0, 1.0);
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
        ImGui::DragFloat3("Ambient", (float*)&pState->dirLight.ambient, 0.05, 0 ,1);
        ImGui::DragFloat3("Diffuse", (float*)&pState->dirLight.diffuse, 0.05, 0 ,1);
        ImGui::DragFloat3("Specular", (float*)&pState->dirLight.specular, 0.05, 0 ,1);
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
                    0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
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

void loadSkyboxVertices(std::string filename, float* vertices, int n){
    std::ifstream in(filename);
    for (int i = 0; in && i < n; i++) {
        in >> vertices[i];
    }
}
